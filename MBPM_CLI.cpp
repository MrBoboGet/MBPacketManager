#include "MBPM_CLI.h"
#include <filesystem>
namespace MBPM
{
	//BEGIN MBPM_CLI
	std::string MBPM_ClI::p_GetPacketInstallDirectory()
	{
		return(std::getenv("MBPM_PACKET_INSTALL_DIRECTORY"));
	}
	MBPP_PacketHost MBPM_ClI::p_GetDefaultPacketHost()
	{
		MBPP_PacketHost ReturnValue = { "mrboboget.se",MBPP_TransferProtocol::HTTPS,-1 };
		return(ReturnValue);
	}
	void MBPM_ClI::p_HandleUpdate(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::vector<std::string> PacketsToUpdate = {};
		if (CommandInput.CommandOptions.find("all") != CommandInput.CommandOptions.end())
		{
			std::string DirectoryToIterate = p_GetPacketInstallDirectory();
			std::filesystem::directory_iterator InstallDirectoryIterator = std::filesystem::directory_iterator(DirectoryToIterate);
			//alla packets måste innehålla MBPM_PacketInfo på toppen, annars är det inte ett giltig packet
			for (auto const& Entries : InstallDirectoryIterator)
			{
				if (Entries.is_directory())
				{
					
					if (std::filesystem::exists(MBUnicode::PathToUTF8(Entries.path()) + "/MBPM_PacketInfo"))
					{
						PacketsToUpdate.push_back(MBUnicode::PathToUTF8(std::filesystem::relative(Entries.path(), DirectoryToIterate)));
					}
				}
			}
		}
		else
		{
			PacketsToUpdate = CommandInput.TopCommandArguments;
		}
		if (PacketsToUpdate.size() == 0)
		{
			AssociatedTerminal->PrintLine("No packets given to update or no installed packets to update");
			return;
		}
		std::string InstallDirectory = p_GetPacketInstallDirectory();
		std::vector<std::string> MissingPackets = {};
		for (size_t i = 0; i < PacketsToUpdate.size(); i++)
		{
			if (!std::filesystem::exists(InstallDirectory+PacketsToUpdate[i] + "/MBPM_PacketInfo"))
			{
				MissingPackets.push_back(PacketsToUpdate[i]);
				continue;
			}
			//ANTAGANDE det här gör bara för packets som man inte ändrar på, annars kommer MBPM_FileInfo behöva uppdateras först, får se hur lång tid det tar för grejer
			MBPP_Client PacketClient;
			PacketClient.Connect(p_GetDefaultPacketHost());
			PacketClient.UpdatePacket(InstallDirectory + PacketsToUpdate[i],PacketsToUpdate[i]);
		}
	}
	void MBPM_ClI::p_HandleInstall(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::vector<std::string> PacketsToInstall = CommandInput.TopCommandArguments;
		std::string InstallDirectory = p_GetPacketInstallDirectory();
		std::vector<std::string> AlreadyInstalledPacktes = {};
		for(size_t i = 0; i < PacketsToInstall.size();i++)
		{
			if (std::filesystem::exists(InstallDirectory + PacketsToInstall[i]))
			{
				AlreadyInstalledPacktes.push_back(PacketsToInstall[i]);
				continue;
			}
			MBPP_Client PacketClient;
			PacketClient.Connect(p_GetDefaultPacketHost());
			PacketClient.DownloadPacket(InstallDirectory + PacketsToInstall[i]+"/", PacketsToInstall[i]);
		}
	}
	void MBPM_ClI::p_HandleGet(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		if (CommandInput.TopCommandArguments.size() < 2)
		{
			AssociatedTerminal->PrintLine("Command \"get\" needs atleast a package and a filesystem object");
			return;
		}
		std::string PacketName = CommandInput.TopCommandArguments[0];
		std::string ObjectToGet = CommandInput.TopCommandArguments[1];
		MBPP_Client PacketClient;
		PacketClient.Connect(p_GetDefaultPacketHost());
		MBError GetPacketError = true;
		MBPP_FileInfoReader PacketFileInfo = PacketClient.GetPacketFileInfo(PacketName,&GetPacketError);
		if (!GetPacketError)
		{
			AssociatedTerminal->PrintLine("get failed with error: " + GetPacketError.ErrorMessage);
			return;
		}
		if (PacketFileInfo.ObjectExists(ObjectToGet))
		{
			std::string OutputDirectory = "./";
			if (CommandInput.TopCommandArguments.size() > 2)
			{
				OutputDirectory = CommandInput.TopCommandArguments[2];
			}
			const MBPP_FileInfo* ObjectFileInfo = nullptr;
			const MBPP_DirectoryInfoNode* ObjectDirectoryInfo = nullptr;
			if ((ObjectFileInfo = PacketFileInfo.GetFileInfo(ObjectToGet)) != nullptr)
			{
				GetPacketError = PacketClient.DownloadPacketDirectories(OutputDirectory, PacketName, { ObjectToGet });
			}
			else if ((ObjectDirectoryInfo = PacketFileInfo.GetDirectoryInfo(ObjectToGet)) != nullptr)
			{
				GetPacketError = PacketClient.DownloadPacketFiles(OutputDirectory, PacketName, { ObjectToGet });
			}
			if (!GetPacketError)
			{
				AssociatedTerminal->PrintLine("get failed with error: " + GetPacketError.ErrorMessage);
				return;
			}
		}
		else
		{
			AssociatedTerminal->PrintLine("Package \""+ PacketName +"\" has no filesystem object \""+ ObjectToGet +"\"");
		}
	}
	void MBPM_ClI::HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		if (CommandInput.TopCommand == "install")
		{
			p_HandleInstall(CommandInput,AssociatedTerminal);
		}
		else if(CommandInput.TopCommand == "get")
		{
			p_HandleGet(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "update")
		{
			p_HandleUpdate(CommandInput, AssociatedTerminal);
		}
		else
		{
			AssociatedTerminal->PrintLine("Invalid command \"" + CommandInput.TopCommand + "\"");
		}
	}
	//END MBPM_CLI

	int MBCLI_Main(int argc, char** argv)
	{
		MBCLI::ProcessedCLInput CommandInput(argc, argv);
		MBCLI::MBTerminal ProgramTerminal;
		MBPM_ClI CLIHandler;
		CLIHandler.HandleCommand(CommandInput, &ProgramTerminal);
		return(0);
	}
}