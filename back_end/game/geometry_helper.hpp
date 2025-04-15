#pragma once


#include <cmath>
#include <corecrt_math_defines.h>
#include <string>


namespace GeometryHelper {

	constexpr double MARGIN_OF_ERROR = 0.01;
	constexpr double NUMBER_OF_HEXES_THAT_SPAN_BOARD = 6.0;
	constexpr double WIDTH_OF_BOARD_IN_VMIN = 100.0;
	constexpr double WIDTH_OF_HEX = WIDTH_OF_BOARD_IN_VMIN / NUMBER_OF_HEXES_THAT_SPAN_BOARD;

	bool areNearlyEqual(double x1, double x2, double epsilon) {
		return std::fabs(x2 - x1) < epsilon;
	}

	inline double distance(double x1, double y1, double x2, double y2) {
		double dx = x2 - x1;
		double dy = y2 - y1;
		return std::sqrt(dx * dx + dy * dy);
	}

	inline double getLengthOfSideOfHex() {
		return WIDTH_OF_HEX * std::tan(M_PI / 6);
	}

	inline std::string getEdgeKey(double x1, double y1, double x2, double y2) {
		// TODO: Consider replacing 50 with the maximum length of an edge key or using `std::ostringstream`.
		char buffer[50];
		if ((x1 < x2) || (areNearlyEqual(x1, x2, MARGIN_OF_ERROR) && y1 <= y2)) {
			std::snprintf(buffer, sizeof(buffer), "%.2f-%.2f_%.2f-%.2f", x1, y1, x2, y2);
		}
		else {
			std::snprintf(buffer, sizeof(buffer), "%.2f-%.2f_%.2f-%.2f", x2, y2, x1, y1);
		}
		std::string edgeKey = std::string(buffer);
		return edgeKey;
	}
}