#include "MB_PacketProtocol.h"
#include <MBParsing/MBParsing.h>
#include <set>
#include <MBCrypto/MBCrypto.h>
#include <MBAlgorithms.h>
namespace MBPM
{
	void h_WriteBigEndianInteger(std::ofstream& FileToWriteTo, uintmax_t IntegerToWrite,uint8_t IntegerSize)
	{
		char BigEndianData[sizeof(uintmax_t)];
		for (size_t i = 0; i < IntegerSize; i++)
		{
			BigEndianData[i] = (IntegerToWrite >> ((IntegerSize * 8) - ((i + 1) * 8)));
		}
		FileToWriteTo.write(BigEndianData, IntegerSize);
	}
	void h_WriteBigEndianInteger(void* Buffer, uintmax_t IntegerToWrite, uint8_t IntegerSize)
	{
		char* CharData = (char*)Buffer;
		for (size_t i = 0; i < IntegerSize; i++)
		{
			CharData[i] = (IntegerToWrite >> ((IntegerSize * 8) - ((i + 1) * 8)));		
		}
	}
	uintmax_t h_ReadBigEndianInteger(std::ifstream& FileToWriteFrom, uint8_t IntegerSize)
	{
		uintmax_t ReturnValue = 0;
		uint8_t ByteBuffer[sizeof(uintmax_t)];
		FileToWriteFrom.read((char*) ByteBuffer, IntegerSize);
		for (size_t i = 0; i < IntegerSize; i++)
		{
			ReturnValue <<= 8;
			ReturnValue += ByteBuffer[i];
		}
		return(ReturnValue);
	}
	std::string h_WriteFileData(std::ofstream& FileToWriteTo, std::filesystem::path const& FilePath,MBCrypto::HashFunction HashFunctionToUse)
	{
		//TODO fixa så det itne blir fel på windows
		std::string FileName = FilePath.filename().generic_u8string();
		h_WriteBigEndianInteger(FileToWriteTo, FileName.size(), 2);
		FileToWriteTo.write(FileName.data(), FileName.size());
		std::string FileHash = MBCrypto::GetFileHash(FilePath.generic_u8string(),HashFunctionToUse);
		FileToWriteTo.write(FileHash.data(), FileHash.size());
		return(FileHash);
	}
	std::string h_WriteDirectoryData_Recursive(std::ofstream& FileToWriteTo, std::string const& DirectoryName,std::string const& SubPathToIterate,
		std::set<std::string>const& ExcludedFiles,MBCrypto::HashFunction HashFunctionToUse)
	{
		std::set<std::string> DirectoriesToWrite = {};
		std::set<std::filesystem::path> FilesToWrite = {};
		std::filesystem::directory_iterator DirectoryIterator = std::filesystem::directory_iterator(SubPathToIterate);
		for (auto const& Entries : DirectoryIterator)
		{
			if (Entries.is_regular_file() && ExcludedFiles.find(Entries.path().filename().generic_u8string()) == ExcludedFiles.end())
			{
				FilesToWrite.insert(Entries.path());
			}
			else if(Entries.is_directory())
			{
				DirectoriesToWrite.insert(Entries.path().generic_u8string());
			}
		}
		uint64_t SkipPointerPosition = FileToWriteTo.tellp();
		h_WriteBigEndianInteger(FileToWriteTo, 0, 8); //offset innan vi en läst filen
		//plats för directory hashet
		char EmptyHashData[20]; //TODO hardcoda inte hash längden...
		FileToWriteTo.write(EmptyHashData, 20);

		//ANTAGANDE pathen är inte på formen /något/hej/ utan /något/hej
		h_WriteBigEndianInteger(FileToWriteTo, DirectoryName.size(), 2);
		FileToWriteTo.write(DirectoryName.data(), DirectoryName.size());
		h_WriteBigEndianInteger(FileToWriteTo, FilesToWrite.size(), 4);
		std::string DataToHash = "";
		for (auto const& Files : FilesToWrite)
		{
			DataToHash += h_WriteFileData(FileToWriteTo, Files, HashFunctionToUse);
		}
		h_WriteBigEndianInteger(FileToWriteTo, DirectoriesToWrite.size(), 4);
		for (auto const& Directories : DirectoriesToWrite)
		{
			DataToHash += h_WriteDirectoryData_Recursive(FileToWriteTo, std::filesystem::path(Directories).filename().generic_u8string(), Directories, ExcludedFiles, HashFunctionToUse);
		}
		uint64_t DirectoryEndPosition = FileToWriteTo.tellp();
		FileToWriteTo.seekp(SkipPointerPosition);
		h_WriteBigEndianInteger(FileToWriteTo, DirectoryEndPosition, 8);
		std::string DirectoryHash = MBCrypto::HashData(DataToHash, HashFunctionToUse);
		FileToWriteTo.write(DirectoryHash.data(), DirectoryHash.size());
		FileToWriteTo.seekp(DirectoryEndPosition);
		return(DirectoryHash);
	}
	void CreatePacketFilesData(std::string const& PacketToHashDirectory,std::string const& OutputName)
	{
		std::ofstream OutputFile = std::ofstream(PacketToHashDirectory + OutputName,std::ios::out|std::ios::binary);
		if (!OutputFile.is_open())
		{
			assert(false);
		}
		std::set<std::string> ExcludedFiles = {"MBPM_FileInfo"};
		h_WriteDirectoryData_Recursive(OutputFile, "/", PacketToHashDirectory, ExcludedFiles, MBCrypto::HashFunction::SHA1);
		OutputFile.flush();
		OutputFile.close();
	}

