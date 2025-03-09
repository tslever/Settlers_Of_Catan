from flask import Blueprint, jsonify


blueprint_for_route_root = Blueprint("root", __name__)


@blueprint_for_route_root.route("/", methods = ["GET"])
def listen_at_root():
    return jsonify(
        {
            "message": "You have requested information from endpoint '/'."
        }
    )