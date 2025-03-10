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
        : currentPlayer(1), phase("TO_PLACE_FIRST_SETTLEMENT"), lastBuilding("")
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

    // TODO: Define additional member functions such as placeSettlement, getSnapshot(), etc.
};