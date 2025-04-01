#pragma once


#include <string>
#include <unordered_map>
#include <memory>
#include "../../game/game_state.hpp"


// Class MCTS is a template for an MCTS node.
// A root node represents a starting game state and the type of next moves.
// A child node represents a game state reached by making a specified move of a specified type.
class MCTSNode {
public:
    inline static int nextIndex = 0;
    int index;
    GameState gameState;
    std::string move;
    std::string moveType;
    int visitCount;
    double totalValue;
    double averageValue;
    double priorProbability;
    std::unordered_map<std::string, std::shared_ptr<MCTSNode>> unorderedMapOfRepresentationsOfMovesToChildren;
    std::weak_ptr<MCTSNode> parent;

    MCTSNode(
        const GameState& gameState,
        const std::string& move = "",
        std::shared_ptr<MCTSNode> parent = nullptr,
        const std::string& moveType = ""
    ) :
        index(nextIndex++),
        gameState(gameState),
        move(move),
        moveType(moveType),
        parent(parent),
        visitCount(0),
        totalValue(0.0),
        averageValue(0.0),
        priorProbability(0.0)
    {
        // Do nothing.
    }

    bool isLeaf() const {
        return unorderedMapOfRepresentationsOfMovesToChildren.empty();
    }

    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["index"] = index;
        json["gameState"] = gameState.toJson();
        json["move"] = move;
        json["moveType"] = moveType;
        json["visitCount"] = visitCount;
        json["totalValue"] = totalValue;
        json["averageValue"] = averageValue;
        json["priorProbability"] = priorProbability;
        
        crow::json::wvalue jsonArrayOfChildren(crow::json::type::List);
        int i = 0;
        for (std::pair<const std::string, std::shared_ptr<MCTSNode>> pairOfRepresentationOfMoveAndChild : unorderedMapOfRepresentationsOfMovesToChildren) {
            std::shared_ptr<MCTSNode> child = pairOfRepresentationOfMoveAndChild.second;
            jsonArrayOfChildren[i++] = child->index;
        }
        json["children"] = std::move(jsonArrayOfChildren);

        if (std::shared_ptr<MCTSNode> sharedPointerToParent = parent.lock()) {
            json["parent"] = sharedPointerToParent->index;
        }
        else {
            json["parent"] = nullptr;
        }
        return json;
    }
};