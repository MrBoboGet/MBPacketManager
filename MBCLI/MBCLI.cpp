#include "MBCLI.h"
#include <iostream>
namespace MBCLI
{
	//BEGIN ProcessedCLInput
	ProcessedCLInput::ProcessedCLInput(int argc, const char* const* argv)
	{
		TotalCommandTokens.push_back(argv[0]);
		for (size_t i = 1; i < argc; i++)
		{
			TotalCommandTokens.push_back(argv[i]);
			std::string ArgumentString = std::string(argv[i]);
			if (ArgumentString.substr(0, 2) == "--")
			{
				CommandOptions[ArgumentString.substr(2)] = i;
			}
			else if (ArgumentString[0] == '-')
			{
				std::string OptionString = ArgumentString.substr(1);
				if (CommandPositionalOptions.find(OptionString) != CommandPositionalOptions.end())
				{
					CommandPositionalOptions[OptionString].push_back(i);
				}
				else
				{
					CommandPositionalOptions[OptionString] = { i };
				}
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