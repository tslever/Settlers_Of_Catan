import random


def predict_best_settlement(available_vertices, vertex_coords, game_state):
    best_score = -float('inf')
    best_vertex = None
    for vertex in available_vertices:
        score = random.random()
        if score > best_score:
            best_score = score
            best_vertex = vertex
    return best_vertex


def predict_best_road(available_edges, vertex_coords, game_state):
    best_score = -float('inf')
    best_edge = None
    best_edge_key = None
    for edge, edge_key in available_edges:
        score = random.random()
        if score > best_score:
            best_score = score
            best_edge = edge
            best_edge_key = edge_key
    return best_edge, best_edge_key