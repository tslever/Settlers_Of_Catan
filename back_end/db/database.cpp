#include "database.hpp"


void Database::initialize() {
    try {
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());

        // Create cities table.
        stmt->execute(
            "CREATE TABLE IF NOT EXISTS cities ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "player INT NOT NULL, "
            "vertex VARCHAR(50) NOT NULL)"
        );

        // Create settlements table.
        stmt->execute(
            "CREATE TABLE IF NOT EXISTS settlements ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "player INT NOT NULL, "
            "vertex VARCHAR(50) NOT NULL)"
        );

        // Create roads table.
        stmt->execute(
            "CREATE TABLE IF NOT EXISTS roads ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "player INT NOT NULL, "
            "edge VARCHAR(50) NOT NULL)"
        );

        // Create state table.
        stmt->execute(
            "CREATE TABLE IF NOT EXISTS state ("
            "id INT PRIMARY KEY, "
            "current_player INT NOT NULL, "
            "phase VARCHAR(100) NOT NULL, "
            "last_building VARCHAR(50))"
        );
    }
    catch (sql::SQLException& e) {
        std::cerr << "The following error occurred while the database was being initialized." << e.what() << std::endl;
        throw; // Rethrow to allow caller to handle the error.
    }
}