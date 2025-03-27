#pragma once


/* Function `simulateRollout` conducts a rollout with one step
* that uses the neural network to evaluate the leaf node.
*/
/* TODO: Consider whether self playing a full game is sufficient to simulate multiple moves, or
* whether multiple moves should be simulated here using a simple multi-step loop with a max depth or something else.
* Does simulating multiple moves better approximate a full game outcome than a one step evaluation?
*/
double simulateRollout(const std::shared_ptr<MCTSNode>& node, Database& db, SettlersNeuralNet& neuralNet) {
	if (node->moveType == "settlement") {
		auto eval = neuralNet.evaluateBuildingFromVertex(node->move);
		return eval.first;
	}
	else if (node->moveType == "city") {
		auto eval = neuralNet.evaluateBuildingFromVertex(node->move);
		return eval.first;
	}
	else if (node->moveType == "road") {
		std::string lastBuilding = (node->parent ? node->parent->move : "");
		if (!lastBuilding.empty()) {
			auto eval = neuralNet.evaluateRoadFromEdge(lastBuilding, node->move);
			return eval.first;
		}
	}
	return 0.0;
}