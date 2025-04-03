#pragma once


namespace AI {
	namespace MCTS {

		// Function `rollout` uses the neural network to evaluate the leaf node.
		/* TODO: Consider whether self playing a full game is sufficient to simulate multiple moves, or
		* whether multiple moves should be simulated here using a simple multi-step loop with a max depth or using another technique.
		*/
		double rollout(const std::shared_ptr<MCTSNode>& node, WrapperOfNeuralNetwork& neuralNet) {
			Board board;
			std::string move = node->move;
			std::vector<float> featureVector = board.getFeatureVector(move);
			std::vector<std::vector<float>> vectorOfFeatureVectors = { featureVector };
			auto eval = neuralNet.evaluateStructures(vectorOfFeatureVectors)[0];
			return eval.first;
		}

	}
}