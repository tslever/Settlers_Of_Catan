#!/usr/bin/env python3
import argparse
from Python_back_end.settings import settings
import pymysql


ADMIN_HOST = settings.mysql_host
ADMIN_USER = settings.mysql_username
ADMIN_PASSWORD = settings.mysql_password

NEW_USER_HOST = settings.db_host
NEW_USER = settings.db_username
NEW_USER_PASSWORD = settings.db_password

DATABASE_NAME = settings.db_name


sql_to_set_up_database = f'''
CREATE USER IF NOT EXISTS '{NEW_USER}'@'{NEW_USER_HOST}' IDENTIFIED BY '{NEW_USER_PASSWORD}';
CREATE DATABASE IF NOT EXISTS {DATABASE_NAME};
GRANT ALL PRIVILEGES ON {DATABASE_NAME}.* TO '{NEW_USER}'@'{NEW_USER_HOST}';
FLUSH PRIVILEGES;
'''


sql_to_tear_down_database = f'''
DROP DATABASE IF EXISTS {DATABASE_NAME};
DROP USER '{NEW_USER}'@'{NEW_USER_HOST}';
'''


def main(mode, sql):

    connection = pymysql.connect(
        host = ADMIN_HOST,
        user = ADMIN_USER,
        password = ADMIN_PASSWORD,
        charset = "utf8mb4",
        cursorclass = pymysql.cursors.DictCursor
    )

    try:
        with connection.cursor() as cursor:
            for command in sql.strip().split(';'):
                command = command.strip()
                if command:
                    print(f"The following command is being executed. {command}")
                    cursor.execute(command)
        connection.commit()
        print(f"{mode} of database completed successfully.")
    except Exception as e:
        print(f"The following error occurred. {e}")
    finally:
        connection.close()


if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description = "Set up or tear down the database.")
    group = parser.add_mutually_exclusive_group(required = True)
    group.add_argument("--set_up", action = "store_true", help = "Set up the database.")
    group.add_argument("--tear_down", action = "store_true", help = "Tear down the database.")
    args = parser.parse_args()

    if args.set_up:
        mode = "Set up"
        sql = sql_to_set_up_database
    elif args.tear_down:
        mode = "Tear down"
        sql = sql_to_tear_down_database
    
    main(mode, sql)