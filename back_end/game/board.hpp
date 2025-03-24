#pragma once


// Fuction `getOccupiedVertices` gets a list of occupied vertex labels by querying the database.
std::vector<std::string> getOccupiedVertices(Database& db) {
	std::vector<std::string> listOfLabelsOfOccupiedVertices;
	auto settlements = db.getSettlements();
	for (const auto& s : settlements) {
		listOfLabelsOfOccupiedVertices.push_back(s.vertex);
	}
	auto cities = db.getCities();
	for (const auto& c : cities) {
		listOfLabelsOfOccupiedVertices.push_back(c.vertex);
	}
	return listOfLabelsOfOccupiedVertices;
}


/* Function `getAvailableVertices` gets the available vertices given a list of already occupied vertices.
* `getAvailableVertices` reads the board geometry and filters out any vertex that is either occupied
* or is too close (i.e. adjacent) to any occupied vertex.
*/
std::vector<std::string> getAvailableVertices(const std::vector<std::string>& addressOfListOfLabelsOfOccupiedVertices) {
	std::ifstream file("../board_geometry.json");
	if (!file.is_open()) {
		throw std::runtime_error("Board geometry file could not be opened.");
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	auto boardGeometry = crow::json::load(buffer.str());
	if (!boardGeometry) {
		throw std::runtime_error("Board geometry file could not be parsed.");
	}
	auto verticesJson = boardGeometry["vertices"];
	if (!verticesJson || verticesJson.size() == 0) {
		throw std::runtime_error("Board geometry file does not contain vertices.");
	}

	// Build a map of vertex label to its (x,y) coordinates.
	std::unordered_map<std::string, std::pair<double, double>> vertexCoords;
	for (size_t i = 0; i < verticesJson.size(); i++) {
		auto v = verticesJson[i];
		std::string label = v["label"].s();
		double x = v["x"].d();
		double y = v["y"].d();
		vertexCoords[label] = { x, y };
	}

	// Set up board constants.
	double width_of_board_in_vmin = 100.0;
	double number_of_hexes_that_span_board = 6.0;
	double width_of_hex = width_of_board_in_vmin / number_of_hexes_that_span_board;
	double length_of_side_of_hex = width_of_hex * std::tan(M_PI / 6);
	double margin_of_error = 0.01;

	std::vector<std::string> available;
	// For each vertex in the board geometry:
	for (const auto& pair : vertexCoords) {
		const std::string& label = pair.first;
		double x1 = pair.second.first;
		double y1 = pair.second.second;

		// Skip if the vertex is already occupied.
		if (
			std::find(
				addressOfListOfLabelsOfOccupiedVertices.begin(),
				addressOfListOfLabelsOfOccupiedVertices.end(),
				label
			) != addressOfListOfLabelsOfOccupiedVertices.end()
			)
			continue;

		bool tooClose = false;
		// Check the distance to every occupied vertex.
		for (const auto& occLabel : addressOfListOfLabelsOfOccupiedVertices) {
			auto it = vertexCoords.find(occLabel);
			if (it != vertexCoords.end()) {
				double x2 = it->second.first;
				double y2 = it->second.second;
				double distance = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
				// If the vertex is within or exactly at the forbidden distance, mark as too close.
				if (distance <= length_of_side_of_hex + margin_of_error) {
					tooClose = true;
					break;
				}
			}
		}
		if (!tooClose) {
			available.push_back(label);
		}
	}
	return available;
}