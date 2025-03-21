#pragma once;

/* Function `simulateRollout` conducts a rollout with one step
* that uses the neural network to evaluate the leaf node.
*/
// TODO: Consider simulating multiple moves.
double simulateRollout(const std::shared_ptr<MCTSNode>& node, Database& db, SettlersNeuralNet& neuralNet) {
	// Construct a dummy feature vector based on the node's game state.
	// TODO: Consider using a feature vector based on the game state / board features.
	std::vector<float> dummyFeatures = { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
	auto eval = neuralNet.evaluateSettlement(dummyFeatures);
	// TODO: Consider using other neural network functions if the step is not placing a settlement.
	return eval.first; // Return value estimate.
}