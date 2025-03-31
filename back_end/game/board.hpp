#pragma once


#include <corecrt_math_defines.h>
#include "../db/database.hpp"
#include <regex>


// TODO: Deduplicate code across files.
// TODO: Consider whether more functions in this file should be encapsulated in class Board.


// Function `readBoardGeometry` reads and parses board geometry file.
crow::json::rvalue readBoardGeometry() {
	std::ifstream file("../board_geometry.json");
	if (!file.is_open()) {
		throw std::runtime_error("Board geometry file could not be opened.");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	crow::json::rvalue boardGeometry = crow::json::load(buffer.str());
	if (!boardGeometry) {
		throw std::runtime_error("Board geometry file could not be parsed.");
	}
	return boardGeometry;
}


class Board {
public:
	static crow::json::rvalue boardGeometryCache;


	static void loadBoardGeometry() {
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


	Board() {
		loadBoardGeometry();
	}


	/* Function `getFeatureVector` computes and returns a 5 element feature vector for a given vertex label.
	* Features include:
	* - normalized total number of pips,
	* - normalized x coordinate,
	* - normalized y coordinate,
	* - normalized hex count, and
	* - bias 1.0.
	*/
	std::vector<float> getFeatureVector(const std::string& labelOfVertexOrEdgeKey) const {

		if (labelOfVertexOrEdgeKey.find('_') != std::string::npos) {
			// TODO: Calculate feature vector corresponding to edge and/or revise feature vector to include more information relating to edge.
			/*float normalizedNumberOfPips = 0.5f;
			float normalizedXCoordinate = 0.5f;
			float normalizedYCoordinate = 0.5f;
			float normalizedNumberOfHexes = 0.5f;
			float bias = 1.0f;*/
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
		float totalNumberOfPips = 0.0f;
		int numberOfHexes = 0;

		for (const crow::json::rvalue jsonObjectOfHexInformation : jsonArrayOfHexInformation) {
			std::vector<std::pair<double, double>> vectorOfPairsOfCoordinatesOfVertices = getVectorOfPairsOfCoordinatesOfVertices(jsonObjectOfHexInformation);
			for (const std::pair<double, double> pairOfCoordinatesOfVertex : vectorOfPairsOfCoordinatesOfVertices) {
				double dx = pairOfCoordinatesOfVertex.first - x;
				double dy = pairOfCoordinatesOfVertex.second - y;
				if (std::sqrt(dx * dx + dy * dy) < marginOfError) {
					std::string idOfHex = jsonObjectOfHexInformation["id"].s();
					std::unordered_map<std::string, int>::iterator iterator = mapOfIdOfHexToNumberOfToken.find(idOfHex);
					if (iterator != mapOfIdOfHexToNumberOfToken.end()) {
						int numberOfToken = iterator->second;
						std::unordered_map<int, int>::iterator iterator = mapOfNumberOfTokenToNumberOfPips.find(numberOfToken);
						if (iterator != mapOfNumberOfTokenToNumberOfPips.end()) {
							totalNumberOfPips += iterator->second;
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
		float bias = 1.0f;
		return { normalizedNumberOfPips, normalizedXCoordinate, normalizedYCoordinate, normalizedNumberOfHexes, bias };
	}


	std::vector<std::pair<double, double>> getVectorOfPairsOfCoordinatesOfVertices(crow::json::rvalue jsonObjectOfHexInformation) const {
		// Extract the top-left coordinate (the hex's reference point) from the JSON object.
		double x = jsonObjectOfHexInformation["x"].d();
		double y = jsonObjectOfHexInformation["y"].d();

		// Hardcoded board dimensions: board width 100 vmin and 6 hexes per row.
		const double boardWidth = 100.0;
		const double numHexesPerRow = 6.0;
		double widthOfHex = boardWidth / numHexesPerRow;

		// For a regular hexagon the height is given by:
		// heightOfHex = widthOfHex * (2 * tan(pi/6))
		double tan_pi_6 = std::tan(M_PI / 6);
		double heightOfHex = widthOfHex * (2 * tan_pi_6);

		std::vector<std::pair<double, double>> vertices;
		// Compute the six vertices (ordered clockwise starting from the top vertex).
		vertices.push_back({ x + 0.5 * widthOfHex, y });
		vertices.push_back({ x + widthOfHex, y + 0.25 * heightOfHex });
		vertices.push_back({ x + widthOfHex, y + 0.75 * heightOfHex });
		vertices.push_back({ x + 0.5 * widthOfHex, y + heightOfHex });
		vertices.push_back({ x, y + 0.75 * heightOfHex });
		vertices.push_back({ x, y + 0.25 * heightOfHex });
		return vertices;
	}


	// Function `getVertexLabelByCoordinates` loads the board geometry,
	// then iterates over the vertices, comparing the provided x and y with each vertex's coordinates.
	std::string getVertexLabelByCoordinates(double x, double y) const {
		double marginOfError = 1e-2;
		crow::json::rvalue verticesJson = boardGeometryCache["vertices"];
		if (!verticesJson || verticesJson.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain vertices.");
		}
		for (const crow::json::rvalue& jsonObjectOfVertexInformation : verticesJson) {
			double vx = jsonObjectOfVertexInformation["x"].d();
			double vy = jsonObjectOfVertexInformation["y"].d();
			if (std::abs(vx - x) < marginOfError && std::abs(vy - y) < marginOfError) {
				return jsonObjectOfVertexInformation["label"].s();
			}
		}
		return "";
	}

	/* Function `getAvailableVertices` gets the available vertices given a list of already occupied vertices.
	* `getAvailableVertices` reads the board geometry and filters out any vertex that is either occupied
	* or is too close (i.e. adjacent) to any occupied vertex.
	*/
	std::vector<std::string> getVectorOfLabelsOfAvailableVertices(const std::vector<std::string>& addressOfListOfLabelsOfOccupiedVertices) {
		crow::json::rvalue verticesJson = boardGeometryCache["vertices"];
		if (!verticesJson || verticesJson.size() == 0) {
			throw std::runtime_error("Board geometry file does not contain vertices.");
		}

		// Build a map of vertex label to its (x,y) coordinates.
		std::unordered_map<std::string, std::pair<double, double>> vertexCoords;
		for (size_t i = 0; i < verticesJson.size(); i++) {
			crow::json::rvalue v = verticesJson[i];
			std::string label = v["label"].s();
			double x = v["x"].d();
			double y = v["y"].d();
			vertexCoords[label] = { x, y };
		}

		// Set up board constants.
		double width_of_board_in_vmin = 100.0;
		double number_of_hexes_that_span_board = 6.0;
		double width_of_hex = width_of_board_in_vmin / number_of_hexes_that_span_board;
		double length_of_side_of_hex = width_of_hex * std::tan(M_PI / 6);
		double margin_of_error = 0.01;

		std::vector<std::string> available;
		// For each vertex in the board geometry:
		for (const std::pair<std::string, std::pair<double, double>>& pair : vertexCoords) {
			const std::string& label = pair.first;
			double x1 = pair.second.first;
			double y1 = pair.second.second;

			// Skip if the vertex is already occupied.
			if (
				std::find(
					addressOfListOfLabelsOfOccupiedVertices.begin(),
					addressOfListOfLabelsOfOccupiedVertices.end(),
					label
				) != addressOfListOfLabelsOfOccupiedVertices.end()
				)
				continue;

			bool tooClose = false;
			// Check the distance to every occupied vertex.
			for (const std::string& occLabel : addressOfListOfLabelsOfOccupiedVertices) {
				std::unordered_map<std::string, std::pair<double, double>>::iterator iterator = vertexCoords.find(occLabel);
				if (iterator != vertexCoords.end()) {
					double x2 = iterator->second.first;
					double y2 = iterator->second.second;
					double distance = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
					// If the vertex is within or exactly at the forbidden distance, mark as too close.
					if (distance <= length_of_side_of_hex + margin_of_error) {
						tooClose = true;
						break;
					}
				}
			}
			if (!tooClose) {
				available.push_back(label);
			}
		}
		return available;
	}

	
	std::string getEdgeKey(double x1, double y1, double x2, double y2) {
		double marginOfError = 1e-2;
		char bufferRepresentingEdgeKey[50];
		if ((x1 < x2) || (std::abs(x1 - x2) < marginOfError && y1 <= y2)) {
			std::snprintf(bufferRepresentingEdgeKey, sizeof(bufferRepresentingEdgeKey), "%.2f-%.2f_%.2f-%.2f", x1, y1, x2, y2);
		}
		else {
			std::snprintf(bufferRepresentingEdgeKey, sizeof(bufferRepresentingEdgeKey), "%.2f-%.2f_%.2f-%.2f", x2, y2, x1, y1);
		}
		std::string edgeKey = std::string(bufferRepresentingEdgeKey);
		return edgeKey;
	}


	std::vector<std::string> getVectorOfKeysOfAvailableEdges(std::vector<std::string> vectorOfKeysOfOccupiedEdges) {
		crow::json::rvalue jsonArrayOfEdgeInformation = boardGeometryCache["edges"];
		std::vector<std::string> vectorOfKeysOfAvailableEdges;
		for (const auto& jsonObjectOfEdgeInformation : jsonArrayOfEdgeInformation) {
			double x1 = jsonObjectOfEdgeInformation["x1"].d();
			double y1 = jsonObjectOfEdgeInformation["y1"].d();
			double x2 = jsonObjectOfEdgeInformation["x2"].d();
			double y2 = jsonObjectOfEdgeInformation["y2"].d();
			std::string edgeKey = getEdgeKey(x1, y1, x2, y2);
			vectorOfKeysOfAvailableEdges.push_back(edgeKey);
		}
		if (vectorOfKeysOfAvailableEdges.empty()) {
			throw std::runtime_error("No adjacent edges were found for placing road.");
		}
		return vectorOfKeysOfAvailableEdges;
	}


	std::vector<std::string> getVectorOfKeysOfAvailableEdgesExtendingFromLastBuilding(std::string labelOfVertexOfLastBuilding, std::vector<std::string> vectorOfKeysOfOccupiedEdges) {
		crow::json::rvalue jsonArrayOfEdgeInformation = boardGeometryCache["edges"];
		std::vector<std::string> vectorOfKeysOfAvailableEdges;
		for (const auto& jsonObjectOfEdgeInformation : jsonArrayOfEdgeInformation) {
			double x1 = jsonObjectOfEdgeInformation["x1"].d();
			double y1 = jsonObjectOfEdgeInformation["y1"].d();
			double x2 = jsonObjectOfEdgeInformation["x2"].d();
			double y2 = jsonObjectOfEdgeInformation["y2"].d();
			std::string labelOfFirstVertex = getVertexLabelByCoordinates(x1, y1);
			std::string labelOfSecondVertex = getVertexLabelByCoordinates(x2, y2);
			if (labelOfFirstVertex == labelOfVertexOfLastBuilding || labelOfSecondVertex == labelOfVertexOfLastBuilding) {
				std::string edgeKey = getEdgeKey(x1, y1, x2, y2);
				vectorOfKeysOfAvailableEdges.push_back(edgeKey);
			}
		}
		if (vectorOfKeysOfAvailableEdges.empty()) {
			throw std::runtime_error("No adjacent edges were found for placing road.");
		}
		return vectorOfKeysOfAvailableEdges;
	}
};


// Fuction `getOccupiedVertices` gets a list of occupied vertex labels by querying the database.
std::vector<std::string> getVectorOfLabelsOfOccupiedVertices(Database& db) {
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


std::vector<std::string> getVectorOfKeysOfOccupiedEdges(Database& db) {
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


bool isEdgeKey(std::string s) {
	std::regex edgePattern("^(\\d+\\.\\d{2})-(\\d+\\.\\d{2})_(\\d+\\.\\d{2})-(\\d+\\.\\d{2})$");
	if (std::regex_match(s, edgePattern)) {
		return true;
	}
	return false;
}


crow::json::rvalue Board::boardGeometryCache;