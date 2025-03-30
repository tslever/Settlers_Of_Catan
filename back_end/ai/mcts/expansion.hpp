#pragma once


#include "../../db/database.hpp"
#include "../neural_network.hpp"
#include "node.hpp"
#include "../../game/board.hpp"


std::pair<double, double> parseCoordinates(const std::string& stringRepresentingEndpoint) {
	size_t positionOfHyphen = stringRepresentingEndpoint.find('-');
	double x = std::stod(stringRepresentingEndpoint.substr(0, positionOfHyphen));
	double y = std::stod(stringRepresentingEndpoint.substr(positionOfHyphen + 1));
	return { x, y };
}


std::pair<std::string, std::string> parseEdgeEndpoints(const Board& board, const std::string& edge) {
	size_t positionOfUnderscore = edge.find('_');
	if (positionOfUnderscore == std::string::npos) {
		return { "", "" };
	}
	std::string stringRepresentingFirstEndpoint = edge.substr(0, positionOfUnderscore);
	std::string stringRepresentingSecondEndpoint = edge.substr(positionOfUnderscore + 1);
	std::pair<double, double> pairOfCoordinatesOfFirstEndpoint = parseCoordinates(stringRepresentingFirstEndpoint);
	std::pair<double, double> pairOfCoordinatesOfSecondEndpoint = parseCoordinates(stringRepresentingSecondEndpoint);
	std::string labelOfFirstEndpoint = board.getVertexLabelByCoordinates(pairOfCoordinatesOfFirstEndpoint.first, pairOfCoordinatesOfFirstEndpoint.second);
	std::string labelOfSecondEndpoint = board.getVertexLabelByCoordinates(pairOfCoordinatesOfSecondEndpoint.first, pairOfCoordinatesOfSecondEndpoint.second);
	return { labelOfFirstEndpoint, labelOfSecondEndpoint };
}


