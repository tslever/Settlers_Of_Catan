from db.database import initialize_database
from flask import Flask
from flask_cors import CORS
from routes import (
    blueprint_for_route_next,
    blueprint_for_route_roads,
    blueprint_for_route_root,
    blueprint_for_route_settlements
)

ORIGIN_OF_FRONT_END = "http://localhost:3000"
PORT_ON_WHICH_BACK_END_LISTENS = 5000
INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED = True

if __name__ == '__main__':
    app = Flask(__name__)
    CORS(
        app,
        resources = {
            r"/*": {
                "origins": ORIGIN_OF_FRONT_END
            }
        }
    )
    app.register_blueprint(blueprint_for_route_next)
    app.register_blueprint(blueprint_for_route_roads)
    app.register_blueprint(blueprint_for_route_root)
    app.register_blueprint(blueprint_for_route_settlements)
    initialize_database()
    app.run(
        port = PORT_ON_WHICH_BACK_END_LISTENS,
        debug = INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED
    )