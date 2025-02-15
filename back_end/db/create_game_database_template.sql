CREATE DATABASE name_of_database;
CREATE USER 'username'@'localhost or something else'
IDENTIFIED BY 'password';
GRANT ALL PRIVILEGES ON game.*
TO 'username'@'localhost or something else';
FLUSH PRIVILEGES;