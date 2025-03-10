#pragma once

#include "crow.h"
#include "game_state.hpp"


// class PhaseStateMachine represents a simple C++ phase state machine that mimics the Python class `PhaseStateMachine`.
// TODO: Incorporate AI and board evaluation functions.

class PhaseStateMachine {
public:

	// Update the game state and return a JSON result.
	crow::json::wvalue handle(GameState& state) {
		crow::json::wvalue result;
		if (state.phase == "phase to place first settlement") {
			// For now we pick a hardcoded vertex.
			// TODO: Replace with AI / board logic.
			std::string chosen_vertex = "V01";
			state.placeSettlement(state.currentPlayer, chosen_vertex);
			state.phase = "phase to place first road";
			state.lastBuilding = chosen_vertex;
			result["message"] = "Settlement placed at " + chosen_vertex;
			result["moveType"] = "settlement";
		}
		else if (state.phase == "phase to place first road") {
			// For now we pick a hardcoded edge.
			// TODO: Replace with AI / board logic.
			std::string chosen_edge = "E01";
			state.placeRoad(state.currentPlayer, chosen_edge);
			state.phase = "phase to place first city";
			result["message"] = "Road placed at " + chosen_edge;
			result["moveType"] = "road";
		}
		else if (state.phase == "phase to place first city") {
			// For now we pick a hardcoded vertex.
			// TODO: Replace with AI / board logic.
			std::string chosen_vertex = "V02";
			state.placeCity(state.currentPlayer, chosen_vertex);
			state.phase = "phase to place second road";
			state.lastBuilding = chosen_vertex;
			result["message"] = "City placed at " + chosen_vertex;
			result["moveType"] = "city";
		}
		else if (state.phase == "phase to place second road") {
			// For now we pick a hardcoded edge.
			// TODO: Replace with AI / board logic.
			std::string chosen_edge = "E02";
			state.placeRoad(state.currentPlayer, chosen_edge);
			state.phase = "turn";
			// Update currentPlayer to the next player (for example, cycle through Players 1, 2, and 3).
			state.currentPlayer = (state.currentPlayer % 3) + 1;
			result["message"] = "Road placed at " + chosen_edge;
			result["moveType"] = "road";
		}
		else if (state.phase == "turn") {
			result["message"] = "Player " + std::to_string(state.currentPlayer) + " is taking their turn.";
			result["moveType"] = "turn";
		}
		else {
			result["error"] = state.phase + " is invalid.";
		}
		return result;
	}
};