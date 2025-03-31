#pragma once


// Function `simulateRollout` conducts a rollout with one step that uses the neural network to evaluate the leaf node.
/* TODO: Consider whether self playing a full game is sufficient to simulate multiple moves, or
* whether multiple moves should be simulated here using a simple multi-step loop with a max depth or using another technique.
*/
double rollout(const std::shared_ptr<MCTSNode>& node, Database& db, WrapperOfNeuralNetwork& neuralNet) {
	Board board;
	std::string move = node->move;
	std::vector<std::vector<float>> featureVector = { board.getFeatureVector(move) };
	auto eval = neuralNet.evaluateStructures(featureVector)[0];
	return eval.first;
}