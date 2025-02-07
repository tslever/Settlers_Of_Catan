import sqlite3

BASE_NAME_OF_DATABASE = "game.db"
ID_OF_STATE = 1

def get_connection_to_database():
    connection = sqlite3.connect(BASE_NAME_OF_DATABASE)
    connection.row_factory = sqlite3.Row
    return connection

def initialize_database():
    connection = get_connection_to_database()
    connection.execute(
        '''
        CREATE TABLE IF NOT EXISTS settlements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player INTEGER NOT NULL,
            vertex TEXT NOT NULL
        )
        '''
    )

    connection.execute(
        '''
        CREATE TABLE IF NOT EXISTS roads (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player INTEGER NOT NULL,
            edge TEXT NOT NULL
        )
        '''
    )

    connection.execute(
        '''
        CREATE TABLE IF NOT EXISTS state (
            id INTEGER PRIMARY KEY,
            current_player INTEGER NOT NULL,
            phase TEXT NOT NULL,
            last_settlement TEXT
        )
        '''
    )
    cur = connection.execute("SELECT COUNT(*) as count FROM state")
    row = cur.fetchone()
    if row["count"] == 0:
        connection.execute(
            "INSERT INTO state (id, current_player, phase, last_settlement) VALUES (?, 1, 'phase to place first settlement', NULL)",
            (ID_OF_STATE,)
        )
    
    connection.commit()
    connection.close()