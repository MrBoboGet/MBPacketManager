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
		//std::vector<std::string> CommandTopDirectives = {};
		ProcessedCLInput(int argc, const char* const* argv);
	};
	class MBTerminal
	{
	private:

	public:
		void Print(std::string const& StringToPrint);
		void PrintLine(std::string const& StringToPrint);
		//void SetTextColor();
	};
}