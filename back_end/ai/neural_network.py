'''
This module defines two separate concerns:
1. The SettlersNeuralNet class handles all inference operations.
2. A separate model watcher mechanism (via start_model_watcher)
   periodically checks for updated weights on disk and triggers a reload.
'''


from ..utilities.board import Board
from ..utilities.board import MARGIN_OF_ERROR
import logging
import math
import os
from back_end.settings import settings
import threading
import time
import torch
import torch.nn as nn
import threading


logger = logging.getLogger(__name__)


class SettlersPolicyValueNet(nn.Module):

    def __init__(self, input_dim, hidden_dim = 128):
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


class SettlersNeuralNet:

    '''
    Handles loading model weights and making predictions for settlement and road moves.
    '''
    def __init__(self, model_path):
        self.model_path = model_path
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        input_dim = 5 # Features: normalized pip sum, x, y, hex count, bias
        self.model = SettlersPolicyValueNet(input_dim)
        self.model.to(self.device)
        self.last_model_mod_time = None
        self.load_model_weights()
        self.board = Board()
        self.model.eval()
    

    def load_model_weights(self):
        '''
        Load the model weights from disk.
        If the file is not found, the untrained model remains active.
        '''
        if os.path.exists(self.model_path):
            try:
                self.model.load_state_dict(torch.load(self.model_path, map_location = self.device))
                self.last_model_mod_time = os.path.getmtime(self.model_path)
                logger.info(f"[NEURAL NETWORK] Weights were loaded from {self.model_path}.")
            except Exception as e:
                logger.exception(f"[NEURAL NETWORK] The following error occurred when loading weights. {e}")
        else:
            logger.warning("[NEURAL NETWORK] Weights file was not found. Untrained model will be used.")
        self.model.eval()
    

    def reload_if_updated(self):
        '''
        Check if the model file has been updated and reload weights if so.
        If so, reload weights.
        '''
        if os.path.exists(self.model_path):
            mod_time = os.path.getmtime(self.model_path)
            if self.last_model_mod_time is None or mod_time > self.last_model_mod_time:
                print("[NEURAL NETWORK] Updated weights were detected and will be reloaded.")
                self.load_model_weights()


    def evaluate_settlement(self, vertex):
        features = self.board.get_vertex_features(vertex)
        if features is None:
            return -1.0, 0.0
        input_tensor = torch.tensor(features, dtype = torch.float32).to(self.device).unsqueeze(0)
        with torch.no_grad():
            value_tensor, prior_tensor = self.model(input_tensor)
            return value_tensor.item(), prior_tensor.item()
        

    def evaluate_city(self, vertex):
        features = self.board.get_vertex_features(vertex)
        if features is None:
            return -1.0, 0.0
        input_tensor = torch.tensor(features, dtype = torch.float32).to(self.device).unsqueeze(0)
        with torch.no_grad():
            value_tensor, prior_tensor = self.model(input_tensor)
            return value_tensor.item(), prior_tensor.item()
    

    def evaluate_road(self, edge, vertex_coords, last_settlement):
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
            return -1.0, 0.0
        return self.evaluate_settlement(other_label)


def start_model_watcher(neural_network, interval = 10):
    '''
    Start a daemon thread that periodically calls neural_network.reload_if_updated.
    '''
    def watcher():
        while True:
            try:
                neural_network.reload_if_updated()
            except Exception as e:
                print(f"[MODEL WATCHER] Error: {e}")
            time.sleep(interval)
    thread = threading.Thread(target = watcher, daemon = True)
    thread.start()


neural_network = SettlersNeuralNet(settings.model_path)
start_model_watcher(neural_network)