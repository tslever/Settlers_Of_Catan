from flask import Blueprint, jsonify
from back_end.db.database import State, Settlement, City, Road
from back_end.db.database import get_db_session
from Python_back_end.game.phase import Phase
import logging
from sqlalchemy import text


logger = logging.getLogger(__name__)
blueprint_for_route_reset = Blueprint("reset", __name__)


@blueprint_for_route_reset.route("/reset", methods = ["POST"])
def reset_game():
    with get_db_session() as session:

        # Delete existing settlements, cities, and roads.
        num_settlements = session.query(Settlement).delete()
        num_cities = session.query(City).delete()
        num_roads = session.query(Road).delete()
        logger.info(f"{num_settlements} settlements, {num_cities} cities, and {num_roads} roads were deleted.")
        session.commit()

        # Reset auto-increment counters so that future settlements, cities, and roads start at ID 1.
        session.execute(text("ALTER TABLE settlements AUTO_INCREMENT = 1"))
        session.execute(text("ALTER TABLE cities AUTO_INCREMENT = 1"))
        session.execute(text("ALTER TABLE roads AUTO_INCREMENT = 1"))
        session.commit()

        # Reset the game state.
        state = session.query(State).filter_by(id = 1).first()
        if state is None:
            state = State(
                id = 1,
                current_player = 1,
                phase = Phase.TO_PLACE_FIRST_SETTLEMENT.value,
                last_building = None
            )
            session.add(state)
        else:
            state.current_player = 1
            state.phase = Phase.TO_PLACE_FIRST_SETTLEMENT.value
            state.last_building = None
        session.commit()
    return jsonify({"message": "Game has been reset to the initial state."})