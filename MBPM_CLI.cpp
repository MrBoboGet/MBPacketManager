#include "MBPM_CLI.h"
#include <filesystem>
#include <map>
#include <MBUnicode/MBUnicode.h>
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
		MBPP_PacketHost ReturnValue = { "127.0.0.1/MBPM/",MBPP_TransferProtocol::HTTPS,443 };
		return(ReturnValue);
	}

	MBPM::MBPP_FileInfoReader MBPM_ClI::p_GetRemoteFileInfo(std::string const& RemoteFile, MBError* OutError)
	{
		MBPM::MBPP_Client ClientToUse;
		ClientToUse.Connect(p_GetDefaultPacketHost());
		MBPM::MBPP_FileInfoReader ReturnValue = ClientToUse.GetPacketFileInfo(RemoteFile, OutError);
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
			AssociatedTerminal->PrintLine(DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName+" "+
				MBUtility::ReplaceAll(MBUtility::HexEncodeString((*DirectoryIterator).FileHash)," ",""));
			DirectoryIterator++;
		}

	}
	void MBPM_ClI::p_PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal)
	{
		AssociatedTerminal->PrintLine("New directories:");
		for (auto const& NewDirectories : InfoToPrint.AddedDirectories)
		{
			AssociatedTerminal->PrintLine(NewDirectories);
		}
		AssociatedTerminal->PrintLine("New files:");
		for (auto const& NewFiles : InfoToPrint.AddedFiles)
		{
			AssociatedTerminal->PrintLine(NewFiles);
		}
		AssociatedTerminal->PrintLine("Updated files:");
		for (auto const& UpdatedFiles : InfoToPrint.UpdatedFiles)
		{
			AssociatedTerminal->PrintLine(UpdatedFiles);
		}
		AssociatedTerminal->PrintLine("Removed directories:");
		for (auto const& RemovedDirectories : InfoToPrint.DeletedDirectories)
		{
			AssociatedTerminal->PrintLine(RemovedDirectories);
		}
		AssociatedTerminal->PrintLine("Removed files:");
		for (auto const& RemovedFiles : InfoToPrint.RemovedFiles)
		{
			AssociatedTerminal->PrintLine(RemovedFiles);
		}
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
		if (!PacketClient.IsConnected())
		{
			AssociatedTerminal->PrintLine("Couldn't establish connection so server");
			return;
		}
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
				GetPacketError = PacketClient.DownloadPacketFiles(OutputDirectory, PacketName, { ObjectToGet });
			}
			else if ((ObjectDirectoryInfo = PacketFileInfo.GetDirectoryInfo(ObjectToGet)) != nullptr)
			{
				GetPacketError = PacketClient.DownloadPacketDirectories(OutputDirectory, PacketName, { ObjectToGet });
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
	void MBPM_ClI::p_HandleUpload(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
	{
		//först så behöver vi skicka data för att se login typen
		if (CommandInput.TopCommandArguments.size() < 2)
		{
			AssociatedTerminal->PrintLine("Command \"upload\" requires arguments");
			return;
		}
		MBPP_UploadRequest_Response RequestResponse;
		MBPP_Client ClientToUse;
		ClientToUse.Connect(p_GetDefaultPacketHost());
		std::string PacketToUploadDirectory = CommandInput.TopCommandArguments[1];
		std::cout << "Packet to upload directory: " << PacketToUploadDirectory << std::endl;
		MBError UploadError = ClientToUse.UploadPacket(PacketToUploadDirectory, CommandInput.TopCommandArguments[0], MBPP_UserCredentialsType::Request, "",&RequestResponse);
		if (RequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
		{
			AssociatedTerminal->PrintLine("Need verification for " + RequestResponse.DomainToLogin + ":");
			//Borde egentligen kolla vilka typer den stödjer
			AssociatedTerminal->Print("Username: ");
			std::string Username;
			AssociatedTerminal->GetLine(Username);
			std::string Password;
			AssociatedTerminal->Print("Password: ");
			AssociatedTerminal->GetLine(Password);
			std::string VerificationData = MBPP_EncodeString(Username) + MBPP_EncodeString(Password);
			ClientToUse.Connect(p_GetDefaultPacketHost());
			UploadError = ClientToUse.UploadPacket(PacketToUploadDirectory, CommandInput.TopCommandArguments[0], MBPP_UserCredentialsType::Plain, VerificationData, &RequestResponse);
			if (!UploadError)
			{
				AssociatedTerminal->PrintLine("Error uploading file: " + UploadError.ErrorMessage);
			}
			else
			{
				AssociatedTerminal->PrintLine("Sucessfully uploaded packet");
			}
			
		}
		else
		{
			AssociatedTerminal->PrintLine("Upload packet sucessful");
		}
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
			if (CommandInput.TopCommandArguments.size() < 2)
			{
				AssociatedTerminal->PrintLine("no directory path provided");
				return;
			}
			std::string PathToList = CommandInput.TopCommandArguments[1];
			if (PathToList.back() != '/')
			{
				PathToList += "/";
			}
			std::string FileInfoName = "MBPM_FileInfo";
			if (CommandInput.TopCommandArguments.size() >= 3)
			{
				FileInfoName = CommandInput.TopCommandArguments[2];
			}
			CreatePacketFilesData(PathToList,FileInfoName);
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
				MBPM::MBPP_Client NewClient;
				NewClient.Connect(p_GetDefaultPacketHost());
				MBError GetFileInfoResult = true;
				MBPM::MBPP_FileInfoReader ServerInfo = NewClient.GetPacketFileInfo(CommandInput.TopCommandArguments[1], &GetFileInfoResult);
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
			//börjar med compile sen update
			for (size_t i = 0; i < PacketDirectories.size(); i++)
			{
				std::string CurrentDirectory = PacketDirectories[i];
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
						p_CreateCmake(CurrentDirectory);
					}
				}
				if (UpdateCmake)
				{
					p_UpdateCmake(CurrentDirectory);//ska om packetet följer MBPM standard inte påverka packetet
				}
			}
		}
		else
		{
			//nu ska vi faktiskt ta och kompilera packetsen
			std::vector<std::string> FailedCompiles = {};
			for (size_t i = 0; i < PacketDirectories.size(); i++)
			{
				MBError CompilationError = true;
				CompilationError = p_UpdateCmake(PacketDirectories[i]);
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
	MBError MBPM_ClI::p_UpdateCmake(std::string const& PacketDirectory)
	{
		return(MBPM::UpdateCmakeMBPMVariables(PacketDirectory));
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
		MBSockets::Init();
		MBCLI::ProcessedCLInput CommandInput(argc, argv);
		MBCLI::MBTerminal ProgramTerminal;
		MBPM_ClI CLIHandler;
		CLIHandler.HandleCommand(CommandInput, &ProgramTerminal);
		return(0);
	}
}