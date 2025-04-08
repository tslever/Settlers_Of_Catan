#pragma once


#include <corecrt_math_defines.h>
#include "../db/database.hpp"
#include "geometry_helper.hpp"
#include <regex>


constexpr double NUMBER_BY_WHICH_TO_NORMALIZE_COORDINATE = 100.0;
constexpr double NUMBER_BY_WHICH_TO_NORMALIZE_NUMBER_OF_HEXES = 3.0;


class Board {
public:
	static crow::json::rvalue boardGeometryCache;

	Board() {
		loadBoardGeometry();
	}

	/* Function `getFeatureVector` computes and returns a 5 element feature vector for a given vertex label or edge key.
	* Features include:
	* - normalized total number of pips,
	* - normalized x coordinate,
	* - normalized y coordinate,
	* - normalized hex count, and
	* - bias 1.0.
	*/
	std::vector<float> getFeatureVector(const std::string& labelOfVertexOrEdgeKey) const {

		if (isEdgeKey(labelOfVertexOrEdgeKey)) {
			// TODO: Calculate feature vector corresponding to edge and/or revise feature vector to include more information relating to edge.
			/*float normalizedNumberOfPips = 0.5f;
			* float normalizedXCoordinate = 0.5f;
			* float normalizedYCoordinate = 0.5f;
			* float normalizedNumberOfHexes = 0.5f;
			* float bias = 1.0f;
			*/
			return { 0.5f, 0.5f, 0.5f, 0.5f, 1.0f };
		}

		crow::json::rvalue jsonArrayOfVertexInformation = boardGeometryCache["vertices"];
		if (!jsonArrayOfVertexInformation || jsonArrayOfVertexInformation.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain vertex information.");
		}

		float x = 0.0f;
		float y = 0.0f;
		bool found = false;
		for (const crow::json::rvalue& jsonObjectOfVertexInformation : jsonArrayOfVertexInformation) {
			if (jsonObjectOfVertexInformation["label"] == labelOfVertexOrEdgeKey) {
				x = static_cast<float>(jsonObjectOfVertexInformation["x"].d());
				y = static_cast<float>(jsonObjectOfVertexInformation["y"].d());
				found = true;
				break;
			}
		}
		if (!found) {
			throw std::runtime_error("Vertex " + labelOfVertexOrEdgeKey + " was not found in board geometry.");
		}

		crow::json::rvalue jsonArrayOfHexInformation = boardGeometryCache["hexes"];
		if (!jsonArrayOfHexInformation || jsonArrayOfHexInformation.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain hex information.");
		}
		std::unordered_map<std::string, int> mapOfIdOfHexToNumberOfToken = {
			{"H01", 10},
			{"H02", 2},
			{"H03", 9},
			{"H04", 12},
			{"H05", 6},
			{"H06", 4},
			{"H07", 10},
			{"H08", 9},
			{"H09", 11},
			{"H10", 1},
			{"H11", 3},
			{"H12", 8},
			{"H13", 8},
			{"H14", 3},
			{"H15", 4},
			{"H16", 5},
			{"H17", 5},
			{"H18", 6},
			{"H19", 11}
		};
		// TODO: Consider moving to a lookup table in C++ code or a data file.
		std::unordered_map<int, int> mapOfNumberOfTokenToNumberOfPips = {
			{1, 0},
			{2, 1},
			{3, 2},
			{4, 3},
			{5, 4},
			{6, 5},
			{8, 5},
			{9, 4},
			{10, 3},
			{11, 2},
			{12, 1}
		};
		// TODO: Consider moving to a lookup table in C++ code or a data file.
		float totalNumberOfPips = 0.0f;
		int numberOfHexes = 0;

		for (const crow::json::rvalue jsonObjectOfHexInformation : jsonArrayOfHexInformation) {
			std::vector<std::pair<double, double>> vectorOfPairsOfCoordinatesOfVertices = getVectorOfPairsOfCoordinatesOfVertices(jsonObjectOfHexInformation);
			for (const std::pair<double, double> pairOfCoordinatesOfVertex : vectorOfPairsOfCoordinatesOfVertices) {
				double xCoordinateOfVertex = pairOfCoordinatesOfVertex.first;
				double yCoordinateOfVertex = pairOfCoordinatesOfVertex.second;
				double distance = GeometryHelper::distance(x, y, xCoordinateOfVertex, yCoordinateOfVertex);
				if (distance < GeometryHelper::MARGIN_OF_ERROR) {
					std::string idOfHex = jsonObjectOfHexInformation["id"].s();
					std::unordered_map<std::string, int>::iterator iteratorOfIdOfHexAndNumberOfToken = mapOfIdOfHexToNumberOfToken.find(idOfHex);
					if (iteratorOfIdOfHexAndNumberOfToken != mapOfIdOfHexToNumberOfToken.end()) {
						int numberOfToken = iteratorOfIdOfHexAndNumberOfToken->second;
						std::unordered_map<int, int>::iterator iteratorOfNumberOfTokenAndNumberOfPips = mapOfNumberOfTokenToNumberOfPips.find(numberOfToken);
						if (iteratorOfNumberOfTokenAndNumberOfPips != mapOfNumberOfTokenToNumberOfPips.end()) {
							totalNumberOfPips += iteratorOfNumberOfTokenAndNumberOfPips->second;
						}
						numberOfHexes++;
						break;
					}
				}
			}
		}
		// Use hardcoded boarded width 100.0 `vmin` to normalize x and 100.0 to normalized y.
		// TODO: Consider revising feature vector to include more information relating to both vertices and edges.
		float normalizedNumberOfPips = totalNumberOfPips / (numberOfHexes * 5.0f);
		float normalizedXCoordinate = x / NUMBER_BY_WHICH_TO_NORMALIZE_COORDINATE;
		float normalizedYCoordinate = y / NUMBER_BY_WHICH_TO_NORMALIZE_COORDINATE;
		float normalizedNumberOfHexes = static_cast<float>(numberOfHexes) / NUMBER_BY_WHICH_TO_NORMALIZE_NUMBER_OF_HEXES;
		//float bias = 1.0f;
		return { normalizedNumberOfPips, normalizedXCoordinate, normalizedYCoordinate, normalizedNumberOfHexes, 1.0f };
	}

