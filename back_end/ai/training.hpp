#pragma once


// Dummy implementation of training function
// TODO: Replace the following implementation with an actual training loop using libtorch and optimizer and loss function.
void trainNeuralNetworkIfNeeded(const std::vector<TrainingExample>& vectorOfTrainingExamples, SettlersNeuralNet* neuralNet) {
    std::clog << "[TRAINING] Neural network will be trained on " << vectorOfTrainingExamples.size() << " examples." << std::endl;
    Board board;

    // For simplicity, we assume that each training example corresponds to a settlement move.
    // TODO: Consider allowing training examples corresponding to first road, city, and second road moves.
    auto getFeatures = [&](const std::string& move) -> std::vector<float> {
        try {
            return board.getFeatureVector(move);
        }
        catch (...) {
            std::cerr << "[TRAINING] Getting feature vector failed." << std::endl;
            throw std::runtime_error("[TRAINING] Getting feature vector failed.");
        }
    };

    std::vector<torch::Tensor> inputList;
    std::vector<torch::Tensor> targetValueList;
    std::vector<torch::Tensor> targetPolicyList;

    for (const auto& trainingExample : vectorOfTrainingExamples) {
        // Compute input features from the move (here assumed to be a vertex label)
        // TODO: Allow input feature to be an edge label or edge key.
        auto vectorOfFeatures = getFeatures(trainingExample.move);
        torch::Tensor inputTensor = torch::tensor(vectorOfFeatures, torch::TensorOptions().dtype(torch::kFloat32));
        inputList.push_back(inputTensor);
        // Target value and target policy are scalars.
        targetValueList.push_back(torch::tensor({ trainingExample.value }, torch::TensorOptions().dtype(torch::kFloat32)));
        targetPolicyList.push_back(torch::tensor({trainingExample.policy}, torch::TensorOptions().dtype(torch::kFloat32)));
    }

    if (inputList.empty()) {
        std::clog << "[TRAINING] No training examples are available. Training will be aborted." << std::endl;
        return;
    }

    // Stack inputs into a single tensor of shape [N, 5].
    torch::Tensor X = torch::stack(inputList);
    // Targets are reshaped to [N, 1].
    torch::Tensor Y_value = torch::stack(targetValueList).view({ -1, 1 });
    torch::Tensor Y_policy = torch::stack(targetPolicyList).view({ -1, 1 });

    // Set the neural network model to training mode.
    neuralNet->model->train();

    // Configure the optimizer AdaM and the learning rate.
    torch::optim::Adam optimizer(neuralNet->model->parameters(), torch::optim::AdamOptions(1e-3));

    // Define training parameters.
    const int numberOfEpochs = 10;
    const int batchSize = 32;
    const int numberOfSamples = X.size(0);

    // Training loop
    for (int indexOfEpoch = 0; indexOfEpoch < numberOfEpochs; indexOfEpoch++) {
        double runningLoss = 0.0;

        // Create a vector of shuffled indices.
        std::vector<int64_t> vectorOfIndices(numberOfSamples);
        std::iota(vectorOfIndices.begin(), vectorOfIndices.end(), 0);
        std::shuffle(vectorOfIndices.begin(), vectorOfIndices.end(), std::default_random_engine{});
        
        // Process mini batches.
        for (int indexOfFirstSample = 0; indexOfFirstSample < numberOfSamples; indexOfFirstSample += batchSize) {
            int indexOfLastSample = std::min(indexOfFirstSample + batchSize, numberOfSamples);
            std::vector<int64_t> vectorOfIndicesOfSamplesInBatch(
                vectorOfIndices.begin() + indexOfFirstSample,
                vectorOfIndices.begin() + indexOfLastSample
            );
            torch::Tensor tensorOfIndices = torch::tensor(vectorOfIndicesOfSamplesInBatch, torch::TensorOptions().dtype(torch::kInt64));
            
            torch::Tensor batchX = X.index_select(0, tensorOfIndices);
            torch::Tensor batchYValue = Y_value.index_select(0, tensorOfIndices);
            torch::Tensor batchYPolicy = Y_policy.index_select(0, tensorOfIndices);

            optimizer.zero_grad();
            auto outputs = neuralNet->model->forward(batchX);
            torch::Tensor predValue = outputs[0];
            torch::Tensor predPolicy = outputs[1];

            torch::Tensor lossValue = torch::mse_loss(predValue, batchYValue);
            torch::Tensor lossPolicy = torch::binary_cross_entropy(predPolicy, batchYPolicy);
            torch::Tensor loss = lossValue + lossPolicy;

            loss.backward();
            optimizer.step();

            runningLoss += loss.item<double>() * (indexOfLastSample - indexOfFirstSample);
        }
        double avgLoss = runningLoss / numberOfSamples;
        std::clog << "[TRAINING] Epoch " << (indexOfEpoch + 1) << "/" << numberOfEpochs << ", Loss: " << avgLoss << std::endl;
    }

    try {
        auto parameters = neuralNet->model->parameters();
        torch::save(parameters, neuralNet->modelPath);
        std::clog << "[TRAINING] Model parameters saved after training." << std::endl;
    }
    catch (const c10::Error& e) {
        std::cerr << "[TRAINING] The following error occurred when saving model parameters. " << e.what() << std::endl;
    }

    // Set the model back to evaluation mode.
    neuralNet->model->eval();
}