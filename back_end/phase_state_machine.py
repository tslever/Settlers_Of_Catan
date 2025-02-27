from abc import ABC
from .phase import Phase
from flask import abort
from abc import abstractmethod
from back_end.game.game_actions import compute_strengths
from back_end.game.game_actions import create_city
from back_end.game.game_actions import create_road
from back_end.game.game_actions import create_settlement
import json
import logging


logger = logging.getLogger(__name__)


class PhaseState(ABC):

    @abstractmethod
    def handle(self, session, state, current_player):
        '''
        Execute the logic for the current phase.
        This method updates the state (e.g., by changing phase or updating last moves) and returns a response dictionary.
        '''
        pass


class PlaceFirstSettlementState(PhaseState):
    
    def handle(self, session, state, current_player):
        chosen_vertex, settlement_id, next_phase, exception = create_settlement(session, current_player, Phase.TO_PLACE_FIRST_SETTLEMENT)
        if exception:
            logger.error(f"The following error occurred when creating a settlement. {exception}")
            abort(400, description = exception)
        state.phase = next_phase.value
        state.last_settlement = chosen_vertex
        session.commit()
        logger.info(f"Settlement {settlement_id} was created for Player {current_player} at vertex {chosen_vertex}.")
        logger.info(f"Phase {next_phase.value} will begin.")
        return {
            "message": f"Settlement {settlement_id} was created for Player {current_player} at vertex {chosen_vertex}.",
            "moveType": "settlement",
            "settlement": {
                "id": settlement_id,
                "player": current_player,
                "vertex": chosen_vertex
            }
        }


class PlaceFirstCityState(PhaseState):
    def handle(self, session, state, current_player):
        chosen_vertex, city_id, next_phase, exception = create_city(session, current_player, Phase.TO_PLACE_FIRST_CITY)
        if exception:
            logger.exception(f"The following error occurred when creating a city. {exception}")
            abort(400, description = exception)
        state.phase = next_phase.value
        state.last_city = chosen_vertex
        session.commit()
        logger.info(f"City {city_id} was created for Player {current_player} at vertex {chosen_vertex}.")
        logger.info(f"Phase {next_phase.value} will begin.")
        return {
            "message": f"City {city_id} was created for Player {current_player}",
            "moveType": "city",
            "city": {
                "id": city_id,
                "player": current_player,
                "vertex": chosen_vertex
            }
        }


class RoadState(PhaseState):

    def __init__(self, phase: Phase):
        self.phase = phase
    

    def handle(self, session, state, current_player):
        last_vertex = state.last_settlement if self.phase == Phase.TO_PLACE_FIRST_ROAD else state.last_city
        chosen_edge_key, road_id, next_phase, next_player, exception = create_road(session, current_player, self.phase, last_vertex)
        if exception:
            logger.exception(f"The following error occurred when creating a road. {exception}")
            abort(400, description = exception)
        state.current_player = next_player
        state.phase = next_phase.value
        if self.phase == Phase.TO_PLACE_FIRST_ROAD:
            state.last_settlement = None
        else:
            state.last_city = None
        session.commit()
        logger.info(f"Road {road_id} was created for Player {current_player} on edge {chosen_edge_key}.")
        logger.info(f"Player {next_player}'s phase {next_phase.value} will begin.")
        response = {
            "message": f"Road {road_id} was created for Player {current_player} on edge {chosen_edge_key}",
            "moveType": "road",
            "road": {
                "id": road_id,
                "player": current_player,
                "edge": chosen_edge_key
            }
        }
        if next_phase == Phase.TURN:
            strengths = compute_strengths(session)
            response["strengths"] = strengths
            response["message"] += "\nGame setup is complete. The following dictionary represents players' strengths.\n" + json.dumps(strengths, indent = 4, sort_keys = True)
        return response


class TurnState(PhaseState):
    
    def handle(self, session, state, current_player):
        # TODO: Implement turn specific logic.
        return {
            "message": f"Player {current_player} will take their turn.",
            "moveType": "turn"
        }


class PhaseStateMachine:

    def __init__(self):
        self.handlers = {
            Phase.TO_PLACE_FIRST_SETTLEMENT: PlaceFirstSettlementState(),
            Phase.TO_PLACE_FIRST_ROAD: RoadState(Phase.TO_PLACE_FIRST_ROAD),
            Phase.TO_PLACE_FIRST_CITY: PlaceFirstCityState(),
            Phase.TO_PLACE_SECOND_ROAD: RoadState(Phase.TO_PLACE_SECOND_ROAD),
            Phase.TURN: TurnState()
        }
    

    def handle(self, session, state):
        try:
            current_phase = Phase(state.phase)
        except ValueError:
            logger.error(f"Phase {state.phase} is invalid.")
            abort(500, description = f"Phase {state.phase} is invalid.")
        current_player = state.current_player
        handler = self.handlers.get(current_phase)
        if not handler:
            logger.error(f"There is no handler for phase {current_phase.value}.")
            abort(400, description = f"There is no handler for phase {current_phase.value}.")
        return handler.handle(session, state, current_player)