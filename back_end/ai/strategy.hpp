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
	WrapperOfNeuralNetwork& neuralNet,
	int numberOfSimulations,
	double cPuct,
	double tolerance
) {
	//std::clog << "        [MCTS] MCTS is being started." << std::endl;

	MCTSNode::nextIndex = 0;

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

	//std::clog << "            [EXPAND ROOT] The root is being expanded." << std::endl;
	//std::clog << "            [EXPAND ROOT] The following root is being expanded.\n            " << root->toJson().dump() << std::endl;
	expandNode(root, db, neuralNet);
	//std::clog << "            [EXPAND ROOT] The root was expanded into the following.\n            " << root->toJson().dump() << std::endl;

	// Inject Dirichlet noise at the root to encourage exploration by changing the prior probabilities of the children of the root.
	injectDirichletNoise(root, 0.25, 0.03);

	for (int i = 0; i < numberOfSimulations; i++) {
		//std::clog << "            [MCTS SIMULATION] MCTS simulation " << i + 1 << " of " << numberOfSimulations << " is running." << std::endl;
		//std::clog << "                [MCTS SIMULATION] Node node is being set to root." << std::endl;
		std::shared_ptr<MCTSNode> node = root;
		//if (node->isLeaf()) {
		//	std::clog << "                [SELECT NO NODE] Node node is leaf and will not be reset to a child." << std::endl;
		//}
		while (!node->isLeaf()) {
			node = selectChild(node, cPuct, tolerance);
			//std::clog << "                [SELECT NODE] Node node was not leaf and was reset to a child." << std::endl;
			//std::clog << "                [SELECT NODE] Node node was not leaf and was reset to the following child.\n                " << node->toJson().dump() << std::endl;
		}
		//if (node->N > 0) {
		//	std::clog << "                [EXPAND NO NODE] Node node was visited before and was not expanded." << std::endl;
		//}
		//else {
		if (node->N == 0) {
			expandNode(node, db, neuralNet);
			//std::clog << "                [EXPAND NODE] Node node was not visited before and was expanded." << std::endl;
			//std::clog << "                [EXPAND NODE] Node node was not visited before and was expanded into the following.\n                " << node->toJson().dump() << std::endl;
		}
		double value = rollout(node, db, neuralNet);
		//std::clog << "                [ROLLOUT] The value of node node is " << value << "." << std::endl;
		backpropagate(node, value);
		//std::clog << "                [BACKPROPAGATION] Statistics of node node and all parents were updated." << std::endl;
	}

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
		//std::clog << "            [SELECT BEST CHILD] The best child has move " << bestChild->move << "." << std::endl;
		//std::clog << "            [SELECT BEST CHILD] The child of the root with the highest visit count is the following.\n" << bestChild->toJson().dump() << std::endl;
		return { bestChild->move, bestChild->N };
	}
	throw std::runtime_error("Best child is not defined.");
	//return { "", 0 };
}