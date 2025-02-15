from ..utilities.board import Board
from ..utilities.board import MARGIN_OF_ERROR
import math
import os
from back_end.settings import settings
import threading
import time
import torch
import torch.nn as nn


class SettlersPolicyValueNet(nn.Module):

    def __init__(self, input_dim, hidden_dim=128):
        super(SettlersPolicyValueNet, self).__init__()
        self.fc1 = nn.Linear(input_dim, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, hidden_dim)
        # Policy head: single output squashed via sigmoid
        self.fc_policy = nn.Linear(hidden_dim, 1)
        # Value head: scalar output squashed via tanh
        self.fc_value = nn.Linear(hidden_dim, 1)


    def forward(self, x):
        x = torch.relu(self.fc1(x))
        x = torch.relu(self.fc2(x))
        policy = torch.sigmoid(self.fc_policy(x))
        value = torch.tanh(self.fc_value(x))
        return value, policy


class NeuralNetwork:
    """
    A neural network that uses an AlphaGo Zero style model.
    For each candidate move it returns:
      - a value estimate in [-1, 1]
      - a prior probability in [0, 1]
    """

    def __init__(self, model_path = settings.model_path, device="cpu"):
        self.model_path = model_path
        self.device = device
        input_dim = 5  # Features: normalized pip sum, x, y, hex count, bias.
        self.model = SettlersPolicyValueNet(input_dim)
        self.model.to(self.device)
        self.last_model_mod_time = None
        self.load_model_weights()
        # Create an instance of the Board for geometry and pip calculations.
        self.board = Board()
        threading.Thread(target=self._watch_model_update, daemon=True).start()
        self.model.eval()


    def load_model_weights(self):
        if os.path.exists(self.model_path):
            try:
                self.model.load_state_dict(torch.load(self.model_path, map_location=self.device))
                self.last_model_mod_time = os.path.getmtime(self.model_path)
                print(f"[NEURAL NETWORK] Weights loaded from {self.model_path}.")
            except Exception as e:
                print(f"[NEURAL NETWORK] Error loading weights from {self.model_path}: {e}")
        else:
            print(f"[NEURAL NETWORK] Weights file not found. Using untrained model.")
        self.model.eval()


    def _watch_model_update(self):
        while True:
            try:
                if os.path.exists(self.model_path):
                    mod_time = os.path.getmtime(self.model_path)
                    if self.last_model_mod_time is None or mod_time > self.last_model_mod_time:
                        print("[NEURAL NETWORK] Weights updated. Reloading...")
                        self.model.load_state_dict(torch.load(self.model_path, map_location=self.device))
                        self.last_model_mod_time = mod_time
                        print("[NEURAL NETWORK] Weights reloaded.")
            except Exception as e:
                print(f"[NEURAL NETWORK] Error checking for updates: {e}")
            time.sleep(10)

    def predict_settlement(self, game_state, vertex, vertex_coords):
        value, prior = self.evaluate_settlement(vertex, vertex_coords, game_state)
        return value, prior

    def predict_road(self, game_state, edge, edge_key, vertex_coords, last_settlement):
        value, prior = self.evaluate_road(edge, edge_key, vertex_coords, last_settlement, game_state)
        return value, prior

    def evaluate_settlement(self, vertex, vertex_coords, game_state):
        # Use the Board’s method to get the feature vector for this vertex.
        features = self.board.get_vertex_features(vertex)
        if features is None:
            return -1.0, 0.0
        input_tensor = torch.tensor(features, dtype=torch.float32).to(self.device).unsqueeze(0)
        with torch.no_grad():
            value_tensor, prior_tensor = self.model(input_tensor)
        return value_tensor.item(), prior_tensor.item()

    def evaluate_road(self, edge, edge_key, vertex_coords, last_settlement, game_state):
        if last_settlement is None or last_settlement not in vertex_coords:
            return -1.0, 0.0
        settlement_coord = vertex_coords[last_settlement]
        v1 = (edge["x1"], edge["y1"])
        v2 = (edge["x2"], edge["y2"])
        if (math.isclose(v1[0], settlement_coord[0], abs_tol=MARGIN_OF_ERROR) and
            math.isclose(v1[1], settlement_coord[1], abs_tol=MARGIN_OF_ERROR)):
            other = v2
        else:
            other = v1
        other_label = None
        for label, coords in vertex_coords.items():
            if (math.isclose(coords[0], other[0], abs_tol=MARGIN_OF_ERROR) and
                math.isclose(coords[1], other[1], abs_tol=MARGIN_OF_ERROR)):
                other_label = label
                break
        if other_label is None:
            return -1.0, 0.0
        # Reuse settlement evaluation for the far vertex.
        return self.evaluate_settlement(other_label, vertex_coords, game_state)

neural_network = NeuralNetwork()