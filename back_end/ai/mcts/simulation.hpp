#pragma once


// Function `simulateRollout` conducts a rollout with one step that uses the neural network to evaluate the leaf node.
/* TODO: Consider whether self playing a full game is sufficient to simulate multiple moves, or
* whether multiple moves should be simulated here using a simple multi-step loop with a max depth or using another technique.
*/
double simulateRollout(const std::shared_ptr<MCTSNode>& node, Database& db, WrapperOfNeuralNetwork& neuralNet) {
	auto parent = node->parent.lock();
	if (parent) {
		std::clog << "Evaluating move of node N" << node->index
			<< " (result of move \"" << node->move
			<< "\" of type \"" << node->moveType << "\") with parent N"
			<< parent->index << std::endl;
	}
	else {
		std::clog << "Evaluating move of node N" << node->index
			<< " (initial game state)" << std::endl;
	}
	if (node->moveType == "settlement" || node->moveType == "road" || node->moveType == "city") {
		Board board;
		std::string move = node->move;
		std::vector<std::vector<float>> featureVector = { board.getFeatureVector(move) };
		auto eval = neuralNet.evaluateStructures(featureVector)[0];
		return eval.first;
	}
	else if (node->moveType == "turn") {
		throw std::runtime_error("Expanding node with move type turn is not implemented yet.");
	}
	return 0.0;
}