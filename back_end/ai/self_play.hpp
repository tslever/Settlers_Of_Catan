#pragma once


#include "../db/Database.hpp"
#include "../game/game_state.hpp"
#include "neural_network.hpp"
#include "strategy.hpp"


// TODO: Consider recording additional data such as full game trajectory, move probabilities, and/or everything about the board and structures on the board.
struct TrainingExample {
    GameState gameState; // `gameState` represents snapshot of game state at time of move.
    std::string move; // move represents label of vertex at which settlement or city is placed or key of edge at which road is placed.
    double value; // value represents target value (e.g., game outcome from perspective of current player).
    double policy; // policy represents target prior probability derived from numbers of visits to nodes that occur during Monte Carlo Tree Search.
};

double computeGameOutcome(const GameState& gameState) {
    // TODO: Determine the winner as the player with the greatest sum of all numbers of all pips on all tokens on all hexes adjacent to all buildings of the player. 
    return (gameState.currentPlayer == 1) ? 1.0 : -1.0;
}

// TODO: Consider replacing function `updatePhase` by using the existing phase state machine.
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

/* Function `runSelfPlayGame` simulates a complete game trajectory
* by repeatedly using Monte Carlo Tree Search to select a move and by updating the game state
* until the game reaches the turn phase when setup is complete.
*/
std::vector<TrainingExample> runSelfPlayGame(
    WrapperOfNeuralNetwork& neuralNet,
    Database& db,
    int numberOfSimulations,
    double cPuct,
    double tolerance
) {
    std::vector<TrainingExample> vectorOfTrainingExamples;
    // Initialize game state using default settings, including initial phase `Phase::TO_PLACE_FIRST_SETTLEMENT`.
    GameState gameState;
    // Simulate moves until phase becomes `Phase::TURN`, or up to a maximum number of steps to safeguard against infinite loops.
    int numberOfStepsCompleted = 0;
    const int maximumNumberOfSteps = 100;
    while (gameState.phase != Phase::TURN && numberOfStepsCompleted < maximumNumberOfSteps) {
        // We're simulating and in simulation now.
        // Use MCTS to select the best move for the current phase.
        std::pair<std::string, int> pairOfLabelOfVertexOrEdgeKeyAndVisitCount = runMcts(
            gameState,
            db,
            neuralNet,
            numberOfSimulations,
            cPuct,
            tolerance
        );
        std::string labelOfVertexOrEdgeKey = pairOfLabelOfVertexOrEdgeKeyAndVisitCount.first;
        if (labelOfVertexOrEdgeKey.empty()) {
            std::clog << "[SELF PLAY] MCTS did not return a valid move." << std::endl;
            break;
        }
        int currentPlayer = gameState.currentPlayer;
        // Update game state based on phase.
        if (gameState.phase.find("settlement") != std::string::npos) {
            gameState.placeSettlement(currentPlayer, labelOfVertexOrEdgeKey);
        }
        else if (gameState.phase.find("city") != std::string::npos) {
            gameState.placeCity(currentPlayer, labelOfVertexOrEdgeKey);
        }
        else if (gameState.phase.find("road") != std::string::npos) {
            gameState.placeRoad(currentPlayer, labelOfVertexOrEdgeKey);
        }
        TrainingExample trainingExample;
        trainingExample.gameState = gameState; // snapshot after move
        trainingExample.move = labelOfVertexOrEdgeKey;
        trainingExample.value = 0.0; // temporary dummy value that will be updated once game outcome is known
        int visitCount = pairOfLabelOfVertexOrEdgeKeyAndVisitCount.second;
        double priorProbabilityOfVisit = static_cast<double>(visitCount) / static_cast<double>(numberOfSimulations);
        trainingExample.policy = priorProbabilityOfVisit;
        vectorOfTrainingExamples.push_back(trainingExample);
        updatePhase(gameState);
        numberOfStepsCompleted++;
    }
    double gameOutcome = computeGameOutcome(gameState);
    for (auto& trainingExample : vectorOfTrainingExamples) {
        trainingExample.value = gameOutcome;
    }
    std::clog <<
        "[SELF PLAY] Game simulation completed in " << numberOfStepsCompleted << " steps " <<
        "with game outcome " << gameOutcome << " for Player " << gameState.currentPlayer << "." << std::endl;
    return vectorOfTrainingExamples;
}