#pragma once;

// Function `selectChild` is a selection function that using PUCT.
std::shared_ptr<MCTSNode> selectChild(
	const std::shared_ptr<MCTSNode>& node,
	double c_puct = 1.0, // TODO: Define c_puct.
	double tolerance = 1e-6 // TODO: Define tolerance.
	// TODO: Consider moving c_puct to a configuration file.
	// TODO: Consider moving tolerance to a configuration file.
) {
	double bestScore = -std::numeric_limits<double>::infinity();
	std::vector<std::shared_ptr<MCTSNode>> bestCandidates;
	for (const auto& pair : node->children) {
		auto child = pair.second;
		// Exploration bonus as in AlphaGo Zero U
		// TODO: Define exploration bonus.
		double u = c_puct * child->P * std::sqrt(node->N) / (1.0 + child->N);
		double score = child->Q + u;
		// TODO: Define `child->Q`.
		// TODO: Define score.
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
		return node; // Return the current node if no children exist.
	}
	// Among equally scoring candidates, pick the candidate with the lowest visit count and highest prior.
	// TODO: Consider whether these criteria are ideal.
	auto best = bestCandidates[0];
	for (const auto& candidate : bestCandidates) {
		if (
			(candidate->N < best->N) ||
			(candidate->N == best->N && candidate->P > best->P)
		) {
			// TODO: Define `candidate->N`.
			// TODO: Define `candidate->P`.
			best = candidate;
		}
	}
	return best;
}