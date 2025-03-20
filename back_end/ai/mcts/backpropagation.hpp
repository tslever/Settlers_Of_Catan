#pragma once;


#include "node.hpp"


// TODO: Replace function `backpropagate` with an actual MCTS backpropagation function.
void backpropagate(std::shared_ptr<MCTSNode> node, double value) {
	// Backpropagate the rollout value up the tree.
	while (node != nullptr) {
		node->N += 1;
		node->W += value;
		node->Q = node->W / node->N;
		node = node->parent;
	}
}