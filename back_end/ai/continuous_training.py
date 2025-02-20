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

import logging
import numpy as np
from back_end.settings import settings
from .self_play import simulate_self_play_game
import time
import threading
from .io_helper import save_training_data
from .train import train_model
from .io_helper import update_training_data


logger = logging.getLogger(__name__)
stop_event = threading.Event()


def continuous_self_play_loop(stop_event):
    while not stop_event.is_set():
        try:
            # Simulate one self play game.
            new_examples = simulate_self_play_game(num_simulations = settings.num_simulations, c_puct = settings.c_puct)
            logger.info(f"[SELF PLAY] Self play generated {len(new_examples)} new training examples.")
            data = update_training_data(new_examples)
            total = len(data)
            logger.info(f"[SELF PLAY] Self play total training examples: {total}")

            # Trigger a training update if enough data has been accumulated.
            if total >= settings.training_threshold:
                logger.info("[TRAINING] Starting training update...")
                # Use a relatively short training run so as not to delay continuous play too much.
                train_model(
                    training_data = data,
                    num_epochs = 10, # relatively few epochs for a quick update
                    batch_size = 32,
                    learning_rate = 1e-3
                )
                logger.info("[TRAINING] Training is complete.")
                save_training_data([])
        except Exception as e:
            logger.exception(f"[SELF PLAY] The following exception occurred in a continuous self play loop. {e}")
        finally:
            time.sleep(settings.game_interval)


def start_continuous_training_in_background():
    thread = threading.Thread(target = continuous_self_play_loop, args = (stop_event,), daemon = True)
    thread.start()
    logger.info("[CONTINUOUS TRAINING] A loop for self play and training has been started in the background.")
    return thread