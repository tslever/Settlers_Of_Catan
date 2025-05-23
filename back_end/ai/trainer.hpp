#pragma once


#include "../db/database.hpp"
#include "neural_network.hpp"
#include "self_play.hpp"


namespace AI {

    class Trainer {
    public:
        Trainer(
            AI::WrapperOfNeuralNetwork* neuralNetToUse,
            int modelWatcherIntervalToUse,
            int trainingThresholdToUse,
            int numberOfSimulationsToUse,
            double cPuctToUse,
            double toleranceToUse,
            double learningRateToUse,
			int numberOfEpochsToUse,
            int batchSizeToUse,
			double dirichletMixingWeightToUse,
			double dirichletShapeToUse
        ) : neuralNet(neuralNetToUse),
            modelWatcherInterval(modelWatcherIntervalToUse),
            trainingThreshold(trainingThresholdToUse),
            numberOfSimulations(numberOfSimulationsToUse),
            cPuct(cPuctToUse),
            tolerance(toleranceToUse),
			learningRate(learningRateToUse),
			numberOfEpochs(numberOfEpochsToUse),
			batchSize(batchSizeToUse),
			dirichletMixingWeight(dirichletMixingWeightToUse),
			dirichletShape(dirichletShapeToUse)
        {
            // Do nothing.
        }

        ~Trainer() {
            stop();
        }

        // Function `startModelWatcher` starts a model watcher thread.
        void startModelWatcher() {
			modelWatcherThread = std::jthread([this](std::stop_token stopToken) {
				modelWatcher(stopToken);
            });
        }

        // Function `runTrainingLoop` starts a training loop thread.
        void runTrainingLoop() {
            trainingThread = std::jthread([this](std::stop_token stopToken) {
				trainingLoop(stopToken);
            });
        }

        // Function `stop` stops the model watcher and training threads and joins them to the main thread.
        void stop() {
            if (modelWatcherThread.joinable()) {
                modelWatcherThread.request_stop();
            }
            if (trainingThread.joinable()) {
                trainingThread.request_stop();
            }
        }

    private:
        int batchSize;
		std::condition_variable trainingConditionVariable;
        double cPuct;
        std::deque<TrainingExample> trainingQueue;
        double learningRate;
        int modelWatcherInterval;
        std::jthread modelWatcherThread;
        WrapperOfNeuralNetwork* neuralNet;
        int numberOfEpochs;
        int numberOfSimulations;
        double tolerance;
        std::mutex trainingMutex;
        std::jthread trainingThread;
        int trainingThreshold;
		double dirichletMixingWeight;
		double dirichletShape;

        /* Function `modelWatcher` runs on a background thread and
        * periodically reloads neural network parameters if file of parameters was updated.
        */
        void modelWatcher(std::stop_token stopToken) {
            while (!stopToken.stop_requested()) {
                neuralNet->reloadIfUpdated();
                std::this_thread::sleep_for(std::chrono::seconds(modelWatcherInterval));
            }
        }

        /* Function `trainingLoop` runs on a background thread and
        * continuously runs full self play games to collect training examples and
        * triggers training when enough examples have been collected.
        */
        void trainingLoop(std::stop_token stopToken) {
            while (!stopToken.stop_requested()) {
                std::vector<AI::TrainingExample> vectorOfTrainingExamplesFromSelfPlayGame = runSelfPlayGame(
                    *neuralNet,
                    numberOfSimulations,
                    cPuct,
                    tolerance,
                    dirichletMixingWeight,
                    dirichletShape
                );
                {
                    std::lock_guard<std::mutex> lock(trainingMutex);
                    for (const auto& trainingExample : vectorOfTrainingExamplesFromSelfPlayGame) {
                        trainingQueue.push_back(trainingExample);
                    }
                }
                trainingConditionVariable.notify_one();
                {
                    std::unique_lock<std::mutex> lock(trainingMutex);
                    if (trainingQueue.size() < static_cast<size_t>(trainingThreshold)) {
                        lock.unlock();
                        continue;
                    }
                    std::vector<TrainingExample> batch(trainingQueue.begin(), trainingQueue.end());
                    trainingQueue.clear();
                    lock.unlock();
                    trainNeuralNetwork(batch, neuralNet);
                }
                std::this_thread::yield();

            }
        }

