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
from .self_play import simulate_self_play_game
import time
import threading
from .io_helper import save_training_data
from .train import train_model
from .io_helper import update_training_data
from back_end.ai.neural_network import neural_network


logger = logging.getLogger(__name__)


def continuous_self_play_loop(neural_network, stop_event, training_settings):
    while not stop_event.is_set():
        try:
            # Simulate one self play game.
            new_examples = simulate_self_play_game(
                neural_network = neural_network,
                number_of_simulations = training_settings.number_of_simulations,
                c_puct = training_settings.c_puct
            )
            logger.info(f"[SELF PLAY] Self play generated {len(new_examples)} new training examples.")
            data = update_training_data(new_examples)
            total = len(data)
            logger.info(f"[SELF PLAY] Self play total training examples: {total}")

            # Trigger a training update if enough data has been accumulated.
            if total >= training_settings.training_threshold:
                logger.info("[TRAINING] Training a neural network will begin.")
                # Use a relatively short training run so as not to delay continuous play too much.
                train_model(
                    training_data = data,
                    num_epochs = training_settings.number_of_epochs, # relatively few epochs for a quick update
                    batch_size = training_settings.batch_size,
                    learning_rate = training_settings.learning_rate
                )
                logger.info("[TRAINING] Training is complete.")
                save_training_data([])
        except Exception as e:
            logger.exception(f"[SELF PLAY] The following exception occurred in a continuous self play loop. {e}")
        finally:
            time.sleep(training_settings.game_interval)


def start_continuous_training_in_background(stop_event, training_settings):
    thread = threading.Thread(
        target = continuous_self_play_loop,
        args = (neural_network, stop_event, training_settings),
        daemon = True
    )
    thread.start()
    logger.info("[CONTINUOUS TRAINING] A loop for self play and training has been started in the background.")
    return thread