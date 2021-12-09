#include <string>
#include <fstream>

#include <map>
#include <vector>


std::vector<std::string> SplitStringBySpace(const std::string& input)
{
	size_t curPos = 0;
	char space = ' ';
	std::vector<std::string> elements;
	std::string curElement;

	do
	{
		size_t pos = input.find(space, curPos);
		if (pos != std::string::npos)
		{
			curElement = input.substr(curPos, pos - curPos);
			curPos = pos + 1;
		}
		else
		{
			curElement = input.substr(curPos, input.length() - curPos);
			curPos = pos;
		}

		if (curElement != std::string(1, space))
		{
			elements.push_back(curElement);
		}
	} while (curPos != std::string::npos);

	return elements;
}

int main(int argc, char** argv)
{
	// 0 - exec name
	// 1 - filepath to original RPRMayaUSD.mod file
	// 2 - app folder - installation folder

	if (argc < 3)
	{
		return -1;
	}

	std::string appInstaallDir = argv[2];

	std::ofstream modFileStreamOut2;

	std::ifstream modFileStream(argv[1]);

	std::map<std::string, std::string> lookupMap = {
		{"USD", "/USD/USDBuild"},
		{"MayaUSD_LIB", "/maya-usd/build/install/RelWithDebInfo"},
		{"MayaUSD", "/maya-usd/build/install/RelWithDebInfo/plugin/adsk"},
		{"MTOH", "/maya-usd/build/install/RelWithDebInfo/lib"} };

	std::string output;

	std::vector<std::string> elements;
	for (std::string currentString; std::getline(modFileStream, currentString); )
	{
		//if (currentString.substr(0, 2) == "+ ")
		elements = SplitStringBySpace(currentString);

		if (elements[0] == "+" && elements.size() > 3)
		{

			std::map<std::string, std::string>::const_iterator it;

			for (it = lookupMap.begin(); it != lookupMap.end(); ++it)
			{
				if (elements[1] == it->first)
				{
					break;
				}
			}

			if (it != lookupMap.end())
			{
				std::string path;

				// in elements array [0] == "+", [1] - name, [2] - version, [3+] - path
				// combine the path in case if it consists of space inside
				for (int i = 3; i < elements.size(); ++i)
				{
					path += elements[i];
				}

				size_t nonModifPartPos = path.find(it->second);

				path.replace(0, nonModifPartPos, appInstaallDir);

				// build currentString again
				currentString = "";
				for (int i = 0; i < 3; ++i)
				{
					currentString += elements[i] + " ";
				}

				currentString += path;
			}
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