        void trainNeuralNetwork(
            const std::vector<AI::TrainingExample>& vectorOfTrainingExamples,
            AI::WrapperOfNeuralNetwork* wrapperOfNeuralNetwork
        ) {
            int numberOfTrainingExamples = vectorOfTrainingExamples.size();
            Logger::info("[TRAINING] Neural network will be trained on " + std::to_string(numberOfTrainingExamples) + " examples.");

			AI::NeuralNetwork neuralNetwork = wrapperOfNeuralNetwork->neuralNetwork;
			c10::Device device = neuralNetwork->parameters()[0].device();
			c10::TensorOptions tensorOptions = torch::TensorOptions().dtype(torch::kFloat32).device(device);

            std::vector<torch::Tensor> vectorOfInputTensors;
            std::vector<torch::Tensor> vectorOfTensorsOfTargetValues;
            std::vector<torch::Tensor> vectorOfTensorsOfTargetPolicies;

            Board board;
            for (const AI::TrainingExample& trainingExample : vectorOfTrainingExamples) {
				std::string move = trainingExample.move;
                std::string moveType = trainingExample.moveType;
                std::vector<float> featureVector = board.getGridRepresentationForMove(move, moveType);
                torch::Tensor inputTensor = torch::tensor(featureVector, tensorOptions);
                vectorOfInputTensors.push_back(inputTensor);

                std::vector<double> vectorOfTargetValue = { trainingExample.value };
                torch::Tensor tensorOfTargetValue = torch::tensor(vectorOfTargetValue, tensorOptions);
                vectorOfTensorsOfTargetValues.push_back(tensorOfTargetValue);

                std::vector<double> vectorOfTargetPolicy = { trainingExample.policy };
                torch::Tensor tensorOfTargetPolicy = torch::tensor(vectorOfTargetPolicy, tensorOptions);
                vectorOfTensorsOfTargetPolicies.push_back(tensorOfTargetPolicy);
            }

            if (vectorOfInputTensors.empty()) {
                throw std::runtime_error("[TRAINING] No training examples are available.");
            }

            /* Tensor `inputTensor` has shape[N, 5].
            * Tensor `tensorOfTargetValues` has shape [N, 1].
            * Tensor `tensorOfTargetPolicies` has shape [N, 1].
            */
            torch::Tensor inputTensor = torch::stack(vectorOfInputTensors).to(device);
            torch::Tensor tensorOfTargetValues = torch::stack(vectorOfTensorsOfTargetValues).view({ -1, 1 }).to(device);
            torch::Tensor tensorOfTargetPolicies = torch::stack(vectorOfTensorsOfTargetPolicies).view({ -1, 1 }).to(device);

            {
				std::lock_guard<std::mutex> netLock(wrapperOfNeuralNetwork->mutex);
                neuralNetwork->train();
            }

            // Configure AdaM optimizer.
            torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
            torch::optim::AdamOptions adamOptions(learningRate);
            torch::optim::Adam adam(variableListOfParameters, adamOptions);

            // Define training parameters.
            int64_t dimension = 0;
            const int numberOfSamples = inputTensor.size(dimension);

            // Train.
            for (int indexOfEpoch = 0; indexOfEpoch < numberOfEpochs; indexOfEpoch++) {
                double runningLoss = 0.0;

                // Create a vector of shuffled indices.
                std::vector<int64_t> vectorOfIndicesOfSamples(numberOfSamples);
                int startingValue = 0;
                std::iota(vectorOfIndicesOfSamples.begin(), vectorOfIndicesOfSamples.end(), startingValue);
                std::default_random_engine defaultRandomEngine;
                std::shuffle(vectorOfIndicesOfSamples.begin(), vectorOfIndicesOfSamples.end(), defaultRandomEngine);

                // Process mini batches.
                for (int indexOfFirstSample = 0; indexOfFirstSample < numberOfSamples; indexOfFirstSample += batchSize) {
                    int indexOfLastSample = std::min(indexOfFirstSample + batchSize, numberOfSamples);
                    std::vector<int64_t> vectorOfIndicesOfSamplesInBatch(
                        vectorOfIndicesOfSamples.begin() + indexOfFirstSample,
                        vectorOfIndicesOfSamples.begin() + indexOfLastSample
                    );
                    torch::TensorOptions optionsForTensorOfIndicesOfSamplesInBatch = torch::TensorOptions().dtype(torch::kInt64).device(device);
                    torch::Tensor tensorOfIndicesOfSamplesInBatch = torch::tensor(vectorOfIndicesOfSamplesInBatch, optionsForTensorOfIndicesOfSamplesInBatch);

                    torch::Tensor inputTensorForBatch = inputTensor.index_select(dimension, tensorOfIndicesOfSamplesInBatch);
                    torch::Tensor tensorOfTargetValuesForBatch = tensorOfTargetValues.index_select(dimension, tensorOfIndicesOfSamplesInBatch);
                    torch::Tensor tensorOfTargetPoliciesForBatch = tensorOfTargetPolicies.index_select(dimension, tensorOfIndicesOfSamplesInBatch);

                    adam.zero_grad();
                    std::vector<torch::Tensor> vectorOfValueAndPolicy = neuralNetwork->forward(inputTensorForBatch);
                    torch::Tensor tensorOfPredictedValues = vectorOfValueAndPolicy[0];
                    torch::Tensor tensorOfPredictedPolicies = vectorOfValueAndPolicy[1];

                    torch::Tensor tensorOfValueLoss = torch::mse_loss(tensorOfPredictedValues, tensorOfTargetValuesForBatch);
                    torch::Tensor tensorOfPolicyLoss = torch::binary_cross_entropy(tensorOfPredictedPolicies, tensorOfTargetPoliciesForBatch);
                    torch::Tensor tensorOfLoss = tensorOfValueLoss + tensorOfPolicyLoss;

                    tensorOfLoss.backward();
                    adam.step();

                    double loss = tensorOfLoss.item<double>();
                    int numberOfSamplesInBatch = indexOfLastSample - indexOfFirstSample + 1;
                    double lossForBatch = loss * numberOfSamplesInBatch;
                    runningLoss += lossForBatch;
                }

                double averageLoss = runningLoss / numberOfSamples;

                Logger::info(
                    "[TRAINING] Epoch " + std::to_string(indexOfEpoch + 1) + " of " + std::to_string(numberOfEpochs) +
                    " completed with average loss " + std::to_string(averageLoss) + "."
                );
            }

            variableListOfParameters = neuralNetwork->parameters();
            std::lock_guard<std::mutex> lock(wrapperOfNeuralNetwork->mutex);
            torch::save(variableListOfParameters, wrapperOfNeuralNetwork->pathToFileOfParameters);
            Logger::info("[TRAINING] Model parameters were saved after training.");

            neuralNetwork->eval();
        }
    };

}