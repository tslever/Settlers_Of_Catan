#pragma once

#include <string>


namespace Game {

	enum class Phase {
		FirstSettlement,
		FirstRoad,
		FirstCity,
		SecondRoad,
		RollDice,
		Turn,
		Done
	};

	std::string toString(Phase phase) {
		switch (phase) {
		case Phase::FirstSettlement: return "phase to place first settlement";
		case Phase::FirstRoad: return "phase to place first road";
		case Phase::FirstCity: return "phase to place first city";
		case Phase::SecondRoad: return "phase to place second road";
		case Phase::RollDice: return "phase to roll dice";
		case Phase::Turn: return "turn";
		case Phase::Done: return "done";
		}
		throw std::runtime_error("Invalid Phase");
	}

	Phase fromString(const std::string& string) {
		if (string == "phase to place first settlement") { return Phase::FirstSettlement; }
		if (string == "phase to place first road") { return Phase::FirstRoad; }
		if (string == "phase to place first city") { return Phase::FirstCity; }
		if (string == "phase to place second road") { return Phase::SecondRoad; }
		if (string == "phase to roll dice") { return Phase::RollDice; }
		if (string == "turn") { return Phase::Turn; }
		if (string == "done") { return Phase::Done; }
		throw std::runtime_error("Unknown Phase String: " + string);
	}
}