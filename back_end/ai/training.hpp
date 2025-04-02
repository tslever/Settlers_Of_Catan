#pragma once


namespace AI {

    void trainNeuralNetwork(const std::vector<AI::TrainingExample>& vectorOfTrainingExamples, AI::WrapperOfNeuralNetwork* wrapperOfNeuralNetwork) {
        int numberOfTrainingExamples = vectorOfTrainingExamples.size();
        std::clog << "[TRAINING] Neural network will be trained on " << numberOfTrainingExamples << " examples." << std::endl;

        std::vector<torch::Tensor> vectorOfInputTensors;
        std::vector<torch::Tensor> vectorOfTensorsOfTargetValues;
        std::vector<torch::Tensor> vectorOfTensorsOfTargetPolicies;

        Board board;
        for (const AI::TrainingExample& trainingExample : vectorOfTrainingExamples) {
            std::vector<float> featureVector = board.getFeatureVector(trainingExample.move);
            c10::TensorOptions tensorOptions = torch::TensorOptions().dtype(torch::kFloat32);
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
        torch::Tensor inputTensor = torch::stack(vectorOfInputTensors);
        torch::IntArrayRef size = { -1, 1 };
        torch::Tensor tensorOfTargetValues = torch::stack(vectorOfTensorsOfTargetValues).view(size);
        torch::Tensor tensorOfTargetPolicies = torch::stack(vectorOfTensorsOfTargetPolicies).view(size);

        AI::NeuralNetwork neuralNetwork = wrapperOfNeuralNetwork->neuralNetwork;
        neuralNetwork->train();

        // Configure AdaM optimizer.
        torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
        double learningRate = 1e-3;
        torch::optim::AdamOptions adamOptions(learningRate);
        torch::optim::Adam adam(variableListOfParameters, adamOptions);

        // Define training parameters.
        const int numberOfEpochs = 10;
        const int batchSize = 32;
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
                torch::TensorOptions tensorOptions = torch::TensorOptions().dtype(torch::kInt64);
                torch::Tensor tensorOfIndicesOfSamplesInBatch = torch::tensor(vectorOfIndicesOfSamplesInBatch, tensorOptions);

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
                double loss = tensorOfLoss.item<double>();

                tensorOfLoss.backward();
                adam.step();

                int numberOfSamplesInBatch = indexOfLastSample - indexOfFirstSample + 1;
                double lossForBatch = loss * numberOfSamplesInBatch;
                runningLoss += lossForBatch;
            }

            double averageLoss = runningLoss / numberOfSamples;

            std::clog <<
                "[TRAINING] Epoch " << (indexOfEpoch + 1) << " of " << numberOfEpochs << " completed " <<
                "with average loss " << averageLoss << "." << std::endl;
        }

        try {
            torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
            torch::save(variableListOfParameters, wrapperOfNeuralNetwork->pathToFileOfParameters);
            std::clog << "[TRAINING] Model parameters were saved after training." << std::endl;
        }
        catch (const c10::Error& e) {
            std::cerr << "[TRAINING] The following error occurred when saving model parameters. " << e.what() << std::endl;
        }

        neuralNetwork->eval();
    }

}