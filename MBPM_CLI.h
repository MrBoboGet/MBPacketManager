#pragma once
#include "MBPacketManager.h"
#include <MBCLI/MBCLI.h>
#include <MBUnicode/MBUnicode.h>
#include <unordered_map>
namespace MBPM
{
	class DownloadPrinter : public MBPP_FileListDownloadHandler
	{
	private:
		MBCLI::MBTerminal* m_AssociatedTerminal = nullptr;
	public:
		DownloadPrinter(MBCLI::MBTerminal* TerminalToUse) { m_AssociatedTerminal = TerminalToUse; }
		virtual MBError NotifyFiles(std::vector<std::string> const& FileToNotify) override;
		virtual MBError Open(std::string const& FileToDownloadName) override;
		virtual MBError InsertData(const void* Data, size_t DataSize) override;
		virtual MBError Close() override;
	};
	MBError DownloadDirectory(std::string const& PacketDirectory, std::vector<std::string> const& FilesystemObjectsToDownload, MBPP_FileListDownloadHandler* DownloadHandler);
	
	void PrintFileInfo(MBPM::MBPP_FileInfoReader const& InfoToPrint, std::vector<std::string> const& FilesystemObjectsToPrint, MBCLI::MBTerminal* AssociatedTerminal);
	void PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint, MBCLI::MBTerminal* AssociatedTerminal);

	

    
	class MBPM_ClI
	{
	private:
		std::string m_PacketInstallDirectory = "";//kan bara finnas en
		MBPM::MBPP_Client m_ClientToUse;


	


		void p_HandleHelp(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
	
		void p_HandleUpdate_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleInstall_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleGet_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleUpload_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleIndex_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleCompile_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		
		void p_HandleUpdate(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleInstall(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleUpload(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleCompile(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);

		void p_HandleGet(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleIndex(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);

		void p_HandlePackets(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);

		void p_UploadPacketsLocal(std::vector<PacketIdentifier> const& PacketsToUpload,MBCLI::ProcessedCLInput const& Command,MBCLI::MBTerminal* AssociatedTerminal);
	
		MBError p_UpdateCmake(std::string const& PacketDirectory,std::string const& OptionalMBPMStaticData);//fyller bara i MBPM variablar och skriver inte ???ver filen
		MBError p_CreateCmake(std::string const& PacketDirectory);
		MBError p_CompilePacket(PacketIdentifier const& PacketDirectory,MBCLI::MBTerminal* AssociatedTerminal,MBCLI::ProcessedCLInput const& Input);
		std::string p_GetPacketInstallDirectory();
		MBPP_PacketHost p_GetDefaultPacketHost();

		MBPM::MBPM_PacketInfo p_GetPacketInfo(PacketIdentifier const& PacketToInspect,MBError* OutError);
		MBPM::MBPP_FileInfoReader p_GetPacketFileInfo(PacketIdentifier const& PacketToInspect,MBError* OutError);

		MBPM::MBPP_FileInfoReader i_GetRemoteFileInfo(std::string const& RemoteFile,MBError* OutError);
		MBPM::MBPP_FileInfoReader i_GetInstalledFileInfo(std::string const& InstalledPacket, MBError* OutError);
		MBPM::MBPP_FileInfoReader i_GetPathFileInfo(std::string const& InstalledPacket, MBError* OutError);
		MBPM::MBPP_FileInfoReader i_GetUserFileInfo(std::string const& PacketName, MBError* OutError);

		bool p_PacketExists(PacketIdentifier const& PacketToCheck);

		//std::string p_GetInstalledPacketDirectory();
		std::vector<PacketIdentifier> p_GetCommandPackets(MBCLI::ProcessedCLInput const& CLIInput, PacketLocationType DefaultType,MBError& OutError, size_t ArgumentOffset = 0);
		PacketIdentifier p_GetPacketIdentifier(std::string const& PacketName, PacketLocationType Type, MBError& OutError);

		std::vector<PacketIdentifier> p_GetInstalledPacketsDependancyOrder(std::vector<std::string>* MissingPackets);

		std::map<std::string, MBPM_PacketDependancyRankInfo> p_GetPacketDependancieInfo(
			std::vector<PacketIdentifier> const& InPacketsToCheck,
			MBError& OutError,
			std::vector<std::string>* OutMissing);

		std::vector<PacketIdentifier> p_GetPacketDependants_DependancyOrder(std::vector<PacketIdentifier> const& PacketsToCheck,MBError& OutError, std::vector<std::string>* MissingPackets);
		std::vector<PacketIdentifier> p_GetPacketDependancies_DependancyOrder(std::vector<PacketIdentifier> const& PacketsToCheck,MBError& OutError,std::vector<std::string>* MissingPackets);

		std::vector<PacketIdentifier> p_UnrollPacketDependancyInfo(std::map<std::string, MBPM_PacketDependancyRankInfo> const& InPackets);

		std::vector<PacketIdentifier> p_GetUserPackets();//ingen garanti p??? dependancy order
		std::vector<PacketIdentifier> p_GetInstalledPackets();//TAR INTE I DEPENDANCY ORDER


		PacketIdentifier p_GetInstalledPacket(std::string const& PacketName);
		PacketIdentifier p_GetUserPacket(std::string const& PacketName);
		PacketIdentifier p_GetLocalPacket(std::string const& PacketPath);
		PacketIdentifier p_GetRemotePacketIdentifier(std::string const& PacketPath);

	public:
		void HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
	};
	int MBCLI_Main(int argv,const char** args);
}
