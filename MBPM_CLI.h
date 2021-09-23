#pragma once
#include "MBPacketManager.h"
#include <MBCLI/MBCLI.h>
#include <MBUnicode/MBUnicode.h>
namespace MBPM
{
	class MBPM_ClI
	{
	private:
		std::string m_PacketInstallDirectory = "";//kan bara finnas en
		void p_HandleUpdate(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleInstall(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleGet(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		std::string p_GetPacketInstallDirectory();
		MBPP_PacketHost p_GetDefaultPacketHost();
	public:
		void HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
	};
	int MBCLI_Main(int argv, char** args);
}