#pragma once


#include <string>
#include <unordered_map>
#include <memory>
#include "../../game/game_state.hpp"


// Class MCTS is a template for an MCTS node, which represents a game state reached by making a particular move.
class MCTSNode {
public:
    inline static int nextIndex = 0;
    int index;
    GameState gameState;
    std::string move; // e.g. vertex label or edge key
    std::string moveType; // "city", "settlement", or "road"
    int N; // visit count
    double W; // total value
    double Q; // mean value = W / N
    double P; // prior probability
    std::unordered_map<std::string, std::shared_ptr<MCTSNode>> children;
    std::weak_ptr<MCTSNode> parent;

    MCTSNode(
        const GameState& state,
        const std::string& move = "",
        std::shared_ptr<MCTSNode> parent = nullptr,
        const std::string& moveType = ""
    ) : index(nextIndex++), gameState(state), move(move), moveType(moveType), parent(parent), N(0), W(0.0), Q(0.0), P(0.0)
    {
        // Do nothing.
    }

    bool isLeaf() const {
        return children.empty();
    }
};