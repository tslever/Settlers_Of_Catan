from utilities.board import MARGIN_OF_ERROR
from utilities.board import TOKEN_MAPPING
from utilities.board import TOKEN_DOT_MAPPING
from utilities.board import get_hex_vertices
from utilities.board import hexes
import math
import random


def evaluate_settlement(vertex, vertex_coords, game_state):
    x, y = vertex_coords[vertex]
    value = 0
    for hex in hexes:
        vertices = get_hex_vertices(hex)
        for vx, vy in vertices:
            if math.isclose(vx, x, abs_tol = MARGIN_OF_ERROR) and math.isclose(vy, y, abs_tol = MARGIN_OF_ERROR):
                hex_id = hex["id"]
                token = TOKEN_MAPPING.get(hex_id)
                if token is not None:
                    dot_count = TOKEN_DOT_MAPPING.get(token, 0)
                    value += dot_count
                break
    value += random.uniform(-0.5, 0.5)
    return value


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
TODO: Ensure as in AlphaGo Zero that
a neural network guides Monte Carlo rollout Searches and returns a value and a policy
instead of the following functions implementing a simple Monte Carlo rollout Search that
simulates many random rollout by using evaluation functions and averaging the results.
'''
def simulate_settlement_rollout(vertex, vertex_coords, game_state):
    '''
    Simulate a rollout starting from placing a settlement at vertex.
    TODO: Ensure that a complete game from this move is simulated instead of simply calling function `evaluate_settlement`.
    '''
    return evaluate_settlement(vertex, vertex_coords, game_state)


def monte_carlo_tree_search_for_settlement(game_state, available_vertices, vertex_coords, num_simulations = 50):
    rollout_results = {}
    for vertex in available_vertices:
        total_value = 0
        for _ in range(num_simulations):
            total_value += simulate_settlement_rollout(vertex, vertex_coords, game_state)
        avg_value = total_value / num_simulations
        rollout_results[vertex] = avg_value
    best_vertex = max(rollout_results, key = rollout_results.get)
    return best_vertex


def simulate_road_rollout(edge, edge_key, vertex_coords, last_settlement, game_state):
    '''
    Simulate a rollout starting from placing a road on the given edge.
    TODO: Ensure that a complete game from this move is simulated instead of simply calling function `evaluate_road`.
    '''
    return evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)


def monte_carlo_tree_search_for_road(game_state, available_edges, vertex_coords, num_simulations = 50):
    last_settlement = game_state.get("last_settlement")
    if last_settlement is None:
        return None, None
    rollout_results = {}
    for edge, edge_key in available_edges:
        total_value = 0
        for _ in range(num_simulations):
            total_value += simulate_road_rollout(edge, edge_key, vertex_coords, last_settlement, game_state)
        avg_value = total_value / num_simulations
        rollout_results[edge_key] = (edge, avg_value)
    best_edge_key = max(rollout_results, key = lambda k: rollout_results[k][1])
    best_edge, _ = rollout_results[best_edge_key]
    return best_edge, best_edge_key


def predict_best_settlement(available_vertices, vertex_coords, game_state):
    '''
    TODO: Implement the search process used in AlphaGo Zero instead of
    running multiple rollout simulations for each available vertex and choosing the one with the best average outcome.
    '''
    best_vertex = monte_carlo_tree_search_for_settlement(game_state, available_vertices, vertex_coords)
    return best_vertex


def predict_best_road(available_edges, vertex_coords, game_state):
    '''
    TODO: Implement the search process used in AlphaGo Zero instead of
    running multiple rollout simulations for each available edge and choosing the one with the best average outcome.
    '''
    best_edge, best_edge_key = monte_carlo_tree_search_for_road(game_state, available_edges, vertex_coords)
    return best_edge, best_edge_key