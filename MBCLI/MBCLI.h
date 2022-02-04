#pragma once
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
		std::vector<std::pair<std::string, int>> GetSingleArgumentOptionList(std::string const& OptionName) const;
		ProcessedCLInput(int argc, const char* const* argv);
	};
	class MBTerminal;
	class MBTerminalLoadingbar
	{
	private:
		friend class MBTerminal;
		MBTerminal* m_AssociatedTerminal = nullptr;
		size_t m_LinePosition = -1;
		bool m_IsFinished = false;
		std::string m_CurrentTag = "";
		float m_CurrentProgress = 0;
		std::string p_GetProgressString();
		std::string p_GetCurrentLine();
		void p_UpdateLine(std::string const& LineData);
		MBTerminalLoadingbar(std::string const& LoadingBarTag, float InitialProgress);
	public:
		MBTerminalLoadingbar() {};
		void UpdateTag(std::string const& NewTag);
		void UpdateProgress(float NewProgress);
		void Finalize();
	};
	struct CursorPosition
	{
		int ColumnIndex = -1;
		int RowIndex = -1;
	};
	struct TerminalInfo
	{
		int Width = -1;
		int Height = -1;
	};
	struct TerminalCharacter
	{

	};
	struct TerminalWindowBuffer
	{
		int Height = -1;
		int Width = -1;
		std::vector<std::vector<TerminalCharacter>> BufferCharacters = {};
	};
	class MBTerminal
	{
	private:
		friend MBTerminalLoadingbar;
		bool m_IsPasswordInput = false;
		size_t m_CurrentLinePosition = 0;
		unsigned char p_GetColorNumber(ANSITerminalColor ColorToSet);
		void p_OverwriteLine(size_t LinePosition, std::string const& DataToWrite);

		const char* p_GetANSIEscapeSequence();


		TerminalWindowBuffer m_CurrentRenderedBuffer;
		TerminalWindowBuffer m_RenderTargetBuffer;
	public:
		MBTerminal();
		void Print(std::string const& StringToPrint);
		//void Print(const void* DataToPrint,size_t DataSize);
		void PrintLine(std::string const& StringToPrint);
		void GetLine(std::string& OutLine);
		void SetPasswordInput(bool IsPassword);
		void SetTextColor(ANSITerminalColor ColorToSet);
		MBTerminalLoadingbar CreateLineLoadingBar(std::string const& LoadingBarTag, float InitialProgress);
		static std::string GetByteSizeString(uint64_t NumberOfBytes);

		TerminalInfo GetTerminalInfo();

		void InitializeWindowMode();
		void DisableWindowMode();
		CursorPosition GetCursorPosition();
		void SetCursorPosition(CursorPosition NewPosition);
		TerminalWindowBuffer const& GetRenderedBuffer();
		TerminalWindowBuffer const& GetCurrentBuffer() const;
		void SwitchBuffers();
		//void SetTextColor();
	};
}