#include "MBCLI.h"
#include <iostream>
#include <MBUtility/MBCompileDefinitions.h>

#if  defined(WIN32) ||  defined(_WIN32) || defined(_WIN32_)
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif // WIN32 || _WIN32

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
	MBTerminal::MBTerminal()
	{
#ifdef WIN32 || _WIN32
		HANDLE hStdin = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode = 0;
		if (!GetConsoleMode(hStdin, &mode))
		{
			std::cout << GetLastError() << std::endl;
		}
		if (!SetConsoleMode(hStdin, mode | (ENABLE_VIRTUAL_TERMINAL_PROCESSING)))
		{
			std::cout << GetLastError() << std::endl;
		}
		
		//Debug grejer
		//GetConsoleMode(hStdin, &mode);
		//std::cout << bool(mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) << std::endl;
#endif // 

	}
	void MBTerminal::Print(std::string const& StringToPrint)
	{
#ifdef WIN32 || _WIN32
		std::cout << StringToPrint;
		//HANDLE hStdin = GetStdHandle(STD_OUTPUT_HANDLE);
		//if (hStdin == INVALID_HANDLE_VALUE)
		//{
		//	std::cout << "Error getting console handle: " << GetLastError() << std::endl;
		//}
		//if (!WriteConsole(hStdin, StringToPrint.data(), StringToPrint.size(), NULL, NULL))
		//{
		//	std::cout << "Error writing to console: " << GetLastError() << std::endl;
		//}
#else
		std::cout << StringToPrint;
#endif // WIN32 || _WIN32
		//printf(StringToPrint.data());
	}
	void MBTerminal::PrintLine(std::string const& DataToPrint)
	{
		Print(DataToPrint + "\n");
		//printf(DataToPrint.data());
		//printf("\n");

	}
	void MBTerminal::GetLine(std::string& OutLine)
	{
		std::cin >> OutLine;
	}
	void MBTerminal::SetPasswordInput(bool IsPassword)
	{
		m_IsPasswordInput = IsPassword;
#ifdef WIN32
		if (IsPassword)
		{
			HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
			DWORD mode = 0;
			GetConsoleMode(hStdin, &mode);
			SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
		}
		else
		{
			HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
			DWORD mode = 0;
			GetConsoleMode(hStdin, &mode);
			SetConsoleMode(hStdin, mode | (ENABLE_ECHO_INPUT));
		}
#else
		if (IsPassword)
		{
			struct termios term;
			tcgetattr(fileno(stdin), &term);

			term.c_lflag &= ~ECHO;
			tcsetattr(fileno(stdin), 0, &term);
		}
		else
		{
			struct termios term;
			tcgetattr(fileno(stdin), &term);

			term.c_lflag |= ECHO;
			tcsetattr(fileno(stdin), 0, &term);
		}
#endif

	}
	unsigned char MBTerminal::p_GetColorNumber(ANSITerminalColor ColorToSet)
	{
		unsigned char ReturnValue = 0;
		if (ColorToSet == ANSITerminalColor::Black)
		{
			ReturnValue = 30;
		}
		else if (ColorToSet == ANSITerminalColor::Red)
		{
			ReturnValue = 31;
		}
		else if (ColorToSet == ANSITerminalColor::Green)
		{
			ReturnValue = 32;
		}
		else if (ColorToSet == ANSITerminalColor::Yellow)
		{
			ReturnValue = 33;
		}
		else if (ColorToSet == ANSITerminalColor::Blue)
		{
			ReturnValue = 34;
		}
		else if (ColorToSet == ANSITerminalColor::Magenta)
		{
			ReturnValue = 35;
		}
		else if (ColorToSet == ANSITerminalColor::Cyan)
		{
			ReturnValue = 36;
		}
		else if (ColorToSet == ANSITerminalColor::White)
		{
			ReturnValue = 37;
		}
		else if (ColorToSet == ANSITerminalColor::Gray)
		{
			ReturnValue = 90;
		}
		else if (ColorToSet == ANSITerminalColor::BrightRed)
		{
			ReturnValue = 91;
		}
		else if (ColorToSet == ANSITerminalColor::BrightGreen)
		{
			ReturnValue = 92;
		}
		else if (ColorToSet == ANSITerminalColor::BrightYellow)
		{
			ReturnValue = 93;
		}
		else if (ColorToSet == ANSITerminalColor::BrightBlue)
		{
			ReturnValue = 94;
		}
		else if (ColorToSet == ANSITerminalColor::BrightMagenta)
		{
			ReturnValue = 95;
		}
		else if (ColorToSet == ANSITerminalColor::BrightCyan)
		{
			ReturnValue = 96;
		}
		else if (ColorToSet == ANSITerminalColor::BrightWhite)
		{
			ReturnValue = 97;
		}
		return(ReturnValue);
	}
	void MBTerminal::SetTextColor(ANSITerminalColor ColorToSet)
	{
		std::string StringToPrint = "\x1B["+std::to_string(p_GetColorNumber(ColorToSet))+"m";
		std::cout << StringToPrint;
		//std::cout.flush();
	}

}