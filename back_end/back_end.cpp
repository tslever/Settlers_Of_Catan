// Solution `back_end` is a C++ back end for playing Settlers of Catan.

// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "config.hpp"
#include "server.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION // This line is required to resolve linker error.
#include "stb_image_write.h"
/* Add to Additional Include Directories "$(SolutionDir)\dependencies".
* Replace `__STDC_LIB_EXT1__` with `_MSC_VER` in `std_image_write.h`.
*/

#include "ai/trainer.hpp"

// TODO: Consider whether database driven state management needs to be implemented more.
// TODO: Consider using a graph database.
// TODO: Consider allowing branching decisions to be made at compile time.

int main() {


	// Create a 21 x 21 2D array of integers, initialized to 0.
	// Index as grid[index_of_row][index_of_column].
	std::vector<std::vector<int>> grid(21, std::vector<int>(21, 0));

	std::vector<std::pair<int, int>> vectorOfPairsOfIndicesOfCentersOfTiles = {
		{0, 0}, // H10
		{4, 2}, // H15
		{2, 4}, // H14
		{-2, 2}, // H09
		{-2, -4}, // H05
		{-4, -2}, // H06
		{2, -2}, // H11
		{6, 0}, // H16
		{8, 4}, // H19
		{6, 6}, // H18
		{4, 8}, // H17
		{0, 6}, // H13
		{-4, 4}, // H08
		{-6, 0}, // H04
		{-8, -4}, // H01
		{-6, -6}, // H02
		{-4, -8}, // H03
		{0, -6}, // H07
		{4, -4} // H12
	};

	for (auto& pairOfIndicesOfCenterOfTile : vectorOfPairsOfIndicesOfCentersOfTiles) {
		int indexOfRow = pairOfIndicesOfCenterOfTile.first + 10;
		int indexOfColumn = pairOfIndicesOfCenterOfTile.second + 10;
		grid[indexOfRow][indexOfColumn] = 1;
	}

	std::vector<std::pair<int, int>> vectorOfPairsOfIndicesOfVertices = {
		{2, 2},
		{0, 2},
		{-2, 0},
		{-2, -2},
		{0, -2},
		{2, 0},
		{4, 0},
		{6, 2},
		{6, 4},
		{4, 4},
		{4, 6},
		{2, 6},
		{0, 4},
		{-2, 4},
		{-4, 2},
		{-4, 0},
		{-6, -2},
		{-6, -4},
		{-4, -4},
		{-4, -6},
		{-2, -6},
		{0, -4},
		{2, -4},
		{4, -2},
		{6, -2},
		{8, 0},
		{8, 2},
		{10, 4},
		{10, 6},
		{8, 6},
		{8, 8},
		{6, 8},
		{6, 10},
		{4, 10},
		{2, 8},
		{0, 8},
		{-2, 6},
		{-4, 6},
		{-6, 4},
		{-6, 2},
		{-8, 0},
		{-8, -2},
		{-10, -4},
		{-10, -6},
		{-8, -6},
		{-8, -8},
		{-6, -8},
		{-6, -10},
		{-4, -10},
		{-2, -8},
		{0, -8},
		{2, -6},
		{4, -6},
		{6, -4}
	};

	for (auto& pairOfIndicesOfVertex : vectorOfPairsOfIndicesOfVertices) {
		int indexOfRow = pairOfIndicesOfVertex.first + 10;
		int indexOfColumn = pairOfIndicesOfVertex.second + 10;
		grid[indexOfRow][indexOfColumn] = 2;
	}

	std::vector<std::pair<int, int>> vectorOfPairsOfIndicesOfEdges = {
		{5, -5},
		{6, -3},
		{3, -6},
		{1, -7},
		{2, -5},

		{-1, -8},
		{-3, -9},
		{-2, -7},
		{-5, -10},
		{-6, -9},
		

		{-7, -8},
		{-5, -7},
		{-8, -7},
		{-9, -6},
		{-7, -5},

		{-10, -5},
		{-9, -3},
		{-8, -1},
		{-7, -2},
		{-7, 1},


		{-6, 3},
		{-5, 2},
		{-5, 5},
		{-3, 6},
		{-1, 7},

		{-2, 5},
		{1, 8},
		{3, 9},
		{2, 7},
		{5, 10},


		{6, 9},
		{7, 8},
		{5, 7},
		{8, 7},
		{9, 6},

		{7, 5},
		{10, 5},
		{9, 3},
		{8, 1},
		{7, 2},


		{7, -1},
		{5, -2},
		{3, -3},
		{4, -1},
		{1, -4},

		{-1, -5},
		{0, -3},
		{-3, -6},
		{-4, -5},
		{-5, -4},


		{-3, -3},
		{-6, -3},
		{-5, -1},
		{-3, 0},
		{-4, 1},

		{-3, 3},
		{-1, 4},
		{0, 3},
		{1, 5},
		{3, 6},


		{4, 5},
		{5, 4},
		{3, 3},
		{6, 3},
		{5, 1},

		{3, 0},
		{1, -1},
		{2, 1},
		{-1, -2},
		{-2, -1},


		{-1, 1},
		{1, 2}
	};

	for (auto& pairOfIndicesOfEdges : vectorOfPairsOfIndicesOfEdges) {
		int indexOfRow = pairOfIndicesOfEdges.first + 10;
		int indexOfColumn = pairOfIndicesOfEdges.second + 10;
		grid[indexOfRow][indexOfColumn] = 3;
	}

	for (int y = 0; y < 21; y++) {
		for (int x = 0; x < 21; x++) {
			std::cout << grid[y][x];
			if (x < 20) {
				std::cout << " ";
			}
		}
		std::cout << std::endl;
	}

	const int cellSize = 10;
	const int gridSize = 21;
	const int imageWidth = cellSize * gridSize;
	const int imageHeight = cellSize * gridSize;

	std::vector<unsigned char> image(imageWidth * imageHeight * 3, 0);

	auto setPixel = [&](int x, int y, int r, int g, int b) {
		int index = (y * imageWidth + x) * 3;
		image[index + 0] = static_cast<unsigned char>(r);
		image[index + 1] = static_cast<unsigned char>(g);
		image[index + 2] = static_cast<unsigned char>(b);
	};

	for (int row = 0; row < gridSize; row++) {
		for (int col = 0; col < gridSize; col++) {
			int value = grid[row][col];
			int r = 0, g = 0, b = 0;
			switch (value) {
			case 0: // Black
				r = 0;   g = 0;   b = 0;
				break;
			case 1: // Red
				r = 255; g = 0;   b = 0;
				break;
			case 2: // Green
				r = 0;   g = 255; b = 0;
				break;
			case 3: // Blue
				r = 0;   g = 0;   b = 255;
				break;
			default:
				break;
			}
			// Top-left pixel of the cell block.
			int xStart = col * cellSize;
			int yStart = row * cellSize;
			for (int dy = 0; dy < cellSize; dy++) {
				for (int dx = 0; dx < cellSize; dx++) {
					setPixel(xStart + dx, yStart + dy, r, g, b);
				}
			}
		}
	}

	// Write the image buffer to a PNG file.
	if (stbi_write_png("output.png", imageWidth, imageHeight, 3, image.data(), imageWidth * 3)) {
		std::cout << "Image successfully written to output.png" << std::endl;
	}
	else {
		std::cerr << "Error writing the image." << std::endl;
	}



	Config::Config config;
	try {
		config = config.load("config.json");
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Loading configuration failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}


    crow::App<Server::CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);


    DB::Database liveDb(config.dbName, config.dbHost, config.dbPassword, config.dbPort, config.dbUsername, "live_");
	try {
		liveDb.initialize();
		std::clog << "[INFO] Database was initialized.\n";
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Initializing database failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	AI::WrapperOfNeuralNetwork neuralNet(config.modelPath, config.numberOfNeurons);

	AI::Trainer trainer(
		&neuralNet,
		config.modelWatcherInterval,
		config.trainingThreshold,
		config.numberOfSimulations,
		config.cPuct,
		config.tolerance,
		config.learningRate,
		config.numberOfEpochs,
		config.batchSize,
		config.dirichletMixingWeight,
		config.dirichletShape
	);
	trainer.startModelWatcher();
	trainer.runTrainingLoop();

	Server::setUpRoutes(app, liveDb, neuralNet, config);

	std::clog << "[INFO] The back end will be started on port " << config.backEndPort << std::endl;
	app.port(config.backEndPort).multithreaded().run();

    return EXIT_SUCCESS;
}