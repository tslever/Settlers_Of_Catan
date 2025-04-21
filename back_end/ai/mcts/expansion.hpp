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


		std::vector<std::string> getVectorOfLabelsOfOccupiedEdges(const MCTSNode* node) {
			std::vector<std::string> vectorOfLabelsOfOccupiedEdges;
			for (const auto& [player, vectorOfLabelsOfEdgesWithRoads] : node->gameState.roads) {
				vectorOfLabelsOfOccupiedEdges.insert(
					vectorOfLabelsOfOccupiedEdges.end(),
					vectorOfLabelsOfEdgesWithRoads.begin(),
					vectorOfLabelsOfEdgesWithRoads.end()
				);
			}
			return vectorOfLabelsOfOccupiedEdges;
		}


		/* Function `expandNode`, for each move determined to be available by board geometry and current state,
		* creates a child node and sets its prior probability based on the move type.
		*/
		void expandNode(MCTSNode* node, WrapperOfNeuralNetwork& neuralNet) {
			Board board;
			std::vector<std::string> vectorOfLabelsOfAvailableVerticesOrEdges;
			std::string phase = node->gameState.phase;

			if (phase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getVectorOfLabelsOfOccupiedVertices(node);
				vectorOfLabelsOfAvailableVerticesOrEdges = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
			}
			else if (phase == Game::Phase::TO_PLACE_FIRST_ROAD || phase == Game::Phase::TO_PLACE_SECOND_ROAD) {
				std::vector<std::string> vectorOfLabelsOfOccupiedEdges = getVectorOfLabelsOfOccupiedEdges(node);
				vectorOfLabelsOfAvailableVerticesOrEdges = board.getVectorOfLabelsOfAvailableEdgesExtendingFromLastBuilding(node->gameState.lastBuilding, vectorOfLabelsOfOccupiedEdges);
			}
			else if (phase == Game::Phase::TO_PLACE_FIRST_CITY) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getVectorOfLabelsOfOccupiedVertices(node);
				vectorOfLabelsOfAvailableVerticesOrEdges = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
			}
			else if (phase == Game::Phase::TURN) {
				const int currentPlayer = node->gameState.currentPlayer;
				auto& resources = node->gameState.resources.at(currentPlayer);

				std::unordered_set<std::string> unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer;
				const auto& labelsOfEdgesWithRoadsOfPlayer = node->gameState.roads.at(currentPlayer);
				for (const std::string& labelOfEdgeWithRoadOfPlayer : labelsOfEdgesWithRoadsOfPlayer) {
					auto verticesOfEdge = board.getVerticesOfEdge(labelOfEdgeWithRoadOfPlayer);
					unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.insert(verticesOfEdge.first);
					unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.insert(verticesOfEdge.second);
				}

				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getVectorOfLabelsOfOccupiedVertices(node);
				std::vector<std::string> vectorOfLabelsOfAvailableVertices = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);

				for (const std::string& labelOfVertex : vectorOfLabelsOfAvailableVertices) {
					if (unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(labelOfVertex)) {
						if (resources["brick"] >= 1 && resources["grain"] >= 1 && resources["lumber"] >= 1 && resources["wool"] >= 1) {
							vectorOfLabelsOfAvailableVerticesOrEdges.push_back(labelOfVertex);
						}
					}
				}

				std::vector<std::string> vectorOfLabelsOfOccupiedEdges = getVectorOfLabelsOfOccupiedEdges(node);
				std::vector<std::string> vectorOfLabelsOfAvailableEdges = board.getVectorOfLabelsOfAvailableEdges(vectorOfLabelsOfOccupiedEdges);

				for (const std::string& labelOfEdge : vectorOfLabelsOfAvailableEdges) {
					auto verticesOfEdge = board.getVerticesOfEdge(labelOfEdge);
					if (unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(verticesOfEdge.first) || unorderedSetOfLabelsOfVerticesOfRoadsOfPlayer.contains(verticesOfEdge.second)) {
						if (resources["brick"] >= 1 && resources["lumber"] >= 1) {
							vectorOfLabelsOfAvailableVerticesOrEdges.push_back(labelOfEdge);
						}
					}
				}

				vectorOfLabelsOfAvailableVerticesOrEdges.push_back("pass");
			}

			// Create a child for each label of available vertex or edge.
			std::vector<std::vector<float>> vectorOfFeatureVectors;
			std::vector<MCTSNode*> vectorOfChildren;
			for (const std::string& labelOfAvailableVertexOrEdge : vectorOfLabelsOfAvailableVerticesOrEdges) {
				if (node->unorderedMapOfMovesToChildren.find(labelOfAvailableVertexOrEdge) != node->unorderedMapOfMovesToChildren.end()) {
					continue;
				}
				GameState gameStateOfChild = node->gameState;
				std::string moveType;

				if (labelOfAvailableVertexOrEdge == "pass") {
					gameStateOfChild.updatePhase();
					moveType = "pass";
				}
				else {
					int currentPlayer = gameStateOfChild.currentPlayer;
					if (phase == Game::Phase::TO_PLACE_FIRST_SETTLEMENT) {
						gameStateOfChild.placeSettlement(currentPlayer, labelOfAvailableVertexOrEdge);
						moveType = "settlement";
					}
					else if (phase == Game::Phase::TO_PLACE_FIRST_ROAD || phase == Game::Phase::TO_PLACE_SECOND_ROAD) {
						gameStateOfChild.placeRoad(currentPlayer, labelOfAvailableVertexOrEdge);
						moveType = "road";
					}
					else if (phase == Game::Phase::TO_PLACE_FIRST_CITY) {
						gameStateOfChild.placeCity(currentPlayer, labelOfAvailableVertexOrEdge);
						moveType = "city";
					}
					else if (phase == Game::Phase::TURN) {
						if (board.isLabelOfVertex(labelOfAvailableVertexOrEdge)) {
							gameStateOfChild.placeSettlement(currentPlayer, labelOfAvailableVertexOrEdge);
							moveType = "settlement";
						}
						else if (board.isLabelOfEdge(labelOfAvailableVertexOrEdge)) {
							gameStateOfChild.placeRoad(currentPlayer, labelOfAvailableVertexOrEdge);
							moveType = "road";
						}
					}
				}
				std::unique_ptr<AI::MCTS::MCTSNode> child = std::make_unique<MCTSNode>(
					gameStateOfChild,
					labelOfAvailableVertexOrEdge,
					node,
					moveType
				);
				MCTSNode* pointerToChild = child.get();
				std::vector<float> featureVector = board.getGridRepresentationForMove(labelOfAvailableVertexOrEdge, moveType);
				vectorOfFeatureVectors.push_back(featureVector);
				vectorOfChildren.push_back(pointerToChild);
				node->unorderedMapOfMovesToChildren[labelOfAvailableVertexOrEdge] = std::move(child);
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