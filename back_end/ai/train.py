#!/usr/bin/env python3
"""
train.py

This script loads self play training examples from
the training data path in settings, constructs the appropriate 5 element
input feature vectors (as used by the SettlersPolicyValueNet in neural_network.py),
trains the network to predict both a value (in [-1,1]) and a policy (a probability in [0,1]),
and then saves the trained model weights to the model path in settings.
"""

from ..board import Board
from torch.utils.data import Dataset
from torch.utils.data import DataLoader
from ..ai.neural_network import SettlersPolicyValueNet
from .io_helper import load_training_data
import logging
import os
from back_end.settings import settings
import torch
import torch.nn as nn
import torch.optim as optim
from .io_helper import save_model_weights
from back_end.logger import set_up_logging


set_up_logging()
logger = logging.getLogger(__name__)


class SelfPlayDataset(Dataset):
    """
    A PyTorch Dataset that loads self play training examples from a .npy file.
    
    Each training example is assumed to be a dictionary with at least the following keys:
      - "move_type": either "settlement" or "road"
      - "policy": a dictionary mapping candidate moves (for settlements, vertex labels)
          to their MCTS derived probabilities.
      - "value": a scalar outcome (e.g. +1 for win, -1 for loss)
    
    For training the settlement network (which uses a 5 feature vector) we only use
    samples with "move_type" == "settlement". For each such sample we choose the move
    with the highest probability (from the MCTS policy) and then compute the feature vector
    for that candidate vertex. (The feature vector is computed exactly as in the neural
    network's evaluate_settlement function:)
    
      1. normalized pip sum from adjacent hexes: total_pips / (hex_count * 5)
      2. normalized x coordinate: x / WIDTH_OF_BOARD_IN_VMIN
      3. normalized y coordinate: y / 100.0
      4. normalized count of adjacent hexes: hex_count / 3.0
      5. bias term: 1.0
    """

    def __init__(self, training_data: list):
        '''
        Training data is expected to be a list of example dictionaries.
        '''
        self.data = training_data
        # Precompute the vertex coordinates dictionary from vertices_with_labels.
        # (Each vertex is a dict with keys "label", "x", and "y".)
        board = Board()
        self.vertex_coords = {v["label"]: (v["x"], v["y"]) for v in board.vertices}
        
        # We will build a list of samples as tuples: (feature_vector, target_value, target_policy)
        self.samples = []
        for sample in self.data:
            if sample.get("move_type") != "settlement":
                continue  # TODO: Consider non-settlement moves.
            policy_dict = sample.get("policy", {})
            if not policy_dict:
                continue
            # Choose the candidate vertex with highest probability.
            chosen_vertex, target_policy = max(policy_dict.items(), key=lambda kv: kv[1])
            target_value = sample.get("value", 0.0)
            
            feature_vector = board.get_vertex_features(chosen_vertex)
            if feature_vector is None:
                feature_vector = [0.0, 0.0, 0.0, 0.0, 1.0]
            
            self.samples.append((feature_vector, target_value, target_policy))
        
        print(f"Loaded {len(self.samples)} settlement training samples.")


    def __len__(self):
        return len(self.samples)


    def __getitem__(self, idx):
        feature_vector, target_value, target_policy = self.samples[idx]
        # Convert to tensors (the network expects inputs of shape [batch, 5])
        x = torch.tensor(feature_vector, dtype = torch.float32)
        y_value = torch.tensor([target_value], dtype = torch.float32)
        y_policy = torch.tensor([target_policy], dtype = torch.float32)
        return x, y_value, y_policy


# ------------------------------------------------------------------------------
# Training loop
# ------------------------------------------------------------------------------

def train_model(training_data, num_epochs = settings.number_of_epochs, batch_size = settings.batch_size, learning_rate = settings.learning_rate):
    # Create the dataset and dataloader.
    dataset = SelfPlayDataset(training_data)
    dataloader = DataLoader(dataset, batch_size = batch_size, shuffle = True)
    
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = SettlersPolicyValueNet(input_dim = settings.length_of_feature_vector, hidden_dim = settings.number_of_neurons_in_hidden_layer)
    model.to(device)
    
    # We use the mean–squared–error loss for both value and policy outputs.
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr = learning_rate)
    
    model.train()
    for epoch in range(num_epochs):
        epoch_loss = 0.0
        for x, target_value, target_policy in dataloader:
            epoch_loss += train_step(model, optimizer, criterion, x, target_value, target_policy, device)
            epoch_loss /= len(dataset)
            logger.info(f"Epoch {epoch + 1} / {num_epochs}, Loss: {epoch_loss:.4f}")
    
    save_model_weights(model)
    logger.info(f"Model has been saved.")


def train_step(model, optimizer, criterion, x, target_value, target_policy, device):
    x = x.to(device)
    target_value = target_value.to(device)
    target_policy = target_policy.to(device)
    optimizer.zero_grad()
    pred_value, pred_policy = model(x)
    loss_value = criterion(pred_value, target_value)
    loss_policy = criterion(pred_policy, target_policy)
    loss = loss_value + loss_policy
    loss.backward()
    optimizer.step()
    return loss.item() * x.size(0)


if __name__ == "__main__":
    current_dir = os.path.dirname(os.path.abspath(__file__))
    npy_file = settings.training_data_path
    model_save_path = settings.model_path
    training_data = load_training_data()
    train_model(training_data = training_data, num_epochs=100, batch_size=32, learning_rate=1e-3)