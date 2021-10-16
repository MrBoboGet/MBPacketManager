#include <vector>
#include <string>
#include <unordered_map>
namespace MBCLI
{
	enum class ANSITerminalColor
	{
		Black,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan,
		White,
		Gray,
		BrightRed,
		BrightGreen,
		BrightYellow,
		BrightBlue,
		BrightMagenta,
		BrightCyan,
		BrightWhite,
	};
	struct ProcessedCLInput
	{
		std::string TopCommand = "";
		std::vector<std::string> TotalCommandTokens = {};
		std::unordered_map<std::string, int> CommandOptions = {};
		std::vector<std::string> TopCommandArguments = {};
		std::unordered_map<std::string, std::vector<size_t>> CommandPositionalOptions = {};
		//std::vector<std::string> CommandTopDirectives = {};
		ProcessedCLInput(int argc, const char* const* argv);
	};
	class MBTerminal
	{
	private:
		bool m_IsPasswordInput = false;
		unsigned char p_GetColorNumber(ANSITerminalColor ColorToSet);
	public:
		MBTerminal();
		void Print(std::string const& StringToPrint);
		void PrintLine(std::string const& StringToPrint);
		void GetLine(std::string& OutLine);
		void SetPasswordInput(bool IsPassword);
		void SetTextColor(ANSITerminalColor ColorToSet);
		//void SetTextColor();
	};
}