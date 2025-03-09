'''
This module defines two separate concerns:
1. The SettlersNeuralNet class handles all inference operations.
2. A separate model watcher mechanism (via start_model_watcher)
   periodically checks for updated weights on disk and triggers a reload.
'''


from ..board import Board
from .io_helper import load_model_weights
import logging
import math
import os
from back_end.settings import settings
import threading
import time
import torch
import torch.nn as nn


logger = logging.getLogger(__name__)


class FeatureExtractionException(Exception):
    pass


class SettlersPolicyValueNet(nn.Module):

    def __init__(self, input_dim, hidden_dim = settings.number_of_neurons_in_hidden_layer):
        super().__init__()
        self.fc1 = nn.Linear(input_dim, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, hidden_dim)
        self.fc_policy = nn.Linear(hidden_dim, 1) # policy head
        self.fc_value = nn.Linear(hidden_dim, 1) # value head


    def forward(self, x):
        x = torch.relu(self.fc1(x))
        x = torch.relu(self.fc2(x))
        value = torch.tanh(self.fc_value(x))
        policy = torch.sigmoid(self.fc_policy(x))
        return value, policy


class ModelInference:

    def __init__(self, model, device):
        self.model = model
        self.device = device
    
    def infer(self, features):
        input_tensor = torch.tensor(features, dtype = torch.float32, device = self.device).unsqueeze(0)
        with torch.no_grad():
            value_tensor, policy_tensor = self.model(input_tensor)
        return value_tensor.item(), policy_tensor.item()


class WeightReloader:
    
    def __init__(self, model, model_path, device):
        self.model = model
        self.model_path = model_path
        self.device = device
        self.last_mod_time = None
        self.load_weights()
    
    def load_weights(self):
        if os.path.exists(self.model_path):
            state_dict = torch.load(self.model_path, map_location = self.device)
            self.model.load_state_dict(state_dict)
            self.last_mod_time = os.path.getmtime(self.model_path)
            logger.info(f"Weights were loaded from {self.model_path}.")
    
    def check_and_reload(self):
        if os.path.exists(self.model_path):
            mod_time = os.path.getmtime(self.model_path)
            if self.last_mod_time is None or mod_time > self.last_mod_time:
                logger.info("Updated weights have been detected and will be reloaded.")
                self.load_weights()


class BoardFeatureExtractor:

    def __init__(self, board: Board):
        self.board = board
    
    def extract_features(self, vertex_label: str):
        return self.board.get_vertex_features(vertex_label)


class SettlersNeuralNet():

    def __init__(self, model_path):
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        input_dim = settings.length_of_feature_vector
        self.model = SettlersPolicyValueNet(input_dim)
        self.model.to(self.device)
        self.weight_reloader = WeightReloader(self.model, model_path, self.device)
        self.inference_engine = ModelInference(self.model, self.device)
        self.feature_extractor = BoardFeatureExtractor(Board())
        self.model.eval()

    def reload_if_updated(self):
        self.weight_reloader.check_and_reload()


    def evaluate_settlement(self, label_of_vertex: str):
        features = self.feature_extractor.extract_features(label_of_vertex)
        if features is None:
            logger.exception(f"Feature extraction failed for settlement at vertex {label_of_vertex}.")
            raise FeatureExtractionException(f"Feature extraction failed for settlement at vertex {label_of_vertex}.")
        return self.inference_engine.infer(features)
        

    def evaluate_city(self, label_of_vertex: str):
        features = self.feature_extractor.extract_features(label_of_vertex)
        if features is None:
            logger.exception(f"Feature extraction failed for city at vertex {label_of_vertex}.")
            raise FeatureExtractionException(f"Feature extraction failed for city at vertex {label_of_vertex}.")
        return self.inference_engine.infer(features)
    

    def evaluate_road(self, edge, vertex_coords, last_building):
        if last_building is None or last_building not in vertex_coords:
            logger.exception(
                "Vertex of last building for road evaluation is invalid because it is None " +
                "or not found in a dictionary of labels of vertices of buildings and tuples of coordinates."
            )
            raise FeatureExtractionException(
                "Vertex of last builidng for road evaluation is invalid because it is None " +
                "or not found in a dictionary of vertices of building and tuples of coordinates."
            )
        building_coord = vertex_coords[last_building]
        v1 = (edge["x1"], edge["y1"])
        v2 = (edge["x2"], edge["y2"])
        if (
            math.isclose(v1[0], building_coord[0], abs_tol = settings.margin_of_error) and
            math.isclose(v1[1], building_coord[1], abs_tol = settings.margin_of_error)
        ):
            other = v2
        else:
            other = v1
        other_label = None
        for label, coords in vertex_coords.items():
            if (
                math.isclose(coords[0], other[0], abs_tol = settings.margin_of_error) and
                math.isclose(coords[1], other[1], abs_tol = settings.margin_of_error)
            ):
                other_label = label
                break
        if other_label is None:
            logger.exception("A matching vertex for the road's endpoint could not be found.")
            raise FeatureExtractionException("A matching vertex for the road's endpoint could not be found.")
        return self.evaluate_settlement(other_label)


def start_model_watcher(neural_network, stop_event, interval = 10):
    '''
    Start a daemon thread that periodically calls neural_network.reload_if_updated.
    '''
    def watcher():
        while not stop_event.is_set():
            try:
                neural_network.reload_if_updated()
            except Exception as e:
                logger.exception(f"[MODEL WATCHER] Exception: {e}")
            time.sleep(interval)
    thread = threading.Thread(target = watcher, daemon = True)
    thread.start()
    logger.info("[MODEL WATCHER] A thread for watching for weights for a neural network was started.")
    return thread

neural_network = SettlersNeuralNet(settings.model_path)