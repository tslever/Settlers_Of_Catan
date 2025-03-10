#pragma once

#include <string>
#include <unordered_map>
#include <vector>


class GameState {
public:
    int currentPlayer;
    std::string phase;
    std::unordered_map<int, std::vector<std::string>> settlements;
    std::unordered_map<int, std::vector<std::string>> cities;
    std::unordered_map<int, std::vector<std::string>> roads;
    std::string lastBuilding;

    GameState()
        : currentPlayer(1), phase("phase to place first settlement"), lastBuilding("")
    {
        settlements[1] = {};
        settlements[2] = {};
        settlements[3] = {};
        cities[1] = {};
        cities[2] = {};
        cities[3] = {};
        roads[1] = {};
        roads[2] = {};
        roads[3] = {};
    }

    // Add a settlement for the given player at the specified vertex.
	void placeSettlement(int player, const std::string& vertex) {
		settlements[player].push_back(vertex);
		lastBuilding = vertex;
	}

    // Add a city for the given player at the specified vertex.
    void placeCity(int player, const std::string& vertex) {
        cities[player].push_back(vertex);
        lastBuilding = vertex;
    }

    // Add a city for the given player at the specified vertex.
    void placeRoad(int player, const std::string& edge) {
        roads[player].push_back(edge);
    }

    // Return a snapshot of the game state as a string.
    std::string getStateSnapshot() const {
        std::ostringstream oss;
        oss << "currentPlayer: " << currentPlayer << "\n";
        oss << "phase: " << phase << "\n";
        oss << "lastBuilding: " << lastBuilding << "\n";

        oss << "settlements:\n";
        for (const auto& pair : settlements) {
            oss << "  Player " << pair.first << ": ";
            for (const auto& v : pair.second) {
                oss << v << " ";
            }
            oss << "\n";
        }

        oss << "cities:\n";
        for (const auto& pair : cities) {
            oss << "  Player " << pair.first << ": ";
            for (const auto& v : pair.second) {
                oss << v << " ";
            }
            oss << "\n";
        }

        oss << "roads:\n";
        for (const auto& pair : roads) {
            oss << "  Player " << pair.first << ": ";
            for (const auto& e : pair.second) {
                oss << e << " ";
            }
            oss << "\n";
        }
        return oss.str();
    }
};