/* Function `expandNode`, for each move determined to be available by board geometry and current state,
* creates a child node and sets its prior probability based on the move type.
*/
void expandNode(const std::shared_ptr<MCTSNode>& node, Database& db, WrapperOfNeuralNetwork& neuralNet) {
	auto parent = node->parent.lock();
	if (parent) {
		std::clog << "Expanding node N" << node->index
			<< " (result of move \"" << node->move
			<< "\" of type \"" << node->moveType << "\") with parent N"
			<< parent->index << std::endl;
	}
	else {
		std::clog << "Expanding root node N" << node->index
			<< " (initial game state)" << std::endl;
	}
	Board board;
	std::vector<std::string> availableMoves;
	if (node->gameState.phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
		std::vector<std::string> occupiedVertices = getOccupiedVertices(db);
		availableMoves = getAvailableVertices(occupiedVertices);
	}
	else if (node->gameState.phase == Phase::TO_PLACE_FIRST_ROAD || node->gameState.phase == Phase::TO_PLACE_SECOND_ROAD) {
		std::vector<std::string> occupiedEdges = getVectorOfKeysOfOccupiedEdges(db);
		availableMoves = getVectorOfKeysOfAvailableEdges(node->gameState.lastBuilding, occupiedEdges);
	}
	else if (node->gameState.phase == Phase::TO_PLACE_FIRST_CITY) {
		std::vector<std::string> occupiedVertices = getOccupiedVertices(db);
		availableMoves = getAvailableVertices(occupiedVertices);
	}
	else if (node->gameState.phase == Phase::TURN) {
		if (node->children.find("pass") == node->children.end()) {
			GameState childState = node->gameState;
			auto passChild = std::make_shared<MCTSNode>(childState, "pass", node, "pass");
			passChild->P = 0.1f;
			node->children["pass"] = passChild;
		}
		int currentPlayer = node->gameState.currentPlayer;
		std::unordered_set<std::string> unorderedSetOfLabelsOfEndpoints;
		for (const std::string& labelOfVertexWithSettlement : node->gameState.settlements[currentPlayer]) {
			unorderedSetOfLabelsOfEndpoints.insert(labelOfVertexWithSettlement);
		}
		for (const std::string& labelOfVertexWithCity : node->gameState.cities[currentPlayer]) {
			unorderedSetOfLabelsOfEndpoints.insert(labelOfVertexWithCity);
		}
		for (std::string& edgeKey : node->gameState.roads[currentPlayer]) {
			std::pair<std::string, std::string> pairOfLabelsOfEndpoints = parseEdgeEndpoints(board, edgeKey);
			if (!pairOfLabelsOfEndpoints.first.empty()) {
				unorderedSetOfLabelsOfEndpoints.insert(pairOfLabelsOfEndpoints.first);
			}
			if (!pairOfLabelsOfEndpoints.second.empty()) {
				unorderedSetOfLabelsOfEndpoints.insert(pairOfLabelsOfEndpoints.second);
			}
		}
		std::vector<std::string> occupiedEdges = getVectorOfKeysOfOccupiedEdges(db);
		for (const std::string& endpoint : unorderedSetOfLabelsOfEndpoints) {
			std::vector<std::string> edgeMoves = getVectorOfKeysOfAvailableEdges(endpoint, occupiedEdges);
			for (const std::string& edgeKey : edgeMoves) {
				if (node->children.find(edgeKey) == node->children.end()) {
					GameState childState = node->gameState;
					childState.placeRoad(currentPlayer, edgeKey);
					auto child = std::make_shared<MCTSNode>(childState, edgeKey, node, "road");
					std::vector<float> featureVector = board.getFeatureVector(edgeKey);
					auto eval = neuralNet.evaluateStructure(featureVector);
					child->P = eval.second;
					node->children[edgeKey] = child;
				}
			}
		}
		std::vector<std::string> vectorOfLabelsOfOccupiedVertices = getOccupiedVertices(db);
		std::vector<std::string> candidateVertices = getAvailableVertices(vectorOfLabelsOfOccupiedVertices);
		std::unordered_map<std::string, int> mapOfLabelsOfVerticesAndNumbersOfRoads;
		for (const std::string& edgeKey : node->gameState.roads[currentPlayer]) {
			std::pair<std::string, std::string> pairOfLabelsOfEndpoints = parseEdgeEndpoints(board, edgeKey);
			if (!pairOfLabelsOfEndpoints.first.empty()) {
				mapOfLabelsOfVerticesAndNumbersOfRoads[pairOfLabelsOfEndpoints.first]++;
			}
			if (!pairOfLabelsOfEndpoints.second.empty()) {
				mapOfLabelsOfVerticesAndNumbersOfRoads[pairOfLabelsOfEndpoints.second]++;
			}
		}
		for (const std::string& labelOfVertex : candidateVertices) {
			if (mapOfLabelsOfVerticesAndNumbersOfRoads[labelOfVertex] >= 2) {
				if (node->children.find(labelOfVertex) == node->children.end()) {
					GameState childState = node->gameState;
					childState.placeSettlement(currentPlayer, labelOfVertex);
					auto child = std::make_shared<MCTSNode>(childState, labelOfVertex, node, "settlement");
					std::vector<float> featureVector = board.getFeatureVector(labelOfVertex);
					auto eval = neuralNet.evaluateStructure(featureVector);
					child->P = eval.second;
					node->children[labelOfVertex] = child;
				}
			}
		}

		return;
	}

	std::string appliedPhase = node->gameState.phase;

	for (const auto& move : availableMoves) {
		if (node->children.find(move) != node->children.end()) {
			continue;
		}
		GameState childState = node->gameState;
		int currentPlayer = childState.currentPlayer;
		if (appliedPhase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
			childState.placeSettlement(currentPlayer, move);
		}
		else if (appliedPhase == Phase::TO_PLACE_FIRST_ROAD || appliedPhase == Phase::TO_PLACE_SECOND_ROAD) {
			childState.placeRoad(currentPlayer, move);
		}
		else if (appliedPhase == Phase::TO_PLACE_FIRST_CITY) {
			childState.placeCity(currentPlayer, move);
		}
		std::string appliedMoveType;
		if (appliedPhase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
			appliedMoveType = "settlement";
		}
		else if (appliedPhase == Phase::TO_PLACE_FIRST_ROAD || appliedPhase == Phase::TO_PLACE_SECOND_ROAD) {
			appliedMoveType = "road";
		}
		else if (appliedPhase == Phase::TO_PLACE_FIRST_CITY) {
			appliedMoveType = "city";
		}
		else if (appliedPhase == Phase::TURN) {
			appliedMoveType = "turn";
		}
		auto child = std::make_shared<MCTSNode>(childState, move, node, appliedMoveType);
		std::vector<float> featureVector = board.getFeatureVector(move);
		auto eval = neuralNet.evaluateStructure(featureVector);
		child->P = eval.second;
		node->children[move] = child;
	}
}