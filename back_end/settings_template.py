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
    num_simulations: int = 50
    training_threshold: int = 500

settings = Settings()