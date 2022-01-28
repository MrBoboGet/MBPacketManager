#pragma once
#include "MBPacketManager.h"
#include <MBCLI/MBCLI.h>
#include <MBUnicode/MBUnicode.h>
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
	enum class PacketLocationType
	{
		Null,
		User,
		Installed,
		Local,
		Remote
	};

	MBError DownloadDirectory(std::string const& PacketDirectory, std::vector<std::string> const& FilesystemObjectsToDownload, MBPP_FileListDownloadHandler* DownloadHandler);
	struct PacketIdentifier
	{
		std::string PacketName = "";
		std::string PacketURI = "";//Default/"" implicerar att man använder default remoten
		PacketLocationType PacketLocation = PacketLocationType::Null;
	};
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
		void p_HandleGet(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleUpload(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleIndex(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
		void p_HandleCompile(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);

		void p_UploadPacketsLocal(std::vector<PacketIdentifier> const& PacketsToUpload,MBCLI::MBTerminal* AssociatedTerminal);
	
		MBError p_UpdateCmake(std::string const& PacketDirectory,std::string const& OptionalMBPMStaticData);//fyller bara i MBPM variablar och skriver inte över filen
		MBError p_CreateCmake(std::string const& PacketDirectory);
		MBError p_CompilePacket(std::string const& PacketDirectory);
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
		std::vector<PacketIdentifier> p_GetInstalledPacketsDependancyOrder(std::vector<std::string>* MissingPackets);
		std::vector<PacketIdentifier> p_GetUserPackets();//ingen garanti på dependancy order
		std::vector<PacketIdentifier> p_GetInstalledPackets();//implicit tar den i installed dependancy order

		//std::string p_GetInstalledPacketDirectory(std::string const& PacketName);

		PacketIdentifier p_GetInstalledPacket(std::string const& PacketName);
		PacketIdentifier p_GetUserPacket(std::string const& PacketName);
		PacketIdentifier p_GetLocalPacket(std::string const& PacketPath);
		PacketIdentifier p_GetRemotePacketIdentifier(std::string const& PacketPath);

		void p_PrintFileInfo(MBPM::MBPP_FileInfoReader const& InfoToPrint,std::vector<std::string> const& FilesystemObjectsToPrint,MBCLI::MBTerminal* AssociatedTerminal);
		void p_PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal);
	public:
		void HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
	};
	int MBCLI_Main(int argv, char** args);
}