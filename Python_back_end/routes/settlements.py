from ..db.database import Settlement
from ..db.database import get_db_session
from flask import Blueprint, jsonify


blueprint_for_route_settlements = Blueprint("settlements", __name__)


@blueprint_for_route_settlements.route("/settlements", methods = ["GET"])
def get_settlements():
    with get_db_session() as session:
        settlements = session.query(Settlement).all()
        list_of_dictionaries_of_settlement_information = [
            {"id": settlement.id, "player": settlement.player, "vertex": settlement.vertex} for settlement in settlements
        ]
        return jsonify({"settlements": list_of_dictionaries_of_settlement_information})