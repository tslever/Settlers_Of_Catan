#pragma once

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
* Add the following string of newline separated commands to Post - Build Event.
* xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\c10.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\torch_cpu.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\fbgemm.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\libiomp5md.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\uv.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\asmjit.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\torch_cuda.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\c10_cuda.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\cudnn64_9.dll" "$(OutDir)"
*/

#include <torch/torch.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include\torch\csrc\api\include;`.

/* TODO: Try again to get `tracer.trace` working to avoid the below implementation of function `createDefaultModel`.
* #include <tracer.h>
* Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include\torch\csrc\jit\frontend;`.
*/


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

// Create a default model by instantiating the network, scripting it, and returning a `TorchScript` module.
torch::jit::script::Module createDefaultModel() {
    // Use default dimensions.
    // TODO: Read from configuration file.
    const int64_t input_dim = 5;
    const int64_t hidden_dim = 128;

    // Create an instance of the default model.
    auto model = SettlersPolicyValueNet(input_dim, hidden_dim);
    model->eval(); // Set the model to evaluation mode.

    // Create a new TorchScript module.
    torch::jit::script::Module script_module("SettlersPolicyValueNet");

    // Manually register the parameters from the model's linear layers.
    script_module.register_parameter("fc1_weight", model->fc1->weight, /*is_buffer=*/false);
    script_module.register_parameter("fc1_bias", model->fc1->bias,   /*is_buffer=*/false);
    script_module.register_parameter("fc2_weight", model->fc2->weight, /*is_buffer=*/false);
    script_module.register_parameter("fc2_bias", model->fc2->bias,   /*is_buffer=*/false);
    script_module.register_parameter("fc_value_weight", model->fc_value->weight, /*is_buffer=*/false);
    script_module.register_parameter("fc_value_bias", model->fc_value->bias,   /*is_buffer=*/false);
    script_module.register_parameter("fc_policy_weight", model->fc_policy->weight, /*is_buffer=*/false);
    script_module.register_parameter("fc_policy_bias", model->fc_policy->bias,   /*is_buffer=*/false);

    // Define the forward method in TorchScript.
    // We mimic the linear layers with torch.addmm: out = bias + x * weight.t()
    std::string method = R"JIT(
def forward(self, x):
    x = torch.relu(torch.addmm(self.fc1_bias, x, self.fc1_weight.t()))
    x = torch.relu(torch.addmm(self.fc2_bias, x, self.fc2_weight.t()))
    value = torch.tanh(torch.addmm(self.fc_value_bias, x, self.fc_value_weight.t()))
    policy = torch.sigmoid(torch.addmm(self.fc_policy_bias, x, self.fc_policy_weight.t()))
    return value, policy
)JIT";

    script_module.define(method);
    return script_module;
}

class SettlersNeuralNet {
private:
    std::string modelPath;
    std::filesystem::file_time_type lastModified;
    torch::Device device;
public:
    torch::jit::script::Module module;

    // Constructor `SettlersNeuralNet` loads model from disk or creates a default one if missing.
    SettlersNeuralNet(const std::string& modelPath) : modelPath(modelPath), device(torch::kCPU) {
        // If model doesn't exist, create default model.
        // TODO: Train model.
        if (!std::filesystem::exists(modelPath)) {
            std::clog << "[WARNING] Model file not found at " << modelPath << ". Creating a default model file." << std::endl;
            torch::jit::script::Module defaultModel = createDefaultModel();
            std::filesystem::path modelFilePath(modelPath);
            std::filesystem::path parentDir = modelFilePath.parent_path();
            if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
                std::filesystem::create_directories(parentDir);
                std::clog << "[INFO] Directory " << parentDir << " was created." << std::endl;
            }

            try {
                defaultModel.save(modelPath);
                std::clog << "[INFO] Default model was created and saved to " << modelPath << "." << std::endl;
            }
            catch (const c10::Error& e) {
                std::cerr << "[ERROR] Saving default model failed with the following error. " << e.what() << std::endl;
                throw std::runtime_error("Saving default model to " + modelPath + "failed.");
            }
        }
        try {
            std::clog << "[INFO] CUDA " << (torch::cuda::is_available() ? "is" : "is not") << " available." << std::endl;
            device = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
            module = torch::jit::load(modelPath);
            module.to(device);
            module.eval();
            lastModified = std::filesystem::last_write_time(modelPath);
            std::clog << "[INFO] Model was successfully loaded from " << modelPath << " on device " << (device == torch::kCUDA ? "CUDA" : "CPU") << "." << std::endl;
        }
        catch (const c10::Error& e) {
            std::cerr << "[ERROR] The following exception occurred while loading model. " << e.what() << std::endl;
            throw std::runtime_error("An exception occurred while loading model from the following path. " + modelPath);
        }
    }

    // Evaluate a settlement move given a feature vector.
    std::pair<double, double> evaluateSettlement(const std::vector<float>& features) {
        torch::Tensor input = torch::tensor(features, torch::TensorOptions().device(device)).unsqueeze(0);
        auto output = module.forward({ input }).toTuple();
        double value = output->elements()[0].toTensor().item<double>();
        double policy = output->elements()[1].toTensor().item<double>();
        return { value, policy };
    }
    // TODO: Similarly implement evaluateCity, evaluateRoad, etc.

    // Reload model if weights have been updated on disk.
    void reloadIfUpdated() {
        try {
            auto currentModTime = std::filesystem::last_write_time(modelPath);
            if (currentModTime > lastModified) {
                std::clog << "[INFO] Updated model weights were detected. Model will be reloaded from " << modelPath << std::endl;
                module = torch::jit::load(modelPath);
                module.to(device);
                module.eval();
                lastModified = currentModTime;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] The following exception in `reloadIfUpdated` occurred. " << e.what() << std::endl;
        }
    }
};