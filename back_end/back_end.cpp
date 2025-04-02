// Solution `back_end` is a C++ back end for playing Settlers of Catan.

// Change C++ Language Standard to ISO C++17 Standard (/std:c++17).

#include "crow.h"
/* Add the following to Additional Include Directories:
- $(SolutionDir)\dependencies\Crow_1_2_1_2\include;
- $(SolutionDir)\dependencies\asio\asio\include;
*/

#include "ai/neural_network.hpp"
#include "ai/self_play.hpp"
#include "ai/trainer.hpp"

#include "config.hpp"

#include "db/database.hpp"

#include "game/game_state.hpp"
#include "game/phase_state_machine.hpp"


/* TODO: Align this back end with the Python back end.
* Migrate self play and training logic.
*/
// TODO: Consider whether database driven state management needs to be implemented more.

// Structure CorsMiddleware is a simple middleware to add CORS headers.
struct CorsMiddleware {
	struct context { };

	void before_handle(crow::request&, crow::response&, context&) {
		// Do nothing.
	}

	// After handling, add CORS headers.
	template <typename AllContext>
	void after_handle(const crow::request&, crow::response& res, context&, AllContext&) {
		res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
		res.add_header("Access-Control-Allow-Headers", "Content-Type");
	}
};

int main() {

	Config::Config config;
	try {
		config = config.load("config.json");
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Loading configuration failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}


    crow::App<CorsMiddleware> app;
	app.loglevel(crow::LogLevel::Info);


    Database db(config.dbName, config.dbHost, config.dbPassword, config.dbPort, config.dbUsername);
	try {
		db.initialize();
		std::clog << "[INFO] Database was initialized.\n";
	}
	catch (const std::exception& e) {
		std::cerr << "[ERROR] Database initialization failed with the following error. " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	AI::WrapperOfNeuralNetwork neuralNet(config.modelPath);

	AI::Trainer trainer(
		&db,
		&neuralNet,
		config.modelWatcherInterval,
		config.trainingThreshold,
		config.numberOfSimulations,
		config.cPuct,
		config.tolerance
	);
	trainer.startModelWatcher();
	trainer.runTrainingLoop();

    CROW_ROUTE(app, "/cities").methods("GET"_method)
    ([&db]() -> crow::json::wvalue {
		try {
			return db.getCitiesJson();
		}
		catch (const std::exception& e) {
			crow::json::wvalue error;
			error["error"] = std::string("Getting cities failed with the following error. ") + e.what();
			return error;
		}
    });


	CROW_ROUTE(app, "/next").methods("POST"_method)
	([&db, &neuralNet, &config]() -> crow::json::wvalue {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint next. The game state will be transitioned." << std::endl;
			GameState currentGameState = db.getGameState();
			PhaseStateMachine phaseStateMachine;
			response = phaseStateMachine.handle(currentGameState, db, neuralNet, config.numberOfSimulations, config.cPuct, config.tolerance);
			db.updateGameState(currentGameState);
		}
		catch (const std::exception& e) {
			response["error"] = std::string("The following error occurred while transitioning the game state. ") + e.what();
			std::cerr << "[ERROR] " << std::string("The following error occurred while transitioning the game state. ") + e.what() << std::endl;
		}
		return response;
	});


	CROW_ROUTE(app, "/reset").methods("POST"_method)
	([&db]() -> crow::json::wvalue {
		crow::json::wvalue response;
		try {
			std::clog << "[INFO] A user posted to endpoint reset. Game state and database will be reset." << std::endl;
			bool success = db.resetGame();
			response["message"] = success ? "Game has been reset to initial state." : "Resetting game failed.";
		}
		catch (const std::exception& e) {
			response["error"] = std::string("Resetting game failed with the following error. ") + e.what();
			std::cerr << "[ERROR] " << std::string("Resetting game failed with the following error. ") + e.what() << std::endl;
		}
		return response;
	});


	CROW_ROUTE(app, "/roads").methods("GET"_method)
	([&db]() -> crow::json::wvalue {
		try {
			return db.getRoadsJson();
		}
		catch (const std::exception& e) {
			crow::json::wvalue error;
			error["error"] = std::string("Getting roads failed with the following error. ") + e.what();
			return error;
		}
	});


	CROW_ROUTE(app, "/")
	([]() -> crow::json::wvalue {
		crow::json::wvalue result;
		result["message"] = "Welcome to the Settlers of Catan API!";
		return result;
	});


	CROW_ROUTE(app, "/settlements").methods("GET"_method)
	([&db]() -> crow::json::wvalue {
		try {
			return db.getSettlementsJson();
		}
		catch (const std::exception& e) {
			crow::json::wvalue error;
			error["error"] = std::string("Getting settlements failed with the following error. ") + e.what();
			return error;
		}
	});

	std::clog << "[INFO] The back end will be started on port " << config.backEndPort << std::endl;
	app.port(config.backEndPort).multithreaded().run();

    return EXIT_SUCCESS;
}