#pragma once

#include "crow.h"
#include "game_state.hpp"
#include <random>


// Function `getRandomEdgeKey` is a helper function that loads board geometry and selects a random edge key.
std::string getRandomEdgeKey() {
	std::ifstream file("../board_geometry.json");
	if (!file.is_open()) {
		throw std::runtime_error("Board geometry file could not be opened.");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	auto jsonVal = crow::json::load(buffer.str());
	if (!jsonVal) {
		throw std::runtime_error("Board geometry file could not be parsed.");
	}
	auto edges = jsonVal["edges"];
	if (!edges || edges.size() == 0) {
		throw std::runtime_error("Board geometry file does not contain edges.");
	}

	// Select a random edge.
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<> dist(0, edges.size() - 1); // [0, edges.size() - 1]
	int index = dist(rng);
	auto edge = edges[index];
	double x1 = edge["x1"].d();
	double y1 = edge["y1"].d();
	double x2 = edge["x2"].d();
	double y2 = edge["y2"].d();

	// Format the edge key: sort endpoints so that order is consistent.
	char buf[50];
	if ((x1 < x2) || (std::abs(x1 - x2) < 1e-2 && y1 <= y2)) {
		std::snprintf(buf, sizeof(buf), "%.2f-%.2f_%.2f-%.2f", x1, y1, x2, y2);
	}
	else {
		std::snprintf(buf, sizeof(buf), "%.2f-%.2f_%.2f-%.2f", x2, y2, x1, y1);
	}
	return std::string(buf);
}


// Class PhaseState is a base abstract class that represents a phase handler / state.
class PhaseState {
public:
	virtual ~PhaseState() = default;
	virtual crow::json::wvalue handle(GameState& state, Database& db) = 0;
};


/* Class PlaceFirstSettlementState is a concrete class
* that represents a handler of the phase / state to place the first settlement.
*/
class PlaceFirstSettlementState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;
		
		// For demonstration, we use a random vertex label.
		// TODO: Replace with AI / board logic.
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<> dist(1, 54); // [1, 54]
		int vertexIndex = dist(rng);
		std::string chosenVertex = (vertexIndex < 10)
			? "V0" + std::to_string(vertexIndex)
			: "V" + std::to_string(vertexIndex);

		int player = state.currentPlayer;
		state.placeSettlement(player, chosenVertex);
		// Transition to road phase.
		state.phase = Phase::TO_PLACE_FIRST_ROAD;
		// Persist settlement in database.
		int settlementId = db.addSettlement(player, chosenVertex);

		result["message"] = "Player " + std::to_string(player) + " placed a settlement at " + chosenVertex + ".";
		result["moveType"] = "settlement";
		crow::json::wvalue settlementJson;
		settlementJson["id"] = settlementId;
		settlementJson["player"] = player;
		settlementJson["vertex"] = chosenVertex;
		result["settlement"] = std::move(settlementJson);
		return result;
	}
};


/* Class PlaceFirstRoadState is a concrete class
* that represents a handler of the phase / state to place the first road.
*/
class PlaceFirstRoadState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		// TODO: Replace with AI / board logic.
		std::string chosenEdge;
		try {
			chosenEdge = getRandomEdgeKey();
		}
		catch (const std::exception& e) {
			result["error"] = std::string("The following error occurred when generating road edge key.") + e.what();
			return result;
		}

		int player = state.currentPlayer;
		state.placeRoad(player, chosenEdge);
		// For players 1 and 2, move to next settlement; for player 3, transition to city.
		if (player < 3) {
			state.currentPlayer = player + 1;
			state.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
		}
		else {
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		int roadId = db.addRoad(player, chosenEdge);

		result["message"] = "Player " + std::to_string(player) + " placed a road at " + chosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = player;
		roadJson["edge"] = chosenEdge;
		result["road"] = std::move(roadJson);
		return result;
	}
};


/* Class PlaceFirstCityState is a concrete class
* that represents a handler of the phase / state to place the first city.
*/
class PlaceFirstCityState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		// For demonstration, we use a random vertex label.
		// TODO: Replace with AI / board logic.
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<> dist(1, 54); // [1, 54]
		int vertexIndex = dist(rng);
		std::string chosenVertex = (vertexIndex < 10)
			? "V0" + std::to_string(vertexIndex)
			: "V" + std::to_string(vertexIndex);

		int player = state.currentPlayer;
		state.placeCity(player, chosenVertex);
		// Transition to the second road phase.
		state.phase = Phase::TO_PLACE_SECOND_ROAD;
		// Persist the city in the database.
		int cityId = db.addCity(player, chosenVertex);

		result["message"] = "Player " + std::to_string(player) + " placed a city at " + chosenVertex + ".";
		result["moveType"] = "city";
		crow::json::wvalue cityJson;
		cityJson["id"] = cityId;
		cityJson["player"] = player;
		cityJson["vertex"] = chosenVertex;
		result["city"] = std::move(cityJson);
		return result;
	}
};


/* Class PlaceSecondRoadState is a concrete class
* that represents a handler of the phase / state to place the second road.
*/
class PlaceSecondRoadState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;

		// TODO: Replace with AI / board logic.
		std::string chosenEdge;
		try {
			chosenEdge = getRandomEdgeKey();
		}
		catch (const std::exception& e) {
			result["error"] = std::string("The following error occurred when generating road edge key.") + e.what();
			return result;
		}

		int player = state.currentPlayer;
		state.placeRoad(player, chosenEdge);
		/* Transition logic:
		* If player number is greater than 1, decrement player number and reset phase for city.
		* If player number is 1, transition to turn.
		*/
		if (player > 1) {
			state.currentPlayer = player - 1;
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		else {
			state.phase = Phase::TURN;
		}
		int roadId = db.addRoad(player, chosenEdge);

		result["message"] = "Player " + std::to_string(player) + " placed a road at " + chosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = player;
		roadJson["edge"] = chosenEdge;
		result["road"] = std::move(roadJson);
		return result;
	}
};


// Class PlaceFirstCityState is a concrete class that represents a handler of a turn.
class TurnState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state, Database& db) override {
		crow::json::wvalue result;
		int player = state.currentPlayer;
		result["message"] = "Player " + std::to_string(player) + " is taking their turn.";
		result["moveType"] = "turn";
		return result;
	}
};


/* Class PhaseStateMachine represents a phase state machine
* that dispatches the current phase to the appropriate handler.
*/
class PhaseStateMachine {
private:
	std::unordered_map<std::string, std::shared_ptr<PhaseState>> stateHandlers;
public:
	PhaseStateMachine() {
		stateHandlers[Phase::TO_PLACE_FIRST_SETTLEMENT] = std::make_shared<PlaceFirstSettlementState>();
		stateHandlers[Phase::TO_PLACE_FIRST_ROAD] = std::make_shared<PlaceFirstRoadState>();
		stateHandlers[Phase::TO_PLACE_FIRST_CITY] = std::make_shared<PlaceFirstCityState>();
		stateHandlers[Phase::TO_PLACE_SECOND_ROAD] = std::make_shared<PlaceSecondRoadState>();
		stateHandlers[Phase::TURN] = std::make_shared<TurnState>();
	}

	crow::json::wvalue handle(GameState& state, Database& db) {
		auto handler = stateHandlers[state.phase];
		if (handler) {
			return handler->handle(state, db);
		}
		else {
			crow::json::wvalue result;
			result["error"] = "The phase " + state.phase + " is not recognized.";
			return result;
		}
	}
};