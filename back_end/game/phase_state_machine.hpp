#pragma once

#include "board.hpp"
#include "crow.h"
#include "game_state.hpp"
#include "../ai/strategy.hpp"
#include <random>


/* TODO: Ensure that the various phase state classes for placing settlements, roads, and cities use Monte Carlo Tree Search
* so that all move decisions are driven by AI logic.
*/


// Class PhaseState is a base abstract class that represents a phase handler / state.
class PhaseState {
public:
	virtual ~PhaseState() = default;
	virtual crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) = 0;
};


/* Class PlaceFirstSettlementState is a concrete class
* that represents a handler of the phase / state to place the first settlement.
*/
class PlaceFirstSettlementState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int currentPlayer = state.currentPlayer;
		std::string chosenVertex = mctsResult.first;
		state.placeSettlement(currentPlayer, chosenVertex);

		state.phase = Phase::TO_PLACE_FIRST_ROAD;
		int settlementId = db.addSettlement(currentPlayer, chosenVertex);

		result["message"] = "Player " + std::to_string(currentPlayer) + " placed a settlement at " + chosenVertex + ".";
		result["moveType"] = "settlement";
		crow::json::wvalue settlementJson;
		settlementJson["id"] = settlementId;
		settlementJson["player"] = currentPlayer;
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
	crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int currentPlayer = state.currentPlayer;
		std::string keyOfChosenEdge = mctsResult.first;
		state.placeRoad(currentPlayer, keyOfChosenEdge);
		state.lastBuilding = "";

		// For players 1 and 2, move to next settlement; for player 3, transition to city.
		if (currentPlayer < 3) {
			state.currentPlayer = currentPlayer + 1;
			state.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
		}
		else {
			state.phase = Phase::TO_PLACE_FIRST_CITY;
		}
		int roadId = db.addRoad(currentPlayer, keyOfChosenEdge);

		result["message"] = "Player " + std::to_string(currentPlayer) + " placed a road at " + keyOfChosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = currentPlayer;
		roadJson["edge"] = keyOfChosenEdge;
		result["road"] = std::move(roadJson);

		return result;
	}
};


/* Class PlaceFirstCityState is a concrete class
* that represents a handler of the phase / state to place the first city.
*/
class PlaceFirstCityState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		// Get the up-to-date list of occupied vertices from the database.
		std::vector<std::string> listOfLabelsOfOccupiedVertices = getOccupiedVertices(db);

		// Determine which vertices are available (not occupied and not adjacent to any occupied vertex).
		auto vectorOfLabelsOfAvailableVertices = getAvailableVertices(listOfLabelsOfOccupiedVertices);
		if (vectorOfLabelsOfAvailableVertices.empty()) {
			throw std::runtime_error("No vertices are available for placing settlement.");
		}

		// Randomly select one available vertex.
		// TODO: Replace with AI / board logic.
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<> dist(0, vectorOfLabelsOfAvailableVertices.size() - 1);
		std::string chosenVertex = vectorOfLabelsOfAvailableVertices.at(dist(rng));

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
	crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
		crow::json::wvalue result;

		auto mctsResult = runMcts(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		if (mctsResult.first.empty()) {
			throw std::runtime_error("MCTS failed to determine a move.");
		}

		int player = state.currentPlayer;
		std::string keyOfChosenEdge = mctsResult.first;
		state.placeRoad(player, keyOfChosenEdge);
		state.lastBuilding = "";
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
		int roadId = db.addRoad(player, keyOfChosenEdge);

		result["message"] = "Player " + std::to_string(player) + " placed a road at " + keyOfChosenEdge + ".";
		result["moveType"] = "road";
		crow::json::wvalue roadJson;
		roadJson["id"] = roadId;
		roadJson["player"] = player;
		roadJson["edge"] = keyOfChosenEdge;
		result["road"] = std::move(roadJson);
		return result;
	}
};


// Class PlaceFirstCityState is a concrete class that represents a handler of a turn.
class TurnState : public PhaseState {
public:
	crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) override {
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

	crow::json::wvalue handle(
		GameState& state,
		Database& db,
		WrapperOfNeuralNetwork& neuralNet,
		int numberOfSimulations,
		double cPuct,
		double tolerance
	) {
		auto handler = stateHandlers[state.phase];
		if (handler) {
			return handler->handle(state, db, neuralNet, numberOfSimulations, cPuct, tolerance);
		}
		else {
			crow::json::wvalue result;
			result["error"] = "The phase " + state.phase + " is not recognized.";
			return result;
		}
	}
};