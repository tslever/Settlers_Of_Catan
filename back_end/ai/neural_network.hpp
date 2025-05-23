#pragma once


#include "../game/board.hpp"

#include <torch/script.h>
/* Add to Additional Include Directories `$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\include;`.
* 
* Add to Additional Library Directories `$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib;`.
* 
* Add to Additional Dependencies `c10.lib;`
* Add to Additional Dependencies `torch_cpu.lib;`.
* Add to Additional Dependencies `torch_cuda.lib;`.
* 
* When debugging, add to command line `/INCLUDE:"?warp_size@cuda@at@@YAHXZ"`.
* See https://github.com/pytorch/pytorch/issues/31611 .
* 
* For a PC with CUDA, add the following string of newline separated commands to Post - Build Event.
* xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\c10.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\torch_cpu.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\fbgemm.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\libiomp5md.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\uv.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\asmjit.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\torch_cuda.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\c10_cuda.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cudnn64_9.dll" "$(OutDir)"
*
* For a PC without a GPU, add the additional string of newline separated commands to Post - Build Event.
* xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cusparse64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cufft64_11.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cusolver64_11.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cublas64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cublasLt64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\cudart64_12.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\lib\nvJitLink_120_0.dll" "$(OutDir)"
*/

#include <torch/torch.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\<debug or release>_version_of_libtorch\include\torch\csrc\api\include;`.


namespace AI {

    /* `struct` `NeuralNetworkImpl` is a concrete implementation of a neural network that defines the network.
    * The name `NeuralNetworkImpl` is required by macro `TORCH_MODULE`.
    */
    struct NeuralNetworkImpl : torch::nn::Module {
        /* Create layers that are members of this structure
        * so that each layer exists as part of the class's state and can be accessed by any member function.
        * Mark submodules as mutable so they can be used in a const forward function.
        */
        mutable torch::nn::Linear layer1{ nullptr };
        mutable torch::nn::Linear layer2{ nullptr };
        mutable torch::nn::Linear layerToCalculateValue{ nullptr };
        mutable torch::nn::Linear layerToCalculatePolicy{ nullptr };

        NeuralNetworkImpl(int64_t numberOfFeatures, int64_t numberOfNeurons) {
            /* Register each layer with libtorch to make libtorch aware of these layers
            * and to allow automatically managing parameters and integrating layers.
            */
            constexpr int64_t numberOfOutputs = 1;
            layer1 = register_module("layer1", torch::nn::Linear(numberOfFeatures, numberOfNeurons));
            layer2 = register_module("layer2", torch::nn::Linear(numberOfNeurons, numberOfNeurons));
            layerToCalculateValue = register_module("layerToCalculateValue", torch::nn::Linear(numberOfNeurons, numberOfOutputs));
            layerToCalculatePolicy = register_module("layerToCalculatePolicy", torch::nn::Linear(numberOfNeurons, numberOfOutputs));
        }

