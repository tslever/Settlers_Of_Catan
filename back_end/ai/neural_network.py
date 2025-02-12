from utilities.board import MARGIN_OF_ERROR
from utilities.board import TOKEN_MAPPING
from utilities.board import TOKEN_DOT_MAPPING
from utilities.board import get_hex_vertices
from utilities.board import hexes
import math
import numpy as np


class NeuralNetwork:
    '''
    A simulated neural network that mimics an AlphaGo Zero style model.
    For each candidate move (settlement or road) it returns:
    - a value estimate (roughly in the range -1 to 1)
    - a prior probability (a number in [0, 1])
    TODO: Create code to train a model.
    TODO: Load a trained model.
    '''

    def __init__(self):
        # TODO: Load a trained model.
        pass


    def predict_settlement(self, game_state, vertex, vertex_coords):
        '''
        Returns (value, prior) for a candidate settlement at the given vertex.
        '''
        value, prior = self.evaluate_settlement(vertex, vertex_coords, game_state)
        return value, prior
    

    def predict_road(self, game_state, edge, edge_key, vertex_coords, last_settlement):
        '''
        Returns (value, prior) for a candidate road move.
        '''
        value, prior = self.evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)
        return value, prior
    

    def evaluate_settlement(self, vertex, vertex_coords, game_state):
        '''
        Heuristic evaluation: sum the pip counts on all adjacent hexes.
        Then normalize (assuming a maximum total pip count of 11), pass through tanh,
        and add a little noise.
        The normalized pip sum is also used as the move's prior.
        TODO: Calculate the maximum total pip count given the board with its hexes and tokens.
        TODO: Consider non-heuristic evaluation.
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
                    break # Move on to the next hex once a match is found.
        # Normalize assuming a maximum of 11 pips (for example, when a settlement touches three top-value hexes)
        normalized = min(total_pips / 11.0, 1.0)
        # Use tanh to bring the value into (roughly) [-1, 1] and add a little Gaussian noise.
        value = np.tanh(normalized) + np.random.normal(0, 0.05)
        # Use the normalized score as the prior probability for this move.
        prior = normalized
        return value, prior
    

    def evaluate_road(self, edge, edge_key, vertex_coords, last_settlement, game_state):
        '''
        Evaluate a road move by "transferring" the settlement evaluation
        from the last settlement to the other vertex of the candidate edge.
        '''
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
            if math.isclose(coords[0], other[0], abs_tol = MARGIN_OF_ERROR) and math.isclose(coords[1], other[1], abs_tol = MARGIN_OF_ERROR):
                other_label = label
                break
        if other_label is None:
            return -1.0, 0.0 # If the other vertex is not found, return a very poor value and zero probability.
        # Reuse the settlement evaluation on the other vertex.
        return self.evaluate_settlement(other_label, vertex_coords, game_state)


neural_network = NeuralNetwork()