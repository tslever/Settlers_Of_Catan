#pragma once


#include <corecrt_math_defines.h>
#include "../db/database.hpp"
#include "geometry_helper.hpp"
#include <regex>

/*#define STB_IMAGE_WRITE_IMPLEMENTATION // This line is required to resolve linker error.
#include "stb_image_write.h"*/
/* Add to Additional Include Directories "$(SolutionDir)\dependencies".
* Replace `__STDC_LIB_EXT1__` with `_MSC_VER` in `std_image_write.h`.
*/


class Board {


public:
	
	
	static crow::json::rvalue boardGeometryCache;
	static crow::json::rvalue isometricCoordinatesCache;


	Board() {
		loadBoardGeometry();
		loadIsometricCoordinates();
	}


	std::string getEdgeLabel(const std::string& providedEdgeKey) const {
		if (!boardGeometryCache || boardGeometryCache["edges"].size() == 0) {
			throw std::runtime_error("Board geometry file does not contain edges.");
		}
		const crow::json::rvalue& jsonArrayOfEdgeInformation = boardGeometryCache["edges"];
		for (size_t i = 0; i < jsonArrayOfEdgeInformation.size(); i++) {
			const crow::json::rvalue& jsonObjectOfEdgeInformation = jsonArrayOfEdgeInformation[i];
			double x1 = jsonObjectOfEdgeInformation["x1"].d();
			double y1 = jsonObjectOfEdgeInformation["y1"].d();
			double x2 = jsonObjectOfEdgeInformation["x2"].d();
			double y2 = jsonObjectOfEdgeInformation["y2"].d();
			std::string edgeKey = GeometryHelper::getEdgeKey(x1, y1, x2, y2);
			if (edgeKey == providedEdgeKey) {
				char buffer[10];
				std::snprintf(buffer, sizeof(buffer), "E%02zu", i + 1);
				std::string edgeLabel = std::string(buffer);
				return edgeLabel;
			}
		}
		throw std::runtime_error("An edge label was not returned.");
	}


