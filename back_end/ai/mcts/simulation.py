from back_end.board import Board
import copy
import random


def simulate_rollout(node, dictionary_of_labels_of_vertices_and_tuples_of_coordinates, neural_network, rollout_depth = 3):
    '''
    Perform a multi-step rollout simulation from the current leaf node.
    # TODO: What is a multi-step rollout simulation from the current leaf node?

    This function simulates a sequence of moves (up to a specific rollout depth) using a simple random rollout policy.
    # TODO: Consider whether a rollout policy that is not simple random is appropriate.

    At each step, for "settlement" and "city" moves, the function computes available building moves,
    randomly selects one, and updates the state by adding the chosen move for the current player.
    For "road" moves, the function finds available road moves adjacent to the last building,
    randomly selects one, and updates the state appropriately.
    
    After the rollout, the final state is evaluated using the neural network's settlement evaluation.
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

    # Simulate moves until the rollout depth is reached or no move is available.
    while current_depth < rollout_depth:
        move_type = state.get("phase")
        if move_type in ("settlement", "city"):
            # Flatten the settlements and cities dictionaries into single lists of occupied vertices.
            occupied = []
            for lst in state.get("settlements", {}).values():
                occupied.extend(lst)
            for lst in state.get("cities", {}).values():
                occupied.extend(lst)
            available_moves = board.get_available_building_moves(occupied)
            if not available_moves:
                break
            chosen_move = random.choice(available_moves)
            # TODO: Consider whether a rollout policy that is not simple random is appropriate.
            if move_type == "settlement":
                state.setdefault("settlements", {}).setdefault(state.get("current_player"), []).append(chosen_move)
            else:
                state.setdefault("cities", {}).setdefault(state.get("current_player"), []).append(chosen_move)
            state["last_building"] = chosen_move
        
        elif move_type == "road":
            last = state.get("last_building")
            if last is None:
                break
            used_edges = []
            current_roads = state.get("roads")
            if isinstance(current_roads, dict):
                for lst in current_roads.values():
                    used_edges.extend(lst)
            else:
                used_edges = current_roads if current_roads is not None else []
            available_edges = board.get_available_road_moves(last, used_edges)
            if not available_edges:
                break
            _, chosen_edge_key = random.choice(available_edges)
            if isinstance(state.get("roads"), dict):
                state.setdefault("roads", {}).setdefault(state.get("current_player"), []).append(chosen_edge_key)
            else:
                state.setdefault("roads", []).append(chosen_edge_key)
        else:
            break

        current_depth += 1
        # TODO: Consider whether updating state["phase"] here according to game rules is appropriate.
        # TODO: Consider whether simulating multiple moves per rollout is appropriate.
        break

    # Evaluate the resulting state.
    # TODO: Consider whether to do something other than using the evaluation for settlements as a proxy.
    if state.get("last_building") is not None:
        value, _ = neural_network.evaluate_settlement(state["last_building"])
    else:
        value = 0.0
    return value