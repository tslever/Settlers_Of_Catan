class GameState:
    def __init__(self):
        self.current_player = 1
        self.phase = "to_place_first_settlement"  # You might later use an Enum.
        self.settlements = {}  # Mapping from player number to vertex label.
        self.roads = {}        # Mapping from player number to a road’s unique key.
        self.last_settlement = None

    def place_settlement(self, player: int, vertex: str):
        self.settlements[player] = vertex
        self.last_settlement = vertex

    def place_road(self, player: int, road_key: str):
        self.roads[player] = road_key

    def get_state_snapshot(self) -> dict:
        """Return a dictionary snapshot of the current game state."""
        return {
            "current_player": self.current_player,
            "phase": self.phase,
            "settlements": self.settlements.copy(),
            "roads": self.roads.copy(),
            "last_settlement": self.last_settlement
        }