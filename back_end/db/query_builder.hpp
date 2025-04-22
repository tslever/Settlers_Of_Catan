#pragma once


#include "crow/json.h"
#include "models.hpp"


class QueryJsonBuilder {
public:
	
	template<typename T>
	static crow::json::wvalue buildJsonArray(
		const std::vector<T>& vectorOfItems,
		const std::function<crow::json::wvalue(const T&)>& convertItemToJsonObject
	) {
		crow::json::wvalue arr(crow::json::type::List);
		size_t i = 0;
		for (const auto& item : vectorOfItems) {
			arr[i++] = convertItemToJsonObject(item);
		}
		return arr;
	}

	static crow::json::wvalue convertVectorOfCitiesToJsonObject(const std::vector<City>& cities) {
		return buildJsonArray<City>(cities, [](const City& city) -> crow::json::wvalue {
			crow::json::wvalue jsonObject;
			jsonObject["id"] = city.id;
			jsonObject["player"] = city.player;
			jsonObject["vertex"] = city.vertex;
			return jsonObject;
		});
	}

	static crow::json::wvalue convertVectorOfWallsToJsonObject(const std::vector<Wall>& walls) {
		return buildJsonArray<Wall>(walls, [](const Wall& wall) -> crow::json::wvalue {
			crow::json::wvalue jsonObject;
			jsonObject["id"] = wall.id;
			jsonObject["player"] = wall.player;
			jsonObject["vertex"] = wall.vertex;
			return jsonObject;
		});
	}

	static crow::json::wvalue convertVectorOfSettlementsToJsonObject(const std::vector<Settlement>& settlements) {
		return buildJsonArray<Settlement>(settlements, [](const Settlement& settlement) -> crow::json::wvalue {
			crow::json::wvalue jsonObject;
			jsonObject["id"] = settlement.id;
			jsonObject["player"] = settlement.player;
			jsonObject["vertex"] = settlement.vertex;
			return jsonObject;
		});
	}

	static crow::json::wvalue convertVectorOfRoadsToJsonObject(const std::vector<Road>& roads) {
		return buildJsonArray<Road>(roads, [](const Road& road) -> crow::json::wvalue {
			crow::json::wvalue jsonObject;
			jsonObject["id"] = road.id;
			jsonObject["player"] = road.player;
			jsonObject["edge"] = road.edge;
			return jsonObject;
		});
	}
};