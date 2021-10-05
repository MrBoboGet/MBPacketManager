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
		void p_HandleUpload(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleInfo(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);


		void p_HandleCompile(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
	
		MBError p_UpdateCmake(std::string const& PacketDirectory);//fyller bara i MBPM variablar och skriver inte över filen
		MBError p_CreateCmake(std::string const& PacketDirectory);
		MBError p_CompilePacket(std::string const& PacketDirectory);
		std::vector<std::string> p_GetInstalledPacketsDependancyOrder();
		std::string p_GetPacketInstallDirectory();
		MBPP_PacketHost p_GetDefaultPacketHost();

		MBPM::MBPP_FileInfoReader p_GetRemoteFileInfo(std::string const& RemoteFile,MBError* OutError);
		MBPM::MBPP_FileInfoReader p_GetInstalledFileInfo(std::string const& InstalledPacket, MBError* OutError);
		MBPM::MBPP_FileInfoReader p_GetPathFileInfo(std::string const& InstalledPacket, MBError* OutError);

		std::vector<std::pair<std::string, std::string>> p_GetUserUploadPackets();

		std::string p_GetInstalledPacketDirectory(std::string const& PacketName);
		void p_PrintFileInfo(MBPM::MBPP_FileInfoReader const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal);
		void p_PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal);
	public:
		void HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
	};
	int MBCLI_Main(int argv, char** args);
}