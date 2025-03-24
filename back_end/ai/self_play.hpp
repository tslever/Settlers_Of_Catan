#pragma once


#include "../db/Database.hpp"
#include "../game/game_state.hpp"
#include "neural_network.hpp"
#include "strategy.hpp"


// TODO: Consider recording additional data (e.g., full game trajectory, move probabilities).
struct TrainingExample {
    GameState gameState; // `gameState` represents snapshot of game state at time of move.
    std::string move; // move represents label of vertex at which settlement or city is placed. TODO: What does move represent when placing a road?
    double value; // value represents target value (e.g., game outcome from perspective of current player).
    double policy; // policy represents target probability (e.g., derived from MCTS visit counts).
};

std::vector<TrainingExample> runSelfPlayGame(SettlersNeuralNet& neuralNet, Database& db) {
    std::vector<TrainingExample> vectorOfTrainingExamples;
    // Initialize game state using default settings.
    GameState gameState;
    // For simplicity, we assume the current phase is phase to place first settlement.
    // TODO: Allow current phase to be phase to place first road, second city, or second road.
    // Consider using MCTS function to pick move.
    auto mctsResult = runMcts(gameState, db, neuralNet);
    if (mctsResult.first.empty()) {
        // If MCTS fails (which should be rare), return an empty vector.
        return vectorOfTrainingExamples;
    }
    // Apply the move to the game state.
    // TODO: Consider simulating several moves and then determine outcome.
    int currentPlayer = gameState.currentPlayer;
    gameState.placeSettlement(currentPlayer, mctsResult.first);
    // Create a training example based on current state and move.
    TrainingExample trainingExample;
    trainingExample.gameState = gameState;
    trainingExample.move = mctsResult.first;
    // For this dummy example, we use neutral dummy targets.
    trainingExample.value = 0.0; // e.g., 0.0 represents a tie outcome.
    trainingExample.policy = 0.5; // 0.5 is an arbitrary policy.
    // TODO: Consider determining value as in function `compute_game_outcome` in `self_play.py` in Python back end.
    // TODO: Consider collecting MCTS derived policy distributions.
    vectorOfTrainingExamples.push_back(trainingExample);
    return vectorOfTrainingExamples;
}