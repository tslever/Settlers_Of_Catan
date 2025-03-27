#pragma once


#include "../db/database.hpp"
#include "mcts/backpropagation.hpp"
#include "mcts/expansion.hpp"
#include "neural_network.hpp"
#include "mcts/selection.hpp"
#include "mcts/simulation.hpp"


void injectDirichletNoise(std::shared_ptr<MCTSNode>& root, double epsilon, double alpha) {
	if (root->children.empty()) {
		return;
	}
	// Prepare noise: Sample gamma variables and normalize them.
	std::vector<double> noise;
	std::default_random_engine generator(std::random_device{}());
	std::gamma_distribution<double> gammaDist(alpha, 1.0);
	double sum = 0.0;
	for (size_t i = 0; i < root->children.size(); i++) {
		double n = gammaDist(generator);
		noise.push_back(n);
		sum += n;
	}
	for (auto& n : noise) {
		n /= sum;
	}
	size_t index = 0;
	for (auto& childPair : root->children) {
		auto child = childPair.second;
		// Adjust the prior using a weighted mix of the original prior and the injected noise.
		child->P = (1 - epsilon) * child->P + epsilon * noise[index++];
	}
}


/* Function `runMcts` runs MCTS by creating a root node from the current game state,
* running a number of simulations, and returning the best move.
*/
// TODO: Consider whether function `runMcts` belongs in another file.
std::pair<std::string, int> runMcts(
	GameState& currentState,
	Database& db,
	SettlersNeuralNet& neuralNet,
	int numberOfSimulations,
	double c_puct,
	double tolerance
) {
	// Create root node for the current phase.
	// TODO: Avoid assuming settlement phase.
	auto root = std::make_shared<MCTSNode>(currentState, "", nullptr, "settlement");

	// Initially expand the root node based on available moves.
	expandNode(root, db, neuralNet);

	// Inject Dirichlet noise at the root to encourage exploration.
	injectDirichletNoise(root, 0.25, 0.03);

	// Run MCTS simulations.
	for (int i = 0; i < numberOfSimulations; i++) {
		auto node = root;
		// Selection: Descend / traverse down the tree until a leaf node is reached.
		while (!node->isLeaf()) {
			node = selectChild(node, c_puct, tolerance);
		}
		// Expansion: If this leaf node was visited before, expand the node.
		if (node->N > 0) {
			expandNode(node, db, neuralNet);
		}
		// Simulation: Evaluate the leaf node via a rollout.
		// TODO: Consider extending to multi-step rollouts.
		double value = simulateRollout(node, db, neuralNet);
		// Backpropagation: Update statistics up the tree / all nodes along the path.
		backpropagate(node, value);
	}

	// Select the child move with the highest visit count.
	// TODO: Consider whether selecting moves based on visit count, evaluation scores, and/or other criteria might be better.
	std::shared_ptr<MCTSNode> bestChild = nullptr;
	int bestVisits = -1;
	for (const auto& pair : root->children) {
		auto child = pair.second;
		if (child->N > bestVisits) {
			bestVisits = child->N;
			bestChild = child;
		}
	}
	if (bestChild) {
		return { bestChild->move, bestChild->N };
	}
	return { "", 0 };
}