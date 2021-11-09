#include "MBPM_CLI.h"
#include <filesystem>
#include <map>
#include <MBUnicode/MBUnicode.h>
#include <exception>
//#include "MBCLI/"

namespace MBPM
{
	//BEGIN MBPM_CLI
	std::string MBPM_ClI::p_GetPacketInstallDirectory()
	{
		return(std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY"));
	}
	struct MBPM_PacketDependancyRankInfo
	{
		size_t DependancyDepth = -1;
		std::string PacketName = "";
		std::vector<std::string> PacketDependancies = {};
		bool operator<(MBPM_PacketDependancyRankInfo const& PacketToCompare) const
		{
			bool ReturnValue = DependancyDepth < PacketToCompare.DependancyDepth;
			if (DependancyDepth == PacketToCompare.DependancyDepth)
			{
				ReturnValue = PacketName < PacketToCompare.PacketName;
			}
			return(ReturnValue);
		}
	};
	size_t h_GetPacketDepth(std::map<std::string, MBPM_PacketDependancyRankInfo>& PacketInfoToUpdate,std::string const& PacketName)
	{
		MBPM_PacketDependancyRankInfo& PacketToEvaluate = PacketInfoToUpdate[PacketName];
		if (PacketToEvaluate.DependancyDepth != -1)
		{
			return(PacketToEvaluate.DependancyDepth);
		}
		else
		{
			std::vector<std::string>& PacketDependancies = PacketToEvaluate.PacketDependancies;
			if (PacketDependancies.size() == 0)
			{
				PacketToEvaluate.DependancyDepth = 0;
				return(0);
			}
			else
			{
				size_t DeepestDepth = 0;
				for (size_t i = 0; i < PacketDependancies.size(); i++)
				{
					size_t NewDepth = h_GetPacketDepth(PacketInfoToUpdate,PacketDependancies[i]) + 1;
					if (NewDepth > DeepestDepth)
					{
						DeepestDepth = NewDepth;
					}
				}
				PacketToEvaluate.DependancyDepth = DeepestDepth;
				return(DeepestDepth);
			}
		}
	}
	std::vector<std::string> MBPM_ClI::p_GetInstalledPacketsDependancyOrder()
	{
		std::map<std::string, MBPM_PacketDependancyRankInfo> Packets = {};
		std::vector<std::string> InstalledPackets = {};
		std::filesystem::directory_iterator PacketDirectoryIterator = std::filesystem::directory_iterator(p_GetPacketInstallDirectory());
		for (auto const& Entries : PacketDirectoryIterator)
		{
			if (Entries.is_directory())
			{
				std::string InfoPath = MBUnicode::PathToUTF8(Entries.path()) + "/MBPM_PacketInfo";
				if (std::filesystem::exists(InfoPath))
				{
					MBPM::MBPM_PacketInfo PacketInfo = MBPM::ParseMBPM_PacketInfo(InfoPath);
					if (PacketInfo.PacketName == "") //kanske också borde asserta att packetens folder är samma som dess namn
					{
						continue;
					}
					MBPM_PacketDependancyRankInfo InfoToAdd;
					InfoToAdd.PacketName = PacketInfo.PacketName;
					InfoToAdd.PacketDependancies = PacketInfo.PacketDependancies;
					InstalledPackets.push_back(InfoToAdd.PacketName);
					Packets[PacketInfo.PacketName] = InfoToAdd;
				}
			}
		}
		std::set<MBPM_PacketDependancyRankInfo> OrderedPackets = {};
		for (auto const& Packet : InstalledPackets)
		{
			Packets[Packet].DependancyDepth = h_GetPacketDepth(Packets, Packet);
			OrderedPackets.insert(Packets[Packet]);
		}
		std::vector<std::string> ReturnValue = {};
		for (auto const& Packet : OrderedPackets)
		{
			ReturnValue.push_back(Packet.PacketName);
		}
		return(ReturnValue);
	}
	std::vector<std::pair<std::string, std::string>> MBPM_ClI::p_GetUserUploadPackets()
	{
		std::vector<std::pair<std::string, std::string>> ReturnValue = {};
		std::set<std::string> SearchDirectories = {};
		std::string PacketInstallDirectory = p_GetPacketInstallDirectory();
		//läser in config filen
		if (std::filesystem::exists(PacketInstallDirectory + "/LocalUploadPackets.txt"))
		{
			std::ifstream UploadPacketsFile = std::ifstream(PacketInstallDirectory + "/LocalUploadPackets.txt",std::ios::in);
			std::string CurrentLine = "";
			while (std::getline(UploadPacketsFile,CurrentLine))
			{
				SearchDirectories.insert(CurrentLine);
			}
		}
		for (std::string const& Directories : SearchDirectories)
		{
			std::filesystem::directory_iterator CurrentDirectoryIterator = std::filesystem::directory_iterator(Directories);
			for (auto const& Entries : CurrentDirectoryIterator)
			{
				if (Entries.is_directory())
				{
					std::string EntryPath = MBUnicode::PathToUTF8(Entries.path())+"/";
					if (std::filesystem::exists(EntryPath+"/MBPM_PacketInfo"))
					{
						MBPM::MBPM_PacketInfo CurrentPacket = MBPM::ParseMBPM_PacketInfo(EntryPath + "/MBPM_PacketInfo");
						if (CurrentPacket.PacketName != "")
						{
							ReturnValue.push_back(std::pair<std::string, std::string>(CurrentPacket.PacketName, EntryPath));
						}
					}
				}
			}
		}

		return(ReturnValue);
	}
	std::string MBPM_ClI::p_GetInstalledPacketDirectory(std::string const& PacketName)
	{
		std::string ReturnValue = p_GetPacketInstallDirectory();
		if (ReturnValue.back() != '/')
		{
			ReturnValue += "/";
		}
		if (std::filesystem::exists(ReturnValue + PacketName) && std::filesystem::exists(ReturnValue + PacketName + "/MBPM_PacketInfo"))
		{
			ReturnValue = ReturnValue + PacketName + "/";
		}
		else
		{
			ReturnValue = "";
		}

		return(ReturnValue);
	}
	MBPP_PacketHost MBPM_ClI::p_GetDefaultPacketHost()
	{
		//MBPP_PacketHost ReturnValue = { "mrboboget.se/MBPM/",MBPP_TransferProtocol::HTTPS,443 };
		MBPP_PacketHost ReturnValue = { "mrboboget.se/MBPM/",MBPP_TransferProtocol::HTTPS,443 };
		return(ReturnValue);
	}

	MBPM::MBPP_FileInfoReader MBPM_ClI::p_GetRemoteFileInfo(std::string const& RemoteFile, MBError* OutError)
	{
		m_ClientToUse.Connect(p_GetDefaultPacketHost());
		MBPM::MBPP_FileInfoReader ReturnValue = m_ClientToUse.GetPacketFileInfo(RemoteFile, OutError);
		return(ReturnValue);
	}
	MBPM::MBPP_FileInfoReader MBPM_ClI::p_GetInstalledFileInfo(std::string const& InstalledPacket,MBError* OutError)
	{
		MBPM::MBPP_FileInfoReader ReturnValue;
		std::string PacketDirectory = p_GetInstalledPacketDirectory(InstalledPacket);
		if (PacketDirectory == "" || !std::filesystem::exists(PacketDirectory + "/MBPM_FileInfo"))
		{
			*OutError = false;
			OutError->ErrorMessage = "Packet not installed or packet FileInfo missing";
			return(ReturnValue);
		}
		ReturnValue = MBPM::MBPP_FileInfoReader(PacketDirectory + "/MBPM_FileInfo");
		return(ReturnValue);
	}
	MBPM::MBPP_FileInfoReader MBPM_ClI::p_GetPathFileInfo(std::string const& PacketPath, MBError* OutError)
	{
		MBPM::MBPP_FileInfoReader ReturnValue;
		if (std::filesystem::exists(PacketPath) && std::filesystem::directory_entry(PacketPath).is_regular_file())
		{
			ReturnValue = MBPM::MBPP_FileInfoReader(PacketPath);
		}
		else if (std::filesystem::exists(PacketPath + "/MBPM_FileInfo") && std::filesystem::directory_entry(PacketPath + "/MBPM_FileInfo").is_regular_file())
		{
			ReturnValue = MBPM::MBPP_FileInfoReader(PacketPath+ "/MBPM_FileInfo");
		}
		else
		{
			*OutError = false;
			OutError->ErrorMessage = "Couldnt find MBPM_FileInfo at \"" + PacketPath + "\" or \"" + PacketPath + "/MBPM_FileInfo\"";
		}
		return(ReturnValue);
	}


	void MBPM_ClI::p_PrintFileInfo(MBPM::MBPP_FileInfoReader const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal)
	{
		const MBPM::MBPP_DirectoryInfoNode* InfoTopNode = InfoToPrint.GetDirectoryInfo("/");
		MBPM::MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = InfoTopNode->begin();
		MBPM::MBPP_DirectoryInfoNode_ConstIterator End = InfoTopNode->end();
		while (DirectoryIterator != End)
		{
			std::string Filename = DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName;
			std::string FileHash = MBUtility::ReplaceAll(MBUtility::HexEncodeString((*DirectoryIterator).FileHash), " ", "");
			std::string FileSizeString = "";
			uint64_t FileSize = (*DirectoryIterator).FileSize;
			uint8_t FileSizeLog = std::log10(FileSize);
			if (FileSizeLog < 3)
			{
				FileSizeString = std::to_string(FileSize) + " B";
			}
			else if (FileSizeLog >= 3 && FileSizeLog < 6)
			{
				FileSizeString = std::to_string(FileSize / 1000) + " KB";
			}
			else if (FileSizeLog >= 6 && FileSizeLog < 9)
			{
				FileSizeString = std::to_string(FileSize / 1000000) + " MB";
			}
			else
			{
				FileSizeString = std::to_string(FileSize / 1000000000) + " GB";
			}
			AssociatedTerminal->PrintLine(Filename+"\t\t\t\t"+FileSizeString+"\t"+ FileHash);
			DirectoryIterator++;
		}

	}
	void MBPM_ClI::p_PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
		AssociatedTerminal->PrintLine("New directories:");
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Green);
		for (auto const& NewDirectories : InfoToPrint.AddedDirectories)
		{
			AssociatedTerminal->PrintLine(NewDirectories);
		}
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
		AssociatedTerminal->PrintLine("New files:");
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Green);
		for (auto const& NewFiles : InfoToPrint.AddedFiles)
		{
			AssociatedTerminal->PrintLine(NewFiles);
		}
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
		AssociatedTerminal->PrintLine("Updated files:");
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Cyan);
		for (auto const& UpdatedFiles : InfoToPrint.UpdatedFiles)
		{
			AssociatedTerminal->PrintLine(UpdatedFiles);
		}
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
		AssociatedTerminal->PrintLine("Removed directories:");
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
		for (auto const& RemovedDirectories : InfoToPrint.DeletedDirectories)
		{
			AssociatedTerminal->PrintLine(RemovedDirectories);
		}
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
		AssociatedTerminal->PrintLine("Removed files:");
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
		for (auto const& RemovedFiles : InfoToPrint.RemovedFiles)
		{
			AssociatedTerminal->PrintLine(RemovedFiles);
		}
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
	}
	void MBPM_ClI::p_HandleUpdate(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::vector<std::string> PacketsToUpdate = {};
		if (CommandInput.CommandOptions.find("all") != CommandInput.CommandOptions.end())
		{
			PacketsToUpdate = p_GetInstalledPacketsDependancyOrder();
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
		std::vector<std::string> FailedPackets = {};
		m_ClientToUse.Connect(p_GetDefaultPacketHost());
		for (size_t i = 0; i < PacketsToUpdate.size(); i++)
		{
			if (!std::filesystem::exists(InstallDirectory+PacketsToUpdate[i] + "/MBPM_PacketInfo"))
			{
				MissingPackets.push_back(PacketsToUpdate[i]);
				continue;
			}
			//ANTAGANDE det här gör bara för packets som man inte ändrar på, annars kommer MBPM_FileInfo behöva uppdateras först, får se hur lång tid det tar för grejer
			AssociatedTerminal->PrintLine("Updating packet " + PacketsToUpdate[i]);
			MBError UpdateResult = m_ClientToUse.UpdatePacket(InstallDirectory + PacketsToUpdate[i],PacketsToUpdate[i]);
			if (!UpdateResult)
			{
				FailedPackets.push_back(PacketsToUpdate[i]);
			}
			else
			{
				AssociatedTerminal->PrintLine("Successfully updated packet " + PacketsToUpdate[i]);
			}
		}
		if (FailedPackets.size() == 0)
		{
			AssociatedTerminal->PrintLine("Successfully updated all packets!");
		}
		else
		{
			AssociatedTerminal->PrintLine("Failed updating following packets:");
			for (size_t i = 0; i < FailedPackets.size(); i++)
			{
				AssociatedTerminal->PrintLine(FailedPackets[i]);
			}
		}
	}
	void MBPM_ClI::p_HandleInstall(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::string InstallDirectory = p_GetPacketInstallDirectory();
		std::stack<std::string> PacketsToInstall = {};
		for (size_t i = 0; i < CommandInput.TopCommandArguments.size(); i++)
		{
			PacketsToInstall.push(CommandInput.TopCommandArguments[i]);
		}
		std::unordered_set<std::string> AlreadyInstalledPackets = {};
		std::stack<std::string> NewPacketsToCompile = {};
		while(PacketsToInstall.size() > 0)
		{
			std::string CurrentPacket = PacketsToInstall.top();
			PacketsToInstall.pop();
			if (std::filesystem::exists(InstallDirectory + CurrentPacket) || AlreadyInstalledPackets.find(CurrentPacket) != AlreadyInstalledPackets.end())
			{
				AlreadyInstalledPackets.insert(CurrentPacket);
				continue;
			}
			MBError DownloadStatus = m_ClientToUse.Connect(p_GetDefaultPacketHost());
			if (DownloadStatus)
			{
				DownloadStatus = m_ClientToUse.DownloadPacket(InstallDirectory + CurrentPacket + "/", CurrentPacket);
			}
			if (DownloadStatus)
			{
				MBPM::CreatePacketFilesData(InstallDirectory + CurrentPacket + "/");
				if (std::filesystem::exists(InstallDirectory + CurrentPacket + "/MBPM_PacketInfo"))
				{
					MBPM::MBPM_PacketInfo InstalledPacketInfo = MBPM::ParseMBPM_PacketInfo(InstallDirectory + CurrentPacket + "/MBPM_PacketInfo");
					if (InstalledPacketInfo.PacketName != "")
					{
						if (InstalledPacketInfo.Attributes.find(MBPM::MBPM_PacketAttribute::Precompiled) == InstalledPacketInfo.Attributes.end())
						{
							NewPacketsToCompile.push(CurrentPacket);
						}
						for (size_t i = 0; i < InstalledPacketInfo.PacketDependancies.size(); i++)
						{
							if (AlreadyInstalledPackets.find(InstalledPacketInfo.PacketDependancies[i]) == AlreadyInstalledPackets.end())
							{
								PacketsToInstall.push(InstalledPacketInfo.PacketDependancies[i]);
							}
						}
					}
				}
			}
			AlreadyInstalledPackets.insert(CurrentPacket);
		}
		while (NewPacketsToCompile.size() > 0)
		{
			std::string CurrentPacket = NewPacketsToCompile.top();
			NewPacketsToCompile.pop();
			MBPM::CompileAndInstallPacket(InstallDirectory + CurrentPacket + "/");
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
		m_ClientToUse.Connect(p_GetDefaultPacketHost());
		if (!m_ClientToUse.IsConnected())
		{
			AssociatedTerminal->PrintLine("Couldn't establish connection so server");
			return;
		}
		MBError GetPacketError = true;
		MBPP_FileInfoReader PacketFileInfo = m_ClientToUse.GetPacketFileInfo(PacketName,&GetPacketError);
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
				GetPacketError = m_ClientToUse.DownloadPacketFiles(OutputDirectory, PacketName, { ObjectToGet });
			}
			else if ((ObjectDirectoryInfo = PacketFileInfo.GetDirectoryInfo(ObjectToGet)) != nullptr)
			{
				GetPacketError = m_ClientToUse.DownloadPacketDirectories(OutputDirectory, PacketName, { ObjectToGet });
			}
			else
			{
				AssociatedTerminal->PrintLine("No file named \"" + PacketName+"\"");
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
	std::vector<std::string> h_GetDirectoryFiles(std::string const& DirectoryPath, std::string const& DirectoryPrefix)
	{
		std::vector<std::string> ReturnValue = {};
		std::filesystem::recursive_directory_iterator DirectoryIterator = std::filesystem::recursive_directory_iterator(DirectoryPath);
		for (auto const& Entry : DirectoryIterator)
		{
			if (Entry.is_regular_file())
			{
				std::filesystem::path RelativePath = std::filesystem::relative(Entry.path(), DirectoryPath);
				ReturnValue.push_back(DirectoryPrefix + MBUnicode::PathToUTF8(RelativePath));
			}
		}
		return(ReturnValue);
	}
	std::set<std::string> h_GetPacketsToSend(MBCLI::ProcessedCLInput const& CommandInput, std::string const& PacketPath, std::string const& PacketDirectory,MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::set<std::string> ReturnValue = {};
		if (!(std::filesystem::exists(PacketPath + PacketDirectory) && std::filesystem::directory_entry(PacketPath + PacketDirectory).is_regular_file()))
		{
			return(ReturnValue);
		}
		std::vector<size_t> const& CommandPosition = CommandInput.CommandPositionalOptions.at("d");
		for (size_t i = 0; i < CommandPosition.size(); i++)
		{
			if (CommandPosition[i] + 1 >= CommandInput.TotalCommandTokens.size())
			{
				AssociatedTerminal->PrintLine("Positional argument -d given without additional input");
				return (ReturnValue);
			}
			std::string DirectoryToSend = CommandInput.TotalCommandTokens[CommandPosition[i] + 1];
			//ANTAGANDE denna funktion används för att specificera directorys som annars kanske inte tas upp af FileInfoIgnoren, och går därför igenom hela directoryn
			std::vector<std::string> NewFiles = h_GetDirectoryFiles(PacketPath + DirectoryToSend, DirectoryToSend);
			for(std::string const& Paths : NewFiles)
			{
				ReturnValue.insert(Paths);
			}
		}
		return(ReturnValue);
	}
	void h_GetUpdateToSend(MBCLI::ProcessedCLInput const& CommandInput, MBPP_FileInfoReader const& ServerFileInfo, std::string const& PacketDirectory, std::vector<std::string>& OutFilesToSend, std::vector<std::string>& OutFilesToDelete, MBCLI::MBTerminal* AssociatedTerminal)
	{
		//aningen innefektivt iomed all kopiering av data men är viktigare med korrekthet just nu
		std::set<std::string> FilesToSend = {};
		std::set<std::string> ObjectsToDelete = {};
		if (CommandInput.CommandPositionalOptions.find("f") != CommandInput.CommandPositionalOptions.end())
		{
			std::vector<size_t> const& FilePosition = CommandInput.CommandPositionalOptions.at("f");
			for (size_t i = 0; i < FilePosition.size(); i++)
			{
				if (FilePosition[i] +1< CommandInput.TotalCommandTokens.size())
				{
					FilesToSend.insert(CommandInput.TotalCommandTokens[FilePosition[i]+1]);
				}
			}
		}
		if (CommandInput.CommandPositionalOptions.find("d") != CommandInput.CommandPositionalOptions.end())
		{
			std::vector<size_t> OptionPositions = CommandInput.CommandPositionalOptions.at("d");
			for (size_t i = 0; i < OptionPositions.size(); i++)
			{
				//för varje directory tar vi diffen
				if (OptionPositions[i] + 1 < CommandInput.TotalCommandTokens.size())
				{
					std::string const& CurrentDirectory = CommandInput.TotalCommandTokens[OptionPositions[i] + 1];
					MBPP_FileInfoReader DirectoryInfo = CreateFileInfo(PacketDirectory + CurrentDirectory);
					if (ServerFileInfo.ObjectExists(CurrentDirectory))
					{
						MBPP_FileInfoDiff DirectoriesDiff = MBPP_FileInfoReader::GetFileInfoDifference(*ServerFileInfo.GetDirectoryInfo(CurrentDirectory), *DirectoryInfo.GetDirectoryInfo("/"));
						for (auto const& NewFiles : DirectoriesDiff.AddedFiles)
						{
							FilesToSend.insert(CurrentDirectory+NewFiles);
						}
						for (auto const& UpdateFiles : DirectoriesDiff.UpdatedFiles)
						{
							FilesToSend.insert(CurrentDirectory + UpdateFiles);
						}
						for (auto const& RemovedFiles : DirectoriesDiff.RemovedFiles)
						{
							ObjectsToDelete.insert(CurrentDirectory + RemovedFiles);
						}
						for (auto const& DeletedDirectories : DirectoriesDiff.DeletedDirectories)
						{
							ObjectsToDelete.insert(CurrentDirectory+DeletedDirectories);
						}
						for (auto const& NewDirectories : DirectoriesDiff.AddedDirectories)
						{
							const MBPP_DirectoryInfoNode* NewDirectory = DirectoryInfo.GetDirectoryInfo(NewDirectories);
							MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = NewDirectory->begin();
							while (DirectoryIterator != NewDirectory->end())
							{
								FilesToSend.insert(CurrentDirectory + DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName);
								DirectoryIterator++;
							}
						}
					}
					else
					{
						//tar bara datan vi har
						const MBPP_DirectoryInfoNode* NewDirectory = DirectoryInfo.GetDirectoryInfo("/");
						MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = NewDirectory->begin();
						while (DirectoryIterator != NewDirectory->end())
						{
							FilesToSend.insert(CurrentDirectory + DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName);
							DirectoryIterator++;
						}
						
					}
				}
			}
		}
		OutFilesToSend = {};
		OutFilesToDelete = {};
		for (auto const& Files : FilesToSend)
		{
			OutFilesToSend.push_back(Files);
		}
		for (auto const& Files : ObjectsToDelete)
		{
			OutFilesToDelete.push_back(Files);
		}
	}
	void MBPM_ClI::p_HandleUpload(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		//först så behöver vi skicka data för att se login typen
		MBPM::MBPP_ComputerInfo PreviousComputerInfo = m_ClientToUse.GetComputerInfo();
		if (CommandInput.TopCommandArguments.size() < 2 && CommandInput.CommandOptions.find("all") == CommandInput.CommandOptions.end() && CommandInput.CommandOptions.find("user") 
			== CommandInput.CommandOptions.end())
		{
			AssociatedTerminal->PrintLine("Invalid \"upload\" arguments, requires packet to upload and packet directory");
		}
		std::vector<std::pair<std::string, std::string>> PacketsToUpload = {};//packet name + packet directory
		if (CommandInput.CommandOptions.find("all") != CommandInput.CommandOptions.end())
		{
			PacketsToUpload = p_GetUserUploadPackets();
		}
		if (CommandInput.TopCommandArguments.size() >= 2 && CommandInput.CommandOptions.find("user") == CommandInput.CommandOptions.end())
		{
			PacketsToUpload.push_back(std::pair<std::string, std::string>(CommandInput.TopCommandArguments[0], CommandInput.TopCommandArguments[1]));
		}
		if (CommandInput.CommandOptions.find("user") != CommandInput.CommandOptions.end())
		{
			std::vector<std::pair<std::string, std::string>> UserPackets = p_GetUserUploadPackets();
			for (size_t i = 0; i < CommandInput.TopCommandArguments.size(); i++)
			{
				bool PacketFound = false;
				for (size_t j = 0; j < UserPackets.size(); j++)
				{
					if (UserPackets[j].first == CommandInput.TopCommandArguments[i])
					{
						PacketFound = true;
						PacketsToUpload.push_back(UserPackets[j]);
						break;
					}
				}
				if (!PacketFound)
				{
					AssociatedTerminal->PrintLine("Couldn't find UserPacket \"" + CommandInput.TopCommandArguments[i] + "\"");
					return;
				}
			}
		}
		//kollar att alla packets har unika namn
		std::unordered_set<std::string> PacketNames = {};
		bool NameClashes = false;
		for (size_t i = 0; i < PacketsToUpload.size(); i++)
		{
			if (PacketNames.find(PacketsToUpload[i].first) != PacketNames.end())
			{
				NameClashes = true;
				break;
			}
			PacketNames.insert(PacketsToUpload[i].first);
		}
		if (NameClashes)
		{
			AssociatedTerminal->PrintLine("Error: you are trying to upload multiple packets with the same name");
			return;
		}
		MBPP_UploadRequest_Response RequestResponse;
		if (CommandInput.CommandOptions.find("computerdiff") == CommandInput.CommandOptions.end())
		{
			m_ClientToUse.SetComputerInfo(MBPM::MBPP_ComputerInfo());
		}
		m_ClientToUse.Connect(p_GetDefaultPacketHost());
		std::string VerificationData = "";
		//std::set<std::string> FilesToSend = {};
		bool UseDirectPaths = false;
		if (CommandInput.CommandPositionalOptions.find("d") != CommandInput.CommandPositionalOptions.end() || CommandInput.CommandPositionalOptions.find("f") != CommandInput.CommandPositionalOptions.end())
		{
			UseDirectPaths = true;
		}
		MBError UploadError = true;
		MBPP_UserCredentialsType CurrentCredentialType = MBPP_UserCredentialsType::Plain;
		for (size_t i = 0; i < PacketsToUpload.size(); i++)
		{
			std::vector<std::string> FilesToUpload = {};
			std::vector<std::string> FilesToDelete = {};
			std::string PacketName = PacketsToUpload[i].first;
			std::string PacketToUploadDirectory = PacketsToUpload[i].second;
			AssociatedTerminal->PrintLine("Uploading packet " + PacketsToUpload[i].first);
			if (!UseDirectPaths)
			{
				MBPM::CreatePacketFilesData(PacketToUploadDirectory);
			}
			else
			{
				MBPP_FileInfoReader ServerInfo = m_ClientToUse.GetPacketFileInfo(PacketName,&UploadError);
				if (!UploadError)
				{
					AssociatedTerminal->PrintLine("Error downloading server file info for packet "+PacketName+" : " + UploadError.ErrorMessage);
					return;
				}
				h_GetUpdateToSend(CommandInput, ServerInfo, PacketToUploadDirectory, FilesToUpload, FilesToDelete, AssociatedTerminal);
			}
			MBError UploadError = true;
			if (VerificationData == "")
			{
				if (!UseDirectPaths)
				{
					UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Request, "", &RequestResponse);
				}
				else
				{
					UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Request, "", FilesToUpload, FilesToDelete, &RequestResponse);
				}
			}
			else
			{
				if (!UseDirectPaths)
				{
					UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, CurrentCredentialType, VerificationData, &RequestResponse);
				}
				else
				{
					UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, CurrentCredentialType, VerificationData, FilesToUpload, FilesToDelete, &RequestResponse);
				}
				if (UploadError)
				{
					AssociatedTerminal->PrintLine("Sucessfully uploaded " + PacketName);
				}
			}
			if (RequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
			{
				AssociatedTerminal->PrintLine("Need verification for " + RequestResponse.DomainToLogin + ":");
				//Borde egentligen kolla vilka typer den stödjer
				AssociatedTerminal->Print("Username: ");
				std::string Username;
				AssociatedTerminal->GetLine(Username);
				std::string Password;
				AssociatedTerminal->Print("Password: ");
				AssociatedTerminal->SetPasswordInput(true);
				AssociatedTerminal->GetLine(Password);
				AssociatedTerminal->SetPasswordInput(false);
				VerificationData = MBPP_EncodeString(Username) + MBPP_EncodeString(Password);
				m_ClientToUse.Connect(p_GetDefaultPacketHost());


				if (!UseDirectPaths)
				{
					UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Plain, VerificationData, &RequestResponse);
				}
				else
				{
					UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Plain, VerificationData, FilesToUpload, FilesToDelete, &RequestResponse);
				}
				
				if (!UploadError)
				{
					AssociatedTerminal->PrintLine("Error uploading "+PacketName+": " + UploadError.ErrorMessage);
					VerificationData = "";
				}
				else
				{
					AssociatedTerminal->PrintLine("Sucessfully uploaded "+PacketName);
				}

			}
		}
		m_ClientToUse.SetComputerInfo(PreviousComputerInfo);
		//else
		//{
		//	AssociatedTerminal->PrintLine("Upload packet sucessful");
		//}
	}
	void MBPM_ClI::p_HandleInfo(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		if (CommandInput.TopCommandArguments.size() < 1)
		{
			AssociatedTerminal->PrintLine("Command \"info\" requires atleast 1 arguments");
			return;
		}
		if (CommandInput.TopCommandArguments[0] == "create")
		{
			if (CommandInput.TopCommandArguments.size() < 2 && CommandInput.CommandOptions.find("alluser") == CommandInput.CommandOptions.end() && CommandInput.CommandOptions.find("allinstalled") == CommandInput.CommandOptions.end())
			{
				AssociatedTerminal->PrintLine("no directory path provided");
				return;
			}
			std::vector<std::string> DirectoriesToProcess = {};
			if (CommandInput.TopCommandArguments.size() >= 2)
			{
				DirectoriesToProcess.push_back(CommandInput.TopCommandArguments[1]+"/");
			}
			if (CommandInput.CommandOptions.find("alluser") != CommandInput.CommandOptions.end())
			{
				std::vector<std::pair<std::string, std::string>> UserPackets = p_GetUserUploadPackets();
				for (size_t i = 0; i < UserPackets.size(); i++)
				{
					DirectoriesToProcess.push_back(UserPackets[i].second);
				}
			}
			if (CommandInput.CommandOptions.find("allinstalled") != CommandInput.CommandOptions.end())
			{
				std::vector<std::string> InstalledPackets = p_GetInstalledPacketsDependancyOrder();
				std::string PacketInstallDirectory = p_GetPacketInstallDirectory();
				for (size_t i = 0; i < InstalledPackets.size(); i++)
				{
					DirectoriesToProcess.push_back(PacketInstallDirectory + "/" + InstalledPackets[i]);
				}
			}
			std::string FileInfoName = "MBPM_FileInfo";
			if (CommandInput.TopCommandArguments.size() >= 3)
			{
				FileInfoName = CommandInput.TopCommandArguments[2];
			}
			for (size_t i = 0; i < DirectoriesToProcess.size(); i++)
			{
				CreatePacketFilesData(DirectoriesToProcess[i], FileInfoName);
			}
		}
		else if (CommandInput.TopCommandArguments[0] == "list")
		{
			MBPM::MBPP_FileInfoReader InfoToPrint;
			if (CommandInput.TopCommandArguments.size() < 2)
			{
				AssociatedTerminal->PrintLine("No packet directory given");
				return;
			}
			if (CommandInput.CommandOptions.find("remote") != CommandInput.CommandOptions.end())
			{
				m_ClientToUse.Connect(p_GetDefaultPacketHost());
				MBError GetFileInfoResult = true;
				MBPM::MBPP_FileInfoReader ServerInfo = m_ClientToUse.GetPacketFileInfo(CommandInput.TopCommandArguments[1], &GetFileInfoResult);
				if (GetFileInfoResult)
				{
					InfoToPrint = std::move(ServerInfo);
				}
				else
				{
					AssociatedTerminal->PrintLine("Error connectin to remote: "+GetFileInfoResult.ErrorMessage);
				}
			}
			else if (CommandInput.CommandOptions.find("installed") != CommandInput.CommandOptions.end())
			{
				std::string PacketDirectory = p_GetInstalledPacketDirectory(CommandInput.TopCommandArguments[1]);
				if (PacketDirectory == "")
				{
					AssociatedTerminal->PrintLine("packet \"" + CommandInput.TopCommandArguments[1] + "\" not intalled");
					return;
				}
				else
				{
					std::string FilePath = PacketDirectory + "MBPM_FileInfo";
					if (std::filesystem::exists(FilePath) && std::filesystem::directory_entry(FilePath).is_regular_file())
					{
						InfoToPrint = MBPM::MBPP_FileInfoReader(FilePath);
					}
					else
					{
						AssociatedTerminal->PrintLine("Couldn't find packet MBPM_FileInfo file");
						return;
					}
				}
			}
			else
			{
				//vanlig, ta directoryn
				if (std::filesystem::exists(CommandInput.TopCommandArguments[1]) && std::filesystem::directory_entry(CommandInput.TopCommandArguments[1]).is_regular_file())
				{
					InfoToPrint = MBPM::MBPP_FileInfoReader((CommandInput.TopCommandArguments[1]));
				}
				else
				{
					std::string FolderPath = CommandInput.TopCommandArguments[1];
					if (FolderPath.back() != '/')
					{
						FolderPath += '/';
					}
					std::string FilePath = FolderPath + "MBPM_FileInfo";
					if (std::filesystem::exists(FilePath) && std::filesystem::directory_entry(FilePath).is_regular_file())
					{
						InfoToPrint = MBPM::MBPP_FileInfoReader(FilePath);
					}
					else
					{
						AssociatedTerminal->PrintLine("Couldn't find MBPM_FileInfo file");
						return;
					}
				}
			}
			p_PrintFileInfo(InfoToPrint,AssociatedTerminal);
		}
		else if (CommandInput.TopCommandArguments[0] == "diff")
		{
			//två strängar som båda leder till en MBPM_FileInfo
			std::vector<size_t> RemoteFiles = (CommandInput.CommandPositionalOptions.find("r") != CommandInput.CommandPositionalOptions.end()) ? CommandInput.CommandPositionalOptions.at("r") : std::vector<size_t>();
			std::vector<size_t> InstalledFiles = (CommandInput.CommandPositionalOptions.find("i") != CommandInput.CommandPositionalOptions.end()) ? CommandInput.CommandPositionalOptions.at("i") : std::vector<size_t>();
			std::vector<size_t> PathFiles = (CommandInput.CommandPositionalOptions.find("p") != CommandInput.CommandPositionalOptions.end()) ? CommandInput.CommandPositionalOptions.at("p") : std::vector<size_t>();
			if (RemoteFiles.size() + InstalledFiles.size() > 2)
			{
				AssociatedTerminal->PrintLine("To many files supplied");
				return;
			}
			if (CommandInput.TopCommandArguments.size() != 3)
			{
				AssociatedTerminal->PrintLine("Two files are needed to compare");
				return;
			}
			MBPM::MBPP_FileInfoReader ClientFileInfo;
			MBPM::MBPP_FileInfoReader ServerFileInfo;
			MBError RetrieveInfoError = true;
			if (RemoteFiles.size() > 0 && (RemoteFiles[0] == 3 && CommandInput.TotalCommandTokens.size() >= 5))
			{
				ClientFileInfo = p_GetRemoteFileInfo(CommandInput.TopCommandArguments[1], &RetrieveInfoError);
			}
			else if (InstalledFiles.size() > 0 && (InstalledFiles[0] == 3 && CommandInput.TotalCommandTokens.size() >= 5))
			{
				ClientFileInfo = p_GetInstalledFileInfo(CommandInput.TopCommandArguments[1],&RetrieveInfoError);
			}
			else if (PathFiles.size() > 0 && (PathFiles[0] == 3 && CommandInput.TotalCommandTokens.size() >= 5))
			{
				ClientFileInfo = p_GetPathFileInfo(CommandInput.TopCommandArguments[1],&RetrieveInfoError);
			}
			else
			{
				AssociatedTerminal->PrintLine("No client file supplied");
				return;
			}
			if (!RetrieveInfoError)
			{
				AssociatedTerminal->PrintLine("Error getting FileInfo: " + RetrieveInfoError.ErrorMessage);
				return;
			}
			if (CommandInput.TotalCommandTokens.size() >= 7 && ((RemoteFiles.size() > 0 && (RemoteFiles[0] == 5 || RemoteFiles[1] == 5)) || (RemoteFiles.size() > 2 && RemoteFiles[1] == 5)))
			{
				ServerFileInfo = p_GetRemoteFileInfo(CommandInput.TopCommandArguments[2], &RetrieveInfoError);
			}
			else if (CommandInput.TotalCommandTokens.size() >= 7 && ((InstalledFiles.size() > 0 && (InstalledFiles[0] == 5 || InstalledFiles[1] == 5)) || (InstalledFiles.size() > 2 && InstalledFiles[1] == 5)))
			{
				ServerFileInfo = p_GetInstalledFileInfo(CommandInput.TopCommandArguments[2],&RetrieveInfoError);
			}
			else if (CommandInput.TotalCommandTokens.size() >= 7 && ((PathFiles.size() > 0 && (PathFiles[0] == 5 || PathFiles[1] == 5)) || (PathFiles.size() > 2 && PathFiles[1] == 5)))
			{
				ServerFileInfo = p_GetPathFileInfo(CommandInput.TopCommandArguments[2],&RetrieveInfoError);
			}
			else
			{
				AssociatedTerminal->PrintLine("No server file supplied");
				return;
			}
			if (!RetrieveInfoError)
			{
				AssociatedTerminal->PrintLine("Error getting FileInfo: " + RetrieveInfoError.ErrorMessage);
				return;
			}
			MBPM::MBPP_FileInfoDiff InfoDiff = MBPM::GetFileInfoDifference(ClientFileInfo, ServerFileInfo);
			p_PrintFileInfoDiff(InfoDiff,AssociatedTerminal);
		}
	}
	std::string h_ReadMBPMStaticData(std::string const& Filepath)
	{
		std::string ReturnValue = "";
		size_t FileSize = std::filesystem::file_size(Filepath);
		std::ifstream FileToRead = std::ifstream(Filepath, std::ios::in|std::ios::binary);
		std::string Filedata = std::string(FileSize, 0);
		FileToRead.read(Filedata.data(), FileSize);
		Filedata = MBUtility::ReplaceAll(Filedata, "\r\n", "\n");
		std::string BeginDelimiter = "R\"1337xd(";
		size_t BeginOffset = Filedata.find(BeginDelimiter);
		if (BeginOffset != Filedata.npos)
		{
			BeginOffset += BeginDelimiter.size();
			size_t EndOffset = Filedata.find(")1337xd\"");
			if (EndOffset == Filedata.npos)
			{
				EndOffset = FileSize;
			}
			ReturnValue = Filedata.substr(BeginOffset, EndOffset - BeginOffset)+"\n";
		}
		else
		{
			ReturnValue = std::move(Filedata);
		}
		return(ReturnValue);
	}
	void MBPM_ClI::p_HandleCompile(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::vector<std::string> PacketDirectories = {};
		//skapar packet directoriesen
		bool UpdateInstalled = false;
		if (CommandInput.CommandOptions.find("allinstalled") != CommandInput.CommandOptions.end())
		{
			PacketDirectories = p_GetInstalledPacketsDependancyOrder(); // ska den göra något om vi inte har alla packet dependancys?
			for (size_t i = 0; i < PacketDirectories.size(); i++)
			{
				PacketDirectories[i] = p_GetInstalledPacketDirectory(PacketDirectories[i]);
			}
		}
		if (CommandInput.CommandOptions.find("alluser") != CommandInput.CommandOptions.end())
		{
			std::vector<std::pair<std::string, std::string>> UserPackets = p_GetUserUploadPackets();
			for (size_t i = 0; i < UserPackets.size(); i++)
			{
				PacketDirectories.push_back(UserPackets[i].second);
			}
		}
		//kanske blir att vi uppdaterar pakcets dubbelt, men är ju användarsen ansvar
		for (size_t i = 0; i < CommandInput.TopCommandArguments.size(); i++)
		{
			std::string CurrentInput = CommandInput.TopCommandArguments[i];
			size_t DotPosition = CurrentInput.find('.');
			size_t SlashPosition = CurrentInput.find('/');
			size_t BackslashPosition = CurrentInput.find('\\');
			if (p_GetInstalledPacketDirectory(CurrentInput) != "" && (SlashPosition == CurrentInput.npos && DotPosition == CurrentInput.npos && BackslashPosition == CurrentInput.npos))
			{
				PacketDirectories.push_back(p_GetInstalledPacketDirectory(CurrentInput));
			}
			else
			{
				PacketDirectories.push_back(CurrentInput);
			}
		}

		
		std::string MBPMStaticData = "";
		if (CommandInput.CommandPositionalOptions.find("sdata") != CommandInput.CommandPositionalOptions.end())
		{
			size_t DataPosition = CommandInput.CommandPositionalOptions.at("sdata")[0];
			if (CommandInput.TotalCommandTokens.size() <= DataPosition + 1)
			{
				AssociatedTerminal->PrintLine("Error with sdata option: No filepath supplied");
				return;
			}
			std::string Filepath = CommandInput.TotalCommandTokens[DataPosition + 1];
			if (!std::filesystem::exists(Filepath))
			{
				AssociatedTerminal->PrintLine("Error with sdata option: Cant't find file \"" + Filepath + "\"");
				return;
			}
			MBPMStaticData = h_ReadMBPMStaticData(Filepath);
		}
		//allinstalled är incompatible med att uppdatera packets manuellt, eftersom den gör det i dependancy ordning
		if (CommandInput.CommandOptions.find("cmake") != CommandInput.CommandOptions.end())
		{
			bool UpdateCmake = true;
			bool CompileCmake = false;
			bool OverwriteAll = false;
			if (CommandInput.CommandOptions.find("create") != CommandInput.CommandOptions.end())
			{
				CompileCmake = true;
			}
			if (CommandInput.CommandOptions.find("update") != CommandInput.CommandOptions.end())
			{
				UpdateCmake = true;
			}
			//börjar med compile sen update
			for (size_t i = 0; i < PacketDirectories.size(); i++)
			{
				std::string CurrentDirectory = PacketDirectories[i];
				if (std::filesystem::exists(CurrentDirectory + "MBPM_PacketInfo"))
				{
					MBPM::MBPM_PacketInfo CurrentPacket = MBPM::ParseMBPM_PacketInfo(CurrentDirectory + "MBPM_PacketInfo");
					if (CurrentPacket.Attributes.find(MBPM::MBPM_PacketAttribute::NonMBBuild) != CurrentPacket.Attributes.end())
					{
						//icke mb builds borde varken uppdateras eller skapas
						continue;
					}
				}
				if (CompileCmake)
				{
					bool OverWrite = true;
					if (std::filesystem::exists(CurrentDirectory + "/CMakeLists.txt") && !OverwriteAll)
					{
						AssociatedTerminal->PrintLine("Packet at location " + CurrentDirectory + " already has a CMakeLists.txt. Do you want to overwrite it? [y/n/overwriteall]");
						std::string UserResponse = "";
						while (true)
						{
							AssociatedTerminal->GetLine(UserResponse);
							if (UserResponse == "y")
							{
								OverWrite = true;
								break;
							}
							else if (UserResponse == "n")
							{
								OverWrite = false;
								break;
							}
							else if (UserResponse == "overwriteall")
							{
								OverwriteAll = true;
								break;
							}
							else
							{
								AssociatedTerminal->PrintLine("Invalid response \"" + UserResponse + "\"");
							}
						}
					}
					if (OverWrite || OverwriteAll)
					{
						MBError CreateResult = p_CreateCmake(CurrentDirectory);
						if (!CreateResult)
						{
							AssociatedTerminal->PrintLine("Failed creating CMake for \"" + CurrentDirectory + "\": " + CreateResult.ErrorMessage);
						}
					}
				}
				if (UpdateCmake)
				{
					MBError UpdateResult = p_UpdateCmake(CurrentDirectory, MBPMStaticData);//ska om packetet följer MBPM standard inte påverka packetet
					if (!UpdateResult)
					{
						AssociatedTerminal->PrintLine("Failed updating CMake for \"" + CurrentDirectory + "\": " + UpdateResult.ErrorMessage);
					}
				}
			}
		}
		else
		{
			//nu ska vi faktiskt ta och kompilera packetsen
			std::vector<std::string> FailedCompiles = {};
			for (size_t i = 0; i < PacketDirectories.size(); i++)
			{
				if (std::filesystem::exists(PacketDirectories[i] + "MBPM_PacketInfo"))
				{
					MBPM::MBPM_PacketInfo CurrentPacket = MBPM::ParseMBPM_PacketInfo(PacketDirectories[i] + "MBPM_PacketInfo");
					if (CurrentPacket.Attributes.find(MBPM::MBPM_PacketAttribute::NonMBBuild) != CurrentPacket.Attributes.end())
					{
						//icke mb builds borde varken uppdateras eller skapas
						continue;
					}
				}
				MBError CompilationError = true;
				CompilationError = p_UpdateCmake(PacketDirectories[i], MBPMStaticData);
				if (CompilationError)
				{
				 	CompilationError = p_CompilePacket(PacketDirectories[i]);
				}
				if (!CompilationError)
				{
					FailedCompiles.push_back(PacketDirectories[i]);
				}
			}
			if (FailedCompiles.size() > 0)
			{
				AssociatedTerminal->PrintLine("Failed building following packets:");
				for (size_t i = 0; i < FailedCompiles.size(); i++)
				{
					AssociatedTerminal->PrintLine(FailedCompiles[i]);
				}
			}
			else
			{
				AssociatedTerminal->PrintLine("All builds successful!");
			}
		}
	}
	MBError MBPM_ClI::p_UpdateCmake(std::string const& PacketDirectory, std::string const& OptionalMBPMStaticData)
	{
		return(MBPM::UpdateCmakeMBPMVariables(PacketDirectory,OptionalMBPMStaticData));
	}
	MBError MBPM_ClI::p_CreateCmake(std::string const& PacketDirectory)
	{
		return(MBPM::GenerateCmakeFile(PacketDirectory));
	}
	MBError MBPM_ClI::p_CompilePacket(std::string const& PacketDirectory)
	{
		return(MBPM::CompileAndInstallPacket(PacketDirectory));
	}
	void MBPM_ClI::HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		//denna är en global inställning
		if (CommandInput.CommandOptions.find("computerdiff") != CommandInput.CommandOptions.end())
		{
			m_ClientToUse.SetComputerInfo(MBPM::MBPP_Client::GetSystemComputerInfo());
		}
		if (CommandInput.CommandOptions.find("nocomputerdiff") != CommandInput.CommandOptions.end())
		{
			m_ClientToUse.SetComputerInfo(MBPM::MBPP_ComputerInfo());
		}
		m_ClientToUse.SetLogTerminal(AssociatedTerminal);
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
		else if (CommandInput.TopCommand == "upload")
		{
			p_HandleUpload(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "index")
		{
			p_HandleInfo(CommandInput, AssociatedTerminal);
		}
		else if(CommandInput.TopCommand == "compile")
		{
			p_HandleCompile(CommandInput, AssociatedTerminal);
		}
		else
		{
			AssociatedTerminal->PrintLine("Invalid command \"" + CommandInput.TopCommand + "\"");
		}
	}
	//END MBPM_CLI

	int MBCLI_Main(int argc, char** argv)
	{
		try
		{
			MBSockets::Init();
			MBCLI::ProcessedCLInput CommandInput(argc, argv);
			MBCLI::MBTerminal ProgramTerminal;
			MBPM_ClI CLIHandler;
			CLIHandler.HandleCommand(CommandInput, &ProgramTerminal);
			return(0);
		}
		catch(std::exception const& e)
		{
			std::cout << "Uncaught error in executing command: " << e.what() << std::endl;
		}
		return(0);
	}
}