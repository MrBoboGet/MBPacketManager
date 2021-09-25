#include "MB_PacketProtocol.h"
#include <MBParsing/MBParsing.h>
#include <set>
#include <MBCrypto/MBCrypto.h>
#include <MBAlgorithms.h>

namespace MBPM
{
	std::string h_PathToUTF8(std::filesystem::path const& PathToConvert)
	{
		return(PathToConvert.generic_u8string());
	}
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
	uintmax_t h_ReadBigEndianInteger(MBUtility::MBOctetInputStream* FileToReadFrom, uint8_t IntegerSize)
	{
		uintmax_t ReturnValue = 0;
		uint8_t ByteBuffer[sizeof(uintmax_t)];
		FileToReadFrom->Read((char*)ByteBuffer, IntegerSize);
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
	MBPP_ComputerInfo GetComputerInfo()
	{
		return(MBPP_ComputerInfo());
	}
	MBPP_GenericRecord MBPP_ParseRecordHeader(const void* Data, size_t DataSize, size_t InOffset, size_t* OutOffset)
	{
		MBPP_GenericRecord ReturnValue;
		size_t ParseOffset = InOffset;
		char* RecordData = (char*)Data;
		ReturnValue.Type = (MBPP_RecordType) MBParsing::ParseBigEndianInteger(Data, 1, ParseOffset, &ParseOffset);
		ReturnValue.ComputerInfo.Type = (MBPP_ComputerInfoType) MBParsing::ParseBigEndianInteger(Data, 1, ParseOffset, &ParseOffset);
		ReturnValue.ComputerInfo.OSType = (MBPP_OSType) MBParsing::ParseBigEndianInteger(Data, 4, ParseOffset, &ParseOffset);
		ReturnValue.ComputerInfo.ProcessorType = (MBPP_ProcessorType) MBParsing::ParseBigEndianInteger(Data, 4, ParseOffset, &ParseOffset);
		ReturnValue.RecordSize = MBParsing::ParseBigEndianInteger(Data, 8, ParseOffset, &ParseOffset);
		
		return(ReturnValue);
	}
	std::string MBPP_EncodeString(std::string const& StringToEncode)
	{
		std::string ReturnValue = std::string(StringToEncode.size() + 2, 0);
		h_WriteBigEndianInteger(ReturnValue.data(), StringToEncode.size(), 2);
		std::memcpy(((char*)ReturnValue.data()) + 2, StringToEncode.data(), StringToEncode.size());
		return(ReturnValue);
	}
	std::string MBPP_GetRecordHeader(MBPP_GenericRecord const& RecordToConvert)
	{
		std::string ReturnValue = "";
		ReturnValue += char(RecordToConvert.Type);
		ReturnValue += char(RecordToConvert.ComputerInfo.Type);
		ReturnValue += std::string(16, 0);
		size_t ParseOffest = 2;
		h_WriteBigEndianInteger(ReturnValue.data()+2, uint32_t(RecordToConvert.ComputerInfo.OSType), 4);
		h_WriteBigEndianInteger(ReturnValue.data()+6, uint32_t(RecordToConvert.ComputerInfo.ProcessorType), 4);
		h_WriteBigEndianInteger(ReturnValue.data()+10, RecordToConvert.RecordSize, 8);
		return(ReturnValue);
	}
	std::string MBPP_GetRecordData(MBPP_GenericRecord const& RecordToConvert)
	{
		return(MBPP_GetRecordHeader(RecordToConvert) + RecordToConvert.RecordData);
	}
	std::string MBPP_GenerateFileList(std::vector<std::string> const& FilesToList)
	{
		size_t TotalFileListSize = 0;
		for (size_t i = 0; i < FilesToList.size(); i++)
		{
			TotalFileListSize += MBPP_StringLengthSize + FilesToList[i].size();
		}
		std::string ReturnValue = std::string(4+TotalFileListSize, 0);
		size_t ParseOffset = 0;
		h_WriteBigEndianInteger(ReturnValue.data() + ParseOffset, TotalFileListSize, 4);
		ParseOffset += 4;
		for (size_t i = 0; i < FilesToList.size(); i++)
		{
			h_WriteBigEndianInteger(ReturnValue.data() + ParseOffset, FilesToList[i].size(), MBPP_StringLengthSize);
			ParseOffset += MBPP_StringLengthSize;
			for (size_t j = 0; j < FilesToList[i].size();j++)
			{
				ReturnValue[ParseOffset] = FilesToList[i][j];
				ParseOffset += 1;
			}
		}
		return(ReturnValue);
	}
	//ANTAGANDE filer är sorterade i ordning
	//BEGIN MBPP_FileInfoReader
	MBPP_DirectoryInfoNode MBPP_FileInfoReader::p_ReadDirectoryInfoFromFile(MBUtility::MBOctetInputStream* FileToReadFrom,size_t HashSize)
	{
		MBPP_DirectoryInfoNode ReturnValue;
		h_ReadBigEndianInteger(FileToReadFrom, 8);//filepointer
		std::string DirectoryHash = std::string(HashSize, 0);
		FileToReadFrom->Read(DirectoryHash.data(), HashSize);
		size_t NameSize = h_ReadBigEndianInteger(FileToReadFrom, 2);
		std::string DirectoryName = std::string(NameSize, 0);
		FileToReadFrom->Read(DirectoryName.data(), NameSize);
		uint32_t NumberOfFiles = h_ReadBigEndianInteger(FileToReadFrom, 4);
		for (size_t i = 0; i < NumberOfFiles; i++)
		{
			ReturnValue.Files.push_back(p_ReadFileInfoFromFile(FileToReadFrom, HashSize));
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
	MBPP_FileInfo MBPP_FileInfoReader::p_ReadFileInfoFromFile(MBUtility::MBOctetInputStream* InputStream,size_t HashSize)
	{
		MBPP_FileInfo ReturnValue;
		uint16_t FileNameSize = h_ReadBigEndianInteger(InputStream, 2);
		ReturnValue.FileName = std::string(FileNameSize, 0);
		InputStream->Read(ReturnValue.FileName.data(), FileNameSize);
		ReturnValue.FileHash = std::string(HashSize, 0);
		InputStream->Read(ReturnValue.FileHash.data(), HashSize);
		return(ReturnValue);
	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(std::string const& FileInfoPath)
	{
		std::ifstream PacketFileInfo = std::ifstream(FileInfoPath, std::ios::in | std::ios::binary);
		if (!PacketFileInfo.is_open())
		{
			return;
		}
		MBUtility::MBFileInputStream FileReader(&PacketFileInfo);
		m_TopNode = p_ReadDirectoryInfoFromFile(&FileReader,20);//sha1 diges size
	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(const void* DataToRead, size_t DataSize)
	{
		MBUtility::MBBufferInputStream BufferReader(DataToRead,DataSize);
		m_TopNode = p_ReadDirectoryInfoFromFile(&BufferReader, 20);//sha1 diges size
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
		if (PathComponents.size() == 1  || PathComponents.size() == 0)
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
	const MBPP_FileInfo * MBPP_FileInfoReader::GetFileInfo(std::string const& ObjectToSearch)
	{
		const MBPP_FileInfo* ReturnValue = nullptr;
		if (ObjectToSearch == "" || ObjectToSearch.front() != '/')
		{
			ReturnValue = nullptr;
		}
		else
		{
			std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToSearch);
			if (PathComponents.size() == 0)
			{
				return(nullptr);
			}
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
			if (PathComponents.size() == 0)//antar att detta bara kan hända om pathen är på formen */
			{
				return(&m_TopNode);
			}
			const MBPP_DirectoryInfoNode* ObjectDirectory = p_GetTargetDirectory(PathComponents);
			int DirectoryPosition = MBAlgorithms::BinarySearch(ObjectDirectory->Directories, PathComponents.back(), h_GetDirectoryName, std::less<std::string>());
			if (DirectoryPosition != -1)
			{
				ReturnValue = &(ObjectDirectory->Directories[DirectoryPosition]);
			}
		}
		return(ReturnValue);
	}
	bool MBPP_FileInfoReader::operator==(MBPP_FileInfoReader const& OtherReader) const
	{
		bool ReturnValue = true;
		//snabb som kanske inte alltid är sann, men går på hash
		if (m_TopNode.DirectoryName != OtherReader.m_TopNode.DirectoryName || m_TopNode.DirectoryHash != OtherReader.m_TopNode.DirectoryHash) 
		{
			ReturnValue = false;
		}
		return(ReturnValue);
	}
	//END MBPP_FileInfoReader

	//BEGIN 	class MBPP_FileListDownloadHandler
	
	MBError MBPP_FileListDownloadHandler::Open(std::string const& FileToDownloadName)
	{
		assert(false);
		return(false);
	}
	MBError MBPP_FileListDownloadHandler::InsertData(const void* Data, size_t DataSize)
	{
		assert(false);
		return(false);
	}
	MBError MBPP_FileListDownloadHandler::Close()
	{
		assert(false);
		return(false);
	}
	//END MBPP_FileListDownloadHandler

	//BEGIN MBPP_FileListMemoryMapper
	MBPP_FileListMemoryMapper::MBPP_FileListMemoryMapper()
	{

	}
	MBError MBPP_FileListMemoryMapper::Open(std::string const& FileToDownloadName)
	{
		m_CurrentFile = FileToDownloadName;
		m_DownloadedFiles[FileToDownloadName] = "";
		return(true);
	}
	MBError MBPP_FileListMemoryMapper::InsertData(const void* Data, size_t DataSize)
	{
		m_DownloadedFiles[m_CurrentFile] += std::string((char*)Data, DataSize);
		return(true);
	}
	MBError MBPP_FileListMemoryMapper::Close()
	{
		m_CurrentFile = "";
		return(true);
	}
	//END MBPP_FileListMemoryMapper

	//BEGIN MBPP_ClientHTTPConverter
	MBPP_ClientHTTPConverter::MBPP_ClientHTTPConverter(MBPP_PacketHost const& HostData) 
		: MBSockets::ClientSocket(HostData.URL.substr(0, std::min(HostData.URL.find('/'), HostData.URL.size())),std::to_string(HostData.Port))
	{
		std::string PortToUse = "";
		std::string Adress = HostData.URL.substr(0, std::min(HostData.URL.find('/'), HostData.URL.size()));
		m_MBPP_ResourceLocation = HostData.URL.substr(std::min(HostData.URL.find('/'), HostData.URL.size()));
		PortToUse = std::to_string(HostData.Port);
		if (HostData.Port == -1)
		{
			if (HostData.TransferProtocol == MBPP_TransferProtocol::HTTPS)
			{
				PortToUse = "443";
			}
			else if(HostData.TransferProtocol == MBPP_TransferProtocol::HTTP)
			{
				PortToUse = "80";
			}
		}
		m_InternalHTTPSocket = std::unique_ptr<MBSockets::HTTPConnectSocket>(new MBSockets::HTTPConnectSocket(Adress, PortToUse));
	}
	void MBPP_ClientHTTPConverter::p_ResetRecieveState()
	{
		m_MBPP_HTTPHeaderRecieved = false;
		m_MBPP_ResponseData = "";
	}
	void MBPP_ClientHTTPConverter::p_ResetSendState()
	{
		m_MBPP_CurrentHeaderData = "";
		m_MBPP_CurrentHeader = MBPP_GenericRecord();
		m_MBPP_HeaderSent = false;
		m_TotalSentRecordData = 0;

		m_MBPP_ResponseData = "";
		m_MBPP_HTTPHeaderRecieved = false;
	}
	std::string MBPP_ClientHTTPConverter::p_GenerateHTTPHeader(MBPP_GenericRecord const& MBPPHeader)
	{
		std::string ReturnValue = "POST " + m_MBPP_ResourceLocation + " HTTP/1.1\r\n";
		ReturnValue += "Content-Type: application/x-MBPP-record\r\n";
		ReturnValue += "Content-Length: " + std::to_string(MBPPHeader.RecordSize+MBPP_GenericRecordHeaderSize) + "\r\n\r\n";
		return(ReturnValue);
	}
	int MBPP_ClientHTTPConverter::Connect()
	{
		return(m_InternalHTTPSocket->Connect());
	}
	bool MBPP_ClientHTTPConverter::IsConnected()
	{
		return(m_InternalHTTPSocket->IsConnected());
	}
	bool MBPP_ClientHTTPConverter::IsValid()
	{
		return(m_InternalHTTPSocket->IsValid());
	}
	MBError MBPP_ClientHTTPConverter::EstablishTLSConnection()
	{
		return(m_InternalHTTPSocket->EstablishTLSConnection());
	}
	std::string MBPP_ClientHTTPConverter::RecieveData(size_t MaxDataToRecieve)
	{
		if (m_MBPP_HTTPHeaderRecieved)
		{
			return(m_InternalHTTPSocket->RecieveData());
		}
		else
		{
			while (m_InternalHTTPSocket->IsConnected() && m_InternalHTTPSocket->IsValid())
			{
				m_MBPP_ResponseData += m_InternalHTTPSocket->RecieveData();
				size_t HeaderEnd = m_MBPP_ResponseData.find("\r\n\r\n");
				if (HeaderEnd != m_MBPP_ResponseData.npos)
				{
					m_MBPP_HTTPHeaderRecieved = true;
					return(m_MBPP_ResponseData.substr(HeaderEnd+4));
				}
			}
		}
		return("");
	}
	int MBPP_ClientHTTPConverter::SendData(const void* DataToSend, size_t DataSize)
	{
		p_ResetRecieveState();
		if (!m_MBPP_HeaderSent)
		{
			m_MBPP_CurrentHeaderData += std::string((char*)DataToSend, DataSize);
			if (m_MBPP_CurrentHeaderData.size() >= MBPP_GenericRecordHeaderSize)
			{
				m_MBPP_CurrentHeader = MBPP_ParseRecordHeader(m_MBPP_CurrentHeaderData.data(), MBPP_GenericRecordHeaderSize, 0, nullptr);
				m_InternalHTTPSocket->SendData(p_GenerateHTTPHeader(m_MBPP_CurrentHeader)+ m_MBPP_CurrentHeaderData);
				m_MBPP_HeaderSent = true;
				m_TotalSentRecordData = m_MBPP_CurrentHeaderData.size() - MBPP_GenericRecordHeaderSize;
				m_MBPP_CurrentHeaderData = "";
			}
		}
		else
		{
			m_InternalHTTPSocket->SendData(DataToSend,DataSize);
			m_TotalSentRecordData += DataSize;
		}
		if (m_TotalSentRecordData == m_MBPP_CurrentHeader.RecordSize)
		{
			p_ResetSendState();
		}
		return(0);
	}
	int MBPP_ClientHTTPConverter::SendData(std::string const& StringToSend)
	{
		return(MBPP_ClientHTTPConverter::SendData(StringToSend.data(), StringToSend.size()));
	}
	//END MBPP_ClientHTTPConverter

	//BEGIN MBPP_Client
	MBError MBPP_Client::Connect(MBPP_TransferProtocol TransferProtocol, std::string const& Domain, std::string const& Port)
	{
		return(Connect({ Domain,TransferProtocol,(uint16_t) std::stoi(Port) }));
	}
	MBError MBPP_Client::Connect(MBPP_PacketHost const& PacketHost)
	{
		MBError ReturnValue = true;
		if (PacketHost.TransferProtocol == MBPP_TransferProtocol::TCP || PacketHost.TransferProtocol == MBPP_TransferProtocol::Null)
		{
			m_ServerConnection = std::unique_ptr<MBSockets::ConnectSocket>(new MBSockets::ClientSocket(PacketHost.URL, std::to_string(PacketHost.Port)));
			MBSockets::ClientSocket* ClientSocket = (MBSockets::ClientSocket*)m_ServerConnection.get();
			ClientSocket->Connect();
		}
		else if(PacketHost.TransferProtocol == MBPP_TransferProtocol::HTTP || PacketHost.TransferProtocol == MBPP_TransferProtocol::HTTPS)
		{
			m_ServerConnection = std::unique_ptr<MBSockets::ConnectSocket>(new MBPP_ClientHTTPConverter(PacketHost));
			MBSockets::ClientSocket* ClientSocket = (MBSockets::ClientSocket*)m_ServerConnection.get();
			ClientSocket->Connect();
			if (PacketHost.TransferProtocol == MBPP_TransferProtocol::HTTPS)
			{
				ReturnValue = ClientSocket->EstablishTLSConnection();
			}
		}
		return(ReturnValue);
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
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.Type = MBPP_RecordType::GetFiles;
		RecordToSend.RecordData = MBPP_EncodeString(PacketName)+MBPP_GenerateFileList(FilesToGet);
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
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), OutputDirectory,FilesToGet);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_GetDirectory(std::string const& Packet,std::string const& DirectoryToGet, std::string const& OutputDirectory) 
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetDirectory;
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.RecordData = MBPP_EncodeString(Packet)+ MBPP_GenerateFileList({ DirectoryToGet });
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
	MBError MBPP_Client::p_DownloadServerFilesInfo(std::string const& PacketName, std::string const& OutputDirectory,std::vector<std::string> const& OutputFileNames)
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetPacketInfo;
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.RecordData = MBPP_EncodeString(PacketName);
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
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), OutputDirectory,OutputFileNames);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_DownloadFileList(std::string const& InitialData,size_t InitialDataOffset, MBSockets::ConnectSocket* SocketToUse, std::string const& OutputDirectory
		,std::vector<std::string> const& OutputFileNames)
	{
		MBError ReturnValue = true;
		size_t MaxRecieveSize = 300000000;
		std::ofstream FileHandle;

		std::string ResponseData = InitialData; //TODO borde kanske mova intial datan istället

		size_t ResponseDataOffset = InitialDataOffset;

		std::vector<std::string> FilesToDownload = {};
		size_t CurrentFileIndex = 0;
		uint64_t CurrentFileSize = -1;
		uint64_t CurrentFileParsedBytes = -1;

		//parsar fil listan
		while (ResponseData.size() - ResponseDataOffset < 4)
		{
			ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
		}
		uint32_t FileListSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 4, ResponseDataOffset, &ResponseDataOffset);
		while (ResponseData.size() - ResponseDataOffset < FileListSize)
		{
			ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
		}
		uint32_t ParsedFileListBytes = 0;
		while (ParsedFileListBytes < FileListSize)
		{
			uint32_t FileNameLength = MBParsing::ParseBigEndianInteger(ResponseData.data(), MBPP_StringLengthSize, ResponseDataOffset, &ResponseDataOffset);
			FilesToDownload.push_back(std::string(ResponseData.data() + ResponseDataOffset, FileNameLength));
			ResponseDataOffset += FileNameLength;
			ParsedFileListBytes += MBPP_StringLengthSize + FileNameLength;
		}
		if (OutputFileNames.size() != 0)
		{
			assert(OutputFileNames.size() == FilesToDownload.size());
			FilesToDownload = OutputFileNames;
		}
		for (size_t i = 0; i < FilesToDownload.size(); i++)
		{
			//kollar om directoryn finns, annars skapar vi dem
			std::string FileDirectory =h_PathToUTF8(std::filesystem::path(OutputDirectory + FilesToDownload[i]).parent_path());
			std::filesystem::create_directories(FileDirectory);
		}
		while (CurrentFileIndex < FilesToDownload.size())
		{
			if (FileHandle.is_open() == false)
			{
				//vi måste få info om den fil vi ska skriva till nu
				while (ResponseData.size() - ResponseDataOffset < 8)
				{
					ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
				}
				CurrentFileSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 8, ResponseDataOffset, &ResponseDataOffset);
				CurrentFileParsedBytes = 0;
				//OBS!!! egentligen en security risk om vi inte kollar att filen är inom directoryn
				FileHandle.open(OutputDirectory + FilesToDownload[CurrentFileIndex],std::ios::out|std::ios::binary);
			}
			else
			{
				//all ny data vi får nu ska tillhöra filen
				//antar också att response data är tom här
				if (ResponseData.size() == ResponseDataOffset)
				{
					ResponseData = SocketToUse->RecieveData(MaxRecieveSize);
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
					CurrentFileIndex += 1;
				}
			}
		}
		return(ReturnValue);
	}
	 MBError MBPP_Client::p_DownloadFileList(std::string const& InitialData, size_t DataOffset, MBSockets::ConnectSocket* SocketToUse, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		size_t MaxRecieveSize = 300000000;

		std::string ResponseData = InitialData; //TODO borde kanske mova intial datan istället

		size_t ResponseDataOffset = DataOffset;

		std::vector<std::string> FilesToDownload = {};
		size_t CurrentFileIndex = 0;
		uint64_t CurrentFileSize = -1;
		uint64_t CurrentFileParsedBytes = -1;

		//parsar fil listan
		while (ResponseData.size() - ResponseDataOffset < 4)
		{
			ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
		}
		uint32_t FileListSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 4, ResponseDataOffset, &ResponseDataOffset);
		while (ResponseData.size() - ResponseDataOffset < FileListSize)
		{
			ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
		}
		uint32_t ParsedFileListBytes = 0;
		while (ParsedFileListBytes < FileListSize)
		{
			uint32_t FileNameLength = MBParsing::ParseBigEndianInteger(ResponseData.data(), MBPP_StringLengthSize, ResponseDataOffset, &ResponseDataOffset);
			FilesToDownload.push_back(std::string(ResponseData.data() + ResponseDataOffset, FileNameLength));
			ResponseDataOffset += FileNameLength;
			ParsedFileListBytes += MBPP_StringLengthSize + FileNameLength;
		}
		ReturnValue = DownloadHandler->NotifyFiles(FilesToDownload);
		if (!ReturnValue)
		{
			return(ReturnValue);
		}
		bool NewFile = true;
		while (CurrentFileIndex < FilesToDownload.size())
		{
			if (NewFile)
			{
				//vi måste få info om den fil vi ska skriva till nu
				while (ResponseData.size() - ResponseDataOffset < 8)
				{
					ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
				}
				CurrentFileSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 8, ResponseDataOffset, &ResponseDataOffset);
				CurrentFileParsedBytes = 0;
				//OBS!!! egentligen en security risk om vi inte kollar att filen är inom directoryn
				DownloadHandler->Open(FilesToDownload[CurrentFileIndex]);
				NewFile = false;
			}
			else
			{
				//all ny data vi får nu ska tillhöra filen
				//antar också att response data är tom här
				if (ResponseData.size() == ResponseDataOffset)
				{
					ResponseData = SocketToUse->RecieveData(MaxRecieveSize);
					ResponseDataOffset = 0;
				}
				uint64_t FileBytes = std::min((uint64_t)ResponseData.size() - ResponseDataOffset, CurrentFileSize - CurrentFileParsedBytes);
				DownloadHandler->InsertData(ResponseData.data() + ResponseDataOffset, FileBytes);
				CurrentFileParsedBytes += FileBytes;
				ResponseDataOffset += FileBytes;
				if (CurrentFileParsedBytes == CurrentFileSize)
				{
					ReturnValue = DownloadHandler->Close();
					NewFile = true;
					CurrentFileIndex += 1;
				}
			}
		}
		DownloadHandler->Close();
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
		MBError UpdateError = p_DownloadServerFilesInfo(PacketName, OutputDirectory, { "MBPM_ServerPacketInfo","MBPM_ServerFileInfo" });
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
		if (FilesToGet.size() > 0)
		{
			UpdateError = p_GetFiles(PacketName, FilesToGet, OutputDirectory);
		}
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
	MBError MBPP_Client::UploadPacket(std::string const& PacketDirectory, std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData)
	{
		//kollar om att ladda ner packet infon är tillräckligt litet och skapar en diff fil, eller gör det genom att manuellt be om directory info för varje fil och se om hashen skiljer
		MBError UploadError;
		MBPP_UploadRequest_Response UploadRequestResponse = p_GetLoginResult(PacketName, CredentialsType, CredentialsData,&UploadError);
		if (UploadRequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
		{
			//update out struct
			return(UploadError);
		}
		//TODO finns det något smidigt sätt att undvika mycket redundant server nedladdande?
		UploadError = p_DownloadServerFilesInfo(PacketName, PacketDirectory, { "MBPM_ServerPacketInfo" , "MBPM_ServerFileInfo" });
		if (!UploadError)
		{
			return(UploadError);
		}
		//ANTAGANDE vi antar här att PacketFilesData är uppdaterad
		//byter plats på client och server så att client tolkas som den uppdaterade q
		MBPP_GenericRecord RecordToSend;
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.Type = MBPP_RecordType::UploadChanges;
		
		MBPP_FileInfoDiff FileInfoDifferance = GetFileInfoDifference( MBPP_FileInfoReader(PacketDirectory + "/MBPM_ServerFileInfo"), MBPP_FileInfoReader(PacketDirectory + "/MBPM_FileInfo"));
		size_t TotalDataSize = 0;
		TotalDataSize += 1+MBPP_StringLengthSize+PacketName.size()+4+CredentialsData.size();//credentials typen,string längden samt längden av credentials datan

		std::vector<std::string> FilesToSend = {};
		std::vector<std::string> ObjectsToDelete = {};
		TotalDataSize += 4 + 4;//storleken av fileliststen
		for (auto const& UpdatedFile : FileInfoDifferance.UpdatedFiles)
		{
			TotalDataSize += MBPP_StringLengthSize + UpdatedFile.size();
			TotalDataSize += MBGetFileSize(PacketDirectory + UpdatedFile);
			FilesToSend.push_back(UpdatedFile);
		}
		for (auto const& NewFiles : FileInfoDifferance.AddedFiles)
		{
			TotalDataSize += MBPP_StringLengthSize + NewFiles.size();
			TotalDataSize += MBGetFileSize(PacketDirectory + NewFiles);
			FilesToSend.push_back(PacketDirectory);
		}
		for (auto const& DeletedFiles : FileInfoDifferance.RemovedFiles)
		{
			TotalDataSize += MBPP_StringLengthSize + DeletedFiles.size();
			ObjectsToDelete.push_back(DeletedFiles);
		}
		for (auto const& DeletedDirectoris : FileInfoDifferance.DeletedDirectories)
		{
			TotalDataSize += MBPP_StringLengthSize + DeletedDirectoris.size();
			ObjectsToDelete.push_back(DeletedDirectoris);
		}
		// nu till att skicka datan
		RecordToSend.RecordSize = TotalDataSize;
		std::string TotalHeaderData = MBPP_GetRecordHeader(RecordToSend);
		TotalHeaderData += MBPP_GetUploadCredentialsData(PacketName, CredentialsType, CredentialsData);
		TotalHeaderData += MBPP_GenerateFileList(FilesToSend);
		TotalHeaderData += MBPP_GenerateFileList(ObjectsToDelete);
		m_ServerConnection->SendData(TotalHeaderData);
		//antar här att alla filer existerar
		for (size_t i = 0; i < FilesToSend.size(); i++)
		{
			
			std::string DataToSend = std::string(8, 0);
			uint64_t FileSize = MBGetFileSize(PacketDirectory + FilesToSend[i]);
			h_WriteBigEndianInteger(DataToSend.data(), FileSize, 8);
			std::ifstream FileInputStream = std::ifstream(PacketDirectory + FilesToSend[i], std::ios::in | std::ios::binary);
			m_ServerConnection->SendData(DataToSend);
			const size_t BlockSize = 4096*4;
			std::string Buffer = std::string(BlockSize, 0);
			while (true)
			{
				FileInputStream.read(Buffer.data(), BlockSize);
				size_t BytesRead = FileInputStream.gcount();
				m_ServerConnection->SendData(Buffer.data(), BytesRead);
				if (BytesRead < BlockSize)
				{
					break;
				}
			}
		}

		//nu kommer vi till att parsa serverns respons
		std::string ResponseData = m_ServerConnection->RecieveData();
		while (ResponseData.size() < (MBPP_GenericRecordHeaderSize+2) && m_ServerConnection->IsConnected() && m_ServerConnection->IsValid())
		{
			ResponseData = m_ServerConnection->RecieveData();
		}
		MBPP_GenericRecord ResponseHeader = MBPP_ParseRecordHeader(ResponseData.data(), ResponseData.size(), 0, nullptr);
		if (ResponseHeader.Type != MBPP_RecordType::UploadChangesResponse)
		{
			if (ResponseHeader.Type == MBPP_RecordType::Error)
			{
				UploadError = false;
				UploadError.ErrorMessage = "Error after sending UploadChanges: " + 
					MBPP_GetErrorCodeString(MBPP_ErrorCode(MBParsing::ParseBigEndianInteger(ResponseData.data(),2, MBPP_GenericRecordHeaderSize,nullptr)));
			}
			else
			{
				UploadError = false;
				UploadError.ErrorMessage = "Error after sending UploadChanges: Invalid response type";
			}
		}
		else
		{
			MBPP_ErrorCode Result = MBPP_ErrorCode(MBParsing::ParseBigEndianInteger(ResponseData.data(), 2, MBPP_GenericRecordHeaderSize, nullptr));
			if (Result != MBPP_ErrorCode::Null)
			{
				UploadError = false;
				UploadError.ErrorMessage = "Error result in UploadChanges: "+MBPP_GetErrorCodeString(Result);
			}
		}
		return(UploadError);
	}
	MBError MBPP_Client::DownloadPacketFiles(std::string const& OutputDirectory, std::string const& PacketName, std::vector<std::string> const& FilesToGet)
	{
		MBError ReturnValue = p_GetFiles(PacketName, FilesToGet, OutputDirectory);
		return(ReturnValue);
	}
	MBError MBPP_Client::DownloadPacketDirectories(std::string const& OutputDirectory, std::string const& PacketName, std::vector<std::string> const& DirectoriesToGet)
	{
		//TODO kanske bara kan ersättas med en del av protocollet som är
		MBError ReturnValue = true;
		for (size_t i = 0;i < DirectoriesToGet.size();i++)
		{
			ReturnValue = p_GetDirectory(PacketName, DirectoriesToGet[i], OutputDirectory);
			if (!ReturnValue)
			{
				break;
			}
		}
		return(ReturnValue);
	}
	MBPP_FileInfoReader MBPP_Client::GetPacketFileInfo(std::string const& PacketName, MBError* OutError)
	{
		MBPP_FileListMemoryMapper MemoryMapper = MBPP_FileListMemoryMapper();
		MBPP_FileInfoReader ReturnValue;
		MBError GetPacketError = true;

		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetPacketInfo;
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.RecordData = MBPP_EncodeString(PacketName);
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		std::string RecievedData = m_ServerConnection->RecieveData();
		while (m_ServerConnection->IsConnected() && RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			RecievedData += m_ServerConnection->RecieveData();
		}
		if (RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			GetPacketError = false;
			GetPacketError.ErrorMessage = "Server didn't send enough data for a MBPPGenericHeader";
		}
		else
		{
			GetPacketError = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), &MemoryMapper);
		}
		if (GetPacketError)
		{
			std::map<std::string, std::string> Result = MemoryMapper.GetDownloadedFiles();
			if (Result.find("MBPM_FileInfo") != Result.end())
			{
				ReturnValue = MBPP_FileInfoReader(Result["MBPM_FileInfo"].data(), Result["MBPM_FileInfo"].size());
			}
			else
			{
				GetPacketError = false;
				GetPacketError.ErrorMessage = "Error getting server FileInfo: no MBPM_FileInfo file sent";
			}
		}
		if (OutError != nullptr)
		{
			*OutError = std::move(GetPacketError);
		}
		return(ReturnValue);
	}
	//END MBPP_Client

	//BEGIN MBPP_Server
	MBPP_Server::MBPP_Server(std::string const& PacketDirectory)
	{
		m_PacketSearchDirectories.push_back(PacketDirectory);
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
	void MBPP_Server::p_FreeRequestResponseData()
	{
		if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetFiles)
		{
			delete (MBPP_GetFileList_ResponseData*)m_RequestResponseData;
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetDirectory)
		{
			delete (MBPP_GetDirectories_ResponseData*)m_RequestResponseData;
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetPacketInfo)
		{
			delete (MBPP_GetPacketInfo_ResponseData*)m_RequestResponseData;
		}
		else
		{
			assert(false);
		}
	}
	void MBPP_Server::p_ResetRequestResponseState()
	{
		p_FreeRequestResponseData();

		m_RequestFinished = false;

		m_CurrentRequestHeader = MBPP_GenericRecord();
		m_RequestResponseData = nullptr;

		m_RecievedData = "";
		m_RecievedDataOffset = 0;
		m_CurrentRequestSize = -1;
	}

	//Get metoder
	MBError MBPP_Server::p_Handle_GetFiles() //manuell parsing av datan som inte väntar in att allt kommit
	{
		MBError ReturnValue = true;
		if (m_RequestResponseData == nullptr)
		{
			//första gången funktionen callas, vi måste se till att 
			m_RequestResponseData = new MBPP_GetFileList_ResponseData();
		}
		MBPP_GetFileList_ResponseData& CurrentData = p_GetResponseData<MBPP_GetFileList_ResponseData>();
		if (CurrentData.PacketName == "")
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= 2)
			{
				CurrentData.PacketNameSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, m_RecievedDataOffset, nullptr);
				m_RecievedDataOffset += MBPP_StringLengthSize;
			}
			if (CurrentData.PacketNameSize != -1)
			{
				if (m_RecievedData.size()+m_RecievedDataOffset>= CurrentData.PacketNameSize)
				{
					CurrentData.PacketName = m_RecievedData.substr(m_RecievedDataOffset, CurrentData.PacketNameSize);
					m_RecievedDataOffset += CurrentData.PacketNameSize;
				}
			}
		}
		if (CurrentData.PacketName != "")
		{
			if (CurrentData.TotalFileListSize == -1)
			{
				if (m_RecievedData.size() - m_RecievedDataOffset >= 4)
				{
					CurrentData.TotalFileListSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), 4, m_RecievedDataOffset, nullptr);
					m_RecievedDataOffset += 4;
					CurrentData.TotalParsedFileListData = 0;
				}
			}
			while (CurrentData.TotalFileListSize != -1 && (CurrentData.TotalParsedFileListData < CurrentData.TotalFileListSize))
			{
				if (CurrentData.CurrentStringSize == -1)
				{
					if (m_RecievedData.size() - m_RecievedDataOffset >= MBPP_StringLengthSize)
					{
						CurrentData.CurrentStringSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, m_RecievedDataOffset, nullptr);
						m_RecievedDataOffset += 2;
						CurrentData.TotalParsedFileListData += 2;
						CurrentData.CurrentStringParsedData = 0;
					}
					else
					{
						break;
					}
				}
				if (CurrentData.CurrentStringSize != -1)
				{
					if (m_RecievedData.size() - m_RecievedDataOffset >= CurrentData.CurrentStringSize)
					{
						CurrentData.FilesToGet.push_back(m_RecievedData.substr(m_RecievedDataOffset, CurrentData.CurrentStringSize));
						m_RecievedDataOffset += CurrentData.CurrentStringSize;
						CurrentData.TotalParsedFileListData += CurrentData.CurrentStringSize;
						CurrentData.CurrentStringSize = -1;
						CurrentData.CurrentStringParsedData = 0;
					}
					else
					{
						break;
					}
				}
			}
			if (CurrentData.TotalFileListSize == CurrentData.TotalParsedFileListData)
			{
				m_RequestFinished = true;
			}
		}
		m_RecievedData = m_RecievedData.substr(m_RecievedDataOffset);
		m_RecievedDataOffset = 0;
		return(ReturnValue);
	}
	MBError MBPP_Server::p_Handle_GetDirectories()
	{
		MBError ReturnValue = true;
		if (m_RequestResponseData == nullptr)
		{
			//första gången funktionen callas, vi måste se till att 
			m_RequestResponseData = new MBPP_GetDirectories_ResponseData();
		}
		MBPP_GetDirectories_ResponseData& CurrentResponseData = p_GetResponseData<MBPP_GetDirectories_ResponseData>();
		if (CurrentResponseData.PacketNameSize == -1)
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= MBPP_StringLengthSize)
			{
				CurrentResponseData.PacketNameSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, m_RecievedDataOffset, &m_RecievedDataOffset);
			}
		}
		if (CurrentResponseData.PacketNameSize != -1)
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= CurrentResponseData.PacketNameSize)
			{
				CurrentResponseData.PacketName = std::string(m_RecievedData.data() + m_RecievedDataOffset, CurrentResponseData.PacketNameSize);
				m_RecievedDataOffset += CurrentResponseData.PacketNameSize;
				if (CurrentResponseData.PacketName == "")
				{
					//Error handling, invalid packetname
					m_RequestFinished = true;
				}
			}
		}
		if (CurrentResponseData.PacketName != "")
		{
			if (CurrentResponseData.DirectoryListSize == -1 && m_RecievedData.size() - m_RecievedDataOffset >= 4)
			{
				CurrentResponseData.DirectoryListSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), 4, m_RecievedDataOffset, &m_RecievedDataOffset);
			}
			if (CurrentResponseData.DirectoryListSize != -1)
			{
				if (m_RecievedData.size() - m_RecievedDataOffset >= CurrentResponseData.DirectoryListSize)
				{
					//parsar listan
					while (m_RecievedDataOffset < m_RecievedData.size())
					{
						size_t NewStringSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), 2, m_RecievedDataOffset, &m_RecievedDataOffset);
						CurrentResponseData.DirectoriesToGet.push_back(std::string(m_RecievedData.data() + m_RecievedDataOffset, NewStringSize));
						m_RecievedDataOffset += NewStringSize;
					}
					m_RequestFinished = true;
					m_RecievedData = std::string();
					m_RecievedDataOffset = 0;
				}
			}
		}
		return(ReturnValue);
	}
	MBError MBPP_Server::p_Handle_GetPacketInfo()
	{
		MBError ReturnValue = true;
		if (m_RequestResponseData == nullptr)
		{
			//första gången funktionen callas, vi måste se till att 
			m_RequestResponseData = new MBPP_GetPacketInfo_ResponseData();
		}
		MBPP_GetPacketInfo_ResponseData& CurrentResponseData = p_GetResponseData<MBPP_GetPacketInfo_ResponseData>();
		if (CurrentResponseData.PacketNameSize == -1)
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= MBPP_StringLengthSize)
			{
				CurrentResponseData.PacketNameSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, m_RecievedDataOffset, &m_RecievedDataOffset);
			}
		}
		if (CurrentResponseData.PacketNameSize != -1)
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= CurrentResponseData.PacketNameSize)
			{
				CurrentResponseData.PacketName = std::string(m_RecievedData.data() + m_RecievedDataOffset, CurrentResponseData.PacketNameSize);
				m_RecievedDataOffset += CurrentResponseData.PacketNameSize;
				m_RequestFinished = true;
			}
		}
		return(ReturnValue);
	}
	MBError MBPP_Server::p_Handle_UploadFiles()
	{
		return(MBError());
	}
	std::string MBPP_Server::p_GetPacketDirectory(std::string const& PacketName)
	{
		//går igenom packet search directorisen och ser om packeten existerar
		std::string ReturnValue = "";
		for (size_t i = 0; i < m_PacketSearchDirectories.size();i++)
		{
			if (std::filesystem::exists(m_PacketSearchDirectories[i] + PacketName))
			{
				ReturnValue = m_PacketSearchDirectories[i] + PacketName + "/";
				break;
			}
		}
		return(ReturnValue);
	}
	void MBPP_Server::AddPacketSearchDirectory(std::string const& PacketSearchDirectoryToAdd)
	{
		m_PacketSearchDirectories.push_back(PacketSearchDirectoryToAdd);
	}
	//Upload/Remove metoder
	MBError MBPP_Server::InsertClientData(const void* Data, size_t DataSize)
	{
		MBError ReturnValue = true;
		char* ClientData = (char*)Data;
		m_RecievedData += std::string(ClientData, DataSize);
		if (m_CurrentRequestHeader.Type == MBPP_RecordType::Null)
		{
			if (m_RecievedData.size() >= MBPP_GenericRecordHeaderSize)
			{
				m_CurrentRequestHeader = MBPP_ParseRecordHeader(m_RecievedData.data(), m_RecievedData.size(), m_RecievedDataOffset, &m_RecievedDataOffset);
				if (m_CurrentRequestHeader.Type > MBPP_RecordType::Null)
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
		if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetFiles)
		{
			p_Handle_GetFiles();
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetDirectory)
		{
			p_Handle_GetDirectories();
		}
		//tom, kräver ingen extra data
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetPacketInfo)
		{
			p_Handle_GetPacketInfo();
		}
		//lägger alltid 
		return(ReturnValue);
	}
	MBError MBPP_Server::InsertClientData(std::string const& ClientData)
	{
		return(InsertClientData(ClientData.data(), ClientData.size()));
	}
	bool MBPP_Server::ClientRequestFinished()
	{
		return(m_RequestFinished);
	}
	MBPP_ServerResponseIterator* MBPP_Server::GetResponseIterator()
	{
		MBPP_ServerResponseIterator* ReturnValue = nullptr;
		if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetDirectory)
		{
			ReturnValue = new MBPP_GetDirectories_ResponseIterator(p_GetResponseData<MBPP_GetDirectories_ResponseData>(),this);
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetFiles)
		{
			ReturnValue = new MBPP_GetFileList_ResponseIterator(p_GetResponseData<MBPP_GetFileList_ResponseData>(),this);
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetPacketInfo)
		{
			ReturnValue = new MBPP_GetPacketInfo_ResponseIterator(p_GetResponseData<MBPP_GetPacketInfo_ResponseData>(),this);
		}
		return(ReturnValue);
	}
	void MBPP_Server::FreeResponseIterator(MBPP_ServerResponseIterator* IteratorToFree)
	{
		m_RequestFinished = false;
		p_ResetRequestResponseState();
	}

	//MBError MBPP_Server::SendResponse(MBSockets::ConnectSocket* SocketToUse)
	//{
	//
	//}
	//END MBPP_Server

	//BEGIN MBPP_ServerResponseIterator
	MBPP_ServerResponseIterator& MBPP_ServerResponseIterator::operator++()
	{
		Increment();
		return(*this);
	}
	MBPP_ServerResponseIterator& MBPP_ServerResponseIterator::operator++(int)//postfix
	{
		Increment();
		return(*this);
	}
	void MBPP_ServerResponseIterator::Increment()
	{
		assert(false);
	}
	std::string MBPP_ServerResponseIterator::p_GetPacketDirectory(std::string const& PacketName)
	{
		return(m_AssociatedServer->p_GetPacketDirectory(PacketName));
	}

	//END MBPP_ServerResponseIterator
	
	//Specifika iterators
	uint64_t h_CalculateFileListResponseSize(std::vector<std::string> const& FilesToSend, std::string const& PacketDirectory)
	{
		uint64_t ReturnValue = 4; //storleken av pointer av FileList
		for (size_t i = 0; i < FilesToSend.size(); i++)
		{
			ReturnValue += MBPP_StringLengthSize+FilesToSend[i].size();//för filelisten
			ReturnValue += MBGetFileSize(PacketDirectory + FilesToSend[i]);
		}
		return(ReturnValue);
	}
	bool h_UpdateFileListIterator(std::ifstream& FileHandle,std::vector<std::string> const& FileList,size_t& CurrentFileIndex,std::string const& PacketDirectory,std::string& ResponseBuffer)
	{
		bool ReturnValue = false;
		if (CurrentFileIndex >= FileList.size())
		{
			return(true);
		}
		ResponseBuffer = "";
		size_t BufferOffset = 0;
		if (!FileHandle.is_open())
		{
			FileHandle.open(PacketDirectory + FileList[CurrentFileIndex], std::ios::in | std::ios::binary);
			uint64_t FileSize = MBGetFileSize(PacketDirectory + FileList[CurrentFileIndex]);
			ResponseBuffer += std::string(8, 0);
			h_WriteBigEndianInteger(ResponseBuffer.data(), FileSize, 8);
			BufferOffset = 8;
		}
		size_t ReadBlockSize = 4096 * 2;//godtyckligt
		ResponseBuffer += std::string(ReadBlockSize, 0);
		FileHandle.read(ResponseBuffer.data()+BufferOffset, ReadBlockSize);
		size_t ReadBytes = FileHandle.gcount();
		ResponseBuffer.resize(ReadBytes+BufferOffset);
		if (ReadBytes != ReadBlockSize)
		{
			FileHandle.close();
			CurrentFileIndex += 1;
		}
		return(ReturnValue);
	}

	//BEGIN MBPP_GetFileList_ResponseIterator
	MBPP_GetFileList_ResponseIterator::MBPP_GetFileList_ResponseIterator(MBPP_GetFileList_ResponseData const& ResponseData, MBPP_Server* AssociatedServer)
	{
		m_AssociatedServer = AssociatedServer;
		m_HeaderToSend.Type = MBPP_RecordType::FilesData;
		m_HeaderToSend.RecordSize = h_CalculateFileListResponseSize(ResponseData.FilesToGet, p_GetPacketDirectory(ResponseData.PacketName));
		Increment();
	}
	void MBPP_GetFileList_ResponseIterator::Increment()
	{
		MBPP_GetFileList_ResponseData& DataToUse = p_GetResponseData<MBPP_GetFileList_ResponseData>();
		if (!m_FileListSent)
		{
			m_CurrentResponse = MBPP_GetRecordHeader(m_HeaderToSend) + MBPP_GenerateFileList(DataToUse.FilesToGet);
			m_FileListSent = true;
		}
		else
		{
			m_Finished = h_UpdateFileListIterator(m_FileHandle, DataToUse.FilesToGet, m_CurrentFileIndex, p_GetPacketDirectory(DataToUse.PacketName), m_CurrentResponse);
		}
	}
	//END MBPP_GetFileList_ResponseIterator

	//BEGIN MBPP_GetDirectories_ResponseIterator
	MBPP_GetDirectories_ResponseIterator::MBPP_GetDirectories_ResponseIterator(MBPP_GetDirectories_ResponseData const& ResponseData, MBPP_Server* AssociatedServer)
	{
		m_AssociatedServer = AssociatedServer;
		m_HeaderToSend.Type = MBPP_RecordType::DirectoryData;
		m_HeaderToSend.RecordSize = 0;
		std::string PacketDirectory = p_GetPacketDirectory(ResponseData.PacketName);
		for (size_t i = 0; i < ResponseData.DirectoriesToGet.size(); i++)
		{
			//TODO ta från MBPM_FileInfo filen istället
			std::filesystem::recursive_directory_iterator DirectoryIterator = std::filesystem::recursive_directory_iterator(PacketDirectory + ResponseData.DirectoriesToGet[i]);
			for (auto const& Entries : DirectoryIterator)
			{
				if (Entries.is_regular_file())
				{
					m_HeaderToSend.RecordSize += std::filesystem::file_size(Entries.path());
					std::filesystem::path PacketPath = std::filesystem::relative(DirectoryIterator->path(), PacketDirectory);
					m_FilesToSend.push_back(h_PathToUTF8(PacketPath));
				}
			}
		}
		m_HeaderToSend.RecordSize += 4;
		for (size_t i = 0; i < m_FilesToSend.size(); i++)
		{
			m_HeaderToSend.RecordSize += MBPP_StringLengthSize + m_FilesToSend.size();
		}
		Increment();
	}
	void MBPP_GetDirectories_ResponseIterator::Increment()
	{
		MBPP_GetDirectories_ResponseData& DataToUse = p_GetResponseData<MBPP_GetDirectories_ResponseData>();
		if (!m_FileListSent)
		{
			m_CurrentResponse = MBPP_GetRecordHeader(m_HeaderToSend) + MBPP_GenerateFileList(m_FilesToSend);
			m_FileListSent = true;
		}
		else
		{
			m_Finished = h_UpdateFileListIterator(m_FileHandle, m_FilesToSend, m_CurrentFileIndex, p_GetPacketDirectory(DataToUse.PacketName), m_CurrentResponse);
		}
	}
	//END MBPP_GetDirectories_ResponseIterator

	//BEGIN MBPP_GetPacketInfo_ResponseIterator
	MBPP_GetPacketInfo_ResponseIterator::MBPP_GetPacketInfo_ResponseIterator(MBPP_GetPacketInfo_ResponseData const& ResponseData, MBPP_Server* AssociatedServer)
	{
		m_AssociatedServer = AssociatedServer;
		m_HeaderToSend.Type = MBPP_RecordType::FilesData;
		m_HeaderToSend.RecordSize = h_CalculateFileListResponseSize(m_FilesToSend, p_GetPacketDirectory(ResponseData.PacketName));
		Increment();
	}
	void MBPP_GetPacketInfo_ResponseIterator::Increment()
	{
		MBPP_GetPacketInfo_ResponseData& DataToUse = p_GetResponseData<MBPP_GetPacketInfo_ResponseData>();
		if (!m_FileListSent)
		{
			m_CurrentResponse =MBPP_GetRecordHeader(m_HeaderToSend)+ MBPP_GenerateFileList(m_FilesToSend);
			m_FileListSent = true;
		}
		else
		{
			m_Finished = h_UpdateFileListIterator(m_FileHandle, m_FilesToSend, m_CurrentFileIndex, p_GetPacketDirectory(DataToUse.PacketName), m_CurrentResponse);
		}
	}
	//END MBPP_GetPacketInfo_ResponseIterator
}