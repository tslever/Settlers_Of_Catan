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

    // Forward pass: Compute activations and return vector of value and policy.
    std::vector<torch::Tensor> forward(torch::Tensor x) const {
        x = torch::relu(fc1->forward(x));
        x = torch::relu(fc2->forward(x));
        auto value = torch::tanh(fc_value->forward(x));
        auto policy = torch::sigmoid(fc_policy->forward(x));
        return { value, policy };
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
    NeuralNetwork model = nullptr;
    std::string modelPath;
    Board board;

    WrapperOfNeuralNetwork(const std::string& modelPath) : modelPath(modelPath), device(torch::kCPU), board() {
        const int64_t inputDim = 5;
        const int64_t hiddenDim = 128;
        
        model = NeuralNetwork(inputDim, hiddenDim);
        model->eval();

        if (!std::filesystem::exists(modelPath)) {
            std::clog << "[WARNING] Model file was not found at " << modelPath << "." << std::endl;
            torch::autograd::variable_list variableListOfParameters = model->parameters();
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
            model->to(device);

            // Load the parameters from file.
            std::vector<torch::Tensor> vectorOfParameters;
            torch::load(vectorOfParameters, modelPath);
            torch::autograd::variable_list variableListOfParameters = model->parameters();
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
        auto output = model->forward(input);
        double value = output[0].item<double>();
        double policy = output[1].item<double>();
        return { value, policy };
    }

    // TODO: Evaluate both buildings and roads based on feature vector and avoid evaluating roads as idiosyncratically as presently.
    std::pair<double, double> evaluateBuildingFromVertex(const std::string& labelOfVertex) {
        std::vector<float> featureVector = board.getFeatureVector(labelOfVertex);
        return evaluateStructure(featureVector);
    }

    std::pair<double, double> evaluateRoadFromEdge(const std::string& labelOfVertexOfLastBuilding, const std::string& edgeKey) {
        float x1;
        float y1;
        float x2;
        float y2;
        if (sscanf_s(edgeKey.c_str(), "%f-%f_%f-%f", &x1, &y1, &x2, &y2) != 4) {
            throw std::runtime_error("Edge key " + edgeKey + " has invalid format.");
        }
        std::string labelOfFirstVertex = board.getVertexLabelByCoordinates(x1, y1);
        std::string labelOfSecondVertex = board.getVertexLabelByCoordinates(x2, y2);
        std::string labelOfVertexWithoutLastBuilding;
        if (labelOfFirstVertex == labelOfVertexOfLastBuilding) {
            labelOfVertexWithoutLastBuilding = labelOfSecondVertex;
        }
        else if (labelOfSecondVertex == labelOfVertexOfLastBuilding) {
            labelOfVertexWithoutLastBuilding = labelOfFirstVertex;
        }
        if (labelOfVertexWithoutLastBuilding.empty()) {
            throw std::runtime_error("Label of vertex without last building cannot be determined.");
        }
        return evaluateBuildingFromVertex(labelOfVertexWithoutLastBuilding);
    }

    void reloadIfUpdated() {
        try {
            auto currentWriteTime = std::filesystem::last_write_time(modelPath);
            if (currentWriteTime > lastWriteTime) {
                std::vector<torch::Tensor> vectorOfParameters;
                torch::load(vectorOfParameters, modelPath);
                torch::autograd::variable_list variableListOfParameters = model->parameters();
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