from utilities.board import MARGIN_OF_ERROR
from utilities.board import TOKEN_MAPPING
from utilities.board import TOKEN_DOT_MAPPING
from utilities.board import get_hex_vertices
from utilities.board import hexes
import math
import numpy as np


# Neural network simulation
# TODO: Replace with a real model
class NeuralNetwork:
    '''
    A placeholder neural network that simulates that predictions of an AlphaGo Zero-style model.
    TODO: Load a trained model and perform inference.
    '''

    def __init__(self):
        # TODO: Load trained model.
        pass


    def predict_settlement(self, game_state, vertex, vertex_coords):
        '''
        Given the current game state and a candidate settlement vertex, return tuple (predicted value, move probability).
        Here we simulate prediction using a simple heuristic plus noise.
        TODO: Fully implement prediction.
        '''
        value = self.evaluate_settlement(vertex, vertex_coords, game_state)
        policy_probability = 1.0
        return value, policy_probability
    

    def predict_road(self, game_state, edge, edge_key, vertex_coords, last_settlement):
        '''
        Given the current game state and a candidate road (edge), return tuple (predicted value, move probability).
        '''
        value = self.evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)
        policy_probability = 1.0
        return value, policy_probability
    

    def evaluate_settlement(self, vertex, vertex_coords, game_state):
        '''
        A simple heuristic: sum the pip (dot) counts on all adjacent hexes.
        Noise is added to simulate uncertainty.
        TODO: Fully implement evaluating a settlement.
        '''
        x, y = vertex_coords[vertex]
        total_pips = 0
        for hex in hexes:
            vertices = get_hex_vertices(hex)
            for vx, vy in vertices:
                if math.isclose(vx, x, abs_tol = MARGIN_OF_ERROR) and math.isclose(vy, y, abs_tol = MARGIN_OF_ERROR):
                    hex_id = hex["id"]
                    token = TOKEN_MAPPING.get(hex_id)
                    if token is not None:
                        total_pips += TOKEN_DOT_MAPPING.get(token, 0)
                    break
        return total_pips + np.random.normal(0, 0.1)
    

    def evaluate_road(self, edge, edge_key, vertex_coords, last_settlement, game_state):
        '''
        Evaluate a road move by "transferring" the settlement evaluation
        from the last settlement to the other vertex of the candidate edge.
        '''
        settlement_coord = vertex_coords[last_settlement]
        v1 = (edge["x1"], edge["y1"])
        v2 = (edge["x2"], edge["y2"])
        if math.isclose(v1[0], settlement_coord[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(v1[1], settlement_coord[1], abs_tol = MARGIN_OF_ERROR):
            other = v2
        else:
            other = v1
        other_label = None
        for label, coords in vertex_coords.items():
            if math.isclose(coords[0], other[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(coords[1], other[1], abs_tol = MARGIN_OF_ERROR):
                other_label = label
                break
        if other_label is None:
            return -float("inf")
        return self.evaluate_settlement(other_label, vertex_coords, game_state)


neural_network = NeuralNetwork()