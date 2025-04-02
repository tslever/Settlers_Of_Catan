#pragma once


#include <corecrt_math_defines.h>
#include "../db/database.hpp"
#include <regex>


class Board {
public:
	static crow::json::rvalue boardGeometryCache;
	double numberOfHexesThatSpanBoard = 6.0;
	double widthOfBoardInVmin = 100.0;
	double widthOfHex = widthOfBoardInVmin / numberOfHexesThatSpanBoard;

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
		const double marginOfError = 0.01;
		// TODO: Move margin of error to configuration file.
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
				double dx = pairOfCoordinatesOfVertex.first - x;
				double dy = pairOfCoordinatesOfVertex.second - y;
				if (std::sqrt(dx * dx + dy * dy) < marginOfError) {
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
		float normalizedNumberOfPips = (numberOfHexes > 0) ? totalNumberOfPips / (numberOfHexes * 5.0f) : 0.0f;
		float normalizedXCoordinate = x / 100.0f;
		float normalizedYCoordinate = y / 100.0f;
		float normalizedNumberOfHexes = numberOfHexes / 3.0f;
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

		double lengthOfSideOfHex = widthOfHex * std::tan(M_PI / 6);
		double marginOfError = 0.01;
		// TODO: Move margin of error to configuration file.

		std::vector<std::string> vectorOfLabelsOfAvailableVertices;
		for (const std::pair<std::string, std::pair<double, double>>& pairOfLabelOfVertexAndPairOfCoordinates : unorderedMapOfLabelOfVertexToPairOfCoordinates) {
			const std::string& labelOfVertex = pairOfLabelOfVertexAndPairOfCoordinates.first;
			double xCoordinateOfPotentiallyAvailableVertex = pairOfLabelOfVertexAndPairOfCoordinates.second.first;
			double yCoordinateOfPotentiallyAvailableVertex = pairOfLabelOfVertexAndPairOfCoordinates.second.second;

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
					double dx = xCoordinateOfPotentiallyAvailableVertex - xCoordinateOfOccupiedVertex;
					double dy = yCoordinateOfPotentiallyAvailableVertex - yCoordinateOfOccupiedVertex;
					double distance = std::sqrt(dx * dx + dy * dy);
					if (distance < lengthOfSideOfHex + marginOfError) {
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

	std::vector<std::string> getVectorOfKeysOfAvailableEdges(std::vector<std::string> vectorOfKeysOfOccupiedEdges) {
		crow::json::rvalue jsonArrayOfEdgeInformation = boardGeometryCache["edges"];
		std::vector<std::string> vectorOfKeysOfAvailableEdges;
		for (const crow::json::rvalue& jsonObjectOfEdgeInformation : jsonArrayOfEdgeInformation) {
			double x1 = jsonObjectOfEdgeInformation["x1"].d();
			double y1 = jsonObjectOfEdgeInformation["y1"].d();
			double x2 = jsonObjectOfEdgeInformation["x2"].d();
			double y2 = jsonObjectOfEdgeInformation["y2"].d();
			std::string edgeKey = getEdgeKey(x1, y1, x2, y2);
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
				std::string edgeKey = getEdgeKey(x1, y1, x2, y2);
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
		double marginOfError = 1e-2;
		// TODO: Move margin of error to configuration file.
		crow::json::rvalue jsonArrayOfVertexInformation = boardGeometryCache["vertices"];
		if (!jsonArrayOfVertexInformation || jsonArrayOfVertexInformation.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain vertices.");
		}
		for (const crow::json::rvalue& jsonObjectOfVertexInformation : jsonArrayOfVertexInformation) {
			double actualXCoordinate = jsonObjectOfVertexInformation["x"].d();
			double actualYCoordinate = jsonObjectOfVertexInformation["y"].d();
			if (
				std::abs(actualXCoordinate - potentialXCoordinate) < marginOfError &&
				std::abs(actualYCoordinate - potentialYCoordinate) < marginOfError
			) {
				return jsonObjectOfVertexInformation["label"].s();
			}
		}
		throw std::runtime_error("No vertex corresponds to given coordinates.");
	}

	std::vector<std::pair<double, double>> getVectorOfPairsOfCoordinatesOfVertices(crow::json::rvalue jsonObjectOfHexInformation) const {
		double heightOfHex = widthOfHex * 2 * std::tan(M_PI / 6);
		
		// Get the coordinates of the hex's top-left, reference point.
		double x = jsonObjectOfHexInformation["x"].d();
		double y = jsonObjectOfHexInformation["y"].d();

		std::vector<std::pair<double, double>> vectorOfPairsOfCoordinates;
		// Get six pairs of coordinates of vertices starting from the top vertex and going clockwise.
		vectorOfPairsOfCoordinates.push_back({ x + 0.5 * widthOfHex, y });
		vectorOfPairsOfCoordinates.push_back({ x + widthOfHex, y + 0.25 * heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x + widthOfHex, y + 0.75 * heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x + 0.5 * widthOfHex, y + heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x, y + 0.75 * heightOfHex });
		vectorOfPairsOfCoordinates.push_back({ x, y + 0.25 * heightOfHex });
		return vectorOfPairsOfCoordinates;
	}

	std::vector<std::string> getVectorOfKeysOfOccupiedEdges(DB::Database& db) {
		std::vector<std::string> vectorOfKeysOfOccupiedEdges;
		std::vector<Road> vectorOfRoads = db.getRoads();
		for (const Road& road : vectorOfRoads) {
			vectorOfKeysOfOccupiedEdges.push_back(road.edge);
		}
		return vectorOfKeysOfOccupiedEdges;
	}

	bool isLabelOfVertex(std::string s) {
		std::regex vertexPattern("^V\\d{2}$");
		if (std::regex_match(s, vertexPattern)) {
			return true;
		}
		return false;
	}

	bool isEdgeKey(std::string s) const {
		std::regex edgePattern("^(\\d+\\.\\d{2})-(\\d+\\.\\d{2})_(\\d+\\.\\d{2})-(\\d+\\.\\d{2})$");
		if (std::regex_match(s, edgePattern)) {
			return true;
		}
		return false;
	}

	std::vector<std::string> getVectorOfLabelsOfOccupiedVertices(DB::Database& db) {
		std::vector<std::string> listOfLabelsOfOccupiedVertices;
		std::vector<Settlement> settlements = db.getSettlements();
		for (const Settlement& s : settlements) {
			listOfLabelsOfOccupiedVertices.push_back(s.vertex);
		}
		std::vector<City> cities = db.getCities();
		for (const auto& c : cities) {
			listOfLabelsOfOccupiedVertices.push_back(c.vertex);
		}
		return listOfLabelsOfOccupiedVertices;
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
	
	std::string getEdgeKey(double x1, double y1, double x2, double y2) const {
		double marginOfError = 1e-2;
		// TODO: Move margin of error to configuration file.
		char bufferRepresentingEdgeKey[50];
		// TODO: Consider replacing 50 with the maximum length of an edge key.
		if ((x1 < x2) || (std::abs(x1 - x2) < marginOfError && y1 <= y2)) {
			std::snprintf(bufferRepresentingEdgeKey, sizeof(bufferRepresentingEdgeKey), "%.2f-%.2f_%.2f-%.2f", x1, y1, x2, y2);
		}
		else {
			std::snprintf(bufferRepresentingEdgeKey, sizeof(bufferRepresentingEdgeKey), "%.2f-%.2f_%.2f-%.2f", x2, y2, x1, y1);
		}
		std::string edgeKey = std::string(bufferRepresentingEdgeKey);
		return edgeKey;
	}
};

crow::json::rvalue Board::boardGeometryCache;