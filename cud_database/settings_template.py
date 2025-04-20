from pydantic_settings import BaseSettings


class Settings(BaseSettings):

    db_host: str = "localhost or another host"
    db_name: str = "name of database"
    db_password: str = "database password"
    db_username: str = "database username"

    mysql_host: str = "localhost or another host"
    mysql_password: str = "MySQL password"
    mysql_username: str = "MySQL username"


settings = Settings()