	std::vector<std::string> getVectorOfLabelsOfAvailableVertices(const std::vector<std::string>& vectorOfLabelsOfOccupiedVertices) const {
		crow::json::rvalue jsonArrayOfVertexInformation = boardGeometryCache["vertices"];
		if (!jsonArrayOfVertexInformation || jsonArrayOfVertexInformation.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain vertices.");
		}

		std::unordered_map<std::string, std::pair<double, double>> unorderedMapOfLabelOfVertexToPairOfCoordinates;
		for (size_t i = 0; i < jsonArrayOfVertexInformation.size(); i++) {
			crow::json::rvalue jsonObjectOfVertexInformation = jsonArrayOfVertexInformation[i];
			std::string labelOfVertex = jsonObjectOfVertexInformation["label"].s();
			double x = jsonObjectOfVertexInformation["x"].d();
			double y = jsonObjectOfVertexInformation["y"].d();
			std::pair<double, double> pairOfCoordinates = { x, y };
			unorderedMapOfLabelOfVertexToPairOfCoordinates[labelOfVertex] = pairOfCoordinates;
		}

		double lengthOfSideOfHex = GeometryHelper::getLengthOfSideOfHex();
		std::vector<std::string> vectorOfLabelsOfAvailableVertices;
		for (const auto& [labelOfVertex, pairOfCoordinates] : unorderedMapOfLabelOfVertexToPairOfCoordinates) {
			double xCoordinateOfPotentiallyAvailableVertex = pairOfCoordinates.first;
			double yCoordinateOfPotentiallyAvailableVertex = pairOfCoordinates.second;

			if (
				std::find(
					vectorOfLabelsOfOccupiedVertices.begin(),
					vectorOfLabelsOfOccupiedVertices.end(),
					labelOfVertex
				) != vectorOfLabelsOfOccupiedVertices.end()
			) {
				continue;
			}

			bool potentiallyAvailableVertexIsTooCloseToOccupiedVertex = false;
			for (const std::string& labelOfOccupiedVertex : vectorOfLabelsOfOccupiedVertices) {
				std::unordered_map<std::string, std::pair<double, double>>::iterator iterator = unorderedMapOfLabelOfVertexToPairOfCoordinates.find(labelOfOccupiedVertex);
				if (iterator != unorderedMapOfLabelOfVertexToPairOfCoordinates.end()) {
					double xCoordinateOfOccupiedVertex = iterator->second.first;
					double yCoordinateOfOccupiedVertex = iterator->second.second;
					double distance = GeometryHelper::distance(
						xCoordinateOfPotentiallyAvailableVertex,
						yCoordinateOfPotentiallyAvailableVertex,
						xCoordinateOfOccupiedVertex,
						yCoordinateOfOccupiedVertex
					);
					if (distance < lengthOfSideOfHex + GeometryHelper::MARGIN_OF_ERROR) {
						potentiallyAvailableVertexIsTooCloseToOccupiedVertex = true;
						break;
					}
				}
			}
			if (!potentiallyAvailableVertexIsTooCloseToOccupiedVertex) {
				vectorOfLabelsOfAvailableVertices.push_back(labelOfVertex);
			}
		}
		return vectorOfLabelsOfAvailableVertices;
	}

