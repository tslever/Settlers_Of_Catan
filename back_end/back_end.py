from flask import Flask, jsonify
from flask_cors import CORS

app = Flask(__name__)

@app.route("/", methods = ["GET"])
def listen_at_root():
    return jsonify(
        {
            "message": "Server response: Next action executed!"
        }
    )

if __name__ == '__main__':
    CORS(
        app,
        resources = {
            r"/*": {
                "origins": "http://localhost:3000"
            }
        }
    )
    app.run(
        port = 5000,
        debug = True
    )