// RprUsdModModifier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <iostream>

#include <string>
#include <fstream>
#include <map>


// argc[0] - executable file itself
// argc[1] - mod file path to modify
// argc[2] - path to rprUsd plugin to be put into rprUsd.mod file
// argc[3] - maya version e.g. "2023"

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		return -1;
	}

	std::string modFilePath = argv[1];
	std::string pathToReplace = argv[2];
	std::string mayaVersion = argv[3];

	std::ifstream rprUsdModFileStream(modFilePath);

	if (!rprUsdModFileStream.is_open())
	{
		std::cout << "RprUsd.mod could not be opened" << std::endl;
		return -1;
	}

	std::string output;

	std::map<std::string, std::string> replacementMap;

	replacementMap["<PATH_TO_REPLACE>"] = pathToReplace;
	replacementMap["<MAYA_VERSION>"] = mayaVersion;

	for (std::string currentString; std::getline(rprUsdModFileStream, currentString); )
	{
		for (auto it = replacementMap.begin(); it != replacementMap.end(); ++it)
		{
			size_t pos = currentString.find(it->first);
			if (pos != std::string::npos)
			{
				currentString.replace(pos, it->first.length(), it->second);
			}
		}

		output += currentString + "\n";
	}

	rprUsdModFileStream.close();

	std::ofstream rprUsdModFileOutpoutStream(modFilePath);
	rprUsdModFileOutpoutStream << output;
	rprUsdModFileOutpoutStream.close();

	return 0;
}
