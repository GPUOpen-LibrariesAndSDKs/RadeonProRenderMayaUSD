#include <string>
#include <fstream>

#include <map>

int main(int argc, char** argv)
{
	// 0 - exec name
	// 1 - filepath to original RPRMayaUSD.mod file
	// 2 - app folder - installation folder

	std::ofstream tempTest("C:\\projects\\test\\a.txt");

	tempTest << argc << std::endl;
	for (int i = 0; i < argc; ++i)
	{
		tempTest << argv[i] << std::endl;
	}

	tempTest.close();

	if (argc < 3)
	{
		return -1;
	}

	std::string appInstaallDir = argv[2];

	std::ofstream modFileStreamOut2;

	std::ifstream modFileStream(argv[1]);

	std::map<std::string, std::string> lookupMap = {
		{"+ USD", "/USD/USDBuild"},
		{"+ MayaUSD_LIB", "/maya-usd/build/install/RelWithDebInfo"},
		{"+ MayaUSD", "/maya-usd/build/install/RelWithDebInfo/plugin/adsk"},
		{"+ MTOH", "/maya-usd/build/install/RelWithDebInfo/lib"} };

	std::string output;

	for (std::string currentString; std::getline(modFileStream, currentString); )
	{
		std::map<std::string, std::string>::const_iterator it;

		for (it = lookupMap.begin(); it != lookupMap.end(); ++it)
		{
			if (currentString.find(it->first) != std::string::npos)
			{
				break;
			}
		}

		if (it != lookupMap.end())
		{
			size_t lastSpacePos = currentString.find_last_of(" ");
			size_t nonModifPartPos = currentString.find(it->second);

			currentString.replace(lastSpacePos + 1, nonModifPartPos - (lastSpacePos + 1), appInstaallDir);
		}

		output += currentString + "\n";
	}
			
	std::ofstream modFileStreamOut(argv[1]);

	if (!modFileStreamOut.is_open())
	{
		return -2;
	}

	modFileStreamOut << output;
	modFileStreamOut.close();

	return 0;
}