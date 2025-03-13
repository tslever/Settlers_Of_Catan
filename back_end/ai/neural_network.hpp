#pragma once

#include <torch/script.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include;`.
// Add to Additional Library Directories `$(SolutionDir)\dependencies\libtorch\lib;`.
// Add to Additional Dependencies `c10.lib;`
// Add to Additional Dependencies `torch_cpu.lib;`.
// TODO: Consider adding to Additional Dependencies a library when allowing use of GPU if GPU is available.
/* TODO: Add the following string of newline separated commands to Post - Build Event.
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\c10.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\torch_cpu.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\fbgemm.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\libiomp5md.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\uv.dll" "$(OutDir)"
xcopy /Y /I "$(SolutionDir)\dependencies\libtorch\lib\asmjit.dll" "$(OutDir)"
*/

#include <torch/torch.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include\torch\csrc\api\include;`.

#include <tracer.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include\torch\csrc\jit\frontend;`.

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

// Create a default model by instantiating the network, tracing it, and returning a scripted module.
torch::jit::script::Module createDefaultModel() {
    // Use default dimensions.
    // TODO: Read from configuration file.
    const int64_t input_dim = 5;
    const int64_t hidden_dim = 128;

    // Create an instance of the default model.
    auto model = SettlersPolicyValueNet(input_dim, hidden_dim);
    model->eval(); // Set the model to evaluation mode.

    // Create a dummy input tensor of shape {1, input_dim} for tracing.
    torch::Tensor dummy_input = torch::zeros({ 1, input_dim });
    // Convert dummy input into a vector of `IValue` (i.e., a Stack).
    torch::jit::Stack inputs;
    inputs.push_back(dummy_input);

    // Define a lambda that wraps the model's forward pass.
    std::function<torch::jit::Stack(const torch::jit::Stack&)> fn =
        [model](const torch::jit::Stack& stack) -> torch::jit::Stack {
        // Extract the input tensor from the stack.
        torch::Tensor x = stack[0].toTensor();
        // Call the model's forward pass.
        auto outputs = model->forward(x);
        // Package the outputs (value, policy) into a Stack.
        torch::jit::Stack result;
        for (const auto& t : outputs) {
            result.push_back(t);
        }
        return result;
    };

    // Define a dummy function for tracing.
    std::function<std::string(const torch::Tensor&)> var_fn =
        [](const torch::Tensor& x) { return ""; };

    torch::jit::script::Module traced_module("traced_module");

    // Trace the model using the given inputs, lambda, and additional parameters.
    torch::jit::tracer::trace(inputs, fn, var_fn, true, false, &traced_module);
    // Return the traced scripted module.
    return traced_module;
}

class SettlersNeuralNet {
private:
    std::string modelPath;
    std::filesystem::file_time_type lastModified;
public:
    torch::jit::script::Module module;

    // Constructor `SettlersNeuralNet` loads model and stores time model was modified.
    SettlersNeuralNet(const std::string& modelPath) : modelPath(modelPath) {
        // Check if a model exists before attempting to load it.
        // TODO: Consider using a default model and creating a model file after first training if the model file doesn't exist.
        if (!std::filesystem::exists(modelPath)) {
            std::clog << "[WARNING] Model file not found at " << modelPath << ". Creating a default model file." << std::endl;
            // Create a default model.
            torch::jit::script::Module defaultModel = createDefaultModel();
            try {
                // Save the default model to disk so that subsequent launches will load the default model.
                defaultModel.save(modelPath);
                std::clog << "[INFO] Default model was created and saved to " << modelPath << "." << std::endl;
            }
            catch (const c10::Error& e) {
                std::cerr << "[ERROR] Saving default model failed with the following error. " << e.what() << std::endl;
                throw std::runtime_error("Saving default model to " + modelPath + "failed.");
            }
        }
        try {
            // Attempt to load model.
            module = torch::jit::load(modelPath);
            // Determine the device by attempting to allocate a dummy tensor on CUDA.
            // TODO: Consider whether a more elegant way of determining whether GPU is available exists.
            torch::Device device(torch::kCPU);
            try {
                auto dummy = torch::zeros({ 1 }, torch::device(torch::kCUDA));
                device = torch::kCUDA;
            }
            catch (const c10::Error&) {
                // If CUDA allocation fails, remain on CPU.
                device = torch::kCPU;
            }
            module.to(device);
            module.eval();
            // Record the last modified time for future reloads.
            lastModified = std::filesystem::last_write_time(modelPath);
            std::clog << "[INFO] Model was successfully loaded from " << modelPath << " on device " << (device == torch::kCUDA ? "CUDA" : "CPU") << std::endl;
        }
        catch (const c10::Error& e) {
            std::cerr << "[ERROR] The following exception occurred while loading model. " << e.what() << std::endl;
            throw std::runtime_error("An exception occurred while loading model from the following path. " + modelPath);
        }
    }

    // Evaluate a settlement move given a feature vector.
    std::pair<double, double> evaluateSettlement(const std::vector<float>& features) {
        torch::Tensor input = torch::tensor(features).unsqueeze(0);
        auto output = module.forward({ input }).toList();
        double value = output.get(0).toTensor().item<double>();
        double policy = output.get(1).toTensor().item<double>();
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
                module.to(torch::kCPU);
                // TODO: Allow use of GPU if GPU is available.
                module.eval();
                lastModified = currentModTime;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ERROR] The following exception in `reloadIfUpdated` occurred. " << e.what() << std::endl;
        }
    }
};