#pragma once


#include "../game/board.hpp"

#include <torch/script.h>
/* Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include;`.
* 
* Add to Additional Library Directories `$(SolutionDir)\dependencies\libtorch\lib;`.
* 
* Add to Additional Dependencies `c10.lib;`
* Add to Additional Dependencies `torch_cpu.lib;`.
* Add to Additional Dependencies `torch_cuda.lib;`.
* 
* Add to command line `/INCLUDE:"?warp_size@cuda@at@@YAHXZ"`.
* See https://github.com/pytorch/pytorch/issues/31611 .
* 
* For a PC with CUDA, add the following string of newline separated commands to Post - Build Event.
* xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\c10.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\torch_cpu.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\fbgemm.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\libiomp5md.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\uv.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\asmjit.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\torch_cuda.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\c10_cuda.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cudnn64_9.dll" "$(OutDir)"
*
* For a PC without a GPU, add the additional string of newline separated commands to Post - Build Event.
* xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cusparse64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cufft64_11.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cusolver64_11.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cublas64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cublasLt64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cudart64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\nvJitLink_120_0.dll" "$(OutDir)"
*/

#include <torch/torch.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include\torch\csrc\api\include;`.


// Define the default neural network as a torch module.
struct SettlersPolicyValueNetImpl : torch::nn::Module {
    // Mark submodules as mutable so they can be used in a const forward function.
    mutable torch::nn::Linear fc1{ nullptr };
    mutable torch::nn::Linear fc2{ nullptr };
    mutable torch::nn::Linear fc_policy{ nullptr };
    mutable torch::nn::Linear fc_value{ nullptr };

    // Constructor: Initialize layers with given dimensions.
    SettlersPolicyValueNetImpl(int64_t input_dim, int64_t hidden_dim) {
        fc1 = register_module("fc1", torch::nn::Linear(input_dim, hidden_dim));
        fc2 = register_module("fc2", torch::nn::Linear(hidden_dim, hidden_dim));
        fc_policy = register_module("fc_policy", torch::nn::Linear(hidden_dim, 1)); // policy head
        fc_value = register_module("fc_value", torch::nn::Linear(hidden_dim, 1)); // value head
    }

    // Forward pass: Compute activations and return (value, policy) tuple.
    std::vector<torch::Tensor> forward(torch::Tensor x) const {
        x = torch::relu(fc1->forward(x));
        x = torch::relu(fc2->forward(x));
        auto value = torch::tanh(fc_value->forward(x));
        auto policy = torch::sigmoid(fc_policy->forward(x));
        return { value, policy };
    }
};

TORCH_MODULE(SettlersPolicyValueNet);

class SettlersNeuralNet {
private:
    std::filesystem::file_time_type lastModified;
    torch::Device device;
public:
    SettlersPolicyValueNet model = nullptr;
    std::string modelPath;
    Board board;

    // Constructor `SettlersNeuralNet` instantiates model, then loads parameters from disk or saves default parameters if missing.
    SettlersNeuralNet(const std::string& modelPath) : modelPath(modelPath), device(torch::kCPU), board() {
        // Define default dimensions.
        const int64_t inputDim = 5;
        const int64_t hiddenDim = 128;
        
        // Initialize the model.
        model = SettlersPolicyValueNet(inputDim, hiddenDim);
        model->eval();

        // If the parameter file doesn't exist, save the default model parameters.
        if (!std::filesystem::exists(modelPath)) {
            std::clog << "[WARNING] Model file not found at " << modelPath << ". Creating default parameter file." << std::endl;
            auto parameters = model->parameters();
            try {
                torch::save(parameters, modelPath);
                std::clog << "[INFO] Default model parameters saved to " << modelPath << "." << std::endl;
            }
            catch (const c10::Error& e) {
                std::cerr << "[ERROR] Saving default model parameters failed with the following error. " << e.what() << std::endl;
                throw std::runtime_error("Saving default model parameters failed.");
            }
        }

        try {
            std::clog << "[INFO] CUDA " << (torch::cuda::is_available() ? "is" : "is not") << " available." << std::endl;
            device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
            model->to(device);

            // Load the parameters from file.
            std::vector<torch::Tensor> parameters;
            torch::load(parameters, modelPath);
            auto modelParameters = model->parameters();
            if (modelParameters.size() != parameters.size()) {
                std::cerr << "[ERROR] A mismatch in parameter count occurred. " << modelParameters.size() << " parameters were expected. There are " << parameters.size() << " parameters." << std::endl;
                throw std::runtime_error("A mismatch in parameter count occurred.");
            }
            torch::NoGradGuard noGrad; // Disable gradient tracking during parameter copy.
            for (size_t i = 0; i < parameters.size(); i++) {
                modelParameters[i].data().copy_(parameters[i].data());
            }
            lastModified = std::filesystem::last_write_time(modelPath);
            std::clog << "[INFO] Model parameters were successfully loaded from " << modelPath << " on device " << (device == torch::kCUDA ? "CUDA" : "CPU") << "." << std::endl;
        }
        catch (const c10::Error& e) {
            std::cerr << "[ERROR] The following exception occurred while loading model. " << e.what() << std::endl;
            throw std::runtime_error("An exception occurred while loading model from the following path. " + modelPath);
        }
    }

