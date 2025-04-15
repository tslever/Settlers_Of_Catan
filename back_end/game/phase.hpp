#pragma once
#include <string_view>


namespace Game {

	namespace Phase {
		constexpr std::string_view TO_PLACE_FIRST_SETTLEMENT = "phase to place first settlement";
		constexpr std::string_view TO_PLACE_FIRST_ROAD = "phase to place first road";
		constexpr std::string_view TO_PLACE_FIRST_CITY = "phase to place first city";
		constexpr std::string_view TO_PLACE_SECOND_ROAD = "phase to place second road";
		constexpr std::string_view TURN = "turn";
		constexpr std::string_view DONE = "done";
	}

}