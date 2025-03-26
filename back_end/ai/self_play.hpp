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


// Function `computeGameOutcome` computes game outcome based on current game state.
// TODO: Replace dummy implementation with evaluation based on `compute_game_outcome` in `self_play.py` in Python back end.
double computeGameOutcome(const GameState& gameState) {
    // For now, we simply return a dummy value based on current player (e.g., win for Player 1, loss for other players).
    // TODO: Use board geometry to sum pip counts on adjacent hexes or simulate the rest of the game.
    return (gameState.currentPlayer == 1) ? 1.0 : -1.0;
}


// Function `updatePhase` updates the game phase.
// TODO: Consider whether function `updatePhase` can be replaced by using the existing phase state machine.
void updatePhase(GameState& gameState) {
    if (gameState.phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
        gameState.phase = Phase::TO_PLACE_FIRST_ROAD;
    }
    else if (gameState.phase == Phase::TO_PLACE_FIRST_ROAD) {
        if (gameState.currentPlayer < 3) {
            gameState.currentPlayer++;
            gameState.phase = Phase::TO_PLACE_FIRST_SETTLEMENT;
        }
        else {
            gameState.phase = Phase::TO_PLACE_FIRST_CITY;
        }
    }
    else if (gameState.phase == Phase::TO_PLACE_FIRST_CITY) {
        gameState.phase = Phase::TO_PLACE_SECOND_ROAD;
    }
    else if (gameState.phase == Phase::TO_PLACE_SECOND_ROAD) {
        if (gameState.currentPlayer > 1) {
            gameState.currentPlayer--;
            gameState.phase = Phase::TO_PLACE_FIRST_CITY;
        }
        else {
            gameState.phase = Phase::TURN;
        }
    }
}


/* Function `runSelfPlayGame` simulates a complete game trajectory.
* Function `runSelfPlayGame` repeatedly uses Monte Carlo Tree Search via function `runMcts` to select a move and update the game state
* until the game reaches the turn phase when setup is complete.
*/
std::vector<TrainingExample> runSelfPlayGame(
    SettlersNeuralNet& neuralNet,
    Database& db,
    int numberOfSimulations,
    double cPuct,
    double tolerance
) {
    std::vector<TrainingExample> vectorOfTrainingExamples;
    // Initialize game state using default settings. Initial phase is `Phase::TO_PLACE_FIRST_SETTLEMENT`.
    GameState gameState;
    // Simulate moves up to a maximum number of steps or until the phase becomes `Phase::TURN`.
    int steps = 0;
    const int maxSteps = 100; // Safeguard against infinite loops.
    while (gameState.phase != Phase::TURN && steps < maxSteps) {
        // Use MCTS to select the best move for the current phase.
        auto mctsResult = runMcts(gameState, db, neuralNet, numberOfSimulations, cPuct, tolerance);
        if (mctsResult.first.empty()) {
            // If MCTS fails, break out of simulation.
            std::clog << "[SELF PLAY] MCTS did not return a valid move." << std::endl;
            break;
        }
        int currentPlayer = gameState.currentPlayer;
        // Update game state based on phase.
        if (gameState.phase.find("settlement") != std::string::npos) {
            gameState.placeSettlement(currentPlayer, mctsResult.first);
        }
        else if (gameState.phase.find("city") != std::string::npos) {
            gameState.placeCity(currentPlayer, mctsResult.first);
        }
        else if (gameState.phase.find("road") != std::string::npos) {
            gameState.placeRoad(currentPlayer, mctsResult.first);
        }
        TrainingExample trainingExample;
        trainingExample.gameState = gameState; // snapshot after move
        trainingExample.move = mctsResult.first;
        trainingExample.value = 0.0; // temporary dummy value that will be updated once game outcome is known
        trainingExample.policy = mctsResult.second; // raw visit count or normalized probability
        vectorOfTrainingExamples.push_back(trainingExample);
        updatePhase(gameState);
        steps++;
    }
    double gameOutcome = computeGameOutcome(gameState);
    // Update every training example with the same final game outcome.
    for (auto& trainingExample : vectorOfTrainingExamples) {
        trainingExample.value = gameOutcome;
    }
    std::clog << "[SELF PLAY] Completed game simulation in " << steps << " steps with game outcome " << gameOutcome << std::endl;
    return vectorOfTrainingExamples;
}