#pragma once


#include "../../db/database.hpp"
#include "../neural_network.hpp"
#include "node.hpp"
#include "../../game/board.hpp"


/* Function `expandNode`, for each move determined to be available by board geometry and current state,
* creates a child node and sets its prior probability based on the move type.
*/
void expandNode(const std::shared_ptr<MCTSNode>& node, Database& db, SettlersNeuralNet& neuralNet) {
	// Determine available moves based on the type of move.
	std::vector<std::string> availableMoves;
	if (node->moveType == "settlement" || node->moveType == "city") {
		// Get the current list of occupied vertices from the database and determine available vertices.
		std::vector<std::string> occupiedVertices = getOccupiedVertices(db);
		availableMoves = getAvailableVertices(occupiedVertices);
	}
	else if (node->moveType == "road") {
		std::vector<std::string> vectorOfOccupiedEdges = getVectorOfKeysOfOccupiedEdges(db);
		availableMoves = getVectorOfKeysOfAvailableEdges(vectorOfOccupiedEdges);
	}
	// Create a child node for each available move.
	for (const auto& move : availableMoves) {
		if (node->children.find(move) != node->children.end()) {
			continue;
		}
		// Create a child node corresponding to this move.
		auto child = std::make_shared<MCTSNode>(node->gameState, move, nullptr, node->moveType);
		child->parent = node;
		double prior = 0.5;

		// Call the appropriate neural network evaluation depending on move type.
		if (node->moveType == "settlement") {
			// Evaluate the move with the neural network.
			auto eval = neuralNet.evaluateBuildingFromVertex(move);
			// Use the network's policy output as the prior probability.
			prior = eval.second;
		} else if (node->moveType == "city") {
			auto eval = neuralNet.evaluateBuildingFromVertex(move);
			prior = eval.second;
		}
		else if (node->moveType == "road") {
			std::string labelOfVertexOfLastBuilding = (node->parent.lock() ? node->parent.lock()->move : "");
			if (!labelOfVertexOfLastBuilding.empty()) {
				auto eval = neuralNet.evaluateRoadFromEdge(labelOfVertexOfLastBuilding, move);
				prior = eval.second;
			}
		}
		child->P = prior;
		node->children[move] = child;
	}
}