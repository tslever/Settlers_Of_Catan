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

	Board() {
		loadBoardGeometry();
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

	void setPixel(std::vector<unsigned char>& image, int widthOfImage, int x, int y, int r, int g, int b) const {
		constexpr int NUMBER_OF_CHANNELS = 3;
		int index = (y * widthOfImage + x) * NUMBER_OF_CHANNELS;
		image[index + 0] = static_cast<unsigned char>(r);
		image[index + 1] = static_cast<unsigned char>(g);
		image[index + 2] = static_cast<unsigned char>(b);
	};

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
		std::vector<std::pair<int, int>> vectorOfPairsOfIndicesOfCentersOfTiles = {
			{2, 6}, // H01
			{4, 4}, // H02
			{6, 2}, // H03
			{4, 10}, // H04
			{8, 6}, // H05
			{6, 8}, // H06
			{10, 4}, // H07
			{6, 14}, // H08
			{8, 12}, // H09
			{10, 10}, // H10
			{12, 8}, // H11
			{14, 6}, // H12
			{10, 16}, // H13
			{12, 14}, // H14
			{14, 12}, // H15
			{16, 10}, // H16
			{14, 18}, // H17
			{16, 16}, // H18
			{18, 14} // H19
		};

		for (auto& pairOfIndicesOfCenterOfTile : vectorOfPairsOfIndicesOfCentersOfTiles) {
			int indexOfRow = pairOfIndicesOfCenterOfTile.first;
			int indexOfColumn = pairOfIndicesOfCenterOfTile.second;
			grid[indexOfRow][indexOfColumn] = 1;
		}

		std::vector<std::pair<int, int>> vectorOfPairsOfIndicesOfVertices = {
			{0, 4}, // V01
			{2, 4}, // V02
			{4, 6}, // V03
			{4, 8}, // V04
			{2, 8}, // V05
			{0, 6}, // V06
			{2, 2}, // V07
			{4, 2}, // V08
			{6, 4}, // V09
			{6, 6}, // V10
			{4, 0}, // V11
			{6, 0}, // V12
			{8, 2}, // V13
			{8, 4}, // V14
			{6, 10}, // V15
			{6, 12}, // V16
			{4, 12}, // V17
			{2, 10}, // V18
			{8, 8}, // V19
			{8, 10}, // V20
			{10, 6}, // V21
			{10, 8}, // V22
			{10, 2}, // V23
			{12, 4}, // V24
			{12, 6}, // V25
			{8, 14}, // V26
			{8, 16}, // V27
			{6, 16}, // V28
			{4, 14}, // V29
			{10, 12}, // V30
			{10, 14}, // V31
			{12, 10}, // V32
			{12, 12}, // V33
			{14, 8}, // V34
			{14, 10}, // V35
			{14, 4}, // V36
			{16, 6}, // V37
			{16, 8}, // V38
			{12, 16}, // V39
			{12, 18}, // V40
			{10, 18}, // V41
			{14, 14}, // V42
			{14, 16}, // V43
			{16, 12}, // V44
			{16, 14}, // V45
			{18, 10}, // V46
			{18, 12}, // V47
			{16, 18}, // V48
			{16, 20}, // V49
			{14, 20}, // V50
			{18, 16}, // V51
			{18, 18}, // V52
			{20, 14}, // V53
			{20, 16}, // V54
		};

		for (auto& pairOfIndicesOfVertex : vectorOfPairsOfIndicesOfVertices) {
			int indexOfRow = pairOfIndicesOfVertex.first;
			int indexOfColumn = pairOfIndicesOfVertex.second;
			grid[indexOfRow][indexOfColumn] = 2;
		}

		std::vector<std::pair<int, int>> vectorOfPairsOfIndicesOfEdges = {
			{1, 4}, // E01
			{3, 5}, // E02
			{4, 7}, // E03
			{3, 8}, // E04
			{1, 7}, // E05
			{0, 5}, // E06
			{3, 2}, // E07
			{5, 3}, // E08
			{6, 5}, // E09
			{5, 6}, // E10
			{2, 3}, // E11
			{5, 0}, // E12
			{7, 1}, // E13
			{8, 3}, // E14
			{7, 4}, // E15
			{4, 1}, // E16
			{5, 9}, // E17
			{6, 11}, // E18
			{5, 12}, // E19
			{3, 11}, // E20
			{2, 9}, // E21
			{7, 7}, // E22
			{8, 9}, // E23
			{7, 10}, // E24
			{9, 5}, // E25
			{10, 7}, // E26
			{9, 8}, // E27
			{9, 2}, // E28
			{11, 3}, // E29
			{12, 5}, // E30
			{11, 6}, // E31
			{7, 13}, // E32
			{8, 15}, // E33
			{7, 16}, // E34
			{5, 15}, // E35
			{4, 13}, // E36
			{9, 11}, // E37
			{10, 13}, // E38
			{9, 14}, // E39
			{11, 9}, // E40
			{12, 11}, // E41
			{11, 12}, // E42
			{13, 7}, // E43
			{14, 9}, // E44
			{13, 10}, // E45
			{13, 4}, // E46
			{15, 5}, // E47
			{16, 7}, // E48
			{15, 8}, // E49
			{11, 15}, // E50
			{12, 17}, // E51
			{11, 18}, // E52
			{9, 17}, // E53
			{13, 13}, // E54
			{14, 15}, // E55
			{13, 16}, // E56
			{15, 11}, // E57
			{16, 13}, // E58
			{15, 14}, // E59
			{17, 9}, // E60
			{18, 11}, // E61
			{17, 12}, // E62
			{15, 17}, // E63
			{16, 19}, // E64
			{15, 20}, // E65
			{13, 19}, // E66
			{17, 15}, // E67
			{18, 17}, // E68
			{17, 18}, // E69
			{19, 13}, // E70
			{20, 15}, // E71
			{19, 16}, // E72
		};

		for (auto& pairOfIndicesOfEdges : vectorOfPairsOfIndicesOfEdges) {
			int indexOfRow = pairOfIndicesOfEdges.first;
			int indexOfColumn = pairOfIndicesOfEdges.second;
			grid[indexOfRow][indexOfColumn] = 3;
		}

		if (typeOfMove == "settlement" || typeOfMove == "city") {
			if (!representationOfMove.empty() && isLabelOfVertex(representationOfMove)) {
				int indexOfVertex = std::stoi(representationOfMove.substr(1)) - 1;
				if (indexOfVertex >= 0 && indexOfVertex < static_cast<int>(vectorOfPairsOfIndicesOfVertices.size())) {
					std::pair<int, int> pairOfIndicesOfVertex = vectorOfPairsOfIndicesOfVertices[indexOfVertex];
					int indexOfRow = pairOfIndicesOfVertex.first;
					int indexOfColumn = pairOfIndicesOfVertex.second;
					grid[indexOfRow][indexOfColumn] = (typeOfMove == "settlement") ? 4 : 6;
				}
			}
		}
		else if (typeOfMove == "road") {
			std::string labelOfEdge = getEdgeLabel(representationOfMove);
			int indexOfEdge = std::stoi(labelOfEdge.substr(1)) - 1;
			if (indexOfEdge >= 0 && indexOfEdge < static_cast<int>(vectorOfPairsOfIndicesOfEdges.size())) {
				std::pair<int, int> pairOfIndicesOfEdge = vectorOfPairsOfIndicesOfEdges[indexOfEdge];
				int indexOfRow = pairOfIndicesOfEdge.first;
				int indexOfColumn = pairOfIndicesOfEdge.second;
				grid[indexOfRow][indexOfColumn] = 5;
			}
		}

		std::vector<float> vectorRepresentingGrid;
		for (const auto& row : grid) {
			for (const auto& cell : row) {
				vectorRepresentingGrid.push_back(static_cast<float>(cell));
			}
		}

		/*
		const int dimensionOfCell = 10;
		const int widthOfImage = dimensionOfCell * dimensionOfGrid;
		const int heightOfImage = dimensionOfCell * dimensionOfGrid;

		std::vector<unsigned char> image(widthOfImage* heightOfImage * 3, 0);

		for (int row = 0; row < dimensionOfGrid; row++) {
			for (int col = 0; col < dimensionOfGrid; col++) {
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

	bool isLabelOfVertex(const std::string& s) const {
		const std::regex vertexPattern("^V\\d{2}$");
		return std::regex_match(s, vertexPattern);
	}

	bool isEdgeKey(const std::string& s) const {
		std::regex edgePattern("^(\\d+\\.\\d{2})-(\\d+\\.\\d{2})_(\\d+\\.\\d{2})-(\\d+\\.\\d{2})$");
		return std::regex_match(s, edgePattern);
	}

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
};

crow::json::rvalue Board::boardGeometryCache;