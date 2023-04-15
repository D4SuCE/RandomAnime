#pragma once

#include <string>
#include <vector>

struct Anime
{
	std::string name;
	std::string rating;
	std::string type;
	std::string episodes;
	std::vector<std::string> genres;
	std::string link;
};