	std::vector<std::string> getVectorOfKeysOfAvailableEdges(std::vector<std::string> vectorOfKeysOfOccupiedEdges) const {
		crow::json::rvalue jsonArrayOfEdgeInformation = boardGeometryCache["edges"];
		std::vector<std::string> vectorOfKeysOfAvailableEdges;
		for (const crow::json::rvalue& jsonObjectOfEdgeInformation : jsonArrayOfEdgeInformation) {
			double x1 = jsonObjectOfEdgeInformation["x1"].d();
			double y1 = jsonObjectOfEdgeInformation["y1"].d();
			double x2 = jsonObjectOfEdgeInformation["x2"].d();
			double y2 = jsonObjectOfEdgeInformation["y2"].d();
			std::string edgeKey = GeometryHelper::getEdgeKey(x1, y1, x2, y2);
			if (
				std::find(
					vectorOfKeysOfOccupiedEdges.begin(),
					vectorOfKeysOfOccupiedEdges.end(),
					edgeKey
				) == vectorOfKeysOfOccupiedEdges.end()
			) {
				vectorOfKeysOfAvailableEdges.push_back(edgeKey);
			}
		}
		if (vectorOfKeysOfAvailableEdges.empty()) {
			throw std::runtime_error("No adjacent edges were found for placing road.");
		}
		return vectorOfKeysOfAvailableEdges;
	}

	std::vector<std::string> getVectorOfKeysOfAvailableEdgesExtendingFromLastBuilding(
		const std::string labelOfVertexOfLastBuilding,
		const std::vector<std::string> vectorOfKeysOfOccupiedEdges
	) const {
		crow::json::rvalue jsonArrayOfEdgeInformation = boardGeometryCache["edges"];
		std::vector<std::string> vectorOfKeysOfAvailableEdges;
		for (const crow::json::rvalue& jsonObjectOfEdgeInformation : jsonArrayOfEdgeInformation) {
			double x1 = jsonObjectOfEdgeInformation["x1"].d();
			double y1 = jsonObjectOfEdgeInformation["y1"].d();
			double x2 = jsonObjectOfEdgeInformation["x2"].d();
			double y2 = jsonObjectOfEdgeInformation["y2"].d();
			std::string labelOfFirstVertex = getLabelOfVertexByCoordinates(x1, y1);
			std::string labelOfSecondVertex = getLabelOfVertexByCoordinates(x2, y2);
			if (labelOfFirstVertex == labelOfVertexOfLastBuilding || labelOfSecondVertex == labelOfVertexOfLastBuilding) {
				std::string edgeKey = GeometryHelper::getEdgeKey(x1, y1, x2, y2);
				if (
					std::find(
						vectorOfKeysOfOccupiedEdges.begin(),
						vectorOfKeysOfOccupiedEdges.end(),
						edgeKey
					) == vectorOfKeysOfOccupiedEdges.end()
				) {
					vectorOfKeysOfAvailableEdges.push_back(edgeKey);
				}
			}
		}
		if (vectorOfKeysOfAvailableEdges.empty()) {
			throw std::runtime_error("No adjacent edges were found for placing road.");
		}
		return vectorOfKeysOfAvailableEdges;
	}

