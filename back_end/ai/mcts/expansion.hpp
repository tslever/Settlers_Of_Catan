#pragma once


#include "../../game/board.hpp"
#include "../../db/database.hpp"
#include "../neural_network.hpp"
#include "node.hpp"


namespace AI {
	namespace MCTS {

		std::vector<std::string> getVectorOfLabelsOfOccupiedVertices(const MCTSNode* node) {
			std::vector<std::string> vectorOfLabelsOfOccupiedVertices;
			for (const auto& [player, vectorOfLabelsOfVerticesWithSettlements] : node->gameState.settlements) {
				vectorOfLabelsOfOccupiedVertices.insert(
					vectorOfLabelsOfOccupiedVertices.end(),
					vectorOfLabelsOfVerticesWithSettlements.begin(),
					vectorOfLabelsOfVerticesWithSettlements.end()
				);
			}
			for (const auto& [player, vectorOfLabelsOfVerticesWithCities] : node->gameState.cities) {
				vectorOfLabelsOfOccupiedVertices.insert(
					vectorOfLabelsOfOccupiedVertices.end(),
					vectorOfLabelsOfVerticesWithCities.begin(),
					vectorOfLabelsOfVerticesWithCities.end()
				);
			}
			return vectorOfLabelsOfOccupiedVertices;
		}


		std::vector<std::string> getVectorOfKeysOfOccupiedEdges(const MCTSNode* node) {
			std::vector<std::string> vectorOfKeysOfOccupiedEdges;
			for (const auto& [player, vectorOfKeysOfEdgesWithRoads] : node->gameState.roads) {
				vectorOfKeysOfOccupiedEdges.insert(
					vectorOfKeysOfOccupiedEdges.end(),
					vectorOfKeysOfEdgesWithRoads.begin(),
					vectorOfKeysOfEdgesWithRoads.end()
				);
			}
			return vectorOfKeysOfOccupiedEdges;
		}


		/* Function `expandNode`, for each move determined to be available by board geometry and current state,
		* creates a child node and sets its prior probability based on the move type.
		*/
		void expandNode(MCTSNode* node, WrapperOfNeuralNetwork& neuralNet) {
			Board board;

			// Create vector of labels of available vertices or keys of available edges.
			std::vector<std::string> vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges;
			std::string phase = node->gameState.phase;
			if (phase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getVectorOfLabelsOfOccupiedVertices(node);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
			}
			else if (phase == Game::Phase::TO_PLACE_FIRST_ROAD || phase == Game::Phase::TO_PLACE_SECOND_ROAD) {
				std::vector<std::string> vectorOfKeysOfOccupiedEdges = getVectorOfKeysOfOccupiedEdges(node);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges = board.getVectorOfKeysOfAvailableEdgesExtendingFromLastBuilding(node->gameState.lastBuilding, vectorOfKeysOfOccupiedEdges);
			}
			else if (phase == Game::Phase::TO_PLACE_FIRST_CITY) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getVectorOfLabelsOfOccupiedVertices(node);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
			}
			else if (node->gameState.phase == Game::Phase::TURN) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getVectorOfLabelsOfOccupiedVertices(node);
				std::vector<std::string> vectorOfLabelsOfAvailableVertices = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);

				std::vector<std::string> vectorOfKeysOfOccupiedEdges = getVectorOfKeysOfOccupiedEdges(node);
				std::vector<std::string> vectorOfKeysOfAvailableEdges = board.getVectorOfKeysOfAvailableEdges(vectorOfKeysOfOccupiedEdges);

				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.insert(
					vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.end(),
					vectorOfLabelsOfAvailableVertices.begin(),
					vectorOfLabelsOfAvailableVertices.end()
				);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.insert(
					vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.end(),
					vectorOfKeysOfAvailableEdges.begin(),
					vectorOfKeysOfAvailableEdges.end()
				);
			}

			// Create a child for each label of available vertex or key of available edge.
			std::vector<std::vector<float>> vectorOfFeatureVectors;
			std::vector<MCTSNode*> vectorOfChildren;
			for (const std::string& labelOfAvailableVertexOrKeyOfAvailableEdge : vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges) {
				if (node->unorderedMapOfRepresentationsOfMovesToChildren.find(labelOfAvailableVertexOrKeyOfAvailableEdge) != node->unorderedMapOfRepresentationsOfMovesToChildren.end()) {
					continue;
				}
				GameState gameStateOfChild = node->gameState;
				int currentPlayer = gameStateOfChild.currentPlayer;
				std::string moveType;
				if (phase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
					gameStateOfChild.placeSettlement(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
					moveType = "settlement";
				}
				else if (phase == Game::Phase::TO_PLACE_FIRST_ROAD || phase == Game::Phase::TO_PLACE_SECOND_ROAD) {
					gameStateOfChild.placeRoad(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
					moveType = "road";
				}
				else if (phase == Game::Phase::TO_PLACE_FIRST_CITY) {
					gameStateOfChild.placeCity(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
					moveType = "city";
				}
				else if (phase == Game::Phase::TURN) {
					if (board.isLabelOfVertex(labelOfAvailableVertexOrKeyOfAvailableEdge)) {
						gameStateOfChild.placeSettlement(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
						moveType = "turn";
					}
					else if (board.isEdgeKey(labelOfAvailableVertexOrKeyOfAvailableEdge)) {
						gameStateOfChild.placeRoad(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
						moveType = "turn";
					}
				}
				std::unique_ptr<AI::MCTS::MCTSNode> child = std::make_unique<MCTSNode>(
					gameStateOfChild,
					labelOfAvailableVertexOrKeyOfAvailableEdge,
					node,
					moveType
				);
				MCTSNode* pointerToChild = child.get();
				std::vector<float> featureVector = board.getGridRepresentationForMove(labelOfAvailableVertexOrKeyOfAvailableEdge, moveType);
				vectorOfFeatureVectors.push_back(featureVector);
				vectorOfChildren.push_back(pointerToChild);
				node->unorderedMapOfRepresentationsOfMovesToChildren[labelOfAvailableVertexOrKeyOfAvailableEdge] = std::move(child);
			}
			if (!vectorOfFeatureVectors.empty()) {
				std::vector<std::pair<double, double>> vectorOfPairsOfValuesAndPolicies = neuralNet.evaluateStructures(vectorOfFeatureVectors);
				size_t i = 0;
				for (const auto& [value, policy] : vectorOfPairsOfValuesAndPolicies) {
					vectorOfChildren[i++]->priorProbability = policy;
				}
			}
		}

	}
}