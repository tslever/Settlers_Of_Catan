#!/usr/bin/env python3
import argparse
from back_end.settings import settings
import pymysql


ADMIN_HOST = settings.mysql_host
ADMIN_USER = settings.mysql_username
CURRENT_ADMIN_PASSWORD = settings.mysql_password


def change_admin_password(new_password):

    sql_to_change_password = f'''
    ALTER USER '{ADMIN_USER}'@'{ADMIN_HOST}' IDENTIFIED BY '{new_password}';
    FLUSH PRIVILEGES;
    '''
    
    connection = pymysql.connect(
        host = ADMIN_HOST,
        user = ADMIN_USER,
        password = CURRENT_ADMIN_PASSWORD,
        charset = "utf8mb4",
        cursorclass = pymysql.cursors.DictCursor
    )
    
    try:
        with connection.cursor() as cursor:
            for command in sql_to_change_password.strip().split(';'):
                command = command.strip()
                if command:
                    print(f"The following command is being executed. {command}")
                    cursor.execute(command)
        connection.commit()
        print("Password change completed successfully.")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        connection.close()

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description = "Change the password for ADMIN_USER.")
    parser.add_argument("--new_password", required = True, help = "New password for ADMIN_USER.")
    args = parser.parse_args()

    change_admin_password(args.new_password)