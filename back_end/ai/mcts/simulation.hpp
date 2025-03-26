#pragma once

/* Function `simulateRollout` conducts a rollout with one step
* that uses the neural network to evaluate the leaf node.
*/
// TODO: Consider simulating multiple moves.
double simulateRollout(const std::shared_ptr<MCTSNode>& node, Database& db, SettlersNeuralNet& neuralNet) {
	if (node->moveType == "settlement") {
		auto eval = neuralNet.evaluateSettlementFromVertex(node->move);
		return eval.first;
	}
	else if (node->moveType == "city") {
		auto eval = neuralNet.evaluateCityFromVertex(node->move);
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