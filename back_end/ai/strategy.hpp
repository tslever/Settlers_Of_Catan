#pragma once


#include "../db/database.hpp"
#include "mcts/backpropagation.hpp"
#include "mcts/expansion.hpp"
#include "neural_network.hpp"
#include "mcts/selection.hpp"
#include "mcts/simulation.hpp"


/* Function `injectDirichletNoise` injects Dirichlet noise at the root to encourage exploration
* by changing the prior probabilities of the children of the root.
*/
void injectDirichletNoise(AI::MCTS::MCTSNode* root, double mixingWeight, double shape) {
	if (root->unorderedMapOfMovesToChildren.empty()) {
		return;
	}
	std::vector<double> vectorOfNoise;
	std::random_device::result_type randomDevice = std::random_device{}();
	std::default_random_engine defaultRandomEngine(randomDevice);
	double scale = 1.0;
	std::gamma_distribution<double> gammaDistribution(shape, scale);

	double sumOfNoise = 0.0;
	for (const auto& pair : root->unorderedMapOfMovesToChildren) {
		double noise = gammaDistribution(defaultRandomEngine);
		vectorOfNoise.push_back(noise);
		sumOfNoise += noise;
	}

	// Convert raw noise values into probabilities.
	for (double& noise : vectorOfNoise) {
		noise /= sumOfNoise;
	}

	size_t index = 0;
	for (auto& [move, child] : root->unorderedMapOfMovesToChildren) {
		// Adjust prior probability using weighted mix of original prior probability and injected noise.
		child->priorProbability = (1 - mixingWeight) * child->priorProbability + mixingWeight * vectorOfNoise[index++];
	}
}


bool comparePairsOfMovesAndChildren(
	const std::pair<const std::string, std::unique_ptr<AI::MCTS::MCTSNode>>& firstPairOfMoveAndChild,
	const std::pair<const std::string, std::unique_ptr<AI::MCTS::MCTSNode>>& secondPairOfMoveAndChild
) {
	const auto& [move1, child1] = firstPairOfMoveAndChild;
	const auto& [move2, child2] = secondPairOfMoveAndChild;
	if (child1->visitCount < child2->visitCount) {
		return true;
	}
	else if (child1->visitCount == child2->visitCount && child1->priorProbability < child2->priorProbability) {
		return true;
	}
	return false;
}


/* Function `runMcts` runs MCTS by creating a root node from the current game state,
* running a number of simulations, and returning the best move.
*/
std::tuple<std::string, std::string, int> runMcts(
	const GameState& currentState,
	AI::WrapperOfNeuralNetwork& neuralNet,
	int numberOfSimulations,
	double cPuct,
	double tolerance,
	double dirichletMixingWeight,
	double dirichletShape
) {
	//Logger::info("        [MCTS] MCTS is being started.");
	AI::MCTS::MCTSNode::nextIndex = 0;

	// Create root node for current phase.
	std::string moveType = "";
	if (currentState.phase == Game::Phase::FirstSettlement) {
		moveType = "settlement";
	}
	else if (currentState.phase == Game::Phase::FirstRoad || currentState.phase == Game::Phase::SecondRoad) {
		moveType = "road";
	}
	else if (currentState.phase == Game::Phase::FirstCity) {
		moveType = "city";
	}
	else if (currentState.phase == Game::Phase::Turn) {
		moveType = "turn";
	}
	std::unique_ptr<AI::MCTS::MCTSNode> root = std::make_unique<AI::MCTS::MCTSNode>(currentState, "", nullptr, moveType);

	//Logger::info("            [EXPAND ROOT] The root is being expanded.");
	//Logger::info("            [EXPAND ROOT] The following root is being expanded.\n            " + root->toJson().dump());
	expandNode(root.get(), neuralNet);
	//Logger::info("            [EXPAND ROOT] The root was expanded into the following.\n            " + root->toJson().dump());

	injectDirichletNoise(root.get(), dirichletMixingWeight, dirichletShape);

	for (int i = 0; i < numberOfSimulations; i++) {
		//Logger::info("            [MCTS SIMULATION] MCTS simulation " + std::to_string(i + 1) + " of " + std::to_string(numberOfSimulations) + " is running.");
		//Logger::info("                [MCTS SIMULATION] Node node is being set to root.");
		AI::MCTS::MCTSNode* node = root.get();
		//if (node->isLeaf()) {
		//    Logger::info("                [SELECT NO NODE] Node node is leaf and will not be reset to a child.");
		//}
		while (!node->isLeaf()) {
			node = AI::MCTS::selectChild(node, cPuct, tolerance);
			//Logger::info("                [SELECT NODE] Node node was not leaf and was reset to a child.");
			//Logger::info("                [SELECT NODE] Node node was not leaf and was reset to the following child.\n                " + node->toJson().dump());
		}
		//if (node->N > 0) {
		//    Logger::info("                [EXPAND NO NODE] Node node was visited before and was not expanded.");
		//}
		//else {
		if (node->visitCount == 0) {
			expandNode(node, neuralNet);
			//Logger::info("                [EXPAND NODE] Node node was not visited before and was expanded.");
			//Logger::info("                [EXPAND NODE] Node node was not visited before and was expanded into the following.\n                " + node->toJson().dump());
		}
		double value = rollout(node, neuralNet);
		//Logger::info("                [ROLLOUT] The value of node node is " + std::to_string(value) + ".");
		backpropagate(node, value);
		//Logger::info("                [BACKPROPAGATION] Statistics of node node and all parents were updated.");
	}


	auto iterator = std::max_element(
		root->unorderedMapOfMovesToChildren.begin(),
		root->unorderedMapOfMovesToChildren.end(),
		comparePairsOfMovesAndChildren
	);
	if (iterator == root->unorderedMapOfMovesToChildren.end()) {
		throw std::runtime_error("Best child is not defined.");
	}
	AI::MCTS::MCTSNode* bestChild = iterator->second.get();


	//Logger::info("            [SELECT BEST CHILD] The best child has move " + bestChild->move + ".");
	//Logger::info("            [SELECT BEST CHILD] The best child has move type " + bestChild->moveType + ".");
	//Logger::info("            [SELECT BEST CHILD] The child of the root with the highest visit count is the following.\n" + bestChild->toJson().dump());
	return std::make_tuple(bestChild->move, bestChild->moveType, bestChild->visitCount);
}