        std::vector<torch::Tensor> forward(torch::Tensor inputTensorForBatch) const {
            torch::Tensor outputOfLayer1 = torch::relu(layer1->forward(inputTensorForBatch));
            torch::Tensor outputOfLayer2 = torch::relu(layer2->forward(outputOfLayer1));
            torch::Tensor tensorOfPredictedValues = torch::tanh(layerToCalculateValue->forward(outputOfLayer2));
            torch::Tensor tensorOfPredictedPolicies = torch::sigmoid(layerToCalculatePolicy->forward(outputOfLayer2));
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
        std::string pathToFileOfParameters;
        Board board;
        mutable std::mutex mutex;

        WrapperOfNeuralNetwork(const std::string& pathToFileOfParameters, const int numberOfNeurons) :
            pathToFileOfParameters(pathToFileOfParameters),
            device(torch::kCPU),
            board()
        {
            const int64_t numberOfFeatures = 21 * 21;
            // TODO: Get number of features based on feature vector.

            neuralNetwork = NeuralNetwork(numberOfFeatures, numberOfNeurons);
            neuralNetwork->eval();

            if (!std::filesystem::exists(pathToFileOfParameters)) {
				Logger::warn("WrapperOfNeuralNetwork", "Model parameters file does not exist.");
                torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
                torch::save(variableListOfParameters, pathToFileOfParameters);
				Logger::info("Default model parameters were saved to " + pathToFileOfParameters + ".");
            }
            bool cudaIsAvailable = torch::cuda::is_available();
            std::string message = "CUDA ";
			if (cudaIsAvailable) {
				message += "is";
			}
			else {
				message += "is not";
			}
			message += " available.";
            Logger::info(message);
            device = cudaIsAvailable ? torch::kCUDA : torch::kCPU;
            neuralNetwork->to(device);

            std::vector<torch::Tensor> vectorOfParameters;
            torch::load(vectorOfParameters, pathToFileOfParameters);
            torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
            if (variableListOfParameters.size() != vectorOfParameters.size()) {
                throw std::runtime_error("Numbers of parameters are mismatched.");
            }
            // Disable gradient tracking during parameter copy.
            torch::NoGradGuard noGrad;
            for (size_t i = 0; i < vectorOfParameters.size(); i++) {
                variableListOfParameters[i].data().copy_(vectorOfParameters[i].data());
            }
            lastWriteTime = std::filesystem::last_write_time(pathToFileOfParameters);
			message = "Model parameters were successfully loaded from " + pathToFileOfParameters + " on device ";
            if (device == torch::kCUDA) {
                message += "CUDA";
			}
			else {
				message += "CPU";
			}
			message += ".";
            Logger::info(message);
        }

        std::pair<double, double> evaluateStructure(const std::vector<float>& featureVector) const {
			std::lock_guard<std::mutex> lock(mutex);
            // Disable gradient calculation for inference.
            torch::NoGradGuard noGrad;
            c10::TensorOptions tensorOptions = torch::TensorOptions().device(device);
            int dimension = 0;
            torch::Tensor inputTensor = torch::tensor(featureVector, tensorOptions).unsqueeze(dimension);
            std::vector<torch::Tensor> vectorOfTensorsOfPredictedValueAndPolicy = neuralNetwork->forward(inputTensor);
            double value = vectorOfTensorsOfPredictedValueAndPolicy[0].item<double>();
            double policy = vectorOfTensorsOfPredictedValueAndPolicy[1].item<double>();
            std::pair<double, double> pairOfPredictedValueAndPolicy = { value, policy };
            return pairOfPredictedValueAndPolicy;
        }

        std::vector<std::pair<double, double>> evaluateStructures(const std::vector<std::vector<float>>& vectorOfFeatureVectors) const {
			std::lock_guard<std::mutex> lock(mutex);
            torch::NoGradGuard noGrad;
            std::vector<torch::Tensor> vectorOfTensorsOfFeatureVectors;
            c10::TensorOptions tensorOptions = torch::TensorOptions().device(device).dtype(torch::kFloat32);
            for (const std::vector<float>& featureVector : vectorOfFeatureVectors) {
                torch::Tensor tensorOfFeatureVector = torch::tensor(featureVector, tensorOptions);
                vectorOfTensorsOfFeatureVectors.push_back(tensorOfFeatureVector);
            }
            torch::Tensor inputTensor = torch::stack(vectorOfTensorsOfFeatureVectors);
            std::vector<torch::Tensor> vectorOfOutputTensors = neuralNetwork->forward(inputTensor);
            int dimension = 1;
            torch::Tensor tensorOfValues = vectorOfOutputTensors[0].cpu().squeeze(dimension);
            torch::Tensor tensorOfPolicies = vectorOfOutputTensors[1].cpu().squeeze(dimension);
            std::vector<float> vectorOfValues(tensorOfValues.data_ptr<float>(), tensorOfValues.data_ptr<float>() + tensorOfValues.numel());
            std::vector<float> vectorOfPolicies(tensorOfPolicies.data_ptr<float>(), tensorOfPolicies.data_ptr<float>() + tensorOfPolicies.numel());
            std::vector<std::pair<double, double>> vectorOfPairsOfValuesAndPolicies;
            for (size_t i = 0; i < vectorOfValues.size(); i++) {
                double value = static_cast<double>(vectorOfValues[i]);
                // TODO: Consider whether double values should be float.
                double policy = static_cast<double>(vectorOfPolicies[i]);
                std::pair<double, double> pairOfValueAndPolicy = { value, policy };
                vectorOfPairsOfValuesAndPolicies.push_back(pairOfValueAndPolicy);
            }
            return vectorOfPairsOfValuesAndPolicies;
        }

        void reloadIfUpdated() {
			std::lock_guard<std::mutex> lock(mutex);

            auto currentWriteTime = std::filesystem::last_write_time(pathToFileOfParameters);
            if (currentWriteTime <= lastWriteTime) {
                return;
            }

            try {
                std::vector<torch::Tensor> vectorOfParameters;
                torch::load(vectorOfParameters, pathToFileOfParameters);
                torch::autograd::variable_list variableListOfParameters = neuralNetwork->parameters();
                if (variableListOfParameters.size() != vectorOfParameters.size()) {
                    throw std::runtime_error("Numbers of parameters were mismatched during reload.");
                }
                torch::NoGradGuard noGrad;
                for (size_t i = 0; i < vectorOfParameters.size(); i++) {
                    variableListOfParameters[i].data().copy_(vectorOfParameters[i].data());
                }
                lastWriteTime = currentWriteTime;
                Logger::info("Model was reloaded after updated model parameters were detected.");
            }
            catch (const std::exception& e) {
                Logger::error("reloadIfUpdated", e);
                throw e;
            }
        }
    };

}