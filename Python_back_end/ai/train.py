#!/usr/bin/env python3
"""
train.py

This script loads self play training examples from the configured file,
builds the appropriate 5 element input feature vectors for the SettlersPolicyValueNet,
trains the network to predict both a value in [-1, 1] and a policy, a probability in [0, 1],
and saves trained model weights.
"""

from ..game.board import Board
from torch.utils.data import Dataset
from torch.utils.data import DataLoader
from ..ai.neural_network import SettlersPolicyValueNet
from .io_helper import load_training_data
import logging
import os
from Python_back_end.settings import settings
import torch
import torch.nn as nn
import torch.optim as optim
from .io_helper import save_model_weights
from Python_back_end.logger import set_up_logging


set_up_logging()
logger = logging.getLogger(__name__)


class SelfPlayDataset(Dataset):
    """
    A PyTorch Dataset that loads self play training examples.
    Each training example is a dictionary with keys:
      - "move_type": "city", "settlement", or "road"
      - "policy": a dictionary mapping candidate moves (for settlements, vertex labels) to their MCTS derived probabilities
      - "value": +1 for win or -1 for loss
    
    Only samples with move type "settlement" are used to train the settlement network (which uses a 5 feature vector).
    TODO: Consider using samples with other move types to train the settlement network.
    For each such sample we choose the move with the highest probability (from the MCTS policy)
    and then compute the feature vector for that candidate vertex.
    The feature vector is computed exactly as in the neural network's evaluate_settlement function.
    The feature vector includes:
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
        # Precompute the vertex coordinates dictionary. `board.vertices` is a list of dictionaries with keys "label", "x", and "y".
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
        x = torch.tensor(feature_vector, dtype = torch.float32)
        y_value = torch.tensor([target_value], dtype = torch.float32)
        y_policy = torch.tensor([target_policy], dtype = torch.float32)
        return x, y_value, y_policy


def train_model(
    training_data,
    num_epochs = settings.number_of_epochs,
    batch_size = settings.batch_size,
    learning_rate = settings.learning_rate
):
    dataset = SelfPlayDataset(training_data)
    dataloader = DataLoader(dataset, batch_size = batch_size, shuffle = True)
    
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = SettlersPolicyValueNet(
        input_dim = settings.length_of_feature_vector,
        hidden_dim = settings.number_of_neurons_in_hidden_layer
    )
    model.to(device)
    
    # We use the mean–squared–error loss for both value and policy outputs.
    criterion_for_determining_loss_from_value_head = nn.MSELoss()
    criterion_for_determining_loss_from_policy_head = nn.BCELoss()
    optimizer = optim.Adam(model.parameters(), lr = learning_rate)
    
    model.train()
    for epoch in range(num_epochs):
        running_loss = 0.0
        total_samples = 0
        for x, target_value, target_policy in dataloader:
            loss = train_step(
                model,
                optimizer,
                criterion_for_determining_loss_from_value_head,
                criterion_for_determining_loss_from_policy_head,
                x,
                target_value,
                target_policy,
                device
            )
            batch_size_actual = x.size(0)
            running_loss += loss
            total_samples += batch_size_actual
        avg_loss = running_loss / total_samples
        logger.info(f"Epoch {epoch + 1} / {num_epochs}, Loss: {avg_loss:.4f}")
        save_model_weights(model)
    logger.info(f"Model has been saved.")


def train_step(
    model,
    optimizer,
    criterion_for_determining_loss_from_value_head,
    criterion_for_determining_loss_from_policy_head,
    x,
    target_value,
    target_policy,
    device
):
    x = x.to(device)
    target_value = target_value.to(device)
    target_policy = target_policy.to(device)
    
    optimizer.zero_grad()
    pred_value, pred_policy = model(x)
    
    loss_value = criterion_for_determining_loss_from_value_head(pred_value, target_value)
    loss_policy = criterion_for_determining_loss_from_policy_head(pred_policy, target_policy)
    loss = loss_value + loss_policy

    loss.backward()
    optimizer.step()
    
    return loss.item() * x.size(0)


if __name__ == "__main__":
    current_dir = os.path.dirname(os.path.abspath(__file__))
    training_data = load_training_data()
    train_model(
        training_data = training_data,
        num_epochs = settings.number_of_epochs,
        batch_size = settings.batch_size,
        learning_rate = settings.learning_rate
    )