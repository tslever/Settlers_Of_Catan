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
    std::clog << "[SELF PLAY GAME] A self play game is running." << std::endl;
    std::vector<TrainingExample> vectorOfTrainingExamples;
    // Initialize game state using default settings, including initial phase `Phase::TO_PLACE_FIRST_SETTLEMENT`.
    GameState gameState;
    // Simulate moves until phase becomes `Phase::TURN`, or up to a maximum number of steps to safeguard against infinite loops.
    int numberOfMovesSimulated = 0;
    const int maximumNumberOfSteps = 100;
    while (gameState.phase != Phase::DONE && numberOfMovesSimulated < maximumNumberOfSteps) {
        if (gameState.phase == Phase::TO_PLACE_FIRST_SETTLEMENT) {
            std::clog << "    [SELF PLAY PHASE] Player " << gameState.currentPlayer << " placing their first settlement is being simulated." << std::endl;
        }
        else if (gameState.phase == Phase::TO_PLACE_FIRST_ROAD) {
            std::clog << "    [SELF PLAY PHASE] Player " << gameState.currentPlayer << " placing their first road is being simulated." << std::endl;
        }
        else if (gameState.phase == Phase::TO_PLACE_FIRST_CITY) {
            std::clog << "    [SELF PLAY PHASE] Player " << gameState.currentPlayer << " placing their first city is being simulated." << std::endl;
        }
        else if (gameState.phase == Phase::TO_PLACE_SECOND_ROAD) {
            std::clog << "    [SELF PLAY PHASE] Player " << gameState.currentPlayer << " placing their second road is being simulated." << std::endl;
        }
        else if (gameState.phase == Phase::TURN) {
            std::clog << "    [SELF PLAY PHASE] Player " << gameState.currentPlayer << " playing their turn is being simulated." << std::endl;
        }
        std::pair<std::string, int> pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount = runMcts(
            gameState,
            db,
            neuralNet,
            numberOfSimulations,
            cPuct,
            tolerance
        );
        std::string labelOfVertexOrEdgeKey = pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount.first;
        if (labelOfVertexOrEdgeKey.empty()) {
            std::clog << "[SELF PLAY] MCTS did not return a valid move." << std::endl;
            break;
        }
        int currentPlayer = gameState.currentPlayer;
        std::string phase = gameState.phase;
        if (phase.find("settlement") != std::string::npos) {
            gameState.placeSettlement(currentPlayer, labelOfVertexOrEdgeKey);
        }
        else if (phase.find("city") != std::string::npos) {
            gameState.placeCity(currentPlayer, labelOfVertexOrEdgeKey);
        }
        else if (phase.find("road") != std::string::npos) {
            gameState.placeRoad(currentPlayer, labelOfVertexOrEdgeKey);
        }
        else if (phase == "turn") {
            if (isLabelOfVertex(labelOfVertexOrEdgeKey)) {
                gameState.placeSettlement(currentPlayer, labelOfVertexOrEdgeKey);
            }
            else if (isEdgeKey(labelOfVertexOrEdgeKey)) {
                gameState.placeRoad(currentPlayer, labelOfVertexOrEdgeKey);
            }
        }
        TrainingExample trainingExample;
        trainingExample.gameState = gameState; // snapshot after move
        trainingExample.move = labelOfVertexOrEdgeKey;
        trainingExample.value = 0.0; // temporary dummy value that will be updated once game outcome is known
        int visitCount = pairOfLabelOfBestVertexOrKeyOfBestEdgeAndVisitCount.second;
        double priorProbabilityOfVisit = static_cast<double>(visitCount) / static_cast<double>(numberOfSimulations);
        trainingExample.policy = priorProbabilityOfVisit;
        vectorOfTrainingExamples.push_back(trainingExample);
        numberOfMovesSimulated++;
    }
    double gameOutcome = computeGameOutcome(gameState);
    for (auto& trainingExample : vectorOfTrainingExamples) {
        trainingExample.value = gameOutcome;
    }
    std::clog <<
        "[SELF PLAY] Game simulation completed in " << numberOfMovesSimulated << " steps " <<
        "with game outcome " << gameOutcome << " for Player " << gameState.currentPlayer << "." << std::endl;
    return vectorOfTrainingExamples;
}