// TODO: Integrate into back end.

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "../../game/game_state.hpp"


class MCTSNode {
public:
    GameState gameState;
    std::string move; // e.g. vertex label or edge key
    std::string moveType; // "settlement", "road", etc.
    int N; // visit count
    double W; // total value
    double Q; // mean value = W / N
    double P; // prior probability
    std::unordered_map<std::string, std::shared_ptr<MCTSNode>> children;
    std::shared_ptr<MCTSNode> parent;

    MCTSNode(const GameState& state,
        const std::string& move = "",
        std::shared_ptr<MCTSNode> parent = nullptr,
        const std::string& moveType = "")
        : gameState(state), move(move), moveType(moveType), parent(parent),
        N(0), W(0.0), Q(0.0), P(0.0)
    {
    }

    bool isLeaf() const {
        return children.empty();
    }
};

// Backpropagation: propagate the rollout value up the tree.
void backpropagate(std::shared_ptr<MCTSNode> node, double value) {
    while (node != nullptr) {
        node->N += 1;
        node->W += value;
        node->Q = node->W / node->N;
        node = node->parent;
    }
}

// TODO: Define selection, expansion and simulation functions similarly.