	std::string MBPP_EncodeString(std::string const& StringToEncode)
	{
		return("");
	}
	std::string MBPP_GetRecordData(MBPP_GenericRecord const& RecordToConvert)
	{
		return("");
	}

	//ANTAGANDE filer är sorterade i ordning
	//BEGIN MBPP_FileInfoReader
	MBPP_DirectoryInfoNode MBPP_FileInfoReader::p_ReadDirectoryInfoFromFile(std::ifstream& FileToReadFrom,size_t HashSize)
	{
		MBPP_DirectoryInfoNode ReturnValue;
		h_ReadBigEndianInteger(FileToReadFrom, 8);//filepointer
		std::string DirectoryHash = std::string(HashSize, 0);
		FileToReadFrom.read(DirectoryHash.data(), HashSize);
		size_t NameSize = h_ReadBigEndianInteger(FileToReadFrom, 2);
		std::string DirectoryName = std::string(NameSize, 0);
		FileToReadFrom.read(DirectoryName.data(), NameSize);
		uint32_t NumberOfFiles = h_ReadBigEndianInteger(FileToReadFrom, 4);
		for (size_t i = 0; i < NumberOfFiles; i++)
		{
			ReturnValue.Files.push_back(p_ReadFileInfoFromDFile(FileToReadFrom, HashSize));
		}
		uint32_t NumberOfDirectories = h_ReadBigEndianInteger(FileToReadFrom, 4);
		for (size_t i = 0; i < NumberOfDirectories; i++)
		{
			ReturnValue.Directories.push_back(p_ReadDirectoryInfoFromFile(FileToReadFrom, HashSize));
		}
		ReturnValue.DirectoryHash = std::move(DirectoryHash);
		ReturnValue.DirectoryName = std::move(DirectoryName);
		return(ReturnValue);
	}
	MBPP_FileInfo MBPP_FileInfoReader::p_ReadFileInfoFromDFile(std::ifstream& FileToReadFrom,size_t HashSize)
	{
		MBPP_FileInfo ReturnValue;
		uint16_t FileNameSize = h_ReadBigEndianInteger(FileToReadFrom, 2);
		ReturnValue.FileName = std::string(FileNameSize, 0);
		FileToReadFrom.read(ReturnValue.FileName.data(), FileNameSize);
		ReturnValue.FileHash = std::string(HashSize, 0);
		FileToReadFrom.read(ReturnValue.FileHash.data(), HashSize);
		return(ReturnValue);
	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(std::string const& FileInfoPath)
	{
		std::ifstream PacketFileInfo = std::ifstream(FileInfoPath, std::ios::in | std::ios::binary);
		if (!PacketFileInfo.is_open())
		{
			return;
		}
		m_TopNode = p_ReadDirectoryInfoFromFile(PacketFileInfo,20);//sha1 diges size
	}
	void h_UpdateDiffDirectoryFiles(MBPP_FileInfoDiff& DiffToUpdate, MBPP_DirectoryInfoNode const& ClientDirectory, MBPP_DirectoryInfoNode const& ServerDirectory, std::string const& CurrentPath)
	{
		std::vector<MBPP_FileInfo>::const_iterator ClientEnd = ClientDirectory.Files.end();
		std::vector<MBPP_FileInfo>::const_iterator ServerEnd = ServerDirectory.Files.end();
		std::vector<MBPP_FileInfo>::const_iterator ClientIterator = ClientDirectory.Files.begin();
		std::vector<MBPP_FileInfo>::const_iterator ServerIterator = ServerDirectory.Files.begin();
		while (ClientIterator != ClientEnd && ServerIterator != ServerEnd)
		{
			if (*ClientIterator < *ServerIterator)
			{
				DiffToUpdate.RemovedFiles.push_back(CurrentPath + (*ClientIterator).FileName);
				ClientIterator++;
			}
			else if (*ServerIterator < *ClientIterator)
			{
				DiffToUpdate.AddedFiles.push_back(CurrentPath + ServerIterator->FileName);
				ServerIterator++;
			}
			else
			{
				//dem är lika
				if (ClientIterator->FileHash != ServerIterator->FileHash)
				{
					DiffToUpdate.UpdatedFiles.push_back(CurrentPath + ServerIterator->FileName);
				}
				ClientIterator++;
				ServerIterator++;
			}
		}
		while (ClientIterator != ClientEnd)
		{
			DiffToUpdate.RemovedFiles.push_back(CurrentPath+ClientIterator->FileName);
			ClientIterator++;
		}
		while (ServerIterator != ServerEnd)
		{
			DiffToUpdate.AddedFiles.push_back(CurrentPath+ServerIterator->FileName);
			ServerIterator++;
		}
	}
	void h_UpdateDiffOverDirectory(MBPP_FileInfoDiff& DiffToUpdate, MBPP_DirectoryInfoNode const& ClientDirectory, MBPP_DirectoryInfoNode const& ServerDirectory,std::string const& CurrentPath)
	{
		//compare files
		h_UpdateDiffDirectoryFiles(DiffToUpdate, ClientDirectory, ServerDirectory, CurrentPath);
		//
		std::vector<MBPP_DirectoryInfoNode>::const_iterator ClientEnd = ClientDirectory.Directories.end();
		std::vector<MBPP_DirectoryInfoNode>::const_iterator ServerEnd = ServerDirectory.Directories.end();
		std::vector<MBPP_DirectoryInfoNode>::const_iterator ClientIterator = ClientDirectory.Directories.begin();
		std::vector<MBPP_DirectoryInfoNode>::const_iterator ServerIterator = ServerDirectory.Directories.begin();
		while (ClientIterator != ClientEnd && ServerIterator != ServerEnd)
		{
			if (*ClientIterator < *ServerIterator)
			{
				DiffToUpdate.DeletedDirectories.push_back(CurrentPath+ClientIterator->DirectoryName);
				ClientIterator++;
			}
			else if (*ServerIterator < *ClientIterator)
			{
				DiffToUpdate.AddedDirectories.push_back(CurrentPath+ServerIterator->DirectoryName);
				ServerIterator++;
			}
			else
			{
				//dem är lika
				if (ClientIterator->DirectoryHash != ServerIterator->DirectoryHash)
				{
					h_UpdateDiffOverDirectory(DiffToUpdate, *ClientIterator, *ServerIterator, CurrentPath + ClientIterator->DirectoryName + "/");
				}
				ClientIterator++;
				ServerIterator++;
			}
		}
		while (ClientIterator != ClientEnd)
		{
			DiffToUpdate.DeletedDirectories.push_back(CurrentPath+ClientIterator->DirectoryName);
			ClientIterator++;
		}
		while (ServerIterator != ServerEnd)
		{
			DiffToUpdate.AddedDirectories.push_back(CurrentPath+ServerIterator->DirectoryName);
			ServerIterator++;
		}
	}
	MBPP_FileInfoDiff GetFileInfoDifference(MBPP_FileInfoReader const& ClientInfo, MBPP_FileInfoReader const& ServerInfo)
	{
		MBPP_FileInfoDiff ReturnValue;
		MBPP_DirectoryInfoNode const& ClientNode = ClientInfo.m_TopNode;
		MBPP_DirectoryInfoNode const& ServerNode = ServerInfo.m_TopNode;
		h_UpdateDiffOverDirectory(ReturnValue, ClientNode, ServerNode, "/");
		return(ReturnValue);
	}
	std::vector<std::string> MBPP_FileInfoReader::sp_GetPathComponents(std::string const& PathToDecompose)
	{
		std::vector<std::string> SplitPath = MBUtility::Split(PathToDecompose, "/");
		std::vector<std::string> ReturnValue = {};
		for (auto const& Part : SplitPath)
		{
			if (Part != "")
			{
				ReturnValue.push_back(Part);
			}
		}
		return(ReturnValue);
	}
	std::string const& h_GetDirectoryName(std::vector<MBPP_DirectoryInfoNode> const& DirectoryVector,size_t Index)
	{
		return(DirectoryVector[Index].DirectoryName);
	}
	std::string const& h_GetFileName(std::vector<MBPP_FileInfo> const& FileVector, size_t Index)
	{
		return(FileVector[Index].FileName);
	}
	const MBPP_DirectoryInfoNode* MBPP_FileInfoReader::p_GetTargetDirectory(std::vector<std::string> const& PathComponents)
	{
		const MBPP_DirectoryInfoNode* ReturnValue = nullptr;
		if (PathComponents.size() == 1)
		{
			ReturnValue = &m_TopNode;
		}
		else
		{
			bool FileExists = true;
			const MBPP_DirectoryInfoNode* CurrentNode = &m_TopNode;
			for (size_t i = 0; i < PathComponents.size() - 1; i++)
			{
				int DirectoryPosition = MBAlgorithms::BinarySearch(CurrentNode->Directories, PathComponents[i], h_GetDirectoryName, std::less<std::string>());
				if (DirectoryPosition == -1)
				{
					FileExists = false;
					break;
				}
				else
				{
					CurrentNode = &(CurrentNode->Directories[DirectoryPosition]);
				}
			}
			if (FileExists)
			{
				ReturnValue = CurrentNode;
			}
		}
		return(ReturnValue);
	}
	bool MBPP_FileInfoReader::ObjectExists(std::string const& ObjectToSearch)
	{
		if (ObjectToSearch == "/")
		{
			return(true);
		}
		std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToSearch);
		const MBPP_DirectoryInfoNode* ObjectDirectory = p_GetTargetDirectory(PathComponents);
		if (ObjectDirectory == nullptr)
		{
			return(false);
		}
		else
		{
			int FilePosition = MBAlgorithms::BinarySearch(ObjectDirectory->Files,PathComponents.back(),h_GetFileName,std::less<std::string>());
			if (FilePosition != -1)
			{
				return(true);
			}
			else
			{
				int DirectoryPosition = MBAlgorithms::BinarySearch(ObjectDirectory->Directories, PathComponents.back(), h_GetDirectoryName, std::less<std::string>());
				if (DirectoryPosition != -1)
				{
					return(true);
				}
				else
				{
					return(false);
				}
			}
		}
	}
	const MBPP_FileInfo * MBPP_FileInfoReader::GeFileInfo(std::string const& ObjectToSearch)
	{
		const MBPP_FileInfo* ReturnValue = nullptr;
		if (ObjectToSearch == "" || ObjectToSearch.front() != '/')
		{
			ReturnValue = nullptr;
		}
		else
		{
			std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToSearch);
			const MBPP_DirectoryInfoNode* ObjectDirectory = p_GetTargetDirectory(PathComponents);
			int FilePosition = MBAlgorithms::BinarySearch(ObjectDirectory->Files, PathComponents.back(), h_GetFileName, std::less<std::string>());
			if (FilePosition != -1)
			{
				ReturnValue = &(ObjectDirectory->Files[FilePosition]);
			}
		}
		return(ReturnValue);
	}
	const MBPP_DirectoryInfoNode* MBPP_FileInfoReader::GetDirectoryInfo(std::string const& ObjectToSearch)
	{
		const MBPP_DirectoryInfoNode* ReturnValue = nullptr;
		if (ObjectToSearch == "" || ObjectToSearch.front() != '/')
		{
			ReturnValue = nullptr;
		}
		else
		{
			std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToSearch);
			const MBPP_DirectoryInfoNode* ObjectDirectory = p_GetTargetDirectory(PathComponents);
			int DirectoryPosition = MBAlgorithms::BinarySearch(ObjectDirectory->Directories, PathComponents.back(), h_GetDirectoryName, std::less<std::string>());
			if (DirectoryPosition != -1)
			{
				ReturnValue = &(ObjectDirectory->Directories[DirectoryPosition]);
			}
		}
		return(ReturnValue);
	}
	//END MBPP_FileInfoReader

	//BEGIN MBPP_Client
	MBError MBPP_Client::Connect(MBPP_TransferProtocol TransferProtocol, std::string const& Domain, std::string const& Port)
	{
		return(MBError());
	}
	bool MBPP_Client::IsConnected()
	{
		return(m_ServerConnection->IsConnected());
	}
	MBError MBPP_Client::p_GetFiles(std::string const& PacketName,std::vector<std::string> const& FilesToGet, std::string const& OutputDirectory)
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		std::string PacketNameData = MBPP_EncodeString(PacketName);
		RecordToSend.Type = MBPP_RecordType::GetFiles;
		RecordToSend.RecordData = PacketNameData+std::string(2,0);
		h_WriteBigEndianInteger(RecordToSend.RecordData.data()+ PacketNameData.size(), FilesToGet.size(), 2);
		for (size_t i = 0; i < FilesToGet.size(); i++)
		{
			RecordToSend.RecordData += MBPP_EncodeString(FilesToGet[i]);
		}
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		std::string RecievedData = m_ServerConnection->RecieveData();
		while (m_ServerConnection->IsConnected() && RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			RecievedData += m_ServerConnection->RecieveData();
		}
		if (RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Server didn't send enough data for a MBPPGenericHeader";
		}
		else
		{
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), OutputDirectory);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_GetDirectory(std::string const& Packet,std::string const& DirectoryToGet, std::string const& OutputDirectory) 
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetDirectory;
		RecordToSend.RecordData = MBPP_EncodeString(Packet)+ MBPP_EncodeString("/");
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		std::string RecievedData = m_ServerConnection->RecieveData();
		while (m_ServerConnection->IsConnected() && RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			RecievedData += m_ServerConnection->RecieveData();
		}
		if (RecievedData.size() < MBPP_GenericRecordHeaderSize+2)
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Server didn't send enough data for a MBPPGenericHeader";
		}
		else
		{
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), OutputDirectory);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_DownloadServerFilesInfo(std::string const& PacketName,std::string const& OutputFilepath,std::string const& OutputDirectory)
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetPacketInfo;
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		RecordToSend.RecordData = MBPP_EncodeString(PacketName);
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		std::string RecievedData = m_ServerConnection->RecieveData();
		while (m_ServerConnection->IsConnected() && RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			RecievedData += m_ServerConnection->RecieveData();
		}
		if (RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Server didn't send enough data for a MBPPGenericHeader";
		}
		else
		{
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), OutputDirectory);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_DownloadFileList(std::string const& InitialData,size_t InitialDataOffset, MBSockets::ConnectSocket* SocketToUse, std::string const& OutputDirectory)
	{
		MBError ReturnValue = true;
		size_t MaxRecieveSize = 300000000;
		std::ofstream FileHandle;
		uint64_t TotalResponseSize = -1;
		uint64_t TotalRecievedBytes = 0;
		uint64_t TotalNumberOfFiles = -1;
		std::string ResponseData = InitialData; //TODO borde kanske mova intial datan istället
		std::string CurrentFile = "";
		size_t CurrentFileSize = -1;
		size_t CurrentFileParsedBytes = -1;

		size_t ResponseDataOffset = InitialDataOffset;
		while (TotalRecievedBytes < TotalResponseSize)
		{
			if (CurrentFile == "")
			{
				//vi måste få info om den fil vi ska skriva till nu
				while (ResponseData.size() - ResponseDataOffset < 2)
				{
					size_t PreviousResponseDataSize = ResponseData.size();
					ResponseData += m_ServerConnection->RecieveData(MaxRecieveSize);
					TotalRecievedBytes += ResponseData.size() - PreviousResponseDataSize;
				}
				uint16_t FileNameSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 2, ResponseDataOffset, &ResponseDataOffset);
				while (ResponseData.size() - ResponseDataOffset < uint64_t(FileNameSize) + 8)
				{
					size_t PreviousResponseDataSize = ResponseData.size();
					ResponseData += m_ServerConnection->RecieveData(MaxRecieveSize);
					TotalRecievedBytes += ResponseData.size() - PreviousResponseDataSize;
				}
				CurrentFile = std::string(ResponseData.data() + ResponseDataOffset, ResponseData.data() + ResponseDataOffset + FileNameSize);
				ResponseDataOffset = ResponseDataOffset + FileNameSize;
				CurrentFileSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 8, ResponseDataOffset, &ResponseDataOffset);
				CurrentFileParsedBytes = 0;
				//OBS!!! egentligen en security risk om vi inte kollar att filen är inom directoryn
				FileHandle.open(OutputDirectory + CurrentFile);
			}
			else
			{
				//all ny data vi får nu ska tillhöra filen
				//antar också att response data är tom här
				if (ResponseData.size() == ResponseDataOffset)
				{
					ResponseData = m_ServerConnection->RecieveData(MaxRecieveSize);
					TotalRecievedBytes += ResponseData.size();
					ResponseDataOffset = 0;
				}
				uint64_t FileBytes = std::min((uint64_t)ResponseData.size() - ResponseDataOffset, CurrentFileSize - CurrentFileParsedBytes);
				FileHandle.write(ResponseData.data() + ResponseDataOffset, FileBytes);
				CurrentFileParsedBytes += FileBytes;
				ResponseDataOffset += FileBytes;
				if (CurrentFileParsedBytes == CurrentFileSize)
				{
					FileHandle.flush();
					FileHandle.close();
					CurrentFile = "";
				}
			}
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::DownloadPacket(std::string const& OutputDirectory, std::string const& PacketName)
	{
		MBError ReturnValue = p_GetDirectory(PacketName,"/",OutputDirectory);
		if (ReturnValue)
		{
			CreatePacketFilesData(OutputDirectory);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::UpdatePacket(std::string const& OutputDirectory, std::string const& PacketName)
	{
		//kollar om att ladda ner packet infon är tillräckligt litet och skapar en diff fil, eller gör det genom att manuellt be om directory info för varje fil och se om hashen skiljer
		MBError UpdateError = p_DownloadServerFilesInfo(PacketName,OutputDirectory + "/MBPM_ServerFileInfo",OutputDirectory);
		if(!UpdateError)
		{
			return(UpdateError);
		}
		//ANTAGANDE vi antar här att PacketFilesData är uppdaterad
		MBPP_FileInfoDiff FileInfoDifferance = GetFileInfoDifference(MBPP_FileInfoReader(OutputDirectory + "/MBPM_FileInfo"), MBPP_FileInfoReader(OutputDirectory + "/MBPM_ServerFileInfo"));
		std::vector<std::string> FilesToGet = {};
		std::vector<std::string> DirectoriesToGet = {};
		for (auto const& UpdatedFile : FileInfoDifferance.UpdatedFiles)
		{
			FilesToGet.push_back(UpdatedFile);
		}
		for (auto const& NewFiles : FileInfoDifferance.AddedFiles)
		{
			FilesToGet.push_back(NewFiles);
		}
		for (auto const& NewDirectories : FileInfoDifferance.AddedDirectories)
		{
			DirectoriesToGet.push_back(NewDirectories);
		}
		UpdateError = p_GetFiles(PacketName,FilesToGet,OutputDirectory);
		if (!UpdateError)
		{
			return(UpdateError);
		}
		for (auto const& Directories : DirectoriesToGet)
		{
			UpdateError = p_GetDirectory(PacketName,Directories,OutputDirectory);
			if (!UpdateError)
			{
				return(UpdateError);
			}
		}
		return(UpdateError);
	}
	MBError MBPP_Client::UploadPacket(std::string const& PacketDirectory, std::string const& PacketName)
	{
		//kollar om att ladda ner packet infon är tillräckligt litet och skapar en diff fil, eller gör det genom att manuellt be om directory info för varje fil och se om hashen skiljer
		MBError UpdateError = p_DownloadServerFilesInfo(PacketName,PacketDirectory + "/MBPM_ServerFileInfo", PacketDirectory);
		if (!UpdateError)
		{
			return(UpdateError);
		}
		//ANTAGANDE vi antar här att PacketFilesData är uppdaterad
		//byter plats på client och server så att client tolkas som den uppdaterade q
		MBPP_FileInfoDiff FileInfoDifferance = GetFileInfoDifference( MBPP_FileInfoReader(PacketDirectory + "/MBPM_ServerFileInfo"), MBPP_FileInfoReader(PacketDirectory + "/MBPM_FileInfo"));
		size_t TotalDataSize = 0;
		std::vector<std::string> FilesToSend = {};
		std::vector<std::string> DirectoriesToSend = {};
		for (auto const& UpdatedFile : FileInfoDifferance.UpdatedFiles)
		{
			TotalDataSize += 2 + UpdatedFile.size();
			TotalDataSize += MBGetFileSize(PacketDirectory + UpdatedFile);
			FilesToSend.push_back(UpdatedFile);
		}
		for (auto const& NewFiles : FileInfoDifferance.AddedFiles)
		{
			TotalDataSize += 2 + NewFiles.size();
			TotalDataSize += MBGetFileSize(PacketDirectory + NewFiles);
			FilesToSend.push_back(PacketDirectory);
		}
		for (auto const& NewDirectories : FileInfoDifferance.AddedDirectories)
		{
			DirectoriesToSend.push_back(NewDirectories);
		}
		//UpdateError = p_GetFiles(PacketName, FilesToGet, OutputDirectory);
		//if (!UpdateError)
		//{
		//	return(UpdateError);
		//}
		//for (auto const& Directories : DirectoriesToGet)
		//{
		//	UpdateError = p_GetDirectory(PacketName, Directories, OutputDirectory);
		//	if (!UpdateError)
		//	{
		//		return(UpdateError);
		//	}
		//}
		return(UpdateError);
	}
	//END MBPP_Client

	//BEGIN MBPP_Server
	MBPP_Server::MBPP_Server(std::string const& PacketDirectory)
	{

	}

	//top level, garanterat att det är klart efter, inte garanterat att funka om inte låg level under är klara
	//std::string MBPP_Server::GenerateResponse(const void* RequestData, size_t RequestSize, MBError* OutError)	
	//{
	//
	//}
	//std::string MBPP_Server::GenerateResponse(std::string const& RequestData, MBError* OutError)
	//{
	//	return(GenerateResponse(RequestData.data(), RequestData.size(), OutError));
	//}
	//
	//MBError MBPP_Server::SendResponse(MBSockets::ConnectSocket* SocketToUse, const void* RequestData, size_t RequestSize)
	//{
	//
	//}
	//MBError MBPP_Server::SendResponse(MBSockets::ConnectSocket* SocketToUse, std::string const& CompleteRequestData)
	//{
	//	return(SendResponse(SocketToUse, CompleteRequestData.data(), CompleteRequestData.size()));
	//}

	//om stora saker skickas kan inte allt sparas i minnet, så detta api så insertas dem rakt in i klassen, 
	MBError MBPP_Server::InsertClientData(const void* Data, size_t DataSize)
	{
		MBError ReturnValue = true;
		char* ClientData = (char*)Data;
		m_RecievedData += std::string(ClientData, DataSize);
		if (m_CurrentRequestType == MBPP_RecordType::Null)
		{
			if (m_RecievedData.size() >= MBPP_GenericRecordHeaderSize)
			{
				m_CurrentRequestType = (MBPP_RecordType)m_RecievedData[0];
				if (m_CurrentRequestType > MBPP_RecordType::Null)
				{
					ReturnValue = false;
					ReturnValue.ErrorMessage = "Invalid MBPP_RecordType";
				}
				else
				{
					m_CurrentRequestSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data()+1, 8, 0, nullptr);
					m_RecievedDataOffset = MBPP_GenericRecordHeaderSize;
				}
			}
		}
		//Dessa kräver extra data
		if (m_CurrentRequestType == MBPP_RecordType::GetFiles)
		{

		}
		else if (m_CurrentRequestType == MBPP_RecordType::GetDirectory)
		{

		}
		//tom, kräver ingen extra data
		else if (m_CurrentRequestType == MBPP_RecordType::GetPacketInfo)
		{

		}
		//lägger alltid 
	}
	MBError MBPP_Server::InsertClientData(std::string const& ClientData)
	{
		return(InsertClientData(ClientData.data(), ClientData.size()));
	}
	bool MBPP_Server::ClientRequestFinished()
	{

	}
	MBPP_ServerResponseIterator MBPP_Server::GetResponseIterator()
	{
		MBPP_ServerResponseIterator ReturnValue(this);
		return(ReturnValue);
	}
	//MBError MBPP_Server::SendResponse(MBSockets::ConnectSocket* SocketToUse)
	//{
	//
	//}
	//END MBPP_Server

	//BEGIN MBPP_ServerResponseIterator
	MBPP_ServerResponseIterator::MBPP_ServerResponseIterator(MBPP_Server* m_AssociatedServer)
	{

		(*this)++;
	}
	MBPP_ServerResponseIterator& MBPP_ServerResponseIterator::operator++()
	{
		return((*this)++);
	}
	MBPP_ServerResponseIterator& MBPP_ServerResponseIterator::operator++(int)//postfix
	{
		

		return(*this);
	}
	MBPP_ServerResponseIterator::~MBPP_ServerResponseIterator()
	{
		m_AssociatedServer->p_ResetRequestResponseState();
	}
	//END MBPP_ServerResponseIterator
}