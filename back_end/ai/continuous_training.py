'''
continuous_training.py

Module `continuous_training` runs a continuous self play loop in a background thread.
Each self play game produces outcome labeled training examples.
The new examples are merged with existing training data.
When enough new data accumulates (or after a fixed interval),
a training run is triggered to update the neural network.
The updated model is saved to disk so that the live Monte Carlo Tree Search
(using NeuralNetwork) can detect and reload the new weights.
'''

import os
import time
import threading
import numpy as np
from .self_play import simulate_self_play_game
from .train import train_model


C_PUCT = 1.0
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
GAME_INTERVAL = 1.0 # seconds to sleep between games
MODEL_PATH = os.path.join(CURRENT_DIR, "neural_network.pt")
NUM_SIMULATIONS = 50 # number of MCTS simulations per self play game
TRAINING_DATA_PATH = os.path.join(CURRENT_DIR, "self_play_training_data.npy")
TRAINING_THRESHOLD = 500 # When total examples exceed this, trigger training.


def load_existing_training_data():
    if os.path.exists(TRAINING_DATA_PATH):
        return np.load(TRAINING_DATA_PATH, allow_pickle = True).tolist()
    return []


def update_training_data(new_examples):
    data = load_existing_training_data()
    data.extend(new_examples)
    # TODO: Use file locks for safety.
    np.save(TRAINING_DATA_PATH, data)
    return data


def continuous_self_play_loop():
    while True:
        # Simulate one self play game.
        new_examples = simulate_self_play_game(num_simulations = NUM_SIMULATIONS, c_puct = C_PUCT)
        print(f"[SELF PLAY] Self play generated {len(new_examples)} new training examples.")
        data = update_training_data(new_examples)
        total = len(data)
        print(f"[SELF PLAY] Self play total training examples: {total}")

        # Trigger a training update if enough data has been accumulated.
        if total >= TRAINING_THRESHOLD:
            print("[TRAINING] Starting training update...")
            # Use a relatively short training run so as not to delay continuous play too much.
            train_model(
                npy_file = TRAINING_DATA_PATH,
                model_save_path = MODEL_PATH,
                num_epochs = 10, # relatively few epochs for a quick update
                batch_size = 32,
                learning_rate = 1e-3
            )
            print("[TRAINING] Training is complete. A new model has been saved.")
            np.save(TRAINING_DATA_PATH, [])
        time.sleep(GAME_INTERVAL)


def start_continuous_training_in_background():
    thread = threading.Thread(target = continuous_self_play_loop, daemon = True)
    thread.start()
    print("[CONTINUOUS TRAINING] A loop for self play and training has been started in the background.")