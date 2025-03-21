from flask import Blueprint
from ..game.board import Board
from Python_back_end.game.phase import Phase
from Python_back_end.game.phase_state_machine import PhaseStateMachine
from ..db.database import State
from ..db.database import get_db_session
from flask import jsonify
import logging


ID_OF_STATE = 1
blueprint_for_route_next = Blueprint("next", __name__)
board = Board()
logger = logging.getLogger(__name__)


@blueprint_for_route_next.route("/next", methods = ["POST"])
def next_move():
    with get_db_session() as session:
        state = session.query(State).filter_by(id = ID_OF_STATE).first()
        if state is None:
            state = State(
                id = ID_OF_STATE,
                current_player = 1,
                phase = Phase.TO_PLACE_FIRST_SETTLEMENT.value,
                last_building = None
            )
            session.add(state)
            session.commit()
            logger.info(f"Initialized state: player = 1, phase = {Phase.TO_PLACE_FIRST_SETTLEMENT.value}")
        logger.info(f"Current state: player = {state.current_player}, phase = {state.phase}, last_building = {state.last_building}")
        machine = PhaseStateMachine()
        response = machine.handle(session, state)
        return jsonify(response)