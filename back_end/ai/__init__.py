from .neural_network import NeuralNetwork
import os
from .self_play import generate_training_data
from .continuous_training import start_continuous_training_in_background
from .train import train_model


'''
current_dir = os.path.dirname(os.path.abspath(__file__))
training_data_path = os.path.join(current_dir, "self_play_training_data.npy")
model_path = os.path.join(current_dir, "neural_network.pt")


if not os.path.exists(training_data_path):
    print("Self play training data not found. Generating training data...")
    generate_training_data(num_games = 100, num_simulations = 100, c_puct = 1.0)
else:
    print("Self play training data found.")


if not os.path.exists(model_path):
    print("Neural network not found. Training the neural network...")
    train_model(
        npy_file = training_data_path,
        model_save_path = model_path,
        num_epochs = 100,
        batch_size = 32,
        learning_rate = 1e-3
    )
else:
    print("Neural network model found.")
'''


# Start the continuous self play and training loop.
start_continuous_training_in_background()


#neural_network = NeuralNetwork(model_path = model_path)
neural_network = NeuralNetwork()