CREATE DATABASE game;
CREATE USER 'administrator'@'localhost'
IDENTIFIED BY 'settlers_of_catan';
GRANT ALL PRIVILEGES ON game.*
TO 'administrator'@'localhost';
FLUSH PRIVILEGES;