#pragma once;

// TODO: Replace function `simulateRollout` with an actual MCTS simulation function.
double simulateRollout(const std::shared_ptr<MCTSNode>& node, Database& db, SettlersNeuralNet& neuralNet) {
	// This placeholder simply returns a dummy value.
	/* TODO: Implement a rollout simulation to evaluate the node.
	* For instance, use the neural network evaluation on a feature vector corresponding to the move represented by this node.
	*/
	std::vector<float> dummyFeatures = { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
	// TODO: Consider using a feature vector based on the game state / board features.
	auto eval = neuralNet.evaluateSettlement(dummyFeatures);
	return eval.first; // Return value estimate.
}