#pragma once;


#include "../../db/database.hpp"
#include "../neural_network.hpp"
#include "node.hpp"
#include "../../game/phase_state_machine.hpp"


/* For settlement and city moves, function `expandNode` queries the board for available vertices
* based on the current state of the board stored in the database.
*/
void expandNode(const std::shared_ptr<MCTSNode>& node, Database& db, SettlersNeuralNet& neuralNet) {
	// Determine available moves based on the type of move.
	std::vector<std::string> availableMoves;
	if (node->moveType == "settlement" || node->moveType == "city") {
		// Get the current list of occupied vertices from the database.
		std::vector<std::string> occupiedVertices = getOccupiedVertices(db);
		availableMoves = getAvailableVertices(occupiedVertices);
	}
	else if (node->moveType == "road") {
		// We use placeholder dummy moves.
		// TODO: Use simular board logic for road moves (e.g., function `getAvailableRoadMoves`).
		availableMoves = { "E01", "E02" };
	}
	// Create a child node for each available move.
	for (const auto& move : availableMoves) {
		if (node->children.find(move) == node->children.end()) {
			auto child = std::make_shared<MCTSNode>(node->gameState, move, node, node->moveType);
			if (node->moveType == "settlement" || node->moveType == "city") {
				// TODO: Construct feature vector from board geometry.
				std::vector<float> features = { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
				// Evaluate the move with the neural network.
				auto eval = neuralNet.evaluateSettlement(features);
				// TODO: If `node->moveType == "city"`, use function `neuralNet.evaluateCity` or `neuralNet.evaluateBuilding`.
				// Use the network's policy output as the prior probability.
				child->P = eval.second;
			}
			else {
				// Use a default dummy prior for other move types.
				// TODO: Consider using an evaluated prior.
				child->P = 0.5;
			}
			node->children[move] = child;
		}
	}
}