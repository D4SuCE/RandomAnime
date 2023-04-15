#include "parser.h"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <urlsCount>" << std::endl;
		return 1;
	}

	int urlsCount;

	try
	{
		urlsCount = std::stoi(argv[1]);
	}
	catch (...)
	{
		std::cerr << "Invalid number: " << argv[1] << '\n';
		return 1;
	}

	Parser parser(urlsCount);
	parser.parse();	
	parser.exportJSON();

	return 0;
}