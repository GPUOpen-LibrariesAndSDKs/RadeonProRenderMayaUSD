// MayaEnvModifier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <shlobj.h>

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

	const std::string cachePath = std::string(std::getenv("LOCALAPPDATA")) + "\\RadeonProRender\\Maya\\USD\\";
	const std::string cachePathString = "HDRPR_CACHE_PATH_OVERRIDE=" + cachePath;

	// We support only ascii path for now (not Unicode)
	std::string mayaEnvFilePath;


	char* mayaAppDirEnvVarValue = std::getenv("MAYA_APP_DIR");
	std::string mayaAppDirEnvVarValueString;
	if (mayaAppDirEnvVarValue != nullptr)
	{
		mayaAppDirEnvVarValueString = mayaAppDirEnvVarValue;
	}

	if (mayaAppDirEnvVarValueString.empty())
	{
		PWSTR ppszPath;    // variable to receive the path memory block pointer.

		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &ppszPath);

		std::wstring myPath;
		if (SUCCEEDED(hr))
		{
			myPath = ppszPath;      // make a local copy of the path
		}
		else
		{
			return -1;
		}

		CoTaskMemFree(ppszPath);    // free up the path memory block

		for (wchar_t x : myPath)
			mayaEnvFilePath += (char)x;

		mayaEnvFilePath += "\\Maya\\" + mayaVersion + "\\Maya.env";
	}
	else
	{
		mayaEnvFilePath = mayaAppDirEnvVarValueString + "\\" + mayaVersion + "\\Maya.env";
	}

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
		if (currentString == pxrPluginPathNameString || currentString == pathString || currentString == cachePathString)
		{
			continue;
		}

		output += currentString + "\n";
	}

	if (addEnv)
	{
		output += pxrPluginPathNameString + "\n";
		output += pathString + "\n";
		output += cachePathString + "\n";
	}

	std::ofstream mayaEnvStreamOut (mayaEnvFilePath);
	mayaEnvStreamOut << output;
	mayaEnvStreamOut.close();

	std::cout << "Maya.env is patched!" << std::endl;
	return 0;
}
