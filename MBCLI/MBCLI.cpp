#include "MBCLI.h"
#include <iostream>
namespace MBCLI
{
	//BEGIN ProcessedCLInput
	ProcessedCLInput::ProcessedCLInput(int argc, const char* const* argv)
	{
		for (size_t i = 1; i < argc; i++)
		{
			TotalCommandTokens.push_back(argv[i]);
			std::string ArgumentString = std::string(argv[i]);
			if (ArgumentString.substr(0, 2) == "--")
			{
				//CommandTopDirectives.push_back(ArgumentString);
			}
			else if (ArgumentString[0] == '-')
			{
				CommandOptions[ArgumentString] = i;
			}
			else
			{
				if (TopCommand == "")
				{
					TopCommand = argv[i];
				}
				else
				{
					TopCommandArguments.push_back(argv[i]);
				}
			}
		}
	}
	//END ProcessedCLInput

	//BEGIN MBTerminal
	void MBTerminal::Print(std::string const& StringToPrint)
	{
		std::cout << StringToPrint;
	}
	void MBTerminal::PrintLine(std::string const& DataToPrint)
	{
		std::cout << DataToPrint << std::endl;
	}
	void MBTerminal::GetLine(std::string& OutLine)
	{
		std::cin >> OutLine;
	}
}