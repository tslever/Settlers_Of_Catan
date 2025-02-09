from utilities.board import MARGIN_OF_ERROR
from utilities.board import TOKEN_MAPPING
from utilities.board import TOKEN_DOT_MAPPING
from utilities.board import get_hex_vertices
from utilities.board import hexes
import math
import numpy as np


def evaluate_settlement(vertex, vertex_coords, game_state):
    """
    Evaluate the settlement move at the given vertex by summing the pip (dot) counts
    on all adjacent hexes. (No random noise is added so that the highest pip total wins.)
    """
    x, y = vertex_coords[vertex]
    total_pips = 0
    for hex_tile in hexes:
        # Get the vertices of this hex.
        vertices = get_hex_vertices(hex_tile)
        for vx, vy in vertices:
            # If this vertex is one of the hex's vertices, add its pip count.
            if math.isclose(vx, x, abs_tol=MARGIN_OF_ERROR) and math.isclose(vy, y, abs_tol=MARGIN_OF_ERROR):
                hex_id = hex_tile["id"]
                token = TOKEN_MAPPING.get(hex_id)
                if token is not None:
                    total_pips += TOKEN_DOT_MAPPING.get(token, 0)
                break
    return total_pips


def evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state):
    settlement_coord = vertex_coords[last_settlement]
    v1 = (edge["x1"], edge["y1"])
    v2 = (edge["x2"], edge["y2"])
    if (
        math.isclose(v1[0], settlement_coord[0], abs_tol = MARGIN_OF_ERROR) and
        math.isclose(v1[1], settlement_coord[1], abs_tol = MARGIN_OF_ERROR)
    ):
        other = v2
    else:
        other = v1
    other_label = None
    for label, coords in vertex_coords.items():
        if (
            math.isclose(coords[0], other[0], abs_tol = MARGIN_OF_ERROR) and
            math.isclose(coords[1], other[1], abs_tol = MARGIN_OF_ERROR)
        ):
            other_label = label
            break
    if other_label is None:
        return -float('inf')
    return evaluate_settlement(other_label, vertex_coords, game_state)


'''
TODO: Call a trained neural network that, given a game state and a move (settlement at vertex),
returns a tuple (value, probability), instead of using an evaluation function and a dummy probability of choosing this vertex.
'''
def neural_network_predict_settlement(game_state, vertex, vertex_coords):
    value = evaluate_settlement(vertex, vertex_coords, game_state)
    probability_of_choosing_this_vertex = 1.0
    return value, probability_of_choosing_this_vertex


'''
TODO: Call a trained neural network that, given a game state and a candidate road,
returns a tuple (value, probability), instead of using an evaluation function and a dummy probability of choosing this road.
'''
def neural_network_predict_road(game_state, edge, edge_key, vertex_coords, last_settlement):
    value = evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)
    probability_of_choosing_this_road = 1.0
    return value, probability_of_choosing_this_road


