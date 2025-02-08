from routes import blueprint_for_route_next
from routes import blueprint_for_route_roads
from routes import blueprint_for_route_root
from routes import blueprint_for_route_settlements
from flask_cors import CORS
from flask import Flask
from config import INDICATOR_OF_WHETHER_DEBUG_MODE_SHOULD_BE_ENABLED
from db.database import initialize_database
from config import PORT_ON_WHICH_BACK_END_LISTENS
from config import URL_OF_FRONT_END

if __name__ == '__main__':
    app = Flask(__name__)
    CORS(
        app,
        resources = {
            r"/*": {
                "origins": URL_OF_FRONT_END
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