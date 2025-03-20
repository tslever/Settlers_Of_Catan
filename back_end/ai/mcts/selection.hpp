#pragma once;

// TODO: Replace function `selectChild` with an actual MCTS selection function.
std::shared_ptr<MCTSNode> selectChild(const std::shared_ptr<MCTSNode>& node) {
	// TODO: Implement selection logic (e.g., PUCT) similar to the Python selection function.
	// For now, simply return the first child if available.
	if (!node->children.empty()) {
		return node->children.begin()->second;
	}
	return node;
}