def alphago_zero_mcts_for_settlement(game_state, available_vertices, vertex_coords, num_simulations = 100, c_puct = 1.0):
    '''
    Run a simple one step MCTS on the available settlement moves.
    TODO: Build a full search tree.
    For each available vertex we initialize:
    - N: visit count
    - W: total value
    - Q: average value (W / N)
    - P: probability of choosing this vertex
    Dirichlet noise is added at the root for exploration.
    '''
    stats = {}
    for vertex in available_vertices:
        value, probability_of_choosing_this_vertex = neural_network_predict_settlement(game_state, vertex, vertex_coords)
        stats[vertex] = {"N": 0, "W": 0.0, "Q": 0.0, "P": probability_of_choosing_this_vertex}
    # Inject Dirichlet noise into the priors at the root
    epsilon = 0.25
    alpha = 0.3
    noise = np.random.dirichlet([alpha] * len(available_vertices))
    for i, vertex in enumerate(available_vertices):
        stats[vertex]["P"] = (1 - epsilon) * stats[vertex]["P"] + epsilon * noise[i]
    # Run MCTS simulations
    for _ in range(0, num_simulations):
        total_N = sum(stats[v]["N"] for v in available_vertices)
        best_score = -float('inf')
        best_vertex = None
        for vertex in available_vertices:
            node = stats[vertex]
            N = node["N"]
            Q = node["Q"]
            P = node["P"]
            ucb = Q + c_puct * P * math.sqrt(total_N) / (1 + N)
            if ucb > best_score:
                best_score = ucb
                best_vertex = vertex
        rollout_value, _ = neural_network_predict_settlement(game_state, best_vertex, vertex_coords)
        # Backpropagation
        # TODO: Deepen tree and update all appropriate moves.
        node = stats[best_vertex]
        node["N"] += 1
        node["W"] += rollout_value
        node["Q"] = node["W"] / node["N"]
    # Chose the move with the highest visit count.
    best_vertex = max(available_vertices, key = lambda v: stats[v]["N"])
    return best_vertex


def alphago_zero_mcts_for_road(game_state, available_edges, vertex_coords, last_settlement, num_simulations = 50, c_puct = 1.0):
    '''
    Run a simple one step MCTS for road moves.
    TODO: Build a full search tree.
    available_edges is a list of tuples (edge, edge_key).
    For each edge we initialize statistics and add Dirichlet noise at the root.
    Finally, we return the best edge and is key.
    '''
    stats = {}
    for edge, edge_key in available_edges:
        value, probability_of_choosing_this_road = neural_network_predict_road(game_state, edge, edge_key, vertex_coords, last_settlement)
        stats[edge_key] = {"edge": edge, "N": 0, "W": 0.0, "Q": 0.0, "P": probability_of_choosing_this_road}
    # Add Dirichlet noise for exploration at the root.
    epsilon = 0.25
    alpha = 0.3
    edge_keys = list(stats.keys())
    noise = np.random.dirichlet([alpha] * len(edge_keys))
    for i, ek in enumerate(edge_keys):
        stats[ek]["P"] = (1 - epsilon) * stats[ek]["P"] + epsilon * noise[i]
    # Run MCTS simulations.
    for _ in range(0, num_simulations):
        total_N = sum(node["N"] for node in stats.values())
        best_score = -float('inf')
        best_edge_key = None
        for ek, node in stats.items():
            N = node["N"]
            Q = node["Q"]
            P = node["P"]
            ucb = Q + c_puct * P * math.sqrt(total_N) / (1 + N)
            if ucb > best_score:
                best_score = ucb
                best_edge_key = ek
        rollout_value, _ = neural_network_predict_road(game_state, stats[best_edge_key]["edge"], best_edge_key, vertex_coords, last_settlement)
        node = stats[best_edge_key]
        node["N"] += 1
        node["W"] += rollout_value
        node["Q"] = node["W"] / node["N"]
    best_edge_key = max(stats.keys(), key = lambda ek: stats[ek]["N"])
    best_edge = stats[best_edge_key]["edge"]
    return best_edge, best_edge_key


def predict_best_settlement(available_vertices, vertex_coords, game_state):
    '''
    Given the available settlement moves, return the vertex chosen by the MCTS.
    TODO: Include more of the game state and pass the game state to an actual neural network.
    TODO: To choose the best vertex run a short MCTS guided by predictions of an actual neural network instead of a simulated neural network.
    '''
    best_vertex = alphago_zero_mcts_for_settlement(game_state, available_vertices, vertex_coords)
    return best_vertex


def predict_best_road(available_edges, vertex_coords, game_state):
    '''
    Given the available road moves, return the best edge.
    '''
    last_settlement = game_state.get("last_settlement")
    if last_settlement is None:
        return None, None
    best_edge, best_edge_key = alphago_zero_mcts_for_road(game_state, available_edges, vertex_coords, last_settlement)
    return best_edge, best_edge_key