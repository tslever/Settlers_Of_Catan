#pragma once


#include "node.hpp"


namespace AI {
	namespace MCTS {

		// Backpropagate the rollout value up the tree.
		// TODO: Consider adding discount factors or more advanced statistics.
		void backpropagate(std::shared_ptr<MCTSNode> node, double value) {
			while (node) {
				node->visitCount += 1;
				node->totalValue += value;
				node->averageValue = node->totalValue / node->visitCount;
				node = node->parent.lock();
			}
		}

	}
}