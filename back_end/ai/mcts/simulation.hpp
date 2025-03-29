#pragma once


// Function `simulateRollout` conducts a rollout with one step that uses the neural network to evaluate the leaf node.
/* TODO: Consider whether self playing a full game is sufficient to simulate multiple moves, or
* whether multiple moves should be simulated here using a simple multi-step loop with a max depth or using another technique.
*/
double simulateRollout(const std::shared_ptr<MCTSNode>& node, Database& db, WrapperOfNeuralNetwork& neuralNet) {
	if (node->moveType == "settlement" || node->moveType == "road" || node->moveType == "city") {
		Board board;
		std::string move = node->move;
		std::vector<float> featureVector = board.getFeatureVector(move);
		auto eval = neuralNet.evaluateStructure(featureVector);
		return eval.first;
	}
	else if (node->moveType == "turn") {
		throw std::runtime_error("Expanding node with move type turn is not implemented yet.");
	}
	return 0.0;
}