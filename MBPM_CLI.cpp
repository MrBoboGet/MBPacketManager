#include "MBPM_CLI.h"
#include <filesystem>
#include <map>
#include <MBUnicode/MBUnicode.h>
#include <exception>
#include <stdexcept>
//#include "MBCLI/"

namespace MBPM
{
	//BEGIN MBPM_CLI
	std::string MBPM_ClI::p_GetPacketInstallDirectory()
	{
		const char* Data = std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY");
		if (Data == nullptr)
		{
			throw std::runtime_error("MBPM_PACKETS_INSTALL_DIRECTORY environment variable is not set");
		}
		std::string ReturnValue = Data;
		if (ReturnValue.back() != '/')
		{
			ReturnValue += "/";
		}
		return(ReturnValue);
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
	std::vector<std::pair<std::string, std::string>> MBPM_ClI::p_GetUserInstalledPackets()
	{
		std::vector<std::pair<std::string, std::string>> ReturnValue = {};
		std::string InstalledPacketsDirectory = p_GetPacketInstallDirectory();
		if (std::filesystem::exists(InstalledPacketsDirectory))
		{
			std::filesystem::directory_iterator DirectoryIterator = std::filesystem::directory_iterator(InstalledPacketsDirectory);
			for (auto const& Entry : DirectoryIterator)
			{
				if (Entry.is_directory())
				{
					if (std::filesystem::exists(Entry.path() / "MBPM_PacketInfo"))
					{
						std::string PacketPath = MBUnicode::PathToUTF8(Entry.path())+"/";
						MBPM_PacketInfo CurrentPacketInfo = ParseMBPM_PacketInfo(PacketPath+"MBPM_PacketInfo");
						if (CurrentPacketInfo.PacketName != "")
						{
							ReturnValue.push_back({ CurrentPacketInfo.PacketName ,PacketPath});
						}
					}
				}
			}
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
		std::vector<std::string> FilePaths = {};
		size_t LargestPath = 0;
		std::vector<std::string> FileSizes = {};
		size_t LargestSize = 0;
		std::vector<std::string> Hashes = {};
		size_t LargestHash = 0;
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
			//förutsätter såklart ascii B)
			if (Filename.size() > LargestPath)
			{
				LargestPath = Filename.size();
			}
			if (FileSizeString.size() > LargestSize)
			{
				LargestSize = FileSizeString.size();
			}
			//AssociatedTerminal->PrintLine(Filename+"\t\t\t\t"+FileSizeString+"\t"+ FileHash);
			FilePaths.push_back(Filename);
			FileSizes.push_back(FileSizeString);
			Hashes.push_back(FileHash);
			DirectoryIterator++;
		}
		for (size_t i = 0; i < FilePaths.size(); i++)
		{
			std::cout << FilePaths[i] << std::string(LargestPath - FilePaths[i].size(), ' ') << std::string(4, ' ');
			std::cout << FileSizes[i] << std::string(LargestSize - FileSizes[i].size(), ' ') << std::string(4, ' ');
			std::cout << Hashes[i] << std::endl;
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
	void MBPM_ClI::p_HandleHelp(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		if (CommandInput.TopCommand == "")
		{
			AssociatedTerminal->PrintLine("mbpm is packet manager that installs user defined packets, consisting of source needed");
			AssociatedTerminal->PrintLine("to compile the library, and depending on platform precompiled libraries. A packet may");
			AssociatedTerminal->PrintLine("also export executables. All installed packets are located under the folder pointed to by");
			AssociatedTerminal->PrintLine("the MBPM_PACKETS_INSTALL_DIRECTORY environemnt variable, and all compiled libraries and executables have");
			AssociatedTerminal->PrintLine("symlinks located under MBPM_PACKETS_INSTALL_DIRECTORY/MBPM_CompiledLibraries/ and");
			AssociatedTerminal->PrintLine("MBPM_PACKETS_INSTALL_DIRECTORY/MBPM_ExportedExecutables respectively.");
			AssociatedTerminal->PrintLine("-----------------------------------------------------------------------");
			AssociatedTerminal->PrintLine("Usage of mbpm:");
			AssociatedTerminal->PrintLine("All commands consist of top level command identified by the first input string not starting with -");
			AssociatedTerminal->PrintLine("followed by additional top command arguments. Options without arguments start with --");
			AssociatedTerminal->PrintLine("and options that have arguments start with - and expect the argument to be the next following");
			AssociatedTerminal->PrintLine("input strings");
			AssociatedTerminal->PrintLine("Global options:");
			AssociatedTerminal->PrintLine("--computerdiff: Makes the command work on the filesystem that differs depending on platform, some commands");
			AssociatedTerminal->PrintLine("                set or unsets this flag automatically like install.");
			AssociatedTerminal->PrintLine("--nocomputerdiff: Makes the command work on the filesystem that is the same for each packet. Only meaningful");
			AssociatedTerminal->PrintLine("                  in conjunction with install that automatically sets the computerdiff");
			AssociatedTerminal->PrintLine("Top level commands:");
			p_HandleUpdate_Help(CommandInput, AssociatedTerminal);
			AssociatedTerminal->PrintLine("-----------------------------------------------------------------------");
			p_HandleInstall_Help(CommandInput, AssociatedTerminal);
			AssociatedTerminal->PrintLine("-----------------------------------------------------------------------");
			p_HandleGet_Help(CommandInput, AssociatedTerminal);
			AssociatedTerminal->PrintLine("-----------------------------------------------------------------------");
			p_HandleUpload_Help(CommandInput, AssociatedTerminal);
			AssociatedTerminal->PrintLine("-----------------------------------------------------------------------");
			p_HandleIndex_Help(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "update")
		{
			p_HandleUpdate_Help(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "install")
		{
			p_HandleInstall_Help(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "get")
		{
			p_HandleGet_Help(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "upload")
		{
			p_HandleUpload_Help(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "index")
		{
			p_HandleIndex_Help(CommandInput, AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "compile")
		{
			p_HandleCompile_Help(CommandInput, AssociatedTerminal);
		}
		else
		{
			AssociatedTerminal->PrintLine("Unrecognized command: " + CommandInput.TopCommand);
		}
	}
	void MBPM_ClI::p_HandleUpdate_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("update:");
		AssociatedTerminal->PrintLine("Command that updates already installed packtets with the current packet on the remote server");
		AssociatedTerminal->PrintLine("Arguments:");
		AssociatedTerminal->PrintLine("Each top command argument is interpreted as the name of a packet to update located under");
		AssociatedTerminal->PrintLine("MBPM_PACKETS_INSTALL_DIRECTORY");
		AssociatedTerminal->PrintLine("Options:");
		AssociatedTerminal->PrintLine("--all: Updates all installed packets in their dependancy order");
	}
	void MBPM_ClI::p_HandleInstall_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("install:");
		AssociatedTerminal->PrintLine("Command that installs a packet under MBPM_PACKETS_INSTALL_DIRECTORY");
		AssociatedTerminal->PrintLine("unlike the get command that only download the files so is the install");
		AssociatedTerminal->PrintLine("command responsible for installing all dependancies and compiling the packet if required.");
		AssociatedTerminal->PrintLine("Installing a already installed packets doesn't do anything.");
		AssociatedTerminal->PrintLine("Arguments:");
		AssociatedTerminal->PrintLine("list of packets to install from remote");
		AssociatedTerminal->PrintLine("Options:");
		AssociatedTerminal->PrintLine("--nocomputerdiff: Makes so the install command doesn't download pre-built binaries");
	}
	void MBPM_ClI::p_HandleGet_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("get:");
		AssociatedTerminal->PrintLine("Command that download individual files or directories from a packet");
		AssociatedTerminal->PrintLine("Note: getting a file and directory have the same syntax, getting");
		AssociatedTerminal->PrintLine("/ for example downlaod the whole packet.");
		AssociatedTerminal->PrintLine("Note: all filesystem objects start with /");
		AssociatedTerminal->PrintLine("Arguments:");
		AssociatedTerminal->PrintLine("[0]: Packet to install");
		AssociatedTerminal->PrintLine("[1]: filesystem object to download");
	}
	void MBPM_ClI::p_HandleUpload_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("upload:");
		AssociatedTerminal->PrintLine("Command that uploads that uploads and replaces the packet stored on a server");
		AssociatedTerminal->PrintLine("Note: specifying --computerdiff uploads all the files to a paralell filesystem to the packet");
		AssociatedTerminal->PrintLine("      associated with the current computers operating system proccessor and so on. The flag");
		AssociatedTerminal->PrintLine("      should therefore only be used specifying specific files, for example files under /MBPM_Builds");
		AssociatedTerminal->PrintLine("      the whole packet source may be out of sync from computer to computer");
		AssociatedTerminal->PrintLine("Arguments:");
		AssociatedTerminal->PrintLine("[0]: Remote packet to update");
		AssociatedTerminal->PrintLine("[1]: Local directory containing the updated packet");
		AssociatedTerminal->PrintLine("Options:");
		AssociatedTerminal->PrintLine("--user: changes the meaning of the command arguments to specify the names of locally stored packets");
		AssociatedTerminal->PrintLine("        located on a path under the paths listed in MBPM_PACKETS_INSTALL_DIRECTORY/LocalUploadPackets.txt");
		AssociatedTerminal->PrintLine("--installed: changes the meaning of the command arguments to specify the names of installed packets");
		AssociatedTerminal->PrintLine("--all:  Uploads all user packets stored under a path listed in MBPM_PACKETS_INSTALL_DIRECTORY/LocalUploadPackets.txt");
		AssociatedTerminal->PrintLine("--allinstalled:  Uploads all installed packets");
		AssociatedTerminal->PrintLine("-d (PacketDirectory): For each packet to upload, instead of uploading the complete diff it uploads the specific");
		AssociatedTerminal->PrintLine("                       directory relative to the packets directory. Can for example be used to upload all compiled");
		AssociatedTerminal->PrintLine("                       binaries under MBPM_Builds/");
		AssociatedTerminal->PrintLine("-f (PacketFile): For each packet to upload, instead of uploading the complete diff it uploads the specific");
		AssociatedTerminal->PrintLine("                 file relative to the packets directory.");
	}
	void MBPM_ClI::p_HandleIndex_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("index:");
		AssociatedTerminal->PrintLine("Top command used to invoke several subcommands associated with the the index MBPM_FileInfo");
		AssociatedTerminal->PrintLine("Note: MBPM_FileInfoIgnore specifies files and directories that is not to be a part of the index and packet");
		AssociatedTerminal->PrintLine("Arguments:");
		AssociatedTerminal->PrintLine("Case [0] == create: creates a index file for directory [1] with optional named specified by [2]");
		AssociatedTerminal->PrintLine("                    Defaults to MBPM_FileInfo.");
		AssociatedTerminal->PrintLine("Case [0] == list:   Lists the contents of the index file specified by [1] or [1]/MBPM_FileInfo");
		AssociatedTerminal->PrintLine("                    if [1] is a directory");
		AssociatedTerminal->PrintLine("     Options:  ");
		AssociatedTerminal->PrintLine("     --remote: lists the content of the index file on a remote packet identified by [1]");
		AssociatedTerminal->PrintLine("     --installed: lists the content of locally installed packet identified by it's packet name [1]");
		AssociatedTerminal->PrintLine("Case [1] == diff: Lists the difference between the index files for 2 packets");
		AssociatedTerminal->PrintLine("                  -p specifies a local directory, and -r packet on a server by name");
		AssociatedTerminal->PrintLine("                  for example: mbpm index diff -p ./ -r MBPacketManager");
	}
	void MBPM_ClI::p_HandleCompile_Help(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("compile:");
		AssociatedTerminal->PrintLine("Command responsible for compiling packets, and for updating and creating CMake files following");
		AssociatedTerminal->PrintLine("the MBPM systems");
		AssociatedTerminal->PrintLine("Arguments:");
		AssociatedTerminal->PrintLine("[..]: List of packets to compile. A packet starting with . or / is assumed to point a directory, and");
		AssociatedTerminal->PrintLine("      otherwise assumed to be the name of a installed packet");
		AssociatedTerminal->PrintLine("Options:");
		AssociatedTerminal->PrintLine("--cmake: Instead of compiling only updates the MBPM variables of packets that are declared to use a MBBuild");
		AssociatedTerminal->PrintLine("--create: Only works in conjunction with --cmake. Instead of updating the MBPM variables of cmake files in");
		AssociatedTerminal->PrintLine("          the directories specified so is a CMakeLists.txt created following the MBPM system, that adds all");
		AssociatedTerminal->PrintLine("          .cpp and .h files in the directory as a part of the build");
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
		MBPM::MBPP_ComputerInfo PreviousInfo = m_ClientToUse.GetComputerInfo();
		if (CommandInput.CommandOptions.find("nocomputerdiff") != CommandInput.CommandOptions.end())
		{
			m_ClientToUse.SetComputerInfo(MBPM::MBPP_ComputerInfo());
		}
		else
		{
			m_ClientToUse.SetComputerInfo(MBPM::MBPP_Client::GetSystemComputerInfo());
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
					if (CurrentDirectory.front() != '/')
					{
						AssociatedTerminal->PrintLine("Invalid directory specification: needs to be an absolute path starting with / relative to packet directory");
						return;
					}
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
	void MBPM_ClI::p_UploadPacketsLocal(std::vector<std::pair<std::string, std::string>> const& PacketsToUpload,MBCLI::MBTerminal* AssociatedTerminal)
	{
		std::string InstalledPacketsDirectory = GetSystemPacketsDirectory();
		std::vector<std::pair<std::string, std::string>> FailedPackets = {};
		for (size_t i = 0; i < PacketsToUpload.size(); i++)
		{
			if (std::filesystem::exists(PacketsToUpload[i].second+"/MBPM_PacketInfo") == false)
			{
				FailedPackets.push_back(std::pair<std::string,std::string>(PacketsToUpload[i].first, "Invalid user packet to upload, No MBPM_PacketInfo"));
			}
			std::string PacketInstallPath = InstalledPacketsDirectory + PacketsToUpload[i].first + "/";
			if (std::filesystem::exists(PacketInstallPath) == false)
			{
				std::filesystem::create_directories(PacketInstallPath);
				MBPM::CreatePacketFilesData(PacketInstallPath);
			}
			//OBS antar här att packets inte är up to date, vi kan om vi vill optimera göra det men blir relevant när det blir relevant
			MBPM::CreatePacketFilesData(PacketInstallPath); // redundant med det ovan men men
			MBPM::CreatePacketFilesData(PacketsToUpload[i].second);
			MBPM::MBPP_FileInfoReader InstalledInfo = MBPM::MBPP_FileInfoReader(PacketInstallPath+"/MBPM_FileInfo");
			MBPM::MBPP_FileInfoReader LocalPacketInfo = MBPM::MBPP_FileInfoReader(PacketsToUpload[i].second+"/MBPM_FileInfo");
			std::string LocalPacketPath = PacketsToUpload[i].second;
			MBPM::MBPP_FileInfoDiff FileInfoDiff = MBPM::GetFileInfoDifference(InstalledInfo, LocalPacketInfo);
			AssociatedTerminal->PrintLine("Files to copy:");
			AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Green);
			std::vector<std::string> ObjectsToCopy = {};
			std::vector<std::string> ObjectsToRemove = {};
			for (size_t i = 0; i < FileInfoDiff.AddedFiles.size(); i++)
			{
				AssociatedTerminal->PrintLine(FileInfoDiff.AddedFiles[i]);
				ObjectsToCopy.push_back(FileInfoDiff.AddedFiles[i]);
			}
			for (size_t i = 0; i < FileInfoDiff.UpdatedFiles.size(); i++)
			{
				AssociatedTerminal->PrintLine(FileInfoDiff.UpdatedFiles[i]);
				ObjectsToCopy.push_back(FileInfoDiff.UpdatedFiles[i]);
			}
			AssociatedTerminal->PrintLine("Directories to copy:");
			for (size_t i = 0; i < FileInfoDiff.AddedDirectories.size(); i++)
			{
				AssociatedTerminal->PrintLine(FileInfoDiff.AddedDirectories[i]);
				ObjectsToCopy.push_back(FileInfoDiff.AddedDirectories[i]);
			}
			AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
			AssociatedTerminal->PrintLine("Files to remove:");
			AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
			for (size_t i = 0; i < FileInfoDiff.RemovedFiles.size(); i++)
			{
				AssociatedTerminal->PrintLine(FileInfoDiff.RemovedFiles[i]);
				ObjectsToRemove.push_back(FileInfoDiff.RemovedFiles[i]);
			}
			AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
			AssociatedTerminal->PrintLine("Directories to remove:");
			AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
			for (size_t i = 0; i < FileInfoDiff.DeletedDirectories.size(); i++)
			{
				AssociatedTerminal->PrintLine(FileInfoDiff.DeletedDirectories[i]);
				ObjectsToRemove.push_back(FileInfoDiff.RemovedFiles[i]);
			}
			AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
			AssociatedTerminal->PrintLine("Copying files:");
			std::filesystem::copy_options CopyOptions = std::filesystem::copy_options::update_existing|std::filesystem::copy_options::recursive;
			for (size_t i = 0; i < ObjectsToCopy.size(); i++)
			{
				std::filesystem::copy(LocalPacketPath + ObjectsToCopy[i], PacketInstallPath + ObjectsToCopy[i], CopyOptions);
				AssociatedTerminal->PrintLine("Copied " + ObjectsToCopy[i]);
			}
			AssociatedTerminal->PrintLine("Removing objects:");
			for (size_t i = 0; i < ObjectsToRemove.size(); i++)
			{
				std::filesystem::remove_all(PacketInstallPath + ObjectsToRemove[i]);
				AssociatedTerminal->PrintLine("Removed " + ObjectsToCopy[i]);
			}
			MBPM::CreatePacketFilesData(PacketInstallPath);
			AssociatedTerminal->PrintLine("Successfully uploaded packet " + PacketsToUpload[i].second + "!");
		}
		if (FailedPackets.size() > 0)
		{
			AssociatedTerminal->PrintLine("Failed uploading following packets:");
		}
		for (size_t i = 0; i < FailedPackets.size(); i++)
		{
			AssociatedTerminal->PrintLine(FailedPackets[i].first + ": " + FailedPackets[i].second);
		}
		AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
	}
	void MBPM_ClI::p_HandleUpload(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		//först så behöver vi skicka data för att se login typen
		MBPM::MBPP_ComputerInfo PreviousComputerInfo = m_ClientToUse.GetComputerInfo();
		if (CommandInput.TopCommandArguments.size() < 2 && CommandInput.CommandOptions.find("all") == CommandInput.CommandOptions.end() && CommandInput.CommandOptions.find("user") 
			== CommandInput.CommandOptions.end() && CommandInput.CommandOptions.find("allinstalled") == CommandInput.CommandOptions.end())
		{
			AssociatedTerminal->PrintLine("Invalid \"upload\" arguments, requires packet to upload and packet directory");
			return;
		}
		std::vector<std::pair<std::string, std::string>> PacketsToUpload = {};//packet name + packet directory
		if (CommandInput.CommandOptions.find("all") != CommandInput.CommandOptions.end())
		{
			PacketsToUpload = p_GetUserUploadPackets();
		}
		if (CommandInput.TopCommandArguments.size() >= 2 && CommandInput.CommandOptions.find("user") == CommandInput.CommandOptions.end() &&
			CommandInput.CommandOptions.find("installed") == CommandInput.CommandOptions.end())
		{
			PacketsToUpload.push_back(std::pair<std::string, std::string>(CommandInput.TopCommandArguments[0], CommandInput.TopCommandArguments[1]));
		}
		if (CommandInput.CommandOptions.find("user") != CommandInput.CommandOptions.end() || CommandInput.CommandOptions.find("installed") != CommandInput.CommandOptions.end())
		{
			//std::vector<std::pair<std::string, std::string>> UserPackets = p_GetUserUploadPackets();
			//std::vector<std::pair<std::string, std::string>> InstalledPacket = p_GetUserInstalledPackets();
			
			std::vector<std::string> SupportedOptions = { "user","installed" };

			for (auto const& CurrentOption : SupportedOptions)
			{
				std::vector<std::pair<std::string, std::string>> PacketsToSearch = {};
				if (CurrentOption == "user")
				{
					if (CommandInput.CommandOptions.find("user") != CommandInput.CommandOptions.end())
					{
						PacketsToSearch = p_GetUserUploadPackets();
					}
					else
					{
						continue;
					}
				}
				if (CurrentOption == "installed")
				{
					if (PacketsToSearch.size() > 0)
					{
						AssociatedTerminal->PrintLine("Can only specify user or installed not both");
						return;
					}
					if (CommandInput.CommandOptions.find("installed") != CommandInput.CommandOptions.end())
					{
						PacketsToSearch = p_GetUserInstalledPackets();
					}
					else
					{
						continue;
					}
				}
				for (size_t i = 0; i < CommandInput.TopCommandArguments.size(); i++)
				{
					bool PacketFound = false;
					std::string CurrentPacket = CommandInput.TopCommandArguments[i];
					bool SkipPacket = false;
					for (auto const& Options : CommandInput.CommandPositionalOptions)
					{
						for (size_t Position : Options.second)
						{
							if (Position + 1 < CommandInput.TotalCommandTokens.size() && CommandInput.TotalCommandTokens[Position + 1] == CurrentPacket)
							{
								SkipPacket = true;
								continue;
							}
						}
					}
					if (SkipPacket)
					{
						continue;
					}
					for (size_t j = 0; j < PacketsToSearch.size(); j++)
					{
						if (PacketsToSearch[j].first == CommandInput.TopCommandArguments[i])
						{
							PacketFound = true;
							PacketsToUpload.push_back(PacketsToSearch[j]);
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
		}
		if (CommandInput.CommandOptions.find("allinstalled") != CommandInput.CommandOptions.end())
		{
			std::vector<std::pair<std::string, std::string>> InstalledPackets = p_GetUserInstalledPackets();
			for (size_t i = 0; i < InstalledPackets.size(); i++)
			{
				PacketsToUpload.push_back(InstalledPackets[i]);
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
		if (CommandInput.CommandOptions.find("local") != CommandInput.CommandOptions.end())
		{
			p_UploadPacketsLocal(PacketsToUpload,AssociatedTerminal);
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
			while (RequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
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
				//m_ClientToUse.Connect(p_GetDefaultPacketHost());


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
	void MBPM_ClI::p_HandleIndex(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
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
			std::vector<std::pair<std::string,std::string>> FailedCompiles = {};
			for (size_t i = 0; i < PacketDirectories.size(); i++)
			{
				bool MBBuild = true;
				if (std::filesystem::exists(PacketDirectories[i] + "MBPM_PacketInfo"))
				{
					MBPM::MBPM_PacketInfo CurrentPacket = MBPM::ParseMBPM_PacketInfo(PacketDirectories[i] + "MBPM_PacketInfo");
					if (CurrentPacket.Attributes.find(MBPM::MBPM_PacketAttribute::NonMBBuild) != CurrentPacket.Attributes.end())
					{
						//icke mb builds borde varken uppdateras eller skapas
						if (!std::filesystem::exists(PacketDirectories[i] + "/MBPM_CompileInfo.json"))
						{
							continue;
						}
						else
						{
							MBBuild = false;
						}
					}
				}
				MBError CompilationError = true;
				if (MBBuild)
				{
					CompilationError = p_UpdateCmake(PacketDirectories[i], MBPMStaticData);
				}
				if (CompilationError)
				{
				 	CompilationError = p_CompilePacket(PacketDirectories[i]);
				}
				if (!CompilationError)
				{
					FailedCompiles.push_back(std::pair<std::string,std::string>(PacketDirectories[i],CompilationError.ErrorMessage));
				}
			}
			if (FailedCompiles.size() > 0)
			{
				AssociatedTerminal->PrintLine("Failed building following packets:");
				for (size_t i = 0; i < FailedCompiles.size(); i++)
				{
					AssociatedTerminal->Print(FailedCompiles[i].first);
					if (FailedCompiles[i].second != "")
					{
						AssociatedTerminal->PrintLine(" : " + FailedCompiles[i].second);
					}
					else
					{
						AssociatedTerminal->PrintLine("");
					}
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
		if (CommandInput.CommandOptions.find("help") != CommandInput.CommandOptions.end())
		{
			p_HandleHelp(CommandInput,AssociatedTerminal);
		}
		else if (CommandInput.TopCommand == "install")
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
			p_HandleIndex(CommandInput, AssociatedTerminal);
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