	std::string getLabelOfVertexByCoordinates(double potentialXCoordinate, double potentialYCoordinate) const {
		crow::json::rvalue jsonArrayOfVertexInformation = boardGeometryCache["vertices"];
		if (!jsonArrayOfVertexInformation || jsonArrayOfVertexInformation.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain vertices.");
		}
		for (const crow::json::rvalue& jsonObjectOfVertexInformation : jsonArrayOfVertexInformation) {
			double actualXCoordinate = jsonObjectOfVertexInformation["x"].d();
			double actualYCoordinate = jsonObjectOfVertexInformation["y"].d();
			if (
				std::abs(actualXCoordinate - potentialXCoordinate) < GeometryHelper::MARGIN_OF_ERROR &&
				std::abs(actualYCoordinate - potentialYCoordinate) < GeometryHelper::MARGIN_OF_ERROR
			) {
				return jsonObjectOfVertexInformation["label"].s();
			}
		}
		throw std::runtime_error("No vertex corresponds to given coordinates.");
	}

	std::vector<std::pair<double, double>> getVectorOfPairsOfCoordinatesOfVertices(crow::json::rvalue jsonObjectOfHexInformation) const {
		double heightOfHex = GeometryHelper::WIDTH_OF_HEX * 2 * std::tan(M_PI / 6);
		
		// Get the coordinates of the hex's top-left, reference point.
		double x = jsonObjectOfHexInformation["x"].d();
		double y = jsonObjectOfHexInformation["y"].d();

		std::vector<std::pair<double, double>> vectorOfPairsOfCoordinates;
		// Get six pairs of coordinates of vertices starting from the top vertex and going clockwise.
		vectorOfPairsOfCoordinates.push_back({ x + 0.5 * GeometryHelper::WIDTH_OF_HEX, y });
		vectorOfPairsOfCoordinates.push_back({ x + GeometryHelper::WIDTH_OF_HEX, y + 0.25 * heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x + GeometryHelper::WIDTH_OF_HEX, y + 0.75 * heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x + 0.5 * GeometryHelper::WIDTH_OF_HEX, y + heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x, y + 0.75 * heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x, y + 0.25 * heightOfHex });
		return vectorOfPairsOfCoordinates;
	}

	bool isLabelOfVertex(std::string s) const {
		std::regex vertexPattern("^V\\d{2}$");
		if (std::regex_match(s, vertexPattern)) {
			return true;
		}
		return false;
	}

	bool isEdgeKey(const std::string& s) const {
		std::regex edgePattern("^(\\d+\\.\\d{2})-(\\d+\\.\\d{2})_(\\d+\\.\\d{2})-(\\d+\\.\\d{2})$");
		return std::regex_match(s, edgePattern);
	}

private:

	void loadBoardGeometry() {
		if (!boardGeometryCache) {
			std::ifstream file("../board_geometry.json");
			if (!file.is_open()) {
				throw std::runtime_error("Board geometry file could not be opened.");
			}
			std::stringstream buffer;
			buffer << file.rdbuf();
			boardGeometryCache = crow::json::load(buffer.str());
			if (!boardGeometryCache) {
				throw std::runtime_error("Board geometry file could not be parsed.");
			}
		}
	}
};

crow::json::rvalue Board::boardGeometryCache;