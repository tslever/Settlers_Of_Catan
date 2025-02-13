from enum import Enum


class Phase(Enum):
    TO_PLACE_FIRST_SETTLEMENT = "phase to place first settlement"
    TO_PLACE_FIRST_ROAD = "phase to place first road"
    TO_PLACE_SECOND_SETTLEMENT = "phase to place second settlement"
    TO_PLACE_SECOND_ROAD = "phase to place second road"
    TURN = "turn"