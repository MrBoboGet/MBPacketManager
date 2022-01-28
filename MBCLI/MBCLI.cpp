#include "MBCLI.h"
#include <iostream>
#include <MBUtility/MBCompileDefinitions.h>
#include <cmath>

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
	std::vector<std::pair<std::string,int>> ProcessedCLInput::GetSingleArgumentOptionList(std::string const& OptionName) const
	{
		//Optionent är på formen -{OptionName}:{OptionArgumentUtanMellanslag}
		std::vector<std::pair<std::string, int>> ReturnValue = {};
		for (size_t i = 1; i < TotalCommandTokens.size(); i++)
		{
			if (TotalCommandTokens[i].substr(0, 1 + OptionName.size() + 1) == "-" + OptionName + ":")
			{
				ReturnValue.push_back(std::pair<std::string, int>(TotalCommandTokens[i].substr(1 + OptionName.size() + 1),i));
			}
		}
		return(ReturnValue);
	}
	//END ProcessedCLInput

	//BEGIN MBTerminalLoadingbar
	std::string MBTerminalLoadingbar::p_GetProgressString()
	{
		std::string ReturnValue = "";
		uint8_t Progress = uint8_t(m_CurrentProgress*100);
		ReturnValue += "[";
		const uint8_t LoadingBarWidth = 20;
		uint8_t CurrentProgressPositions = uint8_t((double(Progress) / double(100)) * LoadingBarWidth);
		uint8_t RemainingPositons = LoadingBarWidth - CurrentProgressPositions;
		for (size_t i = 0; i < CurrentProgressPositions; i++)
		{
			ReturnValue += "-";
		}
		if (CurrentProgressPositions != LoadingBarWidth && CurrentProgressPositions > 0)
		{
			ReturnValue[ReturnValue.size() - 1] = '>';
		}
		for (size_t i = 0; i < RemainingPositons; i++)
		{
			ReturnValue += " ";
		}
		ReturnValue += "] ";
		if (!m_IsFinished)
		{
			ReturnValue += std::to_string(Progress) + "%";
		}
		else
		{
			ReturnValue += "finished";
		}
		return(ReturnValue);
	}
	std::string MBTerminalLoadingbar::p_GetCurrentLine()
	{
		std::string ReturnValue = m_CurrentTag + ": "+p_GetProgressString();
		return(ReturnValue);
	}
	void MBTerminalLoadingbar::p_UpdateLine(std::string const& LineData)
	{
		m_AssociatedTerminal->p_OverwriteLine(m_LinePosition, LineData);
	}
	MBTerminalLoadingbar::MBTerminalLoadingbar(std::string const& LoadingBarTag, float InitialProgress)
	{
		m_CurrentTag = LoadingBarTag;
		m_CurrentProgress = InitialProgress;
	}
	void MBTerminalLoadingbar::UpdateTag(std::string const& NewTag)
	{
		m_CurrentTag = NewTag;
		p_UpdateLine(p_GetCurrentLine());
	}
	void MBTerminalLoadingbar::UpdateProgress(float NewProgress)
	{
		m_CurrentProgress = NewProgress;
		if (m_CurrentProgress < 0)
		{
			m_CurrentProgress = 0;
		}
		if (m_CurrentProgress > 1)
		{
			m_CurrentProgress = 1;
		}
		p_UpdateLine(p_GetCurrentLine());
	}
	void MBTerminalLoadingbar::Finalize()
	{
		m_CurrentProgress = 1;
		m_IsFinished = true;
		p_UpdateLine(p_GetCurrentLine());
	}
	//END MBTerminalLoadingbar
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
		size_t NextNewlinePosition = StringToPrint.find('\n');
		while (NextNewlinePosition != StringToPrint.npos)
		{
			m_CurrentLinePosition += 1;
			NextNewlinePosition = StringToPrint.find('\n', NextNewlinePosition+1);
		}
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
	void MBTerminal::p_OverwriteLine(size_t LinePosition, std::string const& DataToWrite)
	{
		size_t LineDiff = m_CurrentLinePosition - LinePosition;//ANTAGANDE, är alltid över nuvarande linen
		std::cout << "\x1B[" + std::to_string(LineDiff) + "F";
		std::cout << DataToWrite;
		std::cout << "\x1B[0K";
		std::cout << "\x1B[" + std::to_string(LineDiff) + "E";
		std::cout.flush();
	}
	MBTerminalLoadingbar MBTerminal::CreateLineLoadingBar(std::string const& LoadingBarTag, float InitialProgress)
	{
		MBTerminalLoadingbar ReturnValue = MBTerminalLoadingbar(LoadingBarTag, InitialProgress);
		ReturnValue.m_LinePosition = m_CurrentLinePosition;
		ReturnValue.m_AssociatedTerminal = this;
		this->PrintLine("");
		return(ReturnValue);
	}
	std::string MBTerminal::GetByteSizeString(uint64_t NumberOfBytes)
	{
		std::string ReturnValue = "";
		uint8_t FileSizeLog = std::log10(NumberOfBytes);
		if (FileSizeLog < 3)
		{
			ReturnValue = std::to_string(NumberOfBytes) + " B";
		}
		else if (FileSizeLog >= 3 && FileSizeLog < 6)
		{
			ReturnValue = std::to_string(NumberOfBytes / 1000) + " KB";
		}
		else if (FileSizeLog >= 6 && FileSizeLog < 9)
		{
			ReturnValue = std::to_string(NumberOfBytes / 1000000) + " MB";
		}
		else
		{
			ReturnValue = std::to_string(NumberOfBytes / 1000000000) + " GB";
		}
		return(ReturnValue);
	}
}