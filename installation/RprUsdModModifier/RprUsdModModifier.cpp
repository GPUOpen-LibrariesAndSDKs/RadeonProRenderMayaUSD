// RprUsdModModifier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <iostream>

#include <string>
#include <fstream>


// argc[0] - executable file itself
// argc[1] - mod file path to modify
// argc[2] - path to rprUsd plugin to be put into rprUsd.mod file

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		return -1;
	}
	std::string modFilePath = argv[1];

	std::ifstream rprUsdModFileStream(modFilePath);

	if (!rprUsdModFileStream.is_open())
	{
		std::cout << "RprUsd.mod could not be opened" << std::endl;
		return -1;
	}

	std::string output;
	std::string strToSubstitute = "<PATH_TO_SUBSTITUTE>";

	for (std::string currentString; std::getline(rprUsdModFileStream, currentString); )
	{
		size_t pos = currentString.find(strToSubstitute);
		if ( pos != std::string::npos)
		{
			currentString.replace(pos, strToSubstitute.length(), argv[2]);
		}

		output += currentString + "\n";
	}

	rprUsdModFileStream.close();

	std::ofstream rprUsdModFileOutpoutStream(modFilePath);
	rprUsdModFileOutpoutStream << output;
	rprUsdModFileOutpoutStream.close();

	return 0;
}
