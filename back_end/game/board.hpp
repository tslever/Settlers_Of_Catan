#pragma once


#include <corecrt_math_defines.h>
#include "../db/database.hpp"
#include <regex>
#include <unordered_set>

/*#define STB_IMAGE_WRITE_IMPLEMENTATION // This line is required to resolve linker error.
#include "stb_image_write.h"*/
/* Add to Additional Include Directories "$(SolutionDir)\dependencies".
* Replace `__STDC_LIB_EXT1__` with `_MSC_VER` in `std_image_write.h`.
*/


class Board {


public:
	

	static crow::json::rvalue isometricCoordinatesCache;


	Board() {
		loadIsometricCoordinates();
	}


	std::vector<float> getGridRepresentationForMove(const std::string& move, const std::string& typeOfMove) const {

		// Create a 21 x 21 grid of integers initialized to 0.
		// Index as grid[index_of_row][index_of_column].
		constexpr int DIMENSION_OF_GRID = 21;
		std::vector<std::vector<int>> grid(DIMENSION_OF_GRID, std::vector<int>(DIMENSION_OF_GRID, 0));

		const crow::json::rvalue& jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfHexes = isometricCoordinatesCache["objectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfHexes"];
		const crow::json::rvalue& jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices = isometricCoordinatesCache["objectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices"];
		const crow::json::rvalue& jsonObjectOfIdsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges = isometricCoordinatesCache["objectOfIdsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges"];

		const crow::json::rvalue& jsonObjectOfIdsOfHexesAndNamesOfResources = isometricCoordinatesCache["objectOfIdsOfHexesAndNamesOfResources"];

		for (const crow::json::rvalue& pairOfCoordinates : jsonObjectOfIdsOfHexesAndPairsOfCoordinatesOfCentersOfHexes) {
			double x = pairOfCoordinates[0].d();
			double y = pairOfCoordinates[1].d();
			std::string idOfHex = pairOfCoordinates.key();
			std::string nameOfResource = jsonObjectOfIdsOfHexesAndNamesOfResources[idOfHex].s();
			if (nameOfResource == "nothing") {
				grid[static_cast<int>(y)][static_cast<int>(x)] = 1;
			}
			else if (nameOfResource == "brick") {
				grid[static_cast<int>(y)][static_cast<int>(x)] = 2;
			}
			else if (nameOfResource == "grain") {
				grid[static_cast<int>(y)][static_cast<int>(x)] = 3;
			}
			else if (nameOfResource == "lumber") {
				grid[static_cast<int>(y)][static_cast<int>(x)] = 4;
			}
			else if (nameOfResource == "ore") {
				grid[static_cast<int>(y)][static_cast<int>(x)] = 5;
			}
			else if (nameOfResource == "wool") {
				grid[static_cast<int>(y)][static_cast<int>(x)] = 6;
			}
			else {
				throw std::runtime_error(nameOfResource + " is an unknown resource type.");
			}
		}

		for (const crow::json::rvalue& pairOfCoordinates : jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices) {
			double x = pairOfCoordinates[0].d();
			double y = pairOfCoordinates[1].d();
			grid[static_cast<int>(y)][static_cast<int>(x)] = 7;
		}

		for (const crow::json::rvalue& pairOfCoordinates : jsonObjectOfIdsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges) {
			double x = pairOfCoordinates[0].d();
			double y = pairOfCoordinates[1].d();
			grid[static_cast<int>(y)][static_cast<int>(x)] = 8;
		}

		if (typeOfMove == "settlement" || typeOfMove == "city") {
			if (jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices.has(move)) {
				const auto& pairOfCoordinatesOfVertex = jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices[move];
				int x = pairOfCoordinatesOfVertex[0].d();
				int y = pairOfCoordinatesOfVertex[1].d();
				grid[static_cast<int>(y)][static_cast<int>(x)] = (typeOfMove == "settlement") ? 9 : 10;
			}
		}
		else if (typeOfMove == "road") {
			if (jsonObjectOfIdsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges.has(move)) {
				const auto& pairOfCoordinates = jsonObjectOfIdsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges[move];
				int x = pairOfCoordinates[0].d();
				int y = pairOfCoordinates[1].d();
				grid[static_cast<int>(y)][static_cast<int>(x)] = 11;
			}
		}
		else if (typeOfMove == "wall") {
			if (jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices.has(move)) {
				int x = jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices[move][0].d();
				int y = jsonObjectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices[move][1].d();
				grid[static_cast<int>(y)][static_cast<int>(x)] = 12;
			}
		}
		else if (typeOfMove == "pass") {
			// Do nothing.
		}
		else {
			throw std::runtime_error(typeOfMove + " is an unknown type of move.");
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


	std::vector<std::string> getVectorOfLabelsOfAvailableEdges(const std::vector<std::string>& vectorOfLabelsOfOccupiedEdges) const {
		const crow::json::rvalue& jsonObjectOfLabelsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges = isometricCoordinatesCache["objectOfIdsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges"];
		
		std::vector<std::string> vectorOfLabelsOfEdges;
		vectorOfLabelsOfEdges.reserve(jsonObjectOfLabelsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges.size());
		for (const crow::json::rvalue& keyValuePair : jsonObjectOfLabelsOfEdgesAndPairsOfCoordinatesOfCentersOfEdges) {
			vectorOfLabelsOfEdges.push_back(keyValuePair.key());
		}

		std::vector<std::string> vectorOfLabelsOfAvailableEdges;
		for (const auto& labelOfEdge : vectorOfLabelsOfEdges) {
			if (std::find(vectorOfLabelsOfOccupiedEdges.begin(), vectorOfLabelsOfOccupiedEdges.end(), labelOfEdge) == vectorOfLabelsOfOccupiedEdges.end()) {
				vectorOfLabelsOfAvailableEdges.push_back(labelOfEdge);
			}
		}

		return vectorOfLabelsOfAvailableEdges;
	};


	std::vector<std::string> getVectorOfLabelsOfAvailableEdgesExtendingFromLastBuilding(
		const std::string& labelOfVertexOfLastBuilding,
		const std::vector<std::string>& vectorOfLabelsOfOccupiedEdges
	) const {
		const crow::json::rvalue& jsonObjectOfAdjacencyInformation = isometricCoordinatesCache["objectOfIdsOfVerticesAndListsOfEdgesExtendingFromThoseVertices"];

		std::vector<std::string> vectorOfLabelsOfAvailableEdges;
		for (const crow::json::rvalue& jsonObjectWithLabelOfEdge : jsonObjectOfAdjacencyInformation[labelOfVertexOfLastBuilding]) {
			std::string labelOfEdge = jsonObjectWithLabelOfEdge.s();
			if (std::find(vectorOfLabelsOfOccupiedEdges.begin(), vectorOfLabelsOfOccupiedEdges.end(), labelOfEdge) == vectorOfLabelsOfOccupiedEdges.end()) {
				vectorOfLabelsOfAvailableEdges.push_back(labelOfEdge);
			}
		}

		return vectorOfLabelsOfAvailableEdges;
	};


	std::vector<std::string> getVectorOfLabelsOfAvailableVertices(const std::vector<std::string>& vectorOfLabelsOfOccupiedVertices) const {
		const crow::json::rvalue& jsonObjectOfLabelsOfVerticesAndPairsOfCoordinatesOfVertices = isometricCoordinatesCache["objectOfIdsOfVerticesAndPairsOfCoordinatesOfVertices"];
		const crow::json::rvalue& jsonObjectOfLabelsOfVerticesAndListsOfLabelsOfAdjacentVertices = isometricCoordinatesCache["objectOfIdsOfVerticesAndListsOfIdsOfAdjacentVertices"];
		
		std::unordered_set<std::string> unorderedSetOfLabelsOfOccupiedVertices(vectorOfLabelsOfOccupiedVertices.begin(), vectorOfLabelsOfOccupiedVertices.end());
		std::vector<std::string> vectorOfLabelsOfAvailableVertices;
		
		for (const crow::json::rvalue& keyValuePair : jsonObjectOfLabelsOfVerticesAndPairsOfCoordinatesOfVertices) {
			const std::string labelOfVertex = keyValuePair.key();
			if (unorderedSetOfLabelsOfOccupiedVertices.contains(labelOfVertex)) {
				continue;
			}
			bool adjacentVertexIsOccupied = false;
			for (const crow::json::rvalue& jsonObjectWithLabelOfAdjacentVertex : jsonObjectOfLabelsOfVerticesAndListsOfLabelsOfAdjacentVertices[labelOfVertex]) {
				std::string labelOfAdjacentVertex = jsonObjectWithLabelOfAdjacentVertex.s();
				if (unorderedSetOfLabelsOfOccupiedVertices.contains(labelOfAdjacentVertex)) {
					adjacentVertexIsOccupied = true;
					break;
				}
			}
			if (!adjacentVertexIsOccupied) {
				vectorOfLabelsOfAvailableVertices.push_back(labelOfVertex);
			}
		}
		return vectorOfLabelsOfAvailableVertices;
	}


	std::pair<std::string, std::string> getVerticesOfEdge(const std::string& labelOfEdge) const {
		const crow::json::rvalue& jsonObjectOfLabelsOfEdgesAndPairsOfLabelsOfVertices = isometricCoordinatesCache["objectOfIdsOfEdgesAndPairsOfIdsOfVertices"];
		const auto& pair = jsonObjectOfLabelsOfEdgesAndPairsOfLabelsOfVertices[labelOfEdge];
		return { pair[0].s(), pair[1].s() };
	}


	bool isLabelOfEdge(const std::string& s) const {
		const std::regex edgePattern("^E\\d{2}$");
		return std::regex_match(s, edgePattern);
	}


	bool isLabelOfVertex(const std::string& s) const {
		const std::regex vertexPattern("^V\\d{2}$");
		return std::regex_match(s, vertexPattern);
	}


	void setPixel(std::vector<unsigned char>& image, int widthOfImage, int x, int y, int r, int g, int b) const {
		constexpr int NUMBER_OF_CHANNELS = 3;
		int index = (y * widthOfImage + x) * NUMBER_OF_CHANNELS;
		image[index + 0] = static_cast<unsigned char>(r);
		image[index + 1] = static_cast<unsigned char>(g);
		image[index + 2] = static_cast<unsigned char>(b);
	};


private:


	void loadIsometricCoordinates() {
		static std::once_flag onceFlag;
		std::call_once(onceFlag, [] {
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
		});
	}
};


crow::json::rvalue Board::isometricCoordinatesCache;