	std::vector<float> getGridRepresentationForMove(const std::string& representationOfMove, const std::string& typeOfMove) const {

		// Create a 21 x 21 grid of integers initialized to 0.
		// Index as grid[index_of_row][index_of_column].
		constexpr int DIMENSION_OF_GRID = 21;
		std::vector<std::vector<int>> grid(DIMENSION_OF_GRID, std::vector<int>(DIMENSION_OF_GRID, 0));

		/* TODO: The following vectors represent pairs of coordinates in the grid and
		* isometric coordinates of the centers of hexes, vertices, and centers of edges in the board.
		* Consider revising `generate_board_geometry.py`, `board_geometry.json`, back end code, and/or front end code
		* to replace all uses of Cartesian coordinates with isometric coordinates.
		*/
		const crow::json::rvalue& jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfHexes = isometricCoordinatesCache["objectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfHexes"];
		const crow::json::rvalue& jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfVertices = isometricCoordinatesCache["objectOfIdsOfHexesAndPairsOfCoordinatesOfVertices"];
		const crow::json::rvalue& jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfEdges = isometricCoordinatesCache["objectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfEdges"];

		for (const crow::json::rvalue& pairOfCoordinates : jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfHexes) {
			double x = pairOfCoordinates[0].d();
			double y = pairOfCoordinates[1].d();
			grid[static_cast<int>(y)][static_cast<int>(x)] = 1;
		}

		for (const crow::json::rvalue& pairOfCoordinates : jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfVertices) {
			double x = pairOfCoordinates[0].d();
			double y = pairOfCoordinates[1].d();
			grid[static_cast<int>(y)][static_cast<int>(x)] = 2;
		}

		if (typeOfMove == "settlement" || typeOfMove == "city") {
			if (jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfVertices.has(representationOfMove)) {
				const auto& move = jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfVertices[representationOfMove];
				int x = move[0].d();
				int y = move[1].d();
				grid[static_cast<int>(y)][static_cast<int>(x)] = (typeOfMove == "settlement") ? 4 : 5;
			}
		}

		for (const crow::json::rvalue& pairOfCoordinates : jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfEdges) {
			double x = pairOfCoordinates[0].d();
			double y = pairOfCoordinates[1].d();
			grid[static_cast<int>(y)][static_cast<int>(x)] = 3;
		}

		if (typeOfMove == "road") {
			std::string edgeLabel = getEdgeLabel(representationOfMove);
			if (jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfEdges.has(edgeLabel)) {
				const auto& move = jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfEdges[edgeLabel];
				int x = move[0].d();
				int y = move[1].d();
				grid[static_cast<int>(y)][static_cast<int>(x)] = 6;
			}
		}

		std::vector<float> vectorRepresentingGrid;
		vectorRepresentingGrid.reserve(DIMENSION_OF_GRID * DIMENSION_OF_GRID);
		for (const std::vector<int> row : grid) {
			for (int cell : row) {
				vectorRepresentingGrid.push_back(static_cast<float>(cell));
			}
		}

		/*const int dimensionOfCell = 10;
		const int widthOfImage = dimensionOfCell * DIMENSION_OF_GRID;
		const int heightOfImage = dimensionOfCell * DIMENSION_OF_GRID;

		std::vector<unsigned char> image(widthOfImage* heightOfImage * 3, 0);

		for (int row = 0; row < DIMENSION_OF_GRID; row++) {
			for (int col = 0; col < DIMENSION_OF_GRID; col++) {
				int value = grid[row][col];
				int r = 0;
				int g = 0;
				int b = 0;
				switch (value) {
				case 0:
					r = 0;
					g = 0;
					b = 0;
					break;
				case 1:
					r = 255;
					g = 0;
					b = 0;
					break;
				case 2:
					r = 0;
					g = 255;
					b = 0;
					break;
				case 3:
					r = 0;
					g = 0;
					b = 255;
					break;
				case 4:
					r = 255;
					g = 255;
					b = 0;
					break;
				case 5:
					r = 255;
					g = 0;
					b = 255;
					break;
				case 6:
					r = 0;
					g = 255;
					b = 255;
					break;
				default:
					break;
				}
				int horizontalPositionOfTopLeftPixelOfCell = col * dimensionOfCell;
				int verticalPositionOfTopLeftPixelOfCell = row * dimensionOfCell;
				for (int dy = 0; dy < dimensionOfCell; dy++) {
					for (int dx = 0; dx < dimensionOfCell; dx++) {
						setPixel(
							image,
							widthOfImage,
							horizontalPositionOfTopLeftPixelOfCell + dx,
							verticalPositionOfTopLeftPixelOfCell + dy,
							r,
							g,
							b
						);
					}
				}
			}
		}

		if (stbi_write_png("output.png", widthOfImage, heightOfImage, 3, image.data(), widthOfImage * 3)) {
			std::cout << "Image successfully written to output.png" << std::endl;
		}
		else {
			Logger::error("getGridRepresentationForMove", "Writing image failed.");
		}*/

		return vectorRepresentingGrid;
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


	bool isLabelOfVertex(const std::string& s) const {
		const std::regex vertexPattern("^V\\d{2}$");
		return std::regex_match(s, vertexPattern);
	}


	bool isEdgeKey(const std::string& s) const {
		std::regex edgePattern("^(\\d+\\.\\d{2})-(\\d+\\.\\d{2})_(\\d+\\.\\d{2})-(\\d+\\.\\d{2})$");
		return std::regex_match(s, edgePattern);
	}


	void setPixel(std::vector<unsigned char>& image, int widthOfImage, int x, int y, int r, int g, int b) const {
		constexpr int NUMBER_OF_CHANNELS = 3;
		int index = (y * widthOfImage + x) * NUMBER_OF_CHANNELS;
		image[index + 0] = static_cast<unsigned char>(r);
		image[index + 1] = static_cast<unsigned char>(g);
		image[index + 2] = static_cast<unsigned char>(b);
	};


private:


	void loadBoardGeometry() {
		if (!boardGeometryCache) {
			std::ifstream file("../generate_board_geometry/board_geometry.json");
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

	void loadIsometricCoordinates() {
		if (!isometricCoordinatesCache) {
			std::ifstream file("../generate_board_geometry/isometric_coordinates.json");
			if (!file.is_open()) {
				throw std::runtime_error("Isometric coordinates file could not be opened.");
			}
			std::stringstream buffer;
			buffer << file.rdbuf();
			isometricCoordinatesCache = crow::json::load(buffer.str());
			if (!isometricCoordinatesCache) {
				throw std::runtime_error("Isometric coordinates file could not be parsed.");
			}
		}
	}
};


crow::json::rvalue Board::boardGeometryCache;
crow::json::rvalue Board::isometricCoordinatesCache;