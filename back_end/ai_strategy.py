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


def predict_best_settlement(available_vertices, vertex_coords, game_state):
    best_score = -float('inf')
    best_vertex = None
    for vertex in available_vertices:
        score = evaluate_settlement(vertex, vertex_coords, game_state)
        if score > best_score:
            best_score = score
            best_vertex = vertex
    return best_vertex


def predict_best_road(available_edges, vertex_coords, game_state):
    best_score = -float('inf')
    best_edge = None
    best_edge_key = None
    last_settlement = game_state.get("last_settlement")
    if last_settlement is None:
        return None, None
    for edge, edge_key in available_edges:
        score = evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)
        if score > best_score:
            best_score = score
            best_edge = edge
            best_edge_key = edge_key
    return best_edge, best_edge_key