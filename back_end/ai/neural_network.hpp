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


/* `struct` `NeuralNetworkImpl` is a concrete implementation of neural network that defines network,
* including architecture and forward pass.
* The name `NeuralNetworkImpl` is required by macro `TORCH_MODULE`.
*/
struct NeuralNetworkImpl : torch::nn::Module {
    // Mark submodules as mutable so they can be used in a const forward function.
    mutable torch::nn::Linear fc1{ nullptr };
    mutable torch::nn::Linear fc2{ nullptr };
    mutable torch::nn::Linear fc_policy{ nullptr };
    mutable torch::nn::Linear fc_value{ nullptr };

    NeuralNetworkImpl(int64_t input_dim, int64_t hidden_dim) {
        fc1 = register_module("fc1", torch::nn::Linear(input_dim, hidden_dim));
        fc2 = register_module("fc2", torch::nn::Linear(hidden_dim, hidden_dim));
        fc_policy = register_module("fc_policy", torch::nn::Linear(hidden_dim, 1)); // policy head
        fc_value = register_module("fc_value", torch::nn::Linear(hidden_dim, 1)); // value head
    }

    std::vector<torch::Tensor> forward(torch::Tensor inputTensorForBatch) const {
        torch::Tensor outputOfLayer1 = torch::relu(fc1->forward(inputTensorForBatch));
        torch::Tensor outputOfLayer2 = torch::relu(fc2->forward(outputOfLayer1));
        torch::Tensor tensorOfPredictedValues = torch::tanh(fc_value->forward(outputOfLayer2));
        auto tensorOfPredictedPolicies = torch::sigmoid(fc_policy->forward(outputOfLayer2));
        return { tensorOfPredictedValues, tensorOfPredictedPolicies };
    }
};

/* Class `NeuralNetwork` is a template for a wrapper that holds a shared pointer to `NeuralNetworkImpl` and
* simplifies use of `NeuralNetworkImpl` with libtorch's module system.
*/
TORCH_MODULE(NeuralNetwork);

/* Class `WrapperOfNeuralNetwork` is a template for a wrapper of an instance of `NeuralNetwork` that
* - handles network lifecycle by managing saving and loading model parameters from a file and handling device assignment, and
* - handles domain specific evaluation by providing helper methods to perform inference given game specific features. 
*/
class WrapperOfNeuralNetwork {
private:
    std::filesystem::file_time_type lastWriteTime;
    torch::Device device;
public:
    NeuralNetwork neuralNetwork = nullptr;
    std::string modelPath;
    Board board;

    WrapperOfNeuralNetwork(const std::string& modelPath) : modelPath(modelPath), device(torch::kCPU), board() {
        const int64_t inputDim = 5;
        const int64_t hiddenDim = 128;
        
        neuralNetwork = NeuralNetwork(inputDim, hiddenDim);
        neuralNetwork->eval();

        if (!std::filesystem::exists(modelPath)) {
            std::clog << "[WARNING] Model file was not found at " << modelPath << "." << std::endl;
            torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
            try {
                torch::save(variableListOfParameters, modelPath);
                std::clog << "[INFO] Default model parameters were saved to " << modelPath << "." << std::endl;
            }
            catch (const c10::Error& e) {
                std::cerr << "[ERROR] Saving default model parameters failed with the following error. " << e.what() << std::endl;
                throw std::runtime_error("Saving default model parameters failed.");
            }
        }

        try {
            bool cudaIsAvailable = torch::cuda::is_available();
            std::clog << "[INFO] CUDA " << (cudaIsAvailable ? "is" : "is not") << " available." << std::endl;
            device = cudaIsAvailable ? torch::kCUDA : torch::kCPU;
            neuralNetwork->to(device);

            // Load the parameters from file.
            std::vector<torch::Tensor> vectorOfParameters;
            torch::load(vectorOfParameters, modelPath);
            torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
            if (variableListOfParameters.size() != vectorOfParameters.size()) {
                std::cerr <<
                    "[ERROR] Numbers of parameters are mismatched. " <<
                    variableListOfParameters.size() << " parameters were expected. " <<
                    "There are " << vectorOfParameters.size() << " parameters." << std::endl;
                throw std::runtime_error("Numbers of parameters are mismatched.");
            }
            torch::NoGradGuard noGrad; // Disable gradient tracking during parameter copy.
            for (size_t i = 0; i < vectorOfParameters.size(); i++) {
                variableListOfParameters[i].data().copy_(vectorOfParameters[i].data());
            }
            lastWriteTime = std::filesystem::last_write_time(modelPath);
            std::clog <<
                "[INFO] Model parameters were successfully loaded from " << modelPath <<
                " on device " << (device == torch::kCUDA ? "CUDA" : "CPU") << "." << std::endl;
        }
        catch (const c10::Error& e) {
            std::cerr << "[ERROR] The following exception occurred while loading model. " << e.what() << std::endl;
            throw std::runtime_error("An exception occurred while loading model.");
        }
    }

    std::pair<double, double> evaluateStructure(const std::vector<float>& features) {
        torch::NoGradGuard noGrad; // Disable gradient calculation for inference.
        torch::Tensor input = torch::tensor(features, torch::TensorOptions().device(device)).unsqueeze(0);
        auto output = neuralNetwork->forward(input);
        double value = output[0].item<double>();
        double policy = output[1].item<double>();
        return { value, policy };
    }

    std::vector<std::pair<double, double>> evaluateStructures(const std::vector<std::vector<float>>& features) {
        torch::NoGradGuard noGrad;
        std::vector<torch::Tensor> tensors;
        for (const auto& vec : features) {
            tensors.push_back(torch::tensor(vec, torch::TensorOptions().device(device).dtype(torch::kFloat32)));
        }
        torch::Tensor inputTensor = torch::stack(tensors);
        auto outputs = neuralNetwork->forward(inputTensor);
        torch::Tensor valuesTensor = outputs[0].cpu().squeeze(1);
        torch::Tensor policiesTensor = outputs[1].cpu().squeeze(1);
        std::vector<float> values(valuesTensor.data_ptr<float>(), valuesTensor.data_ptr<float>() + valuesTensor.numel());
        std::vector<float> policies(policiesTensor.data_ptr<float>(), policiesTensor.data_ptr<float>() + policiesTensor.numel());
        std::vector<std::pair<double, double>> results;
        for (size_t i = 0; i < values.size(); i++) {
            results.push_back({ static_cast<double>(values[i]), static_cast<double>(policies[i]) });
        }
        return results;
    }

    void reloadIfUpdated() {
        try {
            auto currentWriteTime = std::filesystem::last_write_time(modelPath);
            if (currentWriteTime > lastWriteTime) {
                std::vector<torch::Tensor> vectorOfParameters;
                torch::load(vectorOfParameters, modelPath);
                torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
                if (variableListOfParameters.size() != vectorOfParameters.size()) {
                    std::cerr << "[ERROR] Numbers of parameters were mismatched during reload." << std::endl;
                    throw std::runtime_error("Numbers of parameters were mismatched during reload.");
                }
                torch::NoGradGuard noGrad;
                for (size_t i = 0; i < vectorOfParameters.size(); i++) {
                    variableListOfParameters[i].data().copy_(vectorOfParameters[i].data());
                }
                lastWriteTime = currentWriteTime;
                std::clog << "[INFO] Model was reloaded after updated model parameters were detected." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] The following exception occurred. " << e.what() << std::endl;
        }
    }
};