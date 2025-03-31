#pragma once


#include "node.hpp"


// Backpropagate the rollout value up the tree.
// TODO: Consider adding discount factors or more advanced statistics.
void backpropagate(std::shared_ptr<MCTSNode> node, double value) {
	/*auto parent = node->parent.lock();
	if (parent) {
		std::clog << "Adjusting values of node N" << node->index
			<< " (result of move \"" << node->move
			<< "\" of type \"" << node->moveType << "\") with parent N"
			<< parent->index << std::endl;
	}
	else {
		std::clog << "Adjusting values of node N" << node->index
			<< " (initial game state)" << std::endl;
	}*/
	while (node) {
		node->N += 1;
		node->W += value;
		node->Q = node->W / node->N;
		node = node->parent.lock();
	}
}