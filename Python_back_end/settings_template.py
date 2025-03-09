from pydantic_settings import BaseSettings


class Settings(BaseSettings):

    mysql_host: str = "localhost or something else"
    mysql_username: str = "username"
    mysql_password: str = "password"

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

    number_of_epochs: int = 10
    batch_size: int = 32
    learning_rate: float = 1e-3
    length_of_feature_vector: int = 5
    number_of_neurons_in_hidden_layer: int = 128

    margin_of_error: float = 0.01
    width_of_board_in_vmin: int = 100
    number_of_hexes_that_span_board: int = 6


settings = Settings()