    // TODO: Consider whether functions `evaluateSettlement`, `evaluateCity`, and `evaluateRoad` should be different.

    // Evaluate a settlement move given a feature vector.
    std::pair<double, double> evaluateSettlement(const std::vector<float>& features) {
        torch::NoGradGuard noGrad; // Disable gradient calculation for inference.
        torch::Tensor input = torch::tensor(features, torch::TensorOptions().device(device)).unsqueeze(0);
        auto output = model->forward(input);
        double value = output[0].item<double>();
        double policy = output[1].item<double>();
        return { value, policy };
    }

    std::pair<double, double> evaluateSettlementFromVertex(const std::string& labelOfVertex) {
        std::vector<float> featureVector = board.getFeatureVector(labelOfVertex);
        return evaluateSettlement(featureVector);
    }

    // Evaluate a city move given a feature vector.
    std::pair<double, double> evaluateCity(const std::vector<float>& features) {
        torch::NoGradGuard noGrad; // Disable gradient calculation for inference.
        torch::Tensor input = torch::tensor(features, torch::TensorOptions().device(device)).unsqueeze(0);
        auto output = model->forward(input);
        double value = output[0].item<double>();
        double policy = output[1].item<double>();
        return { value, policy };
    }

    std::pair<double, double> evaluateCityFromVertex(const std::string& labelOfVertex) {
        std::vector<float> featureVector = board.getFeatureVector(labelOfVertex);
        return evaluateCity(featureVector);
    }

    // Evaluate a road move given a feature vector.
    std::pair<double, double> evaluateRoad(const std::vector<float>& features) {
        torch::NoGradGuard noGrad; // Disable gradient calculation for inference.
        torch::Tensor input = torch::tensor(features, torch::TensorOptions().device(device)).unsqueeze(0);
        auto output = model->forward(input);
        double value = output[0].item<double>();
        double policy = output[1].item<double>();
        return { value, policy };
    }

    /* Evaluate a road move given the last builiding vertex and an edge key.
    * Parse the edge key with format "x1-y1_x2-y2" and determine which endpoint is not the last building.
    */
    std::pair<double, double> evaluateRoadFromEdge(const std::string& labelOfVertexOfLastBuilding, const std::string& edgeKey) {
        float x1, y1, x2, y2;
        if (sscanf_s(edgeKey.c_str(), "%f-%f_%f-%f", &x1, &y1, &x2, &y2) != 4) {
            throw std::runtime_error("Edge key " + edgeKey + " has invalid format.");
        }
        std::string otherVertex;
        // Here, for simplicity, we assume that the "other" endpoint is the one with larger x.
        // TODO: Replace the following if else block with logic that compares coordinates to determine which vertex is not the vertex of the last building.
        if (x1 < x2) {
            otherVertex = board.getVertexLabelByCoordinates(x2, y2);
        }
        else {
            otherVertex = board.getVertexLabelByCoordinates(x1, y1);
        }
        if (otherVertex.empty()) {
            throw std::runtime_error("Other endpoint for road edge " + edgeKey + " cannot be determined.");
        }
        return evaluateSettlementFromVertex(otherVertex);
    }

    // Reload model if weights have been updated on disk.
    void reloadIfUpdated() {
        try {
            auto currentModTime = std::filesystem::last_write_time(modelPath);
            if (currentModTime > lastModified) {
                std::vector<torch::Tensor> parameters;
                torch::load(parameters, modelPath);
                auto modelParameters = model->parameters();
                if (modelParameters.size() != parameters.size()) {
                    std::cerr << "[ERROR] A mismatch in parameter count occurred during reload." << std::endl;
                    throw std::runtime_error("A mismatch in parameter count occurred during reload.");
                }
                torch::NoGradGuard noGrad;
                for (size_t i = 0; i < parameters.size(); i++) {
                    modelParameters[i].data().copy_(parameters[i].data());
                }
                lastModified = currentModTime;
                std::clog << "[INFO] Updated model parameters were detected. Model was reloaded." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] The following exception in `reloadIfUpdated` occurred. " << e.what() << std::endl;
        }
    }
};