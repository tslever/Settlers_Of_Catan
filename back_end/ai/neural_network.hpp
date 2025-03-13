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

class SettlersNeuralNet {
private:
    std::string modelPath;
    std::filesystem::file_time_type lastModified;
public:
    torch::jit::script::Module module;

    // Constructor `SettlersNeuralNet` loads model and stores time model was modified.
    SettlersNeuralNet(const std::string& modelPath) : modelPath(modelPath) {
        try {
            module = torch::jit::load(modelPath);
            module.to(torch::kCPU);
            // TODO: Allow use of GPU if GPU is available.
            module.eval();
        }
        catch (const c10::Error& e) {
            throw std::runtime_error("Error loading model");
            /* TODO: Address the following error that may have to do with no model existing.
            * Unhandled exception at 0x00007FFB430CAB6A in back_end.exe: Microsoft C++ exception: std::runtime_error at memory location 0x0000002793BFDC38.
            */
        }
    }

    // Evaluate a settlement move given a feature vector.
    std::pair<double, double> evaluateSettlement(const std::vector<float>& features) {
        torch::Tensor input = torch::tensor(features).unsqueeze(0);
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