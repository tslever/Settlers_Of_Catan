from pydantic_settings import BaseSettings


class Settings(BaseSettings):

    db_username: str = "username"
    db_password: str = "password"
    db_host: str = "localhost or something else"
    db_name: str = "name of database"

    debug: bool = True
    back_end_port: int = 5000
    front_end_url: str = "http://localhost:3000"

    board_geometry_path: str = "board_geometry.json"
    model_path: str = "back_end/ai/neural_network.pt"
    training_data_path: str = "back_end/ai/self_play_training_data.npy"

    game_interval: float = 1.0
    c_puct: float = 1.0
    number_of_simulations: int = 50
    training_threshold: int = 500

    number_of_epochs = 10
    batch_size = 32
    learning_rate = 1e-3
    length_of_feature_vector = 5
    number_of_neurons_in_hidden_layer = 128

    margin_of_error = 0.01
    width_of_board_in_vmin = 100
    number_of_hexes_that_span_board = 6


settings = Settings()