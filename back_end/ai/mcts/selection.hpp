#pragma once


// Function `selectChild` is a selection function that using PUCT.
std::shared_ptr<MCTSNode> selectChild(
	const std::shared_ptr<MCTSNode>& node,
	double c_puct = 1.0,
	double tolerance = 1e-6
) {
	auto parent = node->parent.lock();
	if (parent) {
		std::clog << "Selecting node N" << node->index
			<< " (result of move \"" << node->move
			<< "\" of type \"" << node->moveType << "\") with parent N"
			<< parent->index << std::endl;
	}
	else {
		std::clog << "Selecting root node N" << node->index
			<< " (initial game state)" << std::endl;
	}
	double bestScore = -std::numeric_limits<double>::infinity();
	std::vector<std::shared_ptr<MCTSNode>> bestCandidates;
	for (const auto& pair : node->children) {
		auto child = pair.second;
		// Exploration bonus U as in AlphaGo Zero
		double u = c_puct * child->P * std::sqrt(node->N) / (1.0 + child->N);
		double score = child->Q + u;
		if (score > bestScore + tolerance) {
			bestScore = score;
			bestCandidates.clear();
			bestCandidates.push_back(child);
		}
		else if (std::abs(score - bestScore) <= tolerance) {
			bestCandidates.push_back(child);
		}
	}
	if (bestCandidates.empty()) {
		// Return the current node if no children exist.
		return node;
	}
	auto best = bestCandidates[0];
	for (const auto& candidate : bestCandidates) {
		if (
			(candidate->N < best->N) ||
			(candidate->N == best->N && candidate->P > best->P)
		) {
			best = candidate;
		}
	}
	return best;
}