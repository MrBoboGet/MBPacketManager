#include "MB_PacketProtocol.h"
#include <MBParsing/MBParsing.h>
#include <set>
#include <MBCrypto/MBCrypto.h>
#include <MBUtility/MBAlgorithms.h>
#include <algorithm>
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
	void h_WriteBigEndianInteger(MBUtility::MBOctetOutputStream* OutputStream, uintmax_t IntegerToWrite, uint8_t IntegerSize)
	{
		char BigEndianData[sizeof(uintmax_t)];
		for (size_t i = 0; i < IntegerSize; i++)
		{
			BigEndianData[i] = (IntegerToWrite >> ((IntegerSize * 8) - ((i + 1) * 8)));
		}
		OutputStream->Write(BigEndianData, IntegerSize);
	}
	std::string h_GetBigEndianInteger(uintmax_t IntegerToConvert, uint8_t IntegerSize)
	{
		std::string ReturnValue = std::string(IntegerSize, 0);
		h_WriteBigEndianInteger(ReturnValue.data(), IntegerToConvert, IntegerSize);
		return(ReturnValue);
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
	std::string h_WriteFileData(MBUtility::MBSearchableOutputStream* OutputStream, std::filesystem::path const& FilePath,MBCrypto::HashFunction HashFunctionToUse,uint64_t* OutFileSize)
	{
		//TODO fixa så det itne blir fel på windows
		std::string FileName = FilePath.filename().generic_u8string();
		h_WriteBigEndianInteger(OutputStream, FileName.size(), 2);
		OutputStream->Write(FileName.data(), FileName.size());
		std::string FileHash = MBCrypto::GetFileHash(FilePath.generic_u8string(),HashFunctionToUse);
		OutputStream->Write(FileHash.data(), FileHash.size());
		uint64_t FileSize = std::filesystem::file_size(FilePath);
		h_WriteBigEndianInteger(OutputStream, FileSize, 8);
		if (OutFileSize != nullptr)
		{
			*OutFileSize = FileSize;
		}
		return(FileHash);
	}

	MBPP_DirectoryHashData h_WriteDirectoryData_Recursive(MBUtility::MBSearchableOutputStream* FileToWriteTo, std::string const& TopPacketDirectory, std::string const& DirectoryName, std::string const& SubPathToIterate,
		MBPM_FileInfoExcluder& FileInfoExcluder,MBCrypto::HashFunction HashFunctionToUse,uint64_t* OutFileSize)
	{
		std::set<std::string> DirectoriesToWrite = {};
		std::set<std::filesystem::path> FilesToWrite = {};
		std::filesystem::directory_iterator DirectoryIterator = std::filesystem::directory_iterator(SubPathToIterate);
		for (auto const& Entries : DirectoryIterator)
		{
			std::string AbsolutePacketPath = "/"+h_PathToUTF8(std::filesystem::relative(Entries.path(),TopPacketDirectory));
			if (FileInfoExcluder.Excludes(AbsolutePacketPath) && !FileInfoExcluder.Includes(AbsolutePacketPath))
			{
				continue;
			}
			if (Entries.is_regular_file())
			{
				FilesToWrite.insert(Entries.path());
			}
			else if(Entries.is_directory())
			{
				DirectoriesToWrite.insert(Entries.path().generic_u8string());
			}
		}
		uint64_t SkipPointerPosition = FileToWriteTo->GetOutputPosition();
		h_WriteBigEndianInteger(FileToWriteTo, 0, 8); //offset innan vi en läst filen
		//plats för directory hashet
		char StructureHash[20]; //TODO hardcoda inte hash längden...
		FileToWriteTo->Write(StructureHash, 20);
		char ContentHash[20]; //TODO hardcoda inte hash längden...
		FileToWriteTo->Write(ContentHash, 20);

		//här har vi även directory sizen
		uint64_t TotalDirectorySize = 0;
		h_WriteBigEndianInteger(FileToWriteTo, 0, 8);

		//ANTAGANDE pathen är inte på formen /något/hej/ utan /något/hej
		h_WriteBigEndianInteger(FileToWriteTo, DirectoryName.size(), 2);
		FileToWriteTo->Write(DirectoryName.data(), DirectoryName.size());
		h_WriteBigEndianInteger(FileToWriteTo, FilesToWrite.size(), 4);
		//std::string DataToHash = "";
		std::string StructureHashData = "";
		std::vector<std::string> ContentHashes = {};
		for (auto const& Files : FilesToWrite)
		{
			uint64_t NewSize = 0;
			std::string FileHash = h_WriteFileData(FileToWriteTo, Files, HashFunctionToUse, &NewSize);
			std::string FileName = h_PathToUTF8(Files.filename());
			StructureHashData += FileName+FileHash; //sorterat på filordning
			ContentHashes.push_back(FileHash);
			TotalDirectorySize += NewSize;
		}
		h_WriteBigEndianInteger(FileToWriteTo, DirectoriesToWrite.size(), 4);
		for (auto const& Directories : DirectoriesToWrite)
		{
			uint64_t NewSize = 0;
			MBPP_DirectoryHashData CurrentDirectoryHashData = h_WriteDirectoryData_Recursive(FileToWriteTo, TopPacketDirectory, std::filesystem::path(Directories).filename().generic_u8string(), Directories, FileInfoExcluder, HashFunctionToUse, &NewSize);
			
			std::string DirectoryName = "";
			size_t LastSlashPosition = Directories.find('/');
			if (LastSlashPosition == Directories.npos)
			{
				LastSlashPosition = 0;
			}
			else
			{
				LastSlashPosition += 1;
			}
			DirectoryName = Directories.substr(LastSlashPosition);
			StructureHashData += DirectoryName + CurrentDirectoryHashData.StructureHash;
			ContentHashes.push_back(CurrentDirectoryHashData.ContentHash);
			TotalDirectorySize += NewSize;
		}
		uint64_t DirectoryEndPosition = FileToWriteTo->GetOutputPosition();
		FileToWriteTo->SetOutputPosition(SkipPointerPosition);
		h_WriteBigEndianInteger(FileToWriteTo, DirectoryEndPosition, 8);
		std::string DirectoryStructureHash = MBCrypto::HashData(StructureHashData, HashFunctionToUse);
		FileToWriteTo->Write(DirectoryStructureHash.data(), DirectoryStructureHash.size());
		std::string DirectoryContentHashToHash = "";
		std::sort(ContentHashes.begin(),ContentHashes.end());
		for (auto const& Hashes : ContentHashes)
		{
			DirectoryContentHashToHash += Hashes;
		}
		std::string DirectoryContentHash = MBCrypto::HashData(DirectoryContentHashToHash, HashFunctionToUse);
		FileToWriteTo->Write(DirectoryContentHash.data(), DirectoryContentHash.size());
		MBPP_DirectoryHashData ReturnValue;
		ReturnValue.ContentHash = DirectoryContentHash;
		ReturnValue.StructureHash = DirectoryStructureHash;

		//skriver även storleken här
		h_WriteBigEndianInteger(FileToWriteTo, TotalDirectorySize, 8);
		FileToWriteTo->SetOutputPosition(DirectoryEndPosition);
		if (OutFileSize != nullptr)
		{
			*OutFileSize = TotalDirectorySize;
		}
		return(ReturnValue);
	}
	void h_WriteFileInfoHeader(MBPP_FileInfoHeader const& HeaderToWrite,MBUtility::MBOctetOutputStream* StreamToWriteTo)
	{
		size_t WrittenBytes = StreamToWriteTo->Write(HeaderToWrite.ArbitrarySniffData.data(), HeaderToWrite.ArbitrarySniffData.size());
		//borde ha att h_writeBigEndianInteger skriver 
		for (size_t i = 0; i < 3; i++)
		{
			h_WriteBigEndianInteger(StreamToWriteTo, HeaderToWrite.Version[i], 2);
		}
		h_WriteBigEndianInteger(StreamToWriteTo,uint32_t(HeaderToWrite.HashFunction), 4);
		h_WriteBigEndianInteger(StreamToWriteTo,HeaderToWrite.ExtensionDataSize, 4);
	}
	MBPP_FileInfoHeader h_ReadFileInfoHeader(MBUtility::MBOctetInputStream* StreamToReadFrom,MBError* OutError)
	{
		MBPP_FileInfoHeader ReturnValue;
		char InitialData[MBPP_FileInfoHeaderStaticSize+1];
		InitialData[MBPP_FileInfoHeaderStaticSize] = 0;
		size_t ReadBytes = StreamToReadFrom->Read(InitialData, MBPP_FileInfoHeaderStaticSize);
		size_t ParseOffset = 0;
		if (ReturnValue.ArbitrarySniffData != InitialData)
		{
			*OutError = false;
			OutError->ErrorMessage = "Invalid header bytes";
			return(ReturnValue);
		}
		ParseOffset += ReturnValue.ArbitrarySniffData.size();
		for (size_t i = 0; i < 3; i++)
		{
			ReturnValue.Version[i] = MBParsing::ParseBigEndianInteger(InitialData, 2, ParseOffset, &ParseOffset);
		}
		ReturnValue.HashFunction = MBCrypto::HashFunction(MBParsing::ParseBigEndianInteger(InitialData, 4, ParseOffset, &ParseOffset));
		ReturnValue.ExtensionDataSize = MBParsing::ParseBigEndianInteger(InitialData, 4, ParseOffset, &ParseOffset);
		return(ReturnValue);
	}
	void CreatePacketFilesData(std::string const& PacketToHashDirectory,std::string const& OutputName)
	{
		std::ofstream OutputFile = std::ofstream(PacketToHashDirectory +"/"+ OutputName,std::ios::out|std::ios::binary);
		if (!OutputFile.is_open())
		{
			return;
		}
		MBPM_FileInfoExcluder Excluder;
		if (std::filesystem::exists(PacketToHashDirectory + "/MBPM_FileInfoIgnore"))
		{
			Excluder = MBPM_FileInfoExcluder(PacketToHashDirectory + "/MBPM_FileInfoIgnore");
		}
		Excluder.AddExcludeFile("/"+OutputName);
		Excluder.AddExcludeFile("/MBPM_UploadedChanges/");
		Excluder.AddExcludeFile("/MBPM_BuildFiles/");
		//Excluder.AddExcludeFile("/MBPM_Builds/");
		Excluder.AddExcludeFile("/.mbpm/");
		MBUtility::MBFileOutputStream OutputStream = MBUtility::MBFileOutputStream(&OutputFile);
		h_WriteFileInfoHeader(MBPP_FileInfoHeader(), &OutputStream);
		//DEBUG
		//std::cout << bool(OutputFile.is_open() && OutputFile.good()) << std::endl;
		//OutputFile.flush();
		//OutputFile.close();
		//return;
		h_WriteDirectoryData_Recursive(&OutputStream, PacketToHashDirectory, "/", PacketToHashDirectory, Excluder, MBCrypto::HashFunction::SHA1,nullptr);
		OutputFile.flush();
		OutputFile.close();
	}
	bool MBPP_PathIsValid(std::string const& PathToCheck)
	{
		if (PathToCheck.find("..") != PathToCheck.npos)
		{
			return(false);
		}
		return(true);
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
		ReturnValue.VerificationDataSize = MBParsing::ParseBigEndianInteger(Data, 4, ParseOffset, &ParseOffset);
		if (OutOffset != nullptr)
		{
			*OutOffset = ParseOffset;
		}
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
		ReturnValue += std::string(20, 0);
		size_t ParseOffest = 2;
		h_WriteBigEndianInteger(ReturnValue.data()+2, uint32_t(RecordToConvert.ComputerInfo.OSType), 4);
		h_WriteBigEndianInteger(ReturnValue.data()+6, uint32_t(RecordToConvert.ComputerInfo.ProcessorType), 4);
		h_WriteBigEndianInteger(ReturnValue.data()+10, RecordToConvert.RecordSize, 8);
		h_WriteBigEndianInteger(ReturnValue.data()+18, RecordToConvert.VerificationDataSize, 4);
		return(ReturnValue);
	}
	std::string MBPP_GetRecordData(MBPP_GenericRecord const& RecordToConvert)
	{
		return(MBPP_GetRecordHeader(RecordToConvert) +RecordToConvert.VerificationData+ RecordToConvert.RecordData);
	}
	std::string MBPP_GetErrorCodeString(MBPP_ErrorCode ErrorToConvert)
	{
		return("palla inte implementera det nu lmaooo");
	}
	std::string MBPP_GetUploadCredentialsData(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData)
	{
		std::string ReturnValue = h_GetBigEndianInteger(uint64_t(CredentialsType),sizeof(MBPP_UserCredentialsType));
		ReturnValue += MBPP_EncodeString(PacketName);
		ReturnValue += h_GetBigEndianInteger(CredentialsData.size(), 4);
		ReturnValue += CredentialsData;
		return(ReturnValue);
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
	//BEGIN MBPM_FileInfoExcluder
	bool MBPM_FileInfoExcluder::p_MatchDirectory(std::string const& Directory, std::string const& StringToMatch)
	{
		bool ReturnValue = false;
		std::string ProcessedStringToMatch = StringToMatch;
		if (StringToMatch.back() != '/')
		{
			ProcessedStringToMatch += '/';
		}
		if (Directory.size() > ProcessedStringToMatch.size())
		{
			return(false);
		}
		return(ProcessedStringToMatch.substr(0, Directory.size()) == Directory);
	}
	bool MBPM_FileInfoExcluder::p_MatchFile(std::string const& StringToCompare, std::string const& StringToMatch)
	{
		return(StringToCompare == StringToMatch);
	}
	void MBPM_FileInfoExcluder::AddExcludeFile(std::string const& FileToExlude)
	{
		m_ExcludeStrings.push_back(FileToExlude);
	}
	MBPM_FileInfoExcluder::MBPM_FileInfoExcluder(std::string const& PathPosition)
	{
		std::ifstream InputStream = std::ifstream(PathPosition, std::ios::in);
		std::string CurrentLine = "";
		bool IsInclude = false;
		bool IsExclude = false;
		while (std::getline(InputStream, CurrentLine))
		{
			if (CurrentLine == "Excludes:")
			{
				IsExclude = true;
				IsInclude = false;
			}
			else if (CurrentLine == "Includes:")
			{
				IsExclude = false;
				IsInclude = true;
			}
			else if(CurrentLine != "")
			{
				if (IsInclude)
				{
					m_IncludeStrings.push_back(CurrentLine);
				}
				else
				{
					m_ExcludeStrings.push_back(CurrentLine);
				}
			}
		}
	}
	bool MBPM_FileInfoExcluder::Excludes(std::string const& StringToMatch)
	{
		bool ReturnValue = false;
		for (size_t i = 0; i < m_ExcludeStrings.size(); i++)
		{
			std::string CurrentExludeString = m_ExcludeStrings[i];
			if (CurrentExludeString.front() != '/')
			{
				continue;
			}
			if (CurrentExludeString.back() == '/')
			{
				ReturnValue = p_MatchDirectory(CurrentExludeString, StringToMatch);
			}
			else
			{
				ReturnValue = p_MatchFile(CurrentExludeString, StringToMatch);
			}
			if (ReturnValue)
			{
				break;
			}
		}
		return(ReturnValue);
	}
	bool MBPM_FileInfoExcluder::Includes(std::string const& StringToMatch)
	{
		bool ReturnValue = false;
		for (size_t i = 0; i < m_IncludeStrings.size(); i++)
		{
			std::string CurrentIncludeString = m_IncludeStrings[i];
			if (CurrentIncludeString.front() != '/')
			{
				continue;
			}
			if (CurrentIncludeString.back() == '/')
			{
				ReturnValue = p_MatchDirectory(CurrentIncludeString, StringToMatch);
			}
			else
			{
				ReturnValue = p_MatchFile(CurrentIncludeString, StringToMatch);
			}
			if (ReturnValue)
			{
				break;
			}
		}
		return(ReturnValue);
	}
	//END MBPM_FileInfoExcluder
	
	//BEGIN MBPP_FileInfoReader
	MBPP_DirectoryInfoNode MBPP_FileInfoReader::p_ReadDirectoryInfoFromFile(MBUtility::MBOctetInputStream* FileToReadFrom,size_t HashSize)
	{
		//Skippointer 8 bytes
		//StructureHash
		//ContentHash
		//Namn, 2 bytes längd + längd bytes namn data
		//Number of files 4 bytes, följt av fil bytes
		//number of directories 4 bytes, följt av directoriesen
		//skip poointer pekar till slutet av directory datan
		MBPP_DirectoryInfoNode ReturnValue;
		h_ReadBigEndianInteger(FileToReadFrom, 8);//filepointer
		std::string DirectoryHash = std::string(HashSize, 0);
		std::string ContentHash = std::string(HashSize, 0);
		//vi läser även in storleken
		FileToReadFrom->Read(DirectoryHash.data(), HashSize);
		FileToReadFrom->Read(ContentHash.data(), HashSize);
		uint64_t DirectorySize = h_ReadBigEndianInteger(FileToReadFrom, 8);
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
		ReturnValue.StructureHash = std::move(DirectoryHash);
		ReturnValue.ContentHash = std::move(ContentHash);
		ReturnValue.DirectoryName = std::move(DirectoryName);
		ReturnValue.Size = DirectorySize;
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
		ReturnValue.FileSize = h_ReadBigEndianInteger(InputStream, 8);
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
		try
		{
			MBError ParseError = true;
			m_InfoHeader = h_ReadFileInfoHeader(&FileReader, &ParseError);
			if (ParseError)
			{
				m_TopNode = p_ReadDirectoryInfoFromFile(&FileReader, 20);//sha1 diges size
			}
		}
		catch (const std::exception&)
		{
			m_TopNode = MBPP_DirectoryInfoNode();
		}

	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(const void* DataToRead, size_t DataSize)
	{
		MBUtility::MBBufferInputStream BufferReader(DataToRead,DataSize);
		MBError ParseError = true;
		m_InfoHeader = h_ReadFileInfoHeader(&BufferReader, &ParseError);
		if (ParseError)
		{
			m_TopNode = p_ReadDirectoryInfoFromFile(&BufferReader, 20);//sha1 diges size
		}
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
				if (ClientIterator->StructureHash != ServerIterator->StructureHash)
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
	const MBPP_DirectoryInfoNode* MBPP_FileInfoReader::p_GetTargetDirectory(std::vector<std::string> const& PathComponents) const
	{
		return(p_GetTargetDirectory(PathComponents));
	}

	MBPP_DirectoryInfoNode* MBPP_FileInfoReader::p_GetTargetDirectory(std::vector<std::string> const& PathComponents)
	{
		MBPP_DirectoryInfoNode* ReturnValue = nullptr;
		if (PathComponents.size() == 1  || PathComponents.size() == 0)
		{
			ReturnValue = &m_TopNode;
		}
		else
		{
			bool FileExists = true;
			MBPP_DirectoryInfoNode* CurrentNode = &m_TopNode;
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
	bool MBPP_FileInfoReader::ObjectExists(std::string const& ObjectToSearch) const
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
	const MBPP_FileInfo * MBPP_FileInfoReader::GetFileInfo(std::string const& ObjectToSearch) const
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
	const MBPP_DirectoryInfoNode* MBPP_FileInfoReader::GetDirectoryInfo(std::string const& ObjectToSearch) const
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
		if (m_TopNode.DirectoryName != OtherReader.m_TopNode.DirectoryName || m_TopNode.StructureHash != OtherReader.m_TopNode.StructureHash) 
		{
			ReturnValue = false;
		}
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::p_UpdateDirectoriesParents(MBPP_DirectoryInfoNode* DirectoryToUpdate)
	{
		const size_t DirectoryCount = DirectoryToUpdate->Directories.size();
		for (size_t i = 0; i < DirectoryCount;i++)
		{
			DirectoryToUpdate->Directories[i].m_ParentDirectory = DirectoryToUpdate;
			p_UpdateDirectoriesParents(&(DirectoryToUpdate->Directories[i]));
		}
	}
	void swap(MBPP_FileInfoReader& LeftInfoReader, MBPP_FileInfoReader& RightInfoReader) noexcept
	{
		std::swap(LeftInfoReader.m_TopNode, RightInfoReader.m_TopNode);
		LeftInfoReader.p_UpdateDirectoriesParents(&LeftInfoReader.m_TopNode);
		RightInfoReader.p_UpdateDirectoriesParents(&RightInfoReader.m_TopNode);
	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(MBPP_FileInfoReader&& ReaderToSteal) noexcept
	{
		swap(*this, ReaderToSteal);
	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(MBPP_FileInfoReader const& ReaderToCopy)
	{
		m_TopNode = ReaderToCopy.m_TopNode;
		p_UpdateDirectoriesParents(&m_TopNode);
	}
	MBPP_FileInfoReader& MBPP_FileInfoReader::operator=(MBPP_FileInfoReader ReaderToCopy)
	{
		swap(*this, ReaderToCopy);
		return(*this);
	}
	std::string MBPP_FileInfoReader::GetBinaryString()
	{
		std::string ReturnValue = "";
		MBUtility::MBStringOutputStream OutputStream = MBUtility::MBStringOutputStream(ReturnValue);
		WriteData(&OutputStream);
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::p_WriteHeader(MBPP_FileInfoHeader const& HeaderToWrite, MBUtility::MBOctetOutputStream* OutputStream)
	{
		h_WriteFileInfoHeader(HeaderToWrite, OutputStream);
	}
	void MBPP_FileInfoReader::p_WriteFileInfo(MBPP_FileInfo const& InfoToWrite, MBUtility::MBOctetOutputStream* OutputStream)
	{
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.FileName.size(), 2);
		OutputStream->Write(InfoToWrite.FileName.data(), InfoToWrite.FileName.size());
		OutputStream->Write(InfoToWrite.FileHash.data(), InfoToWrite.FileHash.size());
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.FileSize, 8);
	}
	void MBPP_FileInfoReader::p_WriteDirectoryInfo(MBPP_DirectoryInfoNode const& InfoToWrite, MBUtility::MBSearchableOutputStream* OutputStream)
	{
		//Skippointer 8 bytes
		//StructureHash
		//ContentHash
		//Namn, 2 bytes längd + längd bytes namn data
		//Number of files 4 bytes, följt av fil bytes
		//number of directories 4 bytes, följt av directoriesen
		//skip poointer pekar till slutet av directory datan
		uint64_t DirectoryHeaderPosition = OutputStream->GetOutputPosition();
		h_WriteBigEndianInteger(OutputStream, 0, 8);
		OutputStream->Write(InfoToWrite.StructureHash.data(), InfoToWrite.StructureHash.size());
		OutputStream->Write(InfoToWrite.ContentHash.data(), InfoToWrite.ContentHash.size());
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.Size, 8);

		h_WriteBigEndianInteger(OutputStream, InfoToWrite.DirectoryName.size(), 2);
		OutputStream->Write(InfoToWrite.DirectoryName.data(), InfoToWrite.DirectoryName.size());
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.Files.size(), 4);
		for (size_t i = 0; i < InfoToWrite.Files.size(); i++)
		{
			p_WriteFileInfo(InfoToWrite.Files[i], OutputStream);
		}
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.Directories.size(), 4);
		for (size_t i = 0; i < InfoToWrite.Directories.size(); i++)
		{
			p_WriteDirectoryInfo(InfoToWrite.Directories[i],OutputStream);
		}
		uint64_t EndSkip = OutputStream->GetOutputPosition();
		OutputStream->SetOutputPosition(DirectoryHeaderPosition);
		h_WriteBigEndianInteger(OutputStream, EndSkip, 8);
		OutputStream->SetOutputPosition(EndSkip);
	}
	void MBPP_FileInfoReader::WriteData(MBUtility::MBSearchableOutputStream* OutputStream)
	{
		p_WriteHeader(MBPP_FileInfoHeader(), OutputStream);
		p_WriteDirectoryInfo(m_TopNode, OutputStream);
	}

	void MBPP_FileInfoReader::p_UpdateDirectoryInfo(MBPP_DirectoryInfoNode& NodeToUpdate, MBPP_DirectoryInfoNode const& UpdatedNode)
	{
		std::vector<MBPP_FileInfo>::iterator ThisNodeFiles = NodeToUpdate.Files.begin();
		std::vector<MBPP_FileInfo>::iterator ThisNodeFilesEnd = NodeToUpdate.Files.end();
		std::vector<MBPP_FileInfo>::const_iterator OtherNodeFiles = UpdatedNode.Files.begin();
		std::vector<MBPP_FileInfo>::const_iterator OtherNodeFilesEnd = UpdatedNode.Files.end();
		
		std::vector<MBPP_FileInfo> NewFileList = {};
		NewFileList.reserve(NodeToUpdate.Files.size() + UpdatedNode.Files.size());
		while (ThisNodeFiles != ThisNodeFilesEnd && OtherNodeFiles != OtherNodeFilesEnd)
		{
			if (ThisNodeFiles->FileName < OtherNodeFiles->FileName)
			{
				//innebär egentligen bara att vi har mer lokala filer än filer vi uppdaterar, inget problem vi bara fortsätter
				NewFileList.push_back(std::move(*ThisNodeFiles));
				ThisNodeFiles++;
			}
			else if (ThisNodeFiles->FileName > OtherNodeFiles->FileName)
			{
				NewFileList.push_back(*OtherNodeFiles);
				OtherNodeFiles++;
			}
			else
			{
				NewFileList.push_back(*OtherNodeFiles);
				ThisNodeFiles++;
				OtherNodeFiles++;
			}
		}
		while (ThisNodeFiles != ThisNodeFilesEnd)
		{
			NewFileList.push_back(std::move(*ThisNodeFiles));
			ThisNodeFiles++;
		}
		while (OtherNodeFiles != OtherNodeFilesEnd)
		{
			NewFileList.push_back(std::move(*OtherNodeFiles));
			OtherNodeFiles++;
		}
		NodeToUpdate.Files = std::move(NewFileList);


		std::vector<MBPP_DirectoryInfoNode>::iterator ThisNodeDirectories = NodeToUpdate.Directories.begin();
		std::vector<MBPP_DirectoryInfoNode>::iterator ThisNodeDirectoriesEnd = NodeToUpdate.Directories.end();
		std::vector<MBPP_DirectoryInfoNode>::const_iterator OtherNodeDirectories = UpdatedNode.Directories.begin();
		std::vector<MBPP_DirectoryInfoNode>::const_iterator OtherNodeDirectoriesEnd = UpdatedNode.Directories.end();

		std::vector<MBPP_DirectoryInfoNode> NewDirectoryList = {};
		NewFileList.reserve(NodeToUpdate.Files.size() + UpdatedNode.Files.size());
		while (ThisNodeDirectories != ThisNodeDirectoriesEnd && OtherNodeDirectories != OtherNodeDirectoriesEnd)
		{
			if (ThisNodeDirectories->DirectoryName < OtherNodeDirectories->DirectoryName)
			{
				//innebär egentligen bara att vi har mer lokala filer än filer vi uppdaterar, inget problem vi bara fortsätter
				NewDirectoryList.push_back(std::move(*ThisNodeDirectories));
				ThisNodeDirectories++;
			}
			else if (ThisNodeDirectories->DirectoryName > OtherNodeDirectories->DirectoryName)
			{
				//om det är en ny directory lägger vi helt enkelt till den
				NewDirectoryList.push_back(*OtherNodeDirectories);
				OtherNodeDirectories++;
			}
			else
			{
				//NewDirectoryList.push_back(*OtherNodeDirectories);
				p_UpdateDirectoryInfo(*ThisNodeDirectories, *OtherNodeDirectories);
				NewDirectoryList.push_back(std::move(*ThisNodeDirectories));
				ThisNodeDirectories++;
				OtherNodeDirectories++;
			}
		}
		while (ThisNodeDirectories != ThisNodeDirectoriesEnd)
		{
			NewDirectoryList.push_back(std::move(*ThisNodeDirectories));
			ThisNodeDirectories++;
		}
		while (OtherNodeDirectories != OtherNodeDirectoriesEnd)
		{
			NewDirectoryList.push_back(*OtherNodeDirectories);
			OtherNodeDirectories++;
		}
		NodeToUpdate.Directories = std::move(NewDirectoryList);

		NodeToUpdate.Size = 0;
		for (size_t i = 0; i < NodeToUpdate.Files.size(); i++)
		{
			NodeToUpdate.Size += NodeToUpdate.Files[i].FileSize;
		}
		for (size_t i = 0; i < NodeToUpdate.Directories.size(); i++)
		{
			NodeToUpdate.Size += NodeToUpdate.Directories[i].Size;
		}
		//for (size_t i = 0; i < UpdatedNode.Files.size(); i++)
		//{
		//	int FilePosition = MBAlgorithms::BinarySearch(NodeToUpdate.Files, UpdatedNode.Files[i], h_GetFileName, std::less<std::string>());
		//	if (FilePosition != -1)
		//	{
		//		NodeToUpdate.Files[FilePosition] = UpdatedNode.Files[i];
		//	}
		//	else
		//	{
		//		//vi ska alltså lägga till filen
		//		NodeToUpdate.Files.insert()
		//	}
		//}
	}
	void MBPP_FileInfoReader::p_DeleteObject(std::string const& ObjectPath)
	{
		std::vector<std::string> TargetComponents = sp_GetPathComponents(ObjectPath);
		MBPP_DirectoryInfoNode* TargetDirectory = p_GetTargetDirectory(TargetComponents);
		int TargetIndex = MBAlgorithms::BinarySearch(TargetDirectory->Directories, TargetComponents.back(), h_GetDirectoryName, std::less<std::string>());
		if (TargetIndex != -1)
		{
			TargetDirectory->Directories.erase(TargetDirectory->Directories.begin() + TargetIndex);
		}
		int FilePosition = MBAlgorithms::BinarySearch(TargetDirectory->Files, TargetComponents.back(), h_GetFileName, std::less<std::string>());
		if (TargetIndex != -1)
		{
			TargetDirectory->Files.erase(TargetDirectory->Files.begin() + FilePosition);
		}
	}
	MBPP_DirectoryHashData MBPP_FileInfoReader::p_UpdateDirectoryStructureHash(MBPP_DirectoryInfoNode& NodeToUpdate)
	{
		//antar att directoryn är sorterad
		std::vector<std::string> ContentHashes = {};
		std::string StructureHashData = "";
		for(size_t i = 0; i < NodeToUpdate.Files.size();i++)
		{
			StructureHashData += NodeToUpdate.Files[i].FileName+ NodeToUpdate.Files[i].FileHash;
			ContentHashes.push_back(NodeToUpdate.Files[i].FileHash);
		}
		for (size_t i = 0; i < NodeToUpdate.Directories.size(); i++)
		{
			MBPP_DirectoryHashData DirectoryHash = p_UpdateDirectoryStructureHash(NodeToUpdate.Directories[i]);
			StructureHashData += NodeToUpdate.Directories[i].DirectoryName + DirectoryHash.StructureHash;
			ContentHashes.push_back(DirectoryHash.ContentHash);
		}
		MBPP_DirectoryHashData ReturnValue;
		ReturnValue.StructureHash = MBCrypto::HashData(StructureHashData, m_InfoHeader.HashFunction);
		std::sort(ContentHashes.begin(), ContentHashes.end());
		std::string TotalContentHash = "";
		for (std::string const& Hashes : ContentHashes)
		{
			TotalContentHash += Hashes;
		}
		ReturnValue.ContentHash = MBCrypto::HashData(TotalContentHash,m_InfoHeader.HashFunction);
		NodeToUpdate.ContentHash = ReturnValue.ContentHash;
		NodeToUpdate.StructureHash = ReturnValue.StructureHash;
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::p_UpdateHashes()
	{
		p_UpdateDirectoryStructureHash(m_TopNode);
	}
	void MBPP_FileInfoReader::UpdateInfo(MBPP_FileInfoReader const& NewInfo)
	{
		//hej
		p_UpdateDirectoryInfo(m_TopNode, NewInfo.m_TopNode);
		p_UpdateHashes();
		p_UpdateDirectoriesParents(&m_TopNode);
	}


	//END MBPP_FileInfoReader

	//BEGIN MBPP_DirectoryInfoNode_ConstIterator
	void MBPP_DirectoryInfoNode_ConstIterator::p_Increment()
	{
		while (m_IterationInfo.size() > 0)
		{
			m_IterationInfo.top().CurrentFileOffset += 1;
			if (m_IterationInfo.top().CurrentFileOffset >= m_IterationInfo.top().AssociatedDirectory->Files.size())
			{
				//ska lägga till en ny directory på stacken, eller gå ner för den
				m_IterationInfo.top().CurrentDirectoryOffset += 1;
				if (m_IterationInfo.top().CurrentDirectoryOffset < m_IterationInfo.top().AssociatedDirectory->Directories.size())
				{
					//händer enbart då vi ska lägga till en ny
					DirectoryIterationInfo NewInfo;
					NewInfo.CurrentDirectoryOffset = -1;
					NewInfo.CurrentFileOffset = -1;
					NewInfo.AssociatedDirectory = &(m_IterationInfo.top().AssociatedDirectory->Directories[m_IterationInfo.top().CurrentDirectoryOffset]);
					m_IterationInfo.push(NewInfo);
					m_CurrentDirectoryPath += NewInfo.AssociatedDirectory->DirectoryName + "/";
				}
				else
				{
					//vi har iterat över alla directories och ska då poppa
					if (m_IterationInfo.size() > 1)
					{
						m_CurrentDirectoryPath.resize(m_CurrentDirectoryPath.size() - (m_IterationInfo.top().AssociatedDirectory->DirectoryName.size() + 1));
					}
					m_IterationInfo.pop();
				}
			}
			else
			{
				//den nuvarande offseten är giltig, vi breaker
				break;
			}
		}
		if (m_IterationInfo.size() == 0)
		{
			m_Finished = true;
		}
	}
	MBPP_DirectoryInfoNode_ConstIterator::MBPP_DirectoryInfoNode_ConstIterator(const MBPP_DirectoryInfoNode* InitialNode)
	{
		m_Finished = false;
		DirectoryIterationInfo NewInfo;
		NewInfo.AssociatedDirectory = InitialNode;
		NewInfo.CurrentDirectoryOffset = -1;
		NewInfo.CurrentFileOffset = -1;
		m_IterationInfo.push(NewInfo); //blir relativt till den här nodens grejer
		p_Increment();
	}
	bool MBPP_DirectoryInfoNode_ConstIterator::operator==(MBPP_DirectoryInfoNode_ConstIterator const& IteratorToCompare) const
	{
		return(m_Finished == IteratorToCompare.m_Finished);
	}
	bool MBPP_DirectoryInfoNode_ConstIterator::operator!=(MBPP_DirectoryInfoNode_ConstIterator const& IteratorToCompare) const
	{
		return(!(m_Finished == IteratorToCompare.m_Finished));
	}
	MBPP_DirectoryInfoNode_ConstIterator& MBPP_DirectoryInfoNode_ConstIterator::operator++()
	{
		p_Increment();
		return(*this);
	}
	MBPP_DirectoryInfoNode_ConstIterator& MBPP_DirectoryInfoNode_ConstIterator::operator++(int)
	{
		p_Increment();
		return(*this);
	}
	MBPP_FileInfo const& MBPP_DirectoryInfoNode_ConstIterator::operator*()
	{
		return(m_IterationInfo.top().AssociatedDirectory->Files[m_IterationInfo.top().CurrentFileOffset]);
	}
	MBPP_FileInfo const& MBPP_DirectoryInfoNode_ConstIterator::operator->()
	{
		return(**this);
	}

	MBPP_DirectoryInfoNode_ConstIterator MBPP_DirectoryInfoNode::begin() const
	{
		return(MBPP_DirectoryInfoNode_ConstIterator(this));
	}
	MBPP_DirectoryInfoNode_ConstIterator MBPP_DirectoryInfoNode::end() const
	{
		return(MBPP_DirectoryInfoNode_ConstIterator());
	}
	//END MBPP_DirectoryInfoNode_ConstIterator

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


	//BEGIN MBPP_FileListDownloader : public MBPP_FileListDownloadHandler
	MBPP_FileListDownloader::MBPP_FileListDownloader(std::string const& OutputDirectory)
	{
		m_OutputDirectory = OutputDirectory;
	}
	MBError MBPP_FileListDownloader::Open(std::string const& FileToDownloadName)
	{
		MBError ReturnValue = true;
		std::string FileOutputPath = m_OutputDirectory + FileToDownloadName;
		std::filesystem::create_directories(std::filesystem::path(FileOutputPath).parent_path());
		m_FileHandle.open(FileOutputPath,std::ios::binary|std::ios::out);
		if (!m_FileHandle.is_open())
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Failed to open file \"" + FileOutputPath + "\"";
		}
		return(ReturnValue);
	}
	MBError MBPP_FileListDownloader::InsertData(const void* Data, size_t DataSize)
	{
		MBError ReturnValue = true;
		m_FileHandle.write((char*)Data, DataSize);
		return(ReturnValue);
	}
	MBError MBPP_FileListDownloader::Close()
	{
		MBError ReturnValue = true;
		m_FileHandle.flush();
		m_FileHandle.close();
		return(ReturnValue);
	}
	//END MBPP_FileListDownloader

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

	}
	std::string MBPP_ClientHTTPConverter::p_GenerateHTTPHeader(MBPP_GenericRecord const& MBPPHeader)
	{
		std::string ReturnValue = "POST " + m_MBPP_ResourceLocation + " HTTP/1.1\r\n";
		ReturnValue += "Content-Type: application/x-MBPP-record\r\n";
		ReturnValue += "Content-Length: " + std::to_string(MBPPHeader.RecordSize+MBPPHeader.VerificationDataSize+MBPP_GenericRecordHeaderSize) + "\r\n\r\n";
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
		p_ResetSendState();
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
				//m_TotalSentRecordData = m_MBPP_CurrentHeaderData.size() - MBPP_GenericRecordHeaderSize;
				m_MBPP_CurrentHeaderData = "";
			}
		}
		else
		{
			m_InternalHTTPSocket->SendData(DataToSend,DataSize);
			m_TotalSentRecordData += DataSize;
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
	MBPP_UploadRequest_Response MBPP_Client::p_GetLoginResult(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, MBError* OutError)
	{
		MBPP_UploadRequest_Response ReturnValue;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::UploadRequest;
		RecordToSend.ComputerInfo = GetComputerInfo();

		RecordToSend.VerificationData = uint8_t(CredentialsType);
		RecordToSend.VerificationData += MBPP_EncodeString(PacketName);
		size_t CredentialsLengthOffset = RecordToSend.VerificationData.size();
		RecordToSend.VerificationData += std::string(4, 0);
		h_WriteBigEndianInteger(((uint8_t*)RecordToSend.VerificationData.data()) + CredentialsLengthOffset, CredentialsData.size(), 4);
		RecordToSend.VerificationData += CredentialsData;
		RecordToSend.VerificationDataSize = RecordToSend.VerificationData.size();
		if (m_ServerConnection->IsValid() == false && m_ServerConnection->IsConnected() == false)
		{
			*OutError = false;
			OutError->ErrorMessage = "Error retrieving UploadRequestResponse, not connected";
			return(ReturnValue);
		}
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		std::string ResponseData = m_ServerConnection->RecieveData();
		size_t ResponseDataOffset = 0;
		while (m_ServerConnection->IsValid() && m_ServerConnection->IsConnected() && ResponseData.size() < MBPP_GenericRecordHeaderSize)
		{
			ResponseData += m_ServerConnection->RecieveData();
		}
		if (ResponseData.size() < MBPP_GenericRecordHeaderSize)
		{
			*OutError = false;
			OutError->ErrorMessage = "Error retrieving UploadRequestResponse, not enough data sent from server";
			return(ReturnValue);
		}
		MBPP_GenericRecord RecordHeader = MBPP_ParseRecordHeader(ResponseData.data(), ResponseData.size(), 0, &ResponseDataOffset);
		//reciever all data vi kan, vet att det här kommer få plats i ram
		while (m_ServerConnection->IsValid() && m_ServerConnection->IsConnected() && ResponseData.size() < MBPP_GenericRecordHeaderSize)
		{
			ResponseData += m_ServerConnection->RecieveData();
		}
		if (ResponseData.size() < MBPP_GenericRecordHeaderSize+RecordHeader.RecordSize)
		{
			*OutError = false;
			OutError->ErrorMessage = "Error retrieving UploadRequestResponse, not enough data sent from server";
			return(ReturnValue);
		}
		ReturnValue.Result = MBPP_ErrorCode(MBParsing::ParseBigEndianInteger(ResponseData.data(), sizeof(MBPP_ErrorCode), ResponseDataOffset, &ResponseDataOffset));
		size_t StringSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), MBPP_StringLengthSize, ResponseDataOffset, &ResponseDataOffset);
		ReturnValue.DomainToLogin = std::string(((char*)ResponseData.data())+ResponseDataOffset, StringSize);
		ResponseDataOffset += StringSize;
		uint32_t SupportedLoginMethodsLength = MBParsing::ParseBigEndianInteger(ResponseData.data(), sizeof(MBPP_UserCredentialsType), ResponseDataOffset, &ResponseDataOffset);
		for (size_t i = 0; i < SupportedLoginMethodsLength/sizeof(MBPP_UserCredentialsType); i++)
		{
			ReturnValue.SupportedLoginTypes.push_back(MBPP_UserCredentialsType(MBParsing::ParseBigEndianInteger(ResponseData.data(), sizeof(MBPP_UserCredentialsType), ResponseDataOffset, &ResponseDataOffset)));
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
		MBError UpdateError = true;
		MBPP_FileInfoReader ServerFiles = GetPacketFileInfo(PacketName, &UpdateError);
		if(!UpdateError)
		{
			return(UpdateError);
		}
		//ANTAGANDE vi antar här att PacketFilesData är uppdaterad
		if (!std::filesystem::exists(OutputDirectory + "/MBPM_FileInfo"))
		{
			MBPM::CreatePacketFilesData(OutputDirectory);
		}
		MBPP_FileInfoDiff FileInfoDifferance = GetFileInfoDifference(MBPP_FileInfoReader(OutputDirectory + "/MBPM_FileInfo"), ServerFiles);
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
		//deletar sedan filerna som är över
		for (auto const& FileToRemove : FileInfoDifferance.RemovedFiles)
		{
			if (MBPP_PathIsValid(FileToRemove))
			{
				std::string AbsolutePath = OutputDirectory + FileToRemove;
				if (std::filesystem::exists(AbsolutePath))
				{
					std::filesystem::remove_all(AbsolutePath);
				}
			}
		}
		for (auto const& DirectoryToRemove : FileInfoDifferance.DeletedDirectories)
		{
			if (MBPP_PathIsValid(DirectoryToRemove))
			{
				std::string AbsolutePath = OutputDirectory + DirectoryToRemove;
				if (std::filesystem::exists(AbsolutePath))
				{
					try
					{
						std::filesystem::remove_all(AbsolutePath);
					}
					catch (const std::exception& e)
					{
						std::cout << e.what() << std::endl;
					}
				}
			}
		}
		MBPM::CreatePacketFilesData(OutputDirectory);
		return(UpdateError);
	}
	MBError MBPP_Client::UploadPacket(std::string const& PacketDirectory, std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, 
		MBPP_UploadRequest_Response* OutResponse)
	{
		//kollar om att ladda ner packet infon är tillräckligt litet och skapar en diff fil, eller gör det genom att manuellt be om directory info för varje fil och se om hashen skiljer
		MBError UploadError = true;
		MBPP_UploadRequest_Response UploadRequestResponse = p_GetLoginResult(PacketName, CredentialsType, CredentialsData,&UploadError);
		*OutResponse = UploadRequestResponse;
		if (UploadRequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
		{
			//update out struct
			return(UploadError);
		}
		//TODO finns det något smidigt sätt att undvika mycket redundant server nedladdande?
		MBPP_FileInfoReader  ServerFileInfo = GetPacketFileInfo(PacketName, &UploadError);
		

		if (!UploadError)
		{
			return(UploadError);
		}
		//ANTAGANDE vi antar här att PacketFilesData är uppdaterad
		//byter plats på client och server så att client tolkas som den uppdaterade q
		MBPP_GenericRecord RecordToSend;
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.Type = MBPP_RecordType::UploadChanges;
		
		MBPP_FileInfoReader LocalFileInfo = MBPP_FileInfoReader(PacketDirectory + "/MBPM_FileInfo");
		MBPP_FileInfoDiff FileInfoDifferance = GetFileInfoDifference(ServerFileInfo, LocalFileInfo);
		//DEBUG 
		std::cout << "Local files: " << std::endl;
		MBPP_DirectoryInfoNode_ConstIterator Iterator = LocalFileInfo.GetDirectoryInfo("/")->begin();
		while (!(Iterator == LocalFileInfo.GetDirectoryInfo("/")->end()))
		{
			std::cout << Iterator.GetCurrentDirectory() + (*Iterator).FileName << std::endl;
			Iterator++;
		}


		RecordToSend.VerificationData = MBPP_GetUploadCredentialsData(PacketName, CredentialsType, CredentialsData);
		RecordToSend.VerificationDataSize = RecordToSend.VerificationData.size();
		size_t TotalDataSize = MBPP_StringLengthSize+PacketName.size();

		std::vector<std::string> FilesToSend = {};
		std::vector<std::string> ObjectsToDelete = {};
		TotalDataSize += 4 + 4;//storleken av fileliststen
		for (auto const& UpdatedFile : FileInfoDifferance.UpdatedFiles)
		{
			TotalDataSize += MBPP_StringLengthSize + UpdatedFile.size();
			TotalDataSize += 8+ MBGetFileSize(PacketDirectory + UpdatedFile);
			FilesToSend.push_back(UpdatedFile);
		}
		for (auto const& NewFiles : FileInfoDifferance.AddedFiles)
		{
			TotalDataSize += MBPP_StringLengthSize + NewFiles.size();
			TotalDataSize += 8+MBGetFileSize(PacketDirectory + NewFiles);
			FilesToSend.push_back(NewFiles);
		}
		for (auto const& NewDirectories : FileInfoDifferance.AddedDirectories)
		{
			const MBPP_DirectoryInfoNode* DirectoryNode = LocalFileInfo.GetDirectoryInfo(NewDirectories);
			MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = DirectoryNode->begin();
			MBPP_DirectoryInfoNode_ConstIterator DirectoryIteratorEnd = DirectoryNode->end();
			while (!(DirectoryIterator == DirectoryIteratorEnd))
			{
				std::string PacketPath = NewDirectories + DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName;
				TotalDataSize += MBPP_StringLengthSize + PacketPath.size();
				TotalDataSize += 8+MBGetFileSize(PacketDirectory + PacketPath);
				FilesToSend.push_back(PacketPath);
				DirectoryIterator++;
			}

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
		RecordToSend.RecordData += MBPP_EncodeString(PacketName);
		RecordToSend.RecordData += MBPP_GenerateFileList(ObjectsToDelete);
		RecordToSend.RecordData += MBPP_GenerateFileList(FilesToSend);
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
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
			if (Result != MBPP_ErrorCode::Ok)
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
		if (!m_ServerConnection->IsConnected())
		{
			*OutError = false;
			OutError->ErrorMessage = "Not connected to client";
			return(ReturnValue);
		}
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
			MBPP_GenericRecord ServerRecord = MBPP_ParseRecordHeader(RecievedData.data(), RecievedData.size(), 0, nullptr);
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
	void MBPP_Server::SetUserAuthenticator(MBUtility::MBBasicUserAuthenticator* AuthenticatorToSet) 
	{
		m_UserAuthenticator = AuthenticatorToSet;
	}
	void MBPP_Server::SetTopDomain(std::string const& NewTopDomain)
	{
		m_ServerDomain = NewTopDomain;
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
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadRequest)
		{
			delete (MBPP_UploadRequest_ResponseData*)m_RequestResponseData;
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadChanges)
		{

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
	std::string MBPP_Server::p_GetPacketDomain(std::string const& PacketName)
	{
		return(m_ServerDomain);
	}
	MBPP_ErrorCode MBPP_Server::p_VerifyPlainLogin(std::string const& PacketName)
	{
		size_t ParseOffset = 0;
		if (ParseOffset + MBPP_StringLengthSize > m_RequestVerificationData.CredentialsData.size())
		{
			return(MBPP_ErrorCode::ParseError);
		}
		size_t UsernameSize = MBParsing::ParseBigEndianInteger(m_RequestVerificationData.CredentialsData.data(), MBPP_StringLengthSize, ParseOffset, &ParseOffset);
		std::string Username = m_RequestVerificationData.CredentialsData.substr(ParseOffset,UsernameSize);
		if (UsernameSize + ParseOffset > m_RequestVerificationData.CredentialsData.size())
		{
			return(MBPP_ErrorCode::ParseError);
		}
		ParseOffset += UsernameSize;
		if (ParseOffset + MBPP_StringLengthSize > m_RequestVerificationData.CredentialsData.size())
		{
			return(MBPP_ErrorCode::ParseError);
		}
		size_t PasswordSize = MBParsing::ParseBigEndianInteger(m_RequestVerificationData.CredentialsData.data(), MBPP_StringLengthSize, ParseOffset, &ParseOffset);
		if (PasswordSize + ParseOffset > m_RequestVerificationData.CredentialsData.size())
		{
			return(MBPP_ErrorCode::ParseError);
		}
		std::string Password = m_RequestVerificationData.CredentialsData.substr(ParseOffset, PasswordSize);
		ParseOffset += PasswordSize;
		bool VerificationResult = m_UserAuthenticator->VerifyUser(Username, Password);
		if (VerificationResult)
		{
			return(MBPP_ErrorCode::Ok);
		}
		else
		{
			return(MBPP_ErrorCode::InvalidCredentials);
		}
	}
	MBPP_ErrorCode MBPP_Server::p_VerifyRequest(std::string const& PacketName)
	{
		MBPP_ErrorCode ReturnValue = MBPP_ErrorCode::Null;
		if (m_RequestVerificationData.CredentialsType == MBPP_UserCredentialsType::Null)
		{
			ReturnValue = MBPP_ErrorCode::NoVerification;
		}
		else if(m_RequestVerificationData.CredentialsType == MBPP_UserCredentialsType::Plain)
		{
			ReturnValue = p_VerifyPlainLogin(PacketName);
		}
		else if (m_RequestVerificationData.CredentialsType == MBPP_UserCredentialsType::Request)
		{
			ReturnValue = MBPP_ErrorCode::LoginRequired;
		}
		else
		{
			ReturnValue = MBPP_ErrorCode::NoVerification;
		}
		return(ReturnValue);
	}
	MBPP_FileInfoReader MBPP_Server::GetPacketFileInfo(std::string const& PacketName)
	{
		return(MBPP_FileInfoReader(p_GetPacketDirectory(PacketName) + "/MBPM_FileInfo"));
	}
	std::string MBPP_Server::p_GetTopPacketsDirectory()
	{
		std::string ReturnValue = "";
		if (m_PacketSearchDirectories.size() > 0)
		{
			ReturnValue = m_PacketSearchDirectories[0];
		}
		return(ReturnValue);
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
	MBError MBPP_Server::p_ParseRequestVerificationData()
	{
		MBError ReturnValue = true;
		//vi vet att denna data får plats i minnet
		if (m_RecievedData.size() - m_RecievedDataOffset >= m_CurrentRequestHeader.VerificationDataSize)
		{
			m_RequestVerificationData.CredentialsType = MBPP_UserCredentialsType(MBParsing::ParseBigEndianInteger(m_RecievedData.data(),
				sizeof(MBPP_UserCredentialsType), m_RecievedDataOffset, &m_RecievedDataOffset));
			m_RequestVerificationData.PacketNameStringLength = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, m_RecievedDataOffset, &m_RecievedDataOffset);
			if (m_RecievedData.size() - m_RecievedDataOffset < m_RequestVerificationData.PacketNameStringLength)
			{
				ReturnValue = false;
				ReturnValue.ErrorMessage = "Error parsing UploadRequest record, invalid packet name string length";
				return(ReturnValue);
			}
			m_RequestVerificationData.PacketName = m_RecievedData.substr(m_RecievedDataOffset, m_RequestVerificationData.PacketNameStringLength);
			m_RecievedDataOffset += m_RequestVerificationData.PacketNameStringLength;
			m_RequestVerificationData.CredentialsDataLength = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), 4, m_RecievedDataOffset, &m_RecievedDataOffset);
			if (m_RecievedData.size() - m_RecievedDataOffset < m_RequestVerificationData.CredentialsDataLength)
			{
				ReturnValue = false;
				ReturnValue.ErrorMessage = "Error parsing UploadRequest record, invalid credntials length";
				return(ReturnValue);
			}
			m_RequestVerificationData.CredentialsData = m_RecievedData.substr(m_RecievedDataOffset, m_RequestVerificationData.CredentialsDataLength);
			m_RecievedDataOffset += m_RequestVerificationData.CredentialsDataLength;
			m_VerificationDataParsed = true;
			//vi uppdaterar resultet
			m_VerificationResult = p_VerifyRequest(m_RequestVerificationData.PacketName);
		}
		return(ReturnValue);
	}
	MBError MBPP_Server::p_ParseFileList(MBPP_FileList& FileListToUpdate,bool* BoolToUpdate)
	{
		MBError ReturnValue = true;
		if (FileListToUpdate.FileListSize == -1)
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= MBPP_FileListLengthSize)
			{
				FileListToUpdate.FileListSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_FileListLengthSize, m_RecievedDataOffset, &m_RecievedDataOffset);
			}
		}
		if (FileListToUpdate.FileListSize != -1)
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= FileListToUpdate.FileListSize)
			{
				size_t CurrentOffset = m_RecievedDataOffset;
				while (CurrentOffset-m_RecievedDataOffset < FileListToUpdate.FileListSize)
				{
					size_t CurrentStringSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, CurrentOffset, &CurrentOffset);
					if (CurrentOffset + CurrentStringSize - m_RecievedDataOffset > FileListToUpdate.FileListSize)
					{
						ReturnValue = false;
						ReturnValue.ErrorMessage = "Error parsing FileList: Invalid string size";
						return(ReturnValue);
					}
					std::string NewString = m_RecievedData.substr(CurrentOffset, CurrentStringSize);
					CurrentOffset += CurrentStringSize;
					FileListToUpdate.Files.push_back(std::move(NewString));
				}
				m_RecievedDataOffset = CurrentOffset;
				*BoolToUpdate = true;
			}
		}
		return(ReturnValue);
	}
	MBError MBPP_Server::p_DownloadClientFileList(MBPP_FileListDownloadState* DownloadState, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		while (m_RecievedDataOffset < m_RecievedData.size() && ReturnValue)
		{
			if (DownloadState->CurrentFileIndex >= DownloadState->FileNames.size())
			{
				m_RequestFinished = true;
				break;
			}
			if (DownloadState->NewFile)
			{
				if (!MBPP_PathIsValid(DownloadState->FileNames[DownloadState->CurrentFileIndex]))
				{
					ReturnValue = false;
					ReturnValue.ErrorMessage = "Error downloading file: Invalid filename";
					return(ReturnValue);
				}
				ReturnValue = DownloadHandler->Open(DownloadState->FileNames[DownloadState->CurrentFileIndex]);
				if (!ReturnValue)
				{
					break;
				}
				DownloadState->NewFile = false;
				DownloadState->CurrentFileSize = -1;
				DownloadState->CurrentFileParsedData = 0;
			}
			if (DownloadState->CurrentFileSize == -1)
			{
				if (m_RecievedData.size() - m_RecievedDataOffset >= 8)
				{
					DownloadState->CurrentFileSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), 8, m_RecievedDataOffset, &m_RecievedDataOffset);
				}
			}
			if (DownloadState->CurrentFileSize != -1)
			{
				uint64_t BytesToWrite = std::min(uint64_t(m_RecievedData.size()-m_RecievedDataOffset),uint64_t(DownloadState->CurrentFileSize-DownloadState->CurrentFileParsedData));
				ReturnValue = DownloadHandler->InsertData(m_RecievedData.substr(m_RecievedDataOffset, BytesToWrite));
				if (!ReturnValue)
				{
					break;
				}
				m_RecievedDataOffset += BytesToWrite;
				DownloadState->CurrentFileParsedData += BytesToWrite;
				if (DownloadState->CurrentFileSize == DownloadState->CurrentFileParsedData)
				{
					DownloadState->NewFile = true;
					DownloadState->CurrentFileIndex += 1;
					ReturnValue = DownloadHandler->Close();
				}
			}
		}
		if (DownloadState->CurrentFileIndex >= DownloadState->FileNames.size())
		{
			m_RequestFinished = true;
		}
		return(ReturnValue);
	}
	MBError MBPP_Server::p_Handle_UploadChanges()
	{
		MBError ReturnValue = true;
		if (m_RequestResponseData == nullptr)
		{
			m_RequestResponseData = new MBPP_UploadChanges_ResponseData();
		}
		//behöver packet namnet för att kunna verifiera
		MBPP_UploadChanges_ResponseData& CurrentResponseData = p_GetResponseData<MBPP_UploadChanges_ResponseData>();
		if (CurrentResponseData.PacketName == "")
		{
			if (m_RecievedData.size() - m_RecievedDataOffset >= MBPP_StringLengthSize)
			{
				CurrentResponseData.PacketNameSize = MBParsing::ParseBigEndianInteger(m_RecievedData.data(), MBPP_StringLengthSize, m_RecievedDataOffset, &m_RecievedDataOffset);
			}
			if (CurrentResponseData.PacketNameSize != -1)
			{
				if (m_RecievedData.size() - m_RecievedDataOffset >= CurrentResponseData.PacketNameSize)
				{
					CurrentResponseData.PacketName = m_RecievedData.substr(m_RecievedDataOffset, CurrentResponseData.PacketNameSize);
					m_RecievedDataOffset += CurrentResponseData.PacketNameSize;
				}
			}
		}
		//2 stycken filelists att parsa
		if (CurrentResponseData.PacketName != "" && (CurrentResponseData.ObjectsToDeleteParsed == false || CurrentResponseData.FilesToDownloadParsed ==false))
		{
			if (CurrentResponseData.ObjectsToDeleteParsed == false)
			{
				ReturnValue = p_ParseFileList(CurrentResponseData.ObjectsToDelete, &(CurrentResponseData.ObjectsToDeleteParsed));
			}
			if (!ReturnValue)
			{
				return(ReturnValue);
			}
			if (CurrentResponseData.FilesToDownloadParsed == false)
			{
				ReturnValue = p_ParseFileList(CurrentResponseData.FilesToDownload, &(CurrentResponseData.FilesToDownloadParsed));
			}
			if (CurrentResponseData.FilesToDownloadParsed && CurrentResponseData.ObjectsToDeleteParsed)
			{
				std::string PacketDirectory = p_GetPacketDirectory(CurrentResponseData.PacketName);
				if (!std::filesystem::exists(PacketDirectory+"/MBPM_UploadedChanges/"))
				{
					std::filesystem::create_directories(PacketDirectory + "/MBPM_UploadedChanges/");
				}
				CurrentResponseData.Downloader = std::unique_ptr<MBPP_FileListDownloader>(new MBPP_FileListDownloader(PacketDirectory + "/MBPM_UploadedChanges/"));
				CurrentResponseData.DownloadState.FileNames = CurrentResponseData.FilesToDownload.Files;
			}
		}
		if (CurrentResponseData.PacketName != "" && CurrentResponseData.FilesToDownloadParsed && CurrentResponseData.ObjectsToDeleteParsed)
		{
			ReturnValue = p_DownloadClientFileList(&CurrentResponseData.DownloadState, CurrentResponseData.Downloader.get());
			if (ReturnValue)
			{
				m_PacketUpdated = true;
				m_UpdatedPacket = CurrentResponseData.PacketName;
				m_UpdateRemovedObjects = CurrentResponseData.ObjectsToDelete.Files;
			}
			else
			{
				m_PacketUpdated = false;
				m_UpdatedPacket = "";
				m_UpdateRemovedObjects = {};
			}
		}
		return(ReturnValue);
	}
	void MBPP_Server::AddPacketSearchDirectory(std::string const& PacketSearchDirectoryToAdd)
	{
		m_PacketSearchDirectories.push_back(PacketSearchDirectoryToAdd);
	}
	//Upload/Remove metoder
	bool MBPP_Server::PacketUpdated()
	{
		return(m_PacketUpdated);
	}
	std::string MBPP_Server::GetUpdatedPacket()
	{
		return(m_UpdatedPacket);
	}
	std::vector<std::string> MBPP_Server::GetPacketRemovedFiles()
	{
		return(m_UpdateRemovedObjects);
	}
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
					m_RecievedDataOffset = MBPP_GenericRecordHeaderSize;
					//resettar tidigare packet data vi fått
					m_PacketUpdated = false;
					m_UpdatedPacket = "";
				}
			}
		}
		if (m_CurrentRequestHeader.Type != MBPP_RecordType::Null && m_CurrentRequestHeader.VerificationDataSize != 0 && m_VerificationDataParsed == false)
		{
			ReturnValue = p_ParseRequestVerificationData();
		}
		if (m_CurrentRequestHeader.Type != MBPP_RecordType::Null && (m_VerificationDataParsed || m_CurrentRequestHeader.VerificationDataSize == 0))
		{
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
			else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadRequest)
			{
				//vi vill bara skicka request datan servern hantarerar, så är finished redan här 
				m_RequestResponseData = new MBPP_UploadRequest_ResponseData();
				MBPP_UploadRequest_ResponseData& CurrentData = p_GetResponseData<MBPP_UploadRequest_ResponseData>();
				CurrentData.Response.Result = m_VerificationResult;
				CurrentData.Response.DomainToLogin = p_GetPacketDomain(m_RequestVerificationData.PacketName);
				CurrentData.Response.SupportedLoginTypes = m_SupportedCredentials;
				m_RequestFinished = true;
			}
			else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadChanges)
			{
				//om det misslyckas med autentiseringen så är det inte något vi svarar på, så vi bara avbryter
				if (m_VerificationResult == MBPP_ErrorCode::Ok)
				{
					p_Handle_UploadChanges();
				}
				else
				{
					ReturnValue = false;
					ReturnValue.ErrorMessage = "Client sent UploadChanges message with invalid credentials";
				}
			}
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
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadRequest)
		{
			ReturnValue = new MBPP_UploadRequest_ResponseIterator(p_GetResponseData<MBPP_UploadRequest_ResponseData>(), this);
		}
		else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadChanges)
		{
			ReturnValue = new MBPP_UploadChanges_ResponseIterator();
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
	std::string MBPP_ServerResponseIterator::p_GetTopPacketsDirectory()
	{
		return(m_AssociatedServer->p_GetTopPacketsDirectory());
	}
	//END MBPP_ServerResponseIterator
	
	//Specifika iterators
	uint64_t h_CalculateFileListResponseSize(std::vector<std::string> const& FilesToSend, std::string const& PacketDirectory)
	{
		uint64_t ReturnValue = 4; //storleken av pointer av FileList
		for (size_t i = 0; i < FilesToSend.size(); i++)
		{
			ReturnValue += MBPP_StringLengthSize+FilesToSend[i].size();//för filelisten
			ReturnValue += 8+MBGetFileSize(PacketDirectory + FilesToSend[i]);
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
			//std::filesystem::recursive_directory_iterator DirectoryIterator = std::filesystem::recursive_directory_iterator(PacketDirectory + ResponseData.DirectoriesToGet[i]);
			MBPP_FileInfoReader PacketFileInfo = AssociatedServer->GetPacketFileInfo(ResponseData.PacketName);
			const MBPP_DirectoryInfoNode* DirectoryToIterate = PacketFileInfo.GetDirectoryInfo(ResponseData.DirectoriesToGet[i]);
			if (DirectoryToIterate == nullptr)
			{
				continue;
			}
			MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = DirectoryToIterate->begin();
			while(!(DirectoryIterator == DirectoryToIterate->end()))
			{
				std::string CurrentFilePath = PacketDirectory+ ResponseData.DirectoriesToGet[i] +DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName;
				if (!std::filesystem::exists(CurrentFilePath))
				{
					DirectoryIterator++;
					continue;
				}
				m_HeaderToSend.RecordSize += 8+std::filesystem::file_size(CurrentFilePath);
				m_FilesToSend.push_back(ResponseData.DirectoriesToGet[i] + DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName);
				DirectoryIterator++;
			}
		}
		m_HeaderToSend.RecordSize += 4;
		for (size_t i = 0; i < m_FilesToSend.size(); i++)
		{
			m_HeaderToSend.RecordSize += MBPP_StringLengthSize + m_FilesToSend[i].size();
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
		//för att supporta att man upploadar till packets som inte än existerar, vilket vi kanske inte vill supporta men är ju bara för mina grejer, så skapar vi en tom directory när man
		//begär från ett packet som ej existerar
		if (p_GetPacketDirectory(ResponseData.PacketName) == "")
		{
			//packetet finns inte, vi skapar det, och checkar först att namnet är valid, annars bara skiter vi i
			if (MBPP_PathIsValid(ResponseData.PacketName))
			{
				std::string TopPacketDirectory = p_GetTopPacketsDirectory();
				std::filesystem::create_directories(TopPacketDirectory + ResponseData.PacketName);
				std::string PacketDirectory = TopPacketDirectory + ResponseData.PacketName + "/";
				std::ofstream NewPacketInfo = std::ofstream(PacketDirectory + "MBPM_PacketInfo", std::ios::out | std::ios::binary);
				NewPacketInfo <<
#include "MBPM_EmptyPacketInfo.txt"
					;
				CreatePacketFilesData(PacketDirectory);
			}
			else
			{
				m_FilesToSend = {};
			}
		}
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

	//BEGIN MBPP_GetPacketInfo_UploadRequestResponseIterator
	MBPP_UploadRequest_ResponseIterator::MBPP_UploadRequest_ResponseIterator(MBPP_UploadRequest_ResponseData const& ResponseData, MBPP_Server* AssociatedServer)
	{
		m_RequestResponse = ResponseData.Response;
		m_HeaderToSend.Type = MBPP_RecordType::UploadRequestResponse;
		m_HeaderToSend.ComputerInfo = GetComputerInfo();
		m_HeaderToSend.RecordData += std::string(sizeof(MBPP_ErrorCode), 0);
		h_WriteBigEndianInteger(m_HeaderToSend.RecordData.data(), uint32_t(m_RequestResponse.Result), sizeof(MBPP_ErrorCode));
		m_HeaderToSend.RecordData += MBPP_EncodeString(m_RequestResponse.DomainToLogin);
		size_t ParseOffset = m_HeaderToSend.RecordData.size();
		m_HeaderToSend.RecordData += std::string(sizeof(MBPP_UserCredentialsType), 0);
		h_WriteBigEndianInteger(((char*)m_HeaderToSend.RecordData.data()) + ParseOffset, m_RequestResponse.SupportedLoginTypes.size() * sizeof(MBPP_UserCredentialsType), sizeof(MBPP_UserCredentialsType));
		ParseOffset += sizeof(MBPP_UserCredentialsType);
		m_HeaderToSend.RecordData += std::string(m_RequestResponse.SupportedLoginTypes.size() * sizeof(MBPP_UserCredentialsType), 0);
		for (size_t i = 0; i < m_RequestResponse.SupportedLoginTypes.size();i++)
		{
			h_WriteBigEndianInteger(((char*)m_HeaderToSend.RecordData.data()) + ParseOffset, uint64_t(m_RequestResponse.SupportedLoginTypes[i]), sizeof(MBPP_UserCredentialsType));
			ParseOffset += sizeof(MBPP_UserCredentialsType);
		}
		m_HeaderToSend.RecordSize = m_HeaderToSend.RecordData.size();
		m_CurrentResponse = MBPP_GetRecordData(m_HeaderToSend);
		m_HeaderToSend.RecordSize = m_HeaderToSend.RecordData.size();
		m_ResponseSent = true;
	}
	void MBPP_UploadRequest_ResponseIterator::Increment()
	{
		m_Finished = true;
	}
	//END MBPP_GetPacketInfo_UploadRequestResponseIterator

	//END MBPP_UploadChanges_ResponseIterator
	MBPP_UploadChanges_ResponseIterator::MBPP_UploadChanges_ResponseIterator()
	{
		MBPP_GenericRecord RecordToSend;
		RecordToSend.ComputerInfo = GetComputerInfo();
		RecordToSend.Type = MBPP_RecordType::UploadChangesResponse;
		RecordToSend.RecordData = std::string(sizeof(MBPP_ErrorCode), 0);
		h_WriteBigEndianInteger(RecordToSend.RecordData.data(), uint64_t(MBPP_ErrorCode::Ok), sizeof(MBPP_ErrorCode));
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_CurrentResponse = MBPP_GetRecordData(RecordToSend);
	}
	void MBPP_UploadChanges_ResponseIterator::Increment()
	{
		m_Finished = true;
	}
	//END MBPP_UploadChanges_ResponseIterator
}