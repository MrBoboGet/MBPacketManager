#include <vector>
#include <string>
#include <unordered_map>
namespace MBCLI
{
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

	public:
		void Print(std::string const& StringToPrint);
		void PrintLine(std::string const& StringToPrint);
		void GetLine(std::string& OutLine);
		//void SetTextColor();
	};
}