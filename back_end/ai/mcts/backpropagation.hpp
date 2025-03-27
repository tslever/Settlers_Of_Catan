#pragma once


#include "node.hpp"


// Backpropagate the rollout value up the tree.
// TODO: Consider adding discount factors or more advanced statistics.
void backpropagate(std::shared_ptr<MCTSNode> node, double value) {
	while (node != nullptr) {
		node->N += 1;
		node->W += value;
		node->Q = node->W / node->N;
		node = node->parent;
	}
}