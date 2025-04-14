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