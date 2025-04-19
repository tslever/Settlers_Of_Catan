#pragma once


#include <string>
#include <unordered_map>
#include <memory>
#include "../../game/game_state.hpp"


namespace AI {
    namespace MCTS {

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
            std::unordered_map<std::string, std::unique_ptr<MCTSNode>> unorderedMapOfMovesToChildren;
            MCTSNode* parent;

            MCTSNode(
                const GameState& gameState,
                const std::string& move = "",
                MCTSNode* parent = nullptr,
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
                return unorderedMapOfMovesToChildren.empty();
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
				for (const auto& [representationOfMove, child] : unorderedMapOfMovesToChildren) {
					jsonArrayOfChildren[i++] = child->index;
				}
                json["children"] = std::move(jsonArrayOfChildren);
				json["parent"] = parent ? std::to_string(parent->index) : "null";
                return json;
            }
        };

    }
}