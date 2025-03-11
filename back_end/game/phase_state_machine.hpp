#pragma once

#include "crow.h"
#include "game_state.hpp"


// Class PhaseState is a base abstract class that represents a phase handler / state.
class PhaseState {
public:
	virtual ~PhaseState() = default;
	virtual crow::json::wvalue handle(GameState& state) = 0;
};


// Class PlaceFirstSettlementState is a concrete class that represents the state for placing the first settlement.
class PlaceFirstSettlementState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded vertex.
		// TODO: Replace with AI / board logic.
		std::string chosen_vertex = "V01";
		state.placeSettlement(state.currentPlayer, chosen_vertex);
		state.phase = "phase to place first road";
		state.lastBuilding = chosen_vertex;
		result["message"] = "Player " + std::to_string(state.currentPlayer) + " placed a settlement at " + chosen_vertex + ".";
		/* TODO: After loading the board for the first time, pressing the next button, and
		   expecting a settlement to be placed for Player 1, I don't see the settlement placed.
		*/
		result["moveType"] = "settlement";
		return result;
	}
};


// Class PlaceFirstRoadState is a concrete class that represents the state for placing the first road.
class PlaceFirstRoadState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded edge.
		// TODO: Replace with AI / board logic.
		std::string chosen_edge = "E01";
		state.placeRoad(state.currentPlayer, chosen_edge);
		state.phase = "phase to place first city";
		result["message"] = "Player " + std::to_string(state.currentPlayer) + " placed a road at " + chosen_edge + ".";
		/* TODO: After expecting a settlement to be placed for Player 1, pressing the next button, and
		   expecting a road to be placed for Player 1, I don't see a road placed.
		   After Player 1 places their first road, Player 2 places their first settlement.
		*/
		result["moveType"] = "road";
		return result;
	}
};


// Class PlaceFirstCityState is a concrete class that represents the state for placing the first city.
class PlaceFirstCityState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded vertex.
		// TODO: Replace with AI / board logic.
		std::string chosen_vertex = "V02";
		state.placeCity(state.currentPlayer, chosen_vertex);
		state.phase = "phase to place second road";
		state.lastBuilding = chosen_vertex;
		result["message"] = "Player " + std::to_string(state.currentPlayer) + " placed a city at " + chosen_vertex + ".";
		/* TODO: A city should not be placed immediately after Player 1 places their first road.
		* Even if we expect a city to be placed for Player 3, I don't see a city placed. 
		*/
		result["moveType"] = "city";
		return result;
	}
};


// Class PlaceSecondRoadState is a concrete class that represents the state for placing the second road.
class PlaceSecondRoadState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		// For now we pick a hardcoded edge.
		// TODO: Replace with AI / board logic.
		std::string chosen_edge = "E02";
		state.placeRoad(state.currentPlayer, chosen_edge);
		state.phase = "turn";
		/* TODO: After expecting a city to be placed for Player 3, pressing the next button, and
		   expecting a road to be placed for Player 3, I don't see a road placed.
		*/
		result["message"] = "Player " + std::to_string(state.currentPlayer) + " placed a road placed at " + chosen_edge + ".";
		// Cycle to the next player (assuming 3 players).
		/* TODO: Change player according to the following rules.
			Player 1 places their first settlement, then their first road.
			Player 2 places their first settlement, then their first road.
			Player 3 places their first settlement, then their first road.
			Player 3 places their first city, then their second road.
			Player 2 places their first city, then their second road.
			Player 1 places their first city, then their second road.
			Player 1 takes their turn.
		*/
		state.currentPlayer = (state.currentPlayer % 3) + 1;
		result["moveType"] = "road";
		return result;
	}
};


// Class TurnState is a concrete class that represents the state for taking a turn.
class TurnState : public PhaseState {
public:
	crow::json::wvalue handle(GameState& state) override {
		crow::json::wvalue result;
		result["message"] = "Player " + std::to_string(state.currentPlayer) + " is taking their turn.";
		result["moveType"] = "turn";
		return result;
	}
};


class PhaseStateMachine {
private:
	std::unordered_map<std::string, std::shared_ptr<PhaseState>> stateHandlers;
public:
	PhaseStateMachine() {
		// Initialize handlers corresponding to each phase.
		stateHandlers["phase to place first settlement"] = std::make_shared<PlaceFirstSettlementState>();
		stateHandlers["phase to place first road"] = std::make_shared<PlaceFirstRoadState>();
		stateHandlers["phase to place first city"] = std::make_shared<PlaceFirstCityState>();
		stateHandlers["phase to place second road"] = std::make_shared<PlaceSecondRoadState>();
		stateHandlers["turn"] = std::make_shared<TurnState>();
	}

	// Handle the current phase. If the phase is unrecognized, return an error.
	crow::json::wvalue handle(GameState& state) {
		auto handler = stateHandlers[state.phase];
		if (handler) {
			return handler->handle(state);
		}
		else {
			crow::json::wvalue result;
			result["error"] = "The phase " + state.phase + " is not recognized.";
			return result;
		}
	}
};