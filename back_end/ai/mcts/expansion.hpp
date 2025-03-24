#pragma once


#include "../../db/database.hpp"
#include "../neural_network.hpp"
#include "node.hpp"
#include "../../game/phase_state_machine.hpp"


// Forward declarations so that functions `getOccupiedVertices` and `getAvailableVertices` are found.
// TODO: Consider eliminating forward declarations by decoupling.
std::vector<std::string> getOccupiedVertices(Database& db);
std::vector<std::string> getAvailableVertices(const std::vector<std::string>& occupiedVertices);


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
		// We use placeholder dummy moves.
		// TODO: Use board logic for road moves (e.g., function `getAvailableRoadMoves`).
		availableMoves = { "E01", "E02" };
	}
	// Create a child node for each available move.
	for (const auto& move : availableMoves) {
		if (node->children.find(move) != node->children.end()) {
			continue;
		}
		// Create a child node corresponding to this move.
		auto child = std::make_shared<MCTSNode>(node->gameState, move, node, node->moveType);
		double prior = 0.5;

		// Call the appropriate neural network evaluation depending on move type.
		if (node->moveType == "settlement") {
			// Construct features for building a settlement.
			// TODO: Consider using a function like `board.getVertexFeatures` to compute features for building a settlement.
			std::vector<float> features = { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
			// Evaluate the move with the neural network.
			auto eval = neuralNet.evaluateSettlement(features);
			// Use the network's policy output as the prior probability.
			prior = eval.second;
		} else if (node->moveType == "city") {
			// Construct features for building a city.
			// TODO: Consider using a function like `board.getVertexFeatures` to compute features for building a city.
			std::vector<float> features = { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
			auto eval = neuralNet.evaluateCity(features);
			// Use the network's policy output as the prior probability.
			prior = eval.second;
		}
		else if (node->moveType == "road") {
			// Construct features for building a road.
			// TODO: Consider using a function like `board.getEdgeFeatures` to compute features for building a road.
			std::vector<float> features = { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
			auto eval = neuralNet.evaluateRoad(features);
			prior = eval.second;
		}
		child->P = prior;
		node->children[move] = child;
	}
}