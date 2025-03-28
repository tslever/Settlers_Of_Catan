#pragma once


#include "../db/database.hpp"
#include "mcts/backpropagation.hpp"
#include "mcts/expansion.hpp"
#include "neural_network.hpp"
#include "mcts/selection.hpp"
#include "mcts/simulation.hpp"


void injectDirichletNoise(std::shared_ptr<MCTSNode>& root, double mixingWeight, double shape) {
	if (root->children.empty()) {
		return;
	}
	std::vector<double> vectorOfNoise;
	std::random_device::result_type resultType = std::random_device{}();
	std::default_random_engine defaultRandomEngine(resultType);
	double scale = 1.0;
	std::gamma_distribution<double> gammaDistribution(shape, scale);

	double sumOfNoise = 0.0;
	for (size_t i = 0; i < root->children.size(); i++) {
		double noise = gammaDistribution(defaultRandomEngine);
		vectorOfNoise.push_back(noise);
		sumOfNoise += noise;
	}

	for (double& noise : vectorOfNoise) {
		noise /= sumOfNoise;
	}

	size_t index = 0;
	for (std::pair<const std::string, std::shared_ptr<MCTSNode>>& pairOfLabelOfVertexOrEdgeKeyAndNode : root->children) {
		std::shared_ptr<MCTSNode> child = pairOfLabelOfVertexOrEdgeKeyAndNode.second;
		// Adjust prior probability using weighted mix of original prior probability and injected noise.
		child->P = (1 - mixingWeight) * child->P + mixingWeight * vectorOfNoise[index++];
	}
}


/* Function `runMcts` runs MCTS by creating a root node from the current game state,
* running a number of simulations, and returning the best move.
*/
std::pair<std::string, int> runMcts(
	GameState& currentState,
	Database& db,
	SettlersNeuralNet& neuralNet,
	int numberOfSimulations,
	double cPuct,
	double tolerance
) {
	// Create root node for current phase.
	std::string moveType = "";
	if (currentState.phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
		moveType = "settlement";
	}
	else if (currentState.phase == Phase::TO_PLACE_FIRST_ROAD || currentState.phase == Phase::TO_PLACE_SECOND_ROAD) {
		moveType = "road";
	}
	else if (currentState.phase == Phase::TO_PLACE_FIRST_CITY) {
		moveType = "city";
	}
	else if (currentState.phase == Phase::TURN) {
		moveType = "turn";
	}
	std::shared_ptr<MCTSNode> root = std::make_shared<MCTSNode>(currentState, "", nullptr, moveType);

	// Initially expand the root based on available moves.
	expandNode(root, db, neuralNet);

	// Inject Dirichlet noise at the root to encourage exploration.
	injectDirichletNoise(root, 0.25, 0.03);

	// Run MCTS simulations.
	for (int i = 0; i < numberOfSimulations; i++) {
		std::shared_ptr<MCTSNode> node = root;
		// Selection: Descend / traverse down the tree until a leaf node is reached.
		while (!node->isLeaf()) {
			node = selectChild(node, cPuct, tolerance);
		}
		// Expansion: If this leaf node was visited before, expand the node.
		if (node->N > 0) {
			expandNode(node, db, neuralNet);
		}
		// Simulation: Evaluate the leaf node via by simulating rollout.
		// TODO: Consider extending to multi-step rollouts.
		double value = simulateRollout(node, db, neuralNet);
		// Backpropagation: Update statistics up tree / all nodes along path.
		backpropagate(node, value);
	}

	// Select the child move with the highest visit count.
	// TODO: Consider whether selecting moves based on visit count, evaluation scores, and/or other criteria might be better.
	std::shared_ptr<MCTSNode> bestChild = nullptr;
	int numberOfVisitsToBestChild = -1;
	for (const std::pair<const std::string, std::shared_ptr<MCTSNode>>& pairOfLabelOfVertexOrEdgeKeyAndNode : root->children) {
		std::shared_ptr<MCTSNode> child = pairOfLabelOfVertexOrEdgeKeyAndNode.second;
		if (child->N > numberOfVisitsToBestChild) {
			numberOfVisitsToBestChild = child->N;
			bestChild = child;
		}
	}
	if (bestChild) {
		return { bestChild->move, bestChild->N };
	}
	return { "", 0 };
}