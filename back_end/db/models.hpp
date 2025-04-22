#pragma once

#include <string>


struct City {
    int id;
    int player;
    std::string vertex;
};


struct Settlement {
    int id;
    int player;
    std::string vertex;
};


struct Road {
    int id;
    int player;
    std::string edge;
};


struct Wall {
	int id;
	int player;
	std::string vertex;
};