from back_end.board import Board
from back_end.ai.neural_network import SettlersNeuralNet
import copy
import random


def simulate_rollout(node, dictionary_of_labels_of_vertices_and_tuples_of_coordinates, neural_network: SettlersNeuralNet, rollout_depth = 3):
    '''
    Perform a multi-step rollout simulation from the given node.
    # TODO: What is a multi-step rollout simulation from the current leaf node?

    At each step, for the current phase (settlement / city / road),
    the function evaluates all available moves using the neural network
    and chooses the move with the highest predicted value.
    TODO: Consider sampling from the network's policy distribution.
    Finally, the resulting state is evaluated using the learned evaluation of the last building.
    For simplicity, the same evaluation is used regardless of move type.
    TODO: Consider implementing evaluations for each move type.

    Parameters:
        node: An MCTS node whose game state is dictionary representing the game state
        dictionary_of_labels_of_vertices_and_tuples_of_coordinates: A dictionary mapping labels of vertices to tuples of coordinates
        neural_network: A neural network used for state evaluation
        rollout_depth: The maximum number of moves to simulate in the rollout
    
    Returns:
        A scalar value representing the estimated value of the final state
    '''
    state = copy.deepcopy(node.game_state)
    board = Board()
    current_depth = 0
    phase = state.get("phase")

    # Simulate moves until the rollout depth is reached or no move is available.
    while current_depth < rollout_depth:
        if phase in ("settlement", "city"):
            # Flatten the settlements and cities dictionaries into single lists of occupied vertices.
            occupied = []
            for lst in state.get("settlements", {}).values():
                occupied.extend(lst)
            for lst in state.get("cities", {}).values():
                occupied.extend(lst)
            available_moves = board.get_available_building_moves(occupied)
            if not available_moves:
                break

            best_move = None
            best_value = -float("inf")
            for label_of_vertex in available_moves:
                if phase == "settlement":
                    value, _ = neural_network.evaluate_settlement(label_of_vertex)
                else:
                    value, _ = neural_network.evaluate_city(label_of_vertex)
                if value > best_value:
                    best_value = value
                    best_move = label_of_vertex
                
            if best_move is None:
                break

            if phase == "settlement":
                state.setdefault("settlements", {}).setdefault(state.get("current_player"), []).append(best_move)
            else:
                state.setdefault("cities", {}).setdefault(state.get("current_player"), []).append(best_move)
            state["last_building"] = best_move
        
        elif phase == "road":
            last_building = state.get("last_building")
            if last_building is None:
                break
            used_edges = []
            current_roads = state.get("roads")
            if isinstance(current_roads, dict):
                for lst in current_roads.values():
                    used_edges.extend(lst)
            else:
                used_edges = current_roads if current_roads is not None else []
            available_edges = board.get_available_road_moves(last_building, used_edges)
            if not available_edges:
                break

            best_edge = None
            best_value = -float("inf")
            for edge, edge_key in available_edges:
                value, _ = neural_network.evaluate_road(edge, dictionary_of_labels_of_vertices_and_tuples_of_coordinates, last_building)
                if value > best_value:
                    best_value = value
                    best_edge = (edge, edge_key)
            
            if best_edge is None:
                break

            _, chosen_edge_key = best_edge
            if isinstance(state.get("roads"), dict):
                state.setdefault("roads", {}).setdefault(state.get("current_player"), []).append(chosen_edge_key)
            else:
                state.setdefault("roads", []).append(chosen_edge_key)
        else:
            break

        current_depth += 1
        # TODO: Consider updating state["phase"] here if the game rules change the phase after a move.
        # For simplicity, we break after one iteration in this example.
        # TODO: Consider whether simulating multiple moves per rollout is appropriate.
        break

    # Evaluate the final state using the learned evaluation.
    # TODO: Consider whether to do something other than using the `evaluate_settlement` function with the last settlement or city,
    # such as using more comprehensive state evaluation.
    last_building = state.get("last_building")
    if last_building:
        value, _ = neural_network.evaluate_settlement(last_building)
    else:
        value = 0.0
    return value