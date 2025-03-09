#pragma once

#include <torch/script.h>
// Add to Additional Include Directories `$(SolutionDir)\dependencies\libtorch\include;`.

#include <vector>

#include <utility>

#include <string>


class SettlersNeuralNet {
public:
    torch::jit::script::Module module;

    SettlersNeuralNet(const std::string& modelPath) {
        try {
            module = torch::jit::load(modelPath);
            module.to(torch::kCPU);
            module.eval();
        }
        catch (const c10::Error& e) {
            throw std::runtime_error("Error loading model");
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
};