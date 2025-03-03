from back_end.phase import Phase


class GameState:

    def __init__(self):
        self.current_player = 1
        self.phase = Phase.TO_PLACE_FIRST_SETTLEMENT.value
        self.settlements = {
            1: [],
            2: [],
            3: []
        }
        self.cities = {
            1: [],
            2: [],
            3: []
        }
        self.roads = {
            1: [],
            2: [],
            3: []
        }
        self.last_settlement = None
        self.last_city = None
        self.last_building = None


    def place_city(self, player: int, vertex: str):
        list_of_labels_of_vertices_with_cities_for_player = self.cities[player]
        list_of_labels_of_vertices_with_cities_for_player.append(vertex)
        self.last_city = vertex
        self.last_building = vertex


    def place_road(self, player: int, road_key: str):
        list_of_keys_of_edges_with_roads_for_player = self.roads[player]
        list_of_keys_of_edges_with_roads_for_player.append(road_key)


    def place_settlement(self, player: int, vertex: str):
        list_of_labels_of_vertices_with_settlements_for_player = self.settlements[player]
        list_of_labels_of_vertices_with_settlements_for_player.append(vertex)
        self.last_settlement = vertex
        self.last_building = vertex


    def get_state_snapshot(self) -> dict:
        '''
        Return a dictionary snapshot of the current game state.
        '''
        return {
            "current_player": self.current_player,
            "phase": self.phase,
            "settlements": self.settlements,
            "roads": self.roads,
            "last_settlement": self.last_settlement,
            "last_city": self.last_city,
            "last_building": self.last_building
        }