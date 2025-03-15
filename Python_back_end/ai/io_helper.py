from filelock import FileLock
import logging
import numpy as np
import os
from Python_back_end.settings import settings
import torch


LOCK_PATH = settings.training_data_path + ".lock"
logger = logging.getLogger(__name__)
lock = FileLock(LOCK_PATH)


def load_training_data() -> list:
    '''
    Load training data from the configured file path.
    '''
    if os.path.exists(settings.training_data_path):
        return np.load(settings.training_data_path, allow_pickle = True).tolist()
    return []


def save_training_data(data: list) -> None:
    '''
    Save training data to the configured file path.
    '''
    with lock:
        np.save(settings.training_data_path, data)


def update_training_data(new_examples: list) -> list:
    '''
    Load, extend, and save training examples.
    '''
    with lock:
        data = load_training_data()
        data.extend(new_examples)
        np.save(settings.training_data_path, data)
    return data


def load_model_weights(model, device) -> float:
    '''
    Load model weights from disk if available and return the file's modification time.
    '''
    last_mod_time = None
    if os.path.exists(settings.model_path):
        try:
            state_dict = torch.load(settings.model_path, map_location = device)
            model.load_state_dict(state_dict)
            logger.info(f"[IO HELPER] Weights were loaded from {settings.model_path}.")
            last_mod_time = os.path.getmtime(settings.model_path)
        except Exception as e:
            raise RuntimeError(f"Error loading model weights: {e}")
    return last_mod_time


def save_model_weights(model) -> None:
    '''
    Save the given model weights to disk.
    '''
    torch.save(model.state_dict(), settings.model_path)