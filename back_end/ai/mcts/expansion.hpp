#pragma once


#include "../../game/board.hpp"
#include "../../db/database.hpp"
#include "../neural_network.hpp"
#include "node.hpp"


namespace AI {
	namespace MCTS {

		std::pair<double, double> parseCoordinates(const std::string& stringRepresentingEndpoint) {
			size_t positionOfHyphen = stringRepresentingEndpoint.find('-');
			double x = std::stod(stringRepresentingEndpoint.substr(0, positionOfHyphen));
			double y = std::stod(stringRepresentingEndpoint.substr(positionOfHyphen + 1));
			return { x, y };
		}


		std::pair<std::string, std::string> parseEdgeEndpoints(const Board& board, const std::string& edge) {
			size_t positionOfUnderscore = edge.find('_');
			if (positionOfUnderscore == std::string::npos) {
				throw std::runtime_error("Edge key does not have an underscore.");
			}
			std::string stringRepresentingFirstEndpoint = edge.substr(0, positionOfUnderscore);
			std::string stringRepresentingSecondEndpoint = edge.substr(positionOfUnderscore + 1);
			std::pair<double, double> pairOfCoordinatesOfFirstEndpoint = parseCoordinates(stringRepresentingFirstEndpoint);
			std::pair<double, double> pairOfCoordinatesOfSecondEndpoint = parseCoordinates(stringRepresentingSecondEndpoint);
			std::string labelOfFirstEndpoint = board.getLabelOfVertexByCoordinates(pairOfCoordinatesOfFirstEndpoint.first, pairOfCoordinatesOfFirstEndpoint.second);
			std::string labelOfSecondEndpoint = board.getLabelOfVertexByCoordinates(pairOfCoordinatesOfSecondEndpoint.first, pairOfCoordinatesOfSecondEndpoint.second);
			return { labelOfFirstEndpoint, labelOfSecondEndpoint };
		}


		/* Function `expandNode`, for each move determined to be available by board geometry and current state,
		* creates a child node and sets its prior probability based on the move type.
		*/
		void expandNode(const std::shared_ptr<MCTSNode>& node, DB::Database& db, WrapperOfNeuralNetwork& neuralNet) {
			Board board;

			// Create vector of labels of available vertices or keys of available edges.
			std::vector<std::string> vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges;
			std::string phase = node->gameState.phase;
			if (phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = board.getVectorOfLabelsOfOccupiedVertices(db);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
			}
			else if (phase == Phase::TO_PLACE_FIRST_ROAD || phase == Phase::TO_PLACE_SECOND_ROAD) {
				std::vector<std::string> vectorOfKeysOfOccupiedEdges = board.getVectorOfKeysOfOccupiedEdges(db);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges = board.getVectorOfKeysOfAvailableEdgesExtendingFromLastBuilding(node->gameState.lastBuilding, vectorOfKeysOfOccupiedEdges);
			}
			else if (phase == Phase::TO_PLACE_FIRST_CITY) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = board.getVectorOfLabelsOfOccupiedVertices(db);
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);
			}
			else if (node->gameState.phase == Phase::TURN) {
				std::vector<std::string> vectorOfLabelsOfOccupiedVertices = board.getVectorOfLabelsOfOccupiedVertices(db);
				std::vector<std::string> vectorOfLabelsOfAvailableVertices = board.getVectorOfLabelsOfAvailableVertices(vectorOfLabelsOfOccupiedVertices);

				std::vector<std::string> vectorOfKeysOfOccupiedEdges = board.getVectorOfKeysOfOccupiedEdges(db);
				std::vector<std::string> vectorOfKeysOfAvailableEdges = board.getVectorOfKeysOfAvailableEdges(vectorOfKeysOfOccupiedEdges);

				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.clear();
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.shrink_to_fit();
				size_t numberOfSpacesToReserve = vectorOfLabelsOfAvailableVertices.size() + vectorOfKeysOfAvailableEdges.size();
				vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges.reserve(numberOfSpacesToReserve);
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
			std::vector<std::shared_ptr<MCTSNode>> vectorOfChildren;
			for (const std::string& labelOfAvailableVertexOrKeyOfAvailableEdge : vectorOfLabelsOfAvailableVerticesOrKeysOfAvailableEdges) {
				if (node->unorderedMapOfRepresentationsOfMovesToChildren.find(labelOfAvailableVertexOrKeyOfAvailableEdge) != node->unorderedMapOfRepresentationsOfMovesToChildren.end()) {
					continue;
				}
				GameState gameStateOfChild = node->gameState;
				int currentPlayer = gameStateOfChild.currentPlayer;
				std::string moveType;
				if (phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
					gameStateOfChild.placeSettlement(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
					moveType = "settlement";
				}
				else if (phase == Phase::TO_PLACE_FIRST_ROAD || phase == Phase::TO_PLACE_SECOND_ROAD) {
					gameStateOfChild.placeRoad(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
					moveType = "road";
				}
				else if (phase == Phase::TO_PLACE_FIRST_CITY) {
					gameStateOfChild.placeCity(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
					moveType = "city";
				}
				else if (phase == Phase::TURN) {
					if (board.isLabelOfVertex(labelOfAvailableVertexOrKeyOfAvailableEdge)) {
						gameStateOfChild.placeSettlement(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
						moveType = "turn";
					}
					else if (board.isEdgeKey(labelOfAvailableVertexOrKeyOfAvailableEdge)) {
						gameStateOfChild.placeRoad(currentPlayer, labelOfAvailableVertexOrKeyOfAvailableEdge);
						moveType = "turn";
					}
				}
				auto child = std::make_shared<MCTSNode>(gameStateOfChild, labelOfAvailableVertexOrKeyOfAvailableEdge, node, moveType);
				std::vector<float> featureVector = board.getFeatureVector(labelOfAvailableVertexOrKeyOfAvailableEdge);
				vectorOfFeatureVectors.push_back(featureVector);
				vectorOfChildren.push_back(child);
				node->unorderedMapOfRepresentationsOfMovesToChildren[labelOfAvailableVertexOrKeyOfAvailableEdge] = child;
			}
			if (!vectorOfFeatureVectors.empty()) {
				std::vector<std::pair<double, double>> vectorOfPairsOfValuesAndPolicies = neuralNet.evaluateStructures(vectorOfFeatureVectors);
				for (size_t i = 0; i < vectorOfPairsOfValuesAndPolicies.size(); i++) {
					std::pair<double, double> pairOfValueAndPolicy = vectorOfPairsOfValuesAndPolicies[i];
					double policy = pairOfValueAndPolicy.second;
					vectorOfChildren[i]->priorProbability = policy;
				}
			}
		}

	}
}