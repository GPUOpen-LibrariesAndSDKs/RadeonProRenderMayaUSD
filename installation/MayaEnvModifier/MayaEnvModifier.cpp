// MayaEnvModifier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include <string>
#include <fstream>


// 0 - executable path
// 1 - option - "add" or "remove"
// 2 - path to installation location
// 3 - optional - maya version string

int main(int argc, char* argv[] )
{
	if (argc < 3)
	{
		return -1;
	}

	const std::string addOption = "-add";
	const std::string removeOption = "-remove";

	if (argv[1] != addOption && argv[1] != removeOption)
	{
		return -1;
	}

	bool addEnv = argv[1] == addOption;

	std::string mayaVersion;

	if (argc >= 4)
	{
		mayaVersion = argv[3];
	}
	else
	{
		mayaVersion = "2023"; // by default
	}

	const std::string installationPath = argv[2];

	const std::string pxrPluginPathNameString = "PXR_PLUGINPATH_NAME=%PXR_PLUGINPATH_NAME%;" + installationPath + "\\plugin;";
	const std::string pathString = "PATH=%PATH%;" + installationPath + "\\lib;";

	const std::string mayaEnvFilePath = std::string(std::getenv("USERPROFILE")) + "\\Documents\\Maya\\" + mayaVersion + "\\Maya.env";

	std::cout << "Maya.env filepath to open: " << mayaEnvFilePath.c_str() << std::endl;

	std::ifstream mayaEnvStreamIn (mayaEnvFilePath);

	if (!mayaEnvStreamIn.is_open())
	{
		std::cout << "Maya.env could not be opened" << std::endl;
		return -2;
	}

	std::string output;

	for (std::string currentString; std::getline(mayaEnvStreamIn, currentString); )
	{
		if (currentString == pxrPluginPathNameString || currentString == pathString)
		{
			continue;
		}

		output += currentString + "\n";
	}

	if (addEnv)
	{
		output += pxrPluginPathNameString + "\n";
		output += pathString + "\n";
	}

	std::ofstream mayaEnvStreamOut (mayaEnvFilePath);
	mayaEnvStreamOut << output;
	mayaEnvStreamOut.close();

	std::cout << "Maya.env is patched!" << std::endl;
	return 0;
}
