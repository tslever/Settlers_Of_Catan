#pragma once


namespace AI {
	namespace MCTS {

		// Function `selectChild` is a function that selects a best child using Predictor Upper Confidence bound applied to Trees.
		MCTSNode* selectChild(MCTSNode* node, double cPuct, double tolerance) {
			double bestScore = -std::numeric_limits<double>::infinity();
			std::vector<MCTSNode*> vectorOfBestCandidates;
			for (const auto& pairOfRepresentationOfMoveAndChild : node->unorderedMapOfRepresentationsOfMovesToChildren) {
				// Exploration bonus U as in AlphaGo Zero
				MCTSNode* child = pairOfRepresentationOfMoveAndChild.second.get();
				double u = cPuct * child->priorProbability * std::sqrt(node->visitCount) / (1.0 + child->visitCount);
				double score = child->averageValue + u;
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
				throw std::runtime_error("No best candidate was selected during MCTS.");
				//return node;
			}
			MCTSNode* bestCandidate = vectorOfBestCandidates[0];
			for (MCTSNode* candidate : vectorOfBestCandidates) {
				if (
					(candidate->visitCount < bestCandidate->visitCount) ||
					(candidate->visitCount == bestCandidate->visitCount && candidate->priorProbability > bestCandidate->priorProbability)
				) {
					bestCandidate = candidate;
				}
			}
			return bestCandidate;
		}

	}
}