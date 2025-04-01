#pragma once


// Function `selectChild` is a function that selects a best child using Predictor Upper Confidence bound applied to Trees.
std::shared_ptr<MCTSNode> selectChild(
	const std::shared_ptr<MCTSNode>& node,
	double cPuct,
	double tolerance
) {
	double bestScore = -std::numeric_limits<double>::infinity();
	std::vector<std::shared_ptr<MCTSNode>> vectorOfBestCandidates;
	for (const std::pair<const std::string, std::shared_ptr<MCTSNode>>& pairOfRepresentationOfMoveAndChild : node->unorderedMapOfRepresentationsOfMovesToChildren) {
		std::shared_ptr<MCTSNode> child = pairOfRepresentationOfMoveAndChild.second;
		// Exploration bonus U as in AlphaGo Zero
		double u = cPuct * child->priorProbability * std::sqrt(node->visitCount) / (1.0 + child->visitCount);
		double averageValue = child->averageValue;
		double score = averageValue + u;
		if (score > bestScore + tolerance) {
			bestScore = score;
			vectorOfBestCandidates.clear();
			vectorOfBestCandidates.push_back(child);
		}
		else if (std::abs(score - bestScore) <= tolerance) {
			vectorOfBestCandidates.push_back(child);
		}
	}
	if (vectorOfBestCandidates.empty()) {
		throw std::runtime_error("Vector of best candidates was empty.");
		//return node;
	}
	std::shared_ptr<MCTSNode> bestCandidate = vectorOfBestCandidates[0];
	for (const std::shared_ptr<MCTSNode>& candidate : vectorOfBestCandidates) {
		if (
			(candidate->visitCount < bestCandidate->visitCount) ||
			(candidate->visitCount == bestCandidate->visitCount && candidate->priorProbability > bestCandidate->priorProbability)
		) {
			bestCandidate = candidate;
		}
	}
	return bestCandidate;
}