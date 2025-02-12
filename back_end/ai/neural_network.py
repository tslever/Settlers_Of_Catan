from ..utilities.board import MARGIN_OF_ERROR
from ..utilities.board import TOKEN_MAPPING
from ..utilities.board import TOKEN_DOT_MAPPING
from ..utilities.board import WIDTH_OF_BOARD_IN_VMIN
from ..utilities.board import get_hex_vertices
from ..utilities.board import hexes
import math
import numpy as np
# To install PyTorch on a computer with only a CPU, run `pip3 install torch torchvision torchaudio` per https://pytorch.org/get-started/locally/ .
import torch
import torch.nn as nn
import torch.nn.functional as F


'''
Module neural_network uses a trained PyTorch model that returns, for each candidate move,
a value estimate in [-1, 1] and a move prior probability in [0, 1].
'''


# TODO: Implement a self play system to generate training data / pairs of game state and move evaluations when determining best settlements and roads.
# TODO: Train via self play a neural network with an architecture matching the below input feature definition and architecture.
# TODO: Save a trained model to `back_end/ai/model.pt`.
# TODO: Improve the input representation by including more detailed information aobut the game state.
# TODO: Use a deep residual network with a board state "tower" and a dual head for value and policy.
class SettlersPolicyValueNet(nn.Module):
    
    def __init__(self, input_dim, hidden_dim = 128):
        '''
        input_dim: Number of features in the input vector
        hidden_dim: Number of hidden units
        '''
        super(SettlersPolicyValueNet, self).__init__()
        self.fc1 = nn.Linear(input_dim, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, hidden_dim)
        '''
        Policy head: output one number per candidate move.
        Here we output a single value, which we then squash to (0, 1) as the prior.
        '''
        self.fc_policy = nn.Linear(hidden_dim, 1)
        # Value head: output a scalar value in [-1, 1].
        self.fc_value = nn.Linear(hidden_dim, 1)

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        # The policy head uses a sigmoid so that its output is in (0, 1).
        policy = torch.sigmoid(self.fc_policy(x))
        # The value head uses tanh to output values in [-1, 1].
        value = torch.tanh(self.fc_value(x))
        return value, policy


class NeuralNetwork:
    '''
    A neural network that mimics an AlphaGo Zero style model using a trained PyTorch model.
    For each candidate move (settlement or road) it returns:
    - a value estimate in [-1, 1]
    - a prior probability in [0, 1]
    TODO: Create code to train a model from self play.
    TODO: Load and use a trained model instead of using heuristic evaluations.
    '''

    def __init__(self, model_path = "back_end/ai/model.pt", device = "cpu"):
        '''
        model_path: Path to the saved PyTorch model weights
        device: "cpu" or "cuda" for inference
        '''
        self.device = device
        '''
        In this example we define the 5 input features of
        1. a normalized pip sum from adjacent hexes,
        2. a normalized x coordinate,
        3. a normalized y coordinate,
        4. a normalized count of adjacent hexes (max is 3), and
        5. a bias term (always 1.0).
        '''
        input_dim = 5
        self.model = SettlersPolicyValueNet(input_dim)
        self.model.to(self.device)
        try:
            self.model.load_state_dict(torch.load(model_path, map_location = device))
            print(f"Loaded model weights from {model_path}")
        except Exception as e:
            print(f"Warning: Failed to load model weights from {model_path}: {e}")
        self.model.eval()


    def predict_settlement(self, game_state, vertex, vertex_coords):
        '''
        Returns (value, prior) for a candidate settlement at the given vertex.
        '''
        value, prior = self.evaluate_settlement(vertex, vertex_coords, game_state)
        return value, prior
    

    def predict_road(self, game_state, edge, edge_key, vertex_coords, last_settlement):
        '''
        Returns (value, prior) for a candidate road move (edge, edge_key).
        For a road, we "tranfer" the settlement evaluation to the far vertex.
        '''
        value, prior = self.evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)
        return value, prior
    

    def evaluate_settlement(self, vertex, vertex_coords, game_state):
        '''
        Evaluate a candidate settlement move using the trained model.
        Constructs a feature vector for the candidate vertex.
        Features used (all normalized):
        - Normalized pip sum from adjacent hexes: total_pips / (number_of_adjacent_hexes * 5)
        - normalized x coordinate: x / WIDTH_OF_BOARD_IN_VMIN
        - normalized y coordinate: y / 100 (assumes board height = 100 vmin)
        - normalized count of adjacent hexes: (# adjacent hexes) / 3
        - bias term: 1.0
        '''
        x, y = vertex_coords[vertex]
        total_pips = 0
        hex_count = 0
        for hex in hexes:
            vertices = get_hex_vertices(hex)
            for vx, vy in vertices:
                if math.isclose(vx, x, abs_tol = MARGIN_OF_ERROR) and math.isclose(vy, y, abs_tol = MARGIN_OF_ERROR):
                    hex_id = hex["id"]
                    token = TOKEN_MAPPING.get(hex_id)
                    if token is not None:
                        pip_value = TOKEN_DOT_MAPPING.get(token, 0)
                        total_pips += pip_value
                        hex_count += 1
                    break # Move on to the next hex once a match is found.
        normalized_pip = total_pips / (hex_count * 5) if hex_count > 0 else 0.0
        normalized_x = x / WIDTH_OF_BOARD_IN_VMIN
        normalized_y = y / 100.0
        normalized_hex_count = hex_count / 3.0 # Maximum adjacent hexes for a vertex is 3.
        # Construct the feature vector.
        feature_vector = np.array(
            [normalized_pip, normalized_x, normalized_y, normalized_hex_count, 1.0],
            dtype = np.float32
        )
        input_tensor = torch.tensor(feature_vector).to(self.device).unsqueeze(0) # shape: (1, 5)
        with torch.no_grad():
            value_tensor, prior_tensor = self.model(input_tensor)
        value = value_tensor.item()
        prior = prior_tensor.item()
        return value, prior
    

    def evaluate_road(self, edge, edge_key, vertex_coords, last_settlement, game_state):
        '''
        Evaluate a road move by "transferring" the settlement evaluation
        from the last settlement to the candidate road's far vertex.
        '''
        # Check if `last_settlement` is valid.
        if last_settlement is None or last_settlement not in vertex_coords:
            return -1.0, 0.0
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
            # If the other vertex is not found, return a very poor evaluation.
            return -1.0, 0.0
        # Reuse the settlement evaluation on the far vertex.
        return self.evaluate_settlement(other_label, vertex_coords, game_state)


neural_network = NeuralNetwork()