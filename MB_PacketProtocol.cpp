#include "MB_PacketProtocol.h"
#include <MBUnicode/MBUnicode.h>
#include <MBParsing/MBParsing.h>
#include <set>
#include <MBCrypto/MBCrypto.h>
#include <MBUtility/MBAlgorithms.h>
#include <MBUtility/MBCompileDefinitions.h>
#include <algorithm>

#include <sstream>

#include <regex>
#include <string>
#include <iterator>
#include <system_error>
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
		size_t ReadBytes = FileToReadFrom->Read((char*)ByteBuffer, IntegerSize);
		if (ReadBytes < IntegerSize)
		{
			throw std::runtime_error("Unsufficient bytes when parsing integer");
		}
		for (size_t i = 0; i < IntegerSize; i++)
		{
			ReturnValue <<= 8;
			ReturnValue += ByteBuffer[i];
		}
		return(ReturnValue);
	}
	std::string h_WriteFileData(MBUtility::MBSearchableOutputStream* OutputStream, std::filesystem::path const& FilePath,MBCrypto::HashFunction HashFunctionToUse,uint64_t* OutFileSize)
	{
		//TODO fixa s� det itne blir fel p� windows
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
			std::string AbsolutePacketPath = "/"+MBUnicode::PathToUTF8(std::filesystem::relative(Entries.path(),TopPacketDirectory));
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
		h_WriteBigEndianInteger(FileToWriteTo, 0, 8); //offset innan vi en l�st filen
		//plats f�r directory hashet
		char StructureHash[20]; //TODO hardcoda inte hash längden...
		FileToWriteTo->Write(StructureHash, 20);
		char ContentHash[20]; //TODO hardcoda inte hash längden...
		FileToWriteTo->Write(ContentHash, 20);

		//h�r har vi �ven directory sizen
		uint64_t TotalDirectorySize = 0;
		h_WriteBigEndianInteger(FileToWriteTo, 0, 8);

		//ANTAGANDE pathen �r inte p� formen /n�got/hej/ utan /n�got/hej
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
			std::string FileName = MBUnicode::PathToUTF8(Files.filename());
			StructureHashData += FileName+FileHash; //sorterat p� filordning
			ContentHashes.push_back(FileHash);
			TotalDirectorySize += NewSize;
		}
		h_WriteBigEndianInteger(FileToWriteTo, DirectoriesToWrite.size(), 4);
		for (auto const& Directories : DirectoriesToWrite)
		{
			uint64_t NewSize = 0;
			MBPP_DirectoryHashData CurrentDirectoryHashData = h_WriteDirectoryData_Recursive(FileToWriteTo, TopPacketDirectory, std::filesystem::path(Directories).filename().generic_u8string(), Directories, FileInfoExcluder, HashFunctionToUse, &NewSize);
			
			std::string DirectoryName = "";
			size_t LastSlashPosition = Directories.find_last_of('/');
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

		//skriver �ven storleken h�r
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
	MBPP_FileInfoHeader MBPP_FileInfoReader::p_ReadFileInfoHeader(MBUtility::MBOctetInputStream* StreamToReadFrom)
	{
		MBPP_FileInfoHeader ReturnValue;
		char InitialData[MBPP_FileInfoHeaderStaticSize+1];
		InitialData[MBPP_FileInfoHeaderStaticSize] = 0;
		size_t ReadBytes = StreamToReadFrom->Read(InitialData, MBPP_FileInfoHeaderStaticSize);
		size_t ParseOffset = 0;
		if (ReturnValue.ArbitrarySniffData != InitialData)
		{
			throw std::runtime_error("Invalid FileInfo header");
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
	void UpdateFileInfo(std::string const& PacketToIndex, std::string const& FileName)
	{
		//std::cout << "OBS: Update not implemented yet" << std::endl;
		//CreatePacketFilesData(PacketToIndexm, FileName);
        MBPP_FileInfoReader::UpdateFileInfo(PacketToIndex,FileName);
	}
	void UpdateFileInfo(std::string const& PacketToIndex, MBUtility::MBSearchableOutputStream* OutputStream)
	{
		//std::cout << "OBS: Update not implemented yet" << std::endl;
		//CreatePacketFilesData(PacketToIndexm, OutputStream);
        //MBPP_FileInfoReader::UpdateFileInfo(PacketToIndex,OutputStream);
        std::error_code FSError;
        if(!std::filesystem::exists(PacketToIndex+"/MBPM_FileInfo",FSError))
        {
            MBPP_FileInfoReader::CreateFileInfo(PacketToIndex,"MBPM_FileInfo");
            return;
        }
        MBPP_FileInfoReader LocalFileInfo(PacketToIndex+"/MBPM_FileInfo");
        if(LocalFileInfo.ObjectExists("/") == false)
        {
            MBPP_FileInfoReader::CreateFileInfo(PacketToIndex,"MBPM_FileInfo");
            return;
        }
        MBPP_FileInfoReader::UpdateFileInfo(PacketToIndex,LocalFileInfo);
        LocalFileInfo.WriteData(OutputStream);
	}

	void MBPP_FileInfoReader::CreateFileInfo(std::string const& PacketToHashDirectory, MBPM_FileInfoExcluder const& Excluder, MBPP_FileInfoReader* OutReader)
	{
		if (!std::filesystem::directory_entry(PacketToHashDirectory).is_directory())
		{
			return;
		}
		MBPP_FileInfoReader	 ReturnValue;
		ReturnValue.m_ExtensionData = p_GetLatestVersionExtensionData();
		ReturnValue.m_InfoHeader.ExtensionDataSize = p_SerializeExtensionData(ReturnValue.m_ExtensionData).size();

		ReturnValue.m_TopNode = p_CreateDirectoryInfo(PacketToHashDirectory, PacketToHashDirectory,ReturnValue.m_InfoHeader, ReturnValue.m_ExtensionData, Excluder, nullptr);
		ReturnValue.p_UpdateDirectoriesParents(&ReturnValue.m_TopNode);
		if (OutReader)
		{
			*OutReader = std::move(ReturnValue);
		}
	}
	void MBPP_FileInfoReader::CreateFileInfo(MBUtility::Filesystem& TopDirectoryToIndex, MBPP_FileInfoReader* OutReader)
	{
		MBPP_FileInfoReader Result;

		MBPM_FileInfoExcluder Excluder;
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		std::unique_ptr<MBUtility::MBOctetInputStream> InStream = TopDirectoryToIndex.GetFileInputStream("MBPM_FileInfoIgnore",&FSError);
		if (!!FSError)
		{
			Excluder = MBPM_FileInfoExcluder(InStream.get());
		}

		Result.m_ExtensionData = p_GetLatestVersionExtensionData();
		Result.m_InfoHeader.ExtensionDataSize = p_SerializeExtensionData(Result.m_ExtensionData).size();
		Result.m_TopNode = p_CreateDirectoryInfo("/",".", TopDirectoryToIndex, Result.m_InfoHeader, Result.m_ExtensionData, Excluder, nullptr);
		Result.p_UpdateDirectoriesParents(&Result.m_TopNode);
		*OutReader = std::move(Result);
	}
	void MBPP_FileInfoReader::CreateFileInfo(MBUtility::Filesystem& TopDirectoryToIndex, std::string const& PathToIndex, MBPP_FileInfoReader* OutReader)
	{
		MBPP_FileInfoReader Result;
		MBPM_FileInfoExcluder Excluder;
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		std::unique_ptr<MBUtility::MBOctetInputStream> InStream = TopDirectoryToIndex.GetFileInputStream(PathToIndex+"/MBPM_FileInfoIgnore", &FSError);
		if (!!FSError)
		{
			Excluder = MBPM_FileInfoExcluder(InStream.get());
		}
		Result.m_ExtensionData = p_GetLatestVersionExtensionData();
		Result.m_InfoHeader.ExtensionDataSize = p_SerializeExtensionData(Result.m_ExtensionData).size();
		if (PathToIndex != "")
		{
			Result.m_TopNode = p_CreateDirectoryInfo("/",PathToIndex, TopDirectoryToIndex, Result.m_InfoHeader, Result.m_ExtensionData, Excluder, nullptr);
		}
		else
		{
			Result.m_TopNode = p_CreateDirectoryInfo("/", ".", TopDirectoryToIndex, Result.m_InfoHeader, Result.m_ExtensionData, Excluder, nullptr);
		}
		Result.p_UpdateDirectoriesParents(&Result.m_TopNode);
		*OutReader = std::move(Result);
	}
	void MBPP_FileInfoReader::CreateFileInfo(std::string const& PacketToHashDirectory, MBPP_FileInfoReader* OutReader)
	{
		MBPM_FileInfoExcluder DirectoryExcluder = p_GetDirectoryFileInfoExcluder(PacketToHashDirectory);
		CreateFileInfo(PacketToHashDirectory, DirectoryExcluder, OutReader);
	}
	void MBPP_FileInfoReader::CreateFileInfo(std::string const& PacketToHashDirectory, std::string const& FileName)
	{
		MBPM_FileInfoExcluder DirectoryExcluder = p_GetDirectoryFileInfoExcluder(PacketToHashDirectory);
		DirectoryExcluder.AddExcludeFile("/" + FileName);
		MBPP_FileInfoReader NewInfo;
		CreateFileInfo(PacketToHashDirectory, DirectoryExcluder, &NewInfo);
		std::ofstream FileStream(PacketToHashDirectory+"/"+FileName, std::ios::out | std::ios::binary);
		MBUtility::MBFileOutputStream OutStream(&FileStream);
		NewInfo.WriteData(&OutStream);
	}
	void MBPP_FileInfoReader::CreateFileInfo(std::string const& PacketToHashDirectory, MBUtility::MBSearchableOutputStream* OutputStream)
	{
		MBPM_FileInfoExcluder DirectoryExcluder = p_GetDirectoryFileInfoExcluder(PacketToHashDirectory);
		MBPP_FileInfoReader NewInfo;
		CreateFileInfo(PacketToHashDirectory, DirectoryExcluder, &NewInfo);
		NewInfo.WriteData(OutputStream);
	}
	void MBPP_FileInfoReader::UpdateFileInfo(std::string const& FileInfoTopDirectory, MBPM_FileInfoExcluder const& Excluder, MBPP_FileInfoReader& ReaderToUpdate)
	{
		//Inneficient and ugly as it iterats through the directory twice, but here until a better way to unravel the functionality is concieved
		MBPP_DirectoryInfoNode DiffNode = p_CreateDirectoryInfo(FileInfoTopDirectory, FileInfoTopDirectory, ReaderToUpdate.m_InfoHeader, ReaderToUpdate.m_ExtensionData, Excluder, &ReaderToUpdate);
		MBPP_FileInfoDiff FileInfoDiff = GetLocalInfoDiff(FileInfoTopDirectory, Excluder, ReaderToUpdate);
		ReaderToUpdate.DeleteObjects(FileInfoDiff.DeletedDirectories);
		ReaderToUpdate.DeleteObjects(FileInfoDiff.RemovedFiles);
		ReaderToUpdate.UpdateInfo(DiffNode);
        ReaderToUpdate.m_ExtensionData = p_GetLatestVersionExtensionData();
	}
	void MBPP_FileInfoReader::UpdateFileInfo(MBUtility::Filesystem& Filesystem, std::string DirectoryToUpdate, MBPP_FileInfoReader& ReaderToUpdate)
	{
		MBPM_FileInfoExcluder Excluder;
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		if (DirectoryToUpdate == "")
		{
			DirectoryToUpdate = "./";
		}
		std::unique_ptr<MBUtility::MBOctetInputStream> InStream = Filesystem.GetFileInputStream(DirectoryToUpdate + "/MBPM_FileInfoIgnore", &FSError);
		if (!!FSError)
		{
			Excluder = MBPM_FileInfoExcluder(InStream.get());
		}
		MBPP_DirectoryInfoNode DiffNode = p_CreateDirectoryInfo("/", DirectoryToUpdate, Filesystem, ReaderToUpdate.m_InfoHeader, ReaderToUpdate.m_ExtensionData, Excluder, &ReaderToUpdate);
		//p_CreateDirectoryInfo("/",DirectoryToUpdate,Filesystem, DirectoryToUpdate, ReaderToUpdate.m_InfoHeader, ReaderToUpdate.m_ExtensionData, Excluder, &ReaderToUpdate);
		MBPP_FileInfoDiff FileInfoDiff = GetLocalInfoDiff(Filesystem, DirectoryToUpdate,ReaderToUpdate);
		ReaderToUpdate.DeleteObjects(FileInfoDiff.DeletedDirectories);
		ReaderToUpdate.DeleteObjects(FileInfoDiff.RemovedFiles);
		ReaderToUpdate.UpdateInfo(DiffNode);
	}
	void MBPP_FileInfoReader::UpdateFileInfo(std::string const& FileInfoDirectory,MBPP_FileInfoReader& ReaderToUpdate)
	{
		MBPM_FileInfoExcluder Excluder = p_GetDirectoryFileInfoExcluder(FileInfoDirectory);
		UpdateFileInfo(FileInfoDirectory, Excluder, ReaderToUpdate);
		//MBPP_DirectoryInfoNode DiffNode = p_CreateDirectoryInfo(FileInfoDirectory, FileInfoDirectory, ReaderToUpdate.m_InfoHeader, ReaderToUpdate.m_ExtensionData, Excluder, &ReaderToUpdate);
		//ReaderToUpdate.UpdateInfo(DiffNode);
	}

	void MBPP_FileInfoReader::UpdateFileInfo(std::string const& DirectoryToIndex, std::string const& FileName)
	{
		if (!std::filesystem::exists(DirectoryToIndex + "/" + FileName))
		{
			CreateFileInfo(DirectoryToIndex, FileName);
		}
		else
		{
			MBPP_FileInfoReader StoredFileInfo = MBPP_FileInfoReader(DirectoryToIndex + "/" + FileName);
			if (StoredFileInfo.ObjectExists("/"))
			{
				UpdateFileInfo(DirectoryToIndex, StoredFileInfo);
				std::ofstream OutFile(DirectoryToIndex + "/" + FileName,std::ios::out|std::ios::binary);
				if (OutFile.is_open())
				{
					MBUtility::MBFileOutputStream OutStream(&OutFile);
					StoredFileInfo.WriteData(&OutStream);
				}
			}
			else
			{
				CreateFileInfo(DirectoryToIndex, FileName);
			}
		}
	}
	MBPP_FileInfoReader MBPP_FileInfoReader::GetLocalUpdatedFiles(std::string const& DirectoryToIndex,std::string const& IndexName)
	{
		MBPP_FileInfoReader ReturnValue;
		if (std::filesystem::exists(DirectoryToIndex + "/" + IndexName) && std::filesystem::directory_entry(DirectoryToIndex + "/" + IndexName).is_regular_file())
		{
			MBPP_FileInfoReader StoredInfo(DirectoryToIndex + "/" + IndexName);
			if (StoredInfo.ObjectExists("/"))
			{
				MBPM_FileInfoExcluder Excluder = p_GetDirectoryFileInfoExcluder(DirectoryToIndex);
				ReturnValue.m_InfoHeader = MBPP_FileInfoHeader();
				ReturnValue.m_ExtensionData = p_GetLatestVersionExtensionData();
				ReturnValue.m_InfoHeader.ExtensionDataSize = p_SerializeExtensionData(ReturnValue.m_ExtensionData).size();
				ReturnValue.m_TopNode = p_CreateDirectoryInfo(DirectoryToIndex, DirectoryToIndex, ReturnValue.m_InfoHeader, ReturnValue.m_ExtensionData, Excluder, &StoredInfo);
				ReturnValue.p_UpdateDirectoriesParents(&ReturnValue.m_TopNode);
			}
			else
			{
				CreateFileInfo(DirectoryToIndex, &ReturnValue);
			}
		}
		else
		{
			CreateFileInfo(DirectoryToIndex, &ReturnValue);
		}
		return(ReturnValue);
	}
	MBPP_FileInfoDiff MBPP_FileInfoReader::GetLocalInfoDiff(std::string const& DirectoryToIndex, MBPM_FileInfoExcluder const& Excluder, MBPP_FileInfoReader const& InfoToCompare)
	{
		MBPP_FileInfoDiff ReturnValue;
		if (InfoToCompare.GetDirectoryInfo("/") == nullptr)
		{
			return ReturnValue;
		}
		p_GetLocalInfoDiff_UpdateDirectory(ReturnValue, Excluder, *InfoToCompare.GetDirectoryInfo("/"), DirectoryToIndex, "/");
		assert(std::is_sorted(ReturnValue.AddedDirectories.begin(), ReturnValue.AddedDirectories.end()));
		assert(std::is_sorted(ReturnValue.DeletedDirectories.begin(), ReturnValue.DeletedDirectories.end()));
		assert(std::is_sorted(ReturnValue.AddedFiles.begin(), ReturnValue.AddedFiles.end()));
		assert(std::is_sorted(ReturnValue.UpdatedFiles.begin(), ReturnValue.UpdatedFiles.end()));
		assert(std::is_sorted(ReturnValue.RemovedFiles.begin(), ReturnValue.RemovedFiles.end()));
		return(ReturnValue);
	}
	MBPP_FileInfoDiff MBPP_FileInfoReader::GetLocalInfoDiff(MBUtility::Filesystem& Filesystem, std::string const& PathToIndex, MBPP_FileInfoReader const& InfoToCompare)
	{
		MBPP_FileInfoDiff ReturnValue;
		if (InfoToCompare.GetDirectoryInfo("/") == nullptr)
		{
			return ReturnValue;
		}
		MBPM_FileInfoExcluder Excluder;
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		std::unique_ptr<MBUtility::MBOctetInputStream> InStream = Filesystem.GetFileInputStream(PathToIndex + (PathToIndex == "" ?  "MBPM_FileInfoIgnore" : "/MBPM_FileInfoIgnore"), &FSError);
		if (!!FSError)
		{
			Excluder = MBPM_FileInfoExcluder(InStream.get());
		}
		p_GetLocalInfoDiff_UpdateDirectory(ReturnValue, Filesystem, Excluder, *InfoToCompare.GetDirectoryInfo("/"), PathToIndex, "/");
		assert(std::is_sorted(ReturnValue.AddedDirectories.begin(), ReturnValue.AddedDirectories.end()));
		assert(std::is_sorted(ReturnValue.DeletedDirectories.begin(), ReturnValue.DeletedDirectories.end()));
		assert(std::is_sorted(ReturnValue.AddedFiles.begin(), ReturnValue.AddedFiles.end()));
		assert(std::is_sorted(ReturnValue.UpdatedFiles.begin(), ReturnValue.UpdatedFiles.end()));
		assert(std::is_sorted(ReturnValue.RemovedFiles.begin(), ReturnValue.RemovedFiles.end()));
		return(ReturnValue);
	}

	MBPP_FileInfoDiff MBPP_FileInfoReader::GetLocalInfoDiff(std::string const& DirectoryToIndex, std::string const& IndexName)
	{
		MBPP_FileInfoDiff ReturnValue;
		if (std::filesystem::exists(DirectoryToIndex + "/" + IndexName) && std::filesystem::directory_entry(DirectoryToIndex + "/" + IndexName).is_regular_file())
		{
			MBPP_FileInfoReader StoredInfo(DirectoryToIndex + "/" + IndexName);
			if (StoredInfo.ObjectExists("/"))
			{
				MBPM_FileInfoExcluder Excluder = p_GetDirectoryFileInfoExcluder(DirectoryToIndex);
				ReturnValue = GetLocalInfoDiff(DirectoryToIndex, Excluder, StoredInfo);
			}
		}
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::p_GetLocalInfoDiff_UpdateDirectory(MBPP_FileInfoDiff& OutResult,MBPM_FileInfoExcluder const& Excluder,MBPP_DirectoryInfoNode const& 
		StoredNode, std::filesystem::path const& LocalDirectory,std::string const& CurrentIndexPath)
	{
		std::set<std::string> FilePaths;
		std::set<std::string> DirectoryPaths;
		std::filesystem::directory_iterator DirectoryIterator(LocalDirectory);
		for (auto const& Entry : DirectoryIterator)
		{
			std::string FileName = MBUnicode::PathToUTF8(Entry.path().filename());
			if (!(Entry.is_regular_file() || Entry.is_directory()))
			{
				continue;
			}
			std::string PacketName = CurrentIndexPath + FileName;
			if (Excluder.Excludes(PacketName) && !Excluder.Includes(PacketName))
			{
				continue;
			}
			if (Entry.is_regular_file())
			{
				FilePaths.insert(FileName);
			}
			else if (Entry.is_directory())
			{
				DirectoryPaths.insert(FileName);
			}
		}

		auto LocalFilesIterator= FilePaths.begin();
		auto LocalFilesEnd= FilePaths.end();
		auto StoredFilesIterator = StoredNode.Files.begin();
		auto StoredFilesEnd = StoredNode.Files.end();
		while (StoredFilesIterator != StoredFilesEnd && LocalFilesIterator != LocalFilesEnd)
		{
			if (*LocalFilesIterator < StoredFilesIterator->FileName)
			{
				//More files
				OutResult.AddedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
				LocalFilesIterator++;
			}
			else if (*LocalFilesIterator > StoredFilesIterator->FileName)
			{
				OutResult.RemovedFiles.push_back(CurrentIndexPath + StoredFilesIterator->FileName);
				StoredFilesIterator++;
			}
			else
			{
				//Equal
				if (std::filesystem::exists(LocalDirectory/ *LocalFilesIterator))
				{
					uint64_t LocalLastWriteTime = p_GetPathWriteTime(LocalDirectory/ *LocalFilesIterator);
					uint64_t StoredLastWriteTime = StoredFilesIterator->LastWriteTime;
					if (!(StoredLastWriteTime == 0 || LocalLastWriteTime == 0))
					{
						if (LocalLastWriteTime > StoredLastWriteTime)
						{
							OutResult.UpdatedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
						}
					}
					else
					{
						std::string LocalHash = MBCrypto::GetFileHash(MBUnicode::PathToUTF8(LocalDirectory) + "/" + *LocalFilesIterator, MBCrypto::HashFunction::SHA1);
						if (LocalHash != StoredFilesIterator->FileHash)
						{
							OutResult.UpdatedFiles.push_back(CurrentIndexPath+*LocalFilesIterator);
						}
					}
				}
				StoredFilesIterator++;
				LocalFilesIterator++;
			}
		}
		while (LocalFilesIterator != LocalFilesEnd)
		{
			OutResult.AddedFiles.push_back(CurrentIndexPath+*LocalFilesIterator);
			LocalFilesIterator++;
		}
		while (StoredFilesIterator != StoredFilesEnd)
		{
			OutResult.RemovedFiles.push_back(CurrentIndexPath + StoredFilesIterator->FileName);
			StoredFilesIterator++;
		}
		auto LocalDirectoriesIterator = DirectoryPaths.begin();
		auto LocalDirectoriesEnd = DirectoryPaths.end();
		auto StoredDirectoriesIterator = StoredNode.Directories.begin();
		auto StoredDirectoriesEnd = StoredNode.Directories.end();
		while (LocalDirectoriesIterator != LocalDirectoriesEnd && StoredDirectoriesIterator != StoredDirectoriesEnd)
		{
			if (*LocalDirectoriesIterator < StoredDirectoriesIterator->DirectoryName)
			{
				OutResult.AddedDirectories.push_back(CurrentIndexPath + *LocalDirectoriesIterator);
				LocalDirectoriesIterator++;
			}
			else if (*LocalDirectoriesIterator > StoredDirectoriesIterator->DirectoryName)
			{
				OutResult.DeletedDirectories.push_back(CurrentIndexPath + StoredDirectoriesIterator->DirectoryName);
				StoredDirectoriesIterator++;
			}
			else
			{
				p_GetLocalInfoDiff_UpdateDirectory(OutResult, Excluder, *StoredDirectoriesIterator, LocalDirectory/StoredDirectoriesIterator->DirectoryName, CurrentIndexPath + StoredDirectoriesIterator->DirectoryName + "/");
				LocalDirectoriesIterator++;
				StoredDirectoriesIterator++;
			}
		}
		while (LocalDirectoriesIterator != LocalDirectoriesEnd)
		{
			OutResult.AddedDirectories.push_back(CurrentIndexPath + *LocalDirectoriesIterator);
			LocalDirectoriesIterator++;
		}
		while (StoredDirectoriesIterator != StoredDirectoriesEnd)
		{
			OutResult.DeletedDirectories.push_back(CurrentIndexPath + StoredDirectoriesIterator->DirectoryName);
			StoredDirectoriesIterator++;
		}
		//uneccsarilly slow, but quick fix
		std::sort(OutResult.AddedDirectories.begin(), OutResult.AddedDirectories.end());
		std::sort(OutResult.AddedFiles.begin(), OutResult.AddedFiles.end());
		std::sort(OutResult.UpdatedFiles.begin(), OutResult.UpdatedFiles.end());
		std::sort(OutResult.DeletedDirectories.begin(), OutResult.DeletedDirectories.end());
		std::sort(OutResult.RemovedFiles.begin(), OutResult.RemovedFiles.end());
	}
	void MBPP_FileInfoReader::p_GetLocalInfoDiff_UpdateDirectory(MBPP_FileInfoDiff& OutResult, MBUtility::Filesystem& Filesystem, MBPM_FileInfoExcluder const& Excluder, MBPP_DirectoryInfoNode const& StoredNode, std::string const& CurrentDirectoryPath, std::string const& CurrentIndexPath)
	{
		std::set<std::string> FilePaths;
		std::set<std::string> DirectoryPaths;
		//std::filesystem::directory_iterator DirectoryIterator(LocalDirectory);
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		std::vector<MBUtility::FSObjectInfo> DirectoryInfo = Filesystem.ListDirectory(CurrentDirectoryPath, &FSError);
		for (auto const& Entry : DirectoryInfo)
		{
			std::string EntryName = Entry.Name;
			if (!(Entry.Type == MBUtility::FileSystemType::File || Entry.Type == MBUtility::FileSystemType::Directory))
			{
				continue;
			}
			if (Excluder.Excludes(EntryName) && !Excluder.Includes(EntryName))
			{
				continue;
			}
			if (Entry.Type == MBUtility::FileSystemType::File)
			{
				FilePaths.insert(EntryName);
			}
			else if (Entry.Type == MBUtility::FileSystemType::Directory)
			{
				DirectoryPaths.insert(EntryName);
			}
		}
		auto LocalFilesIterator = FilePaths.begin();
		auto LocalFilesEnd = FilePaths.end();
		auto StoredFilesIterator = StoredNode.Files.begin();
		auto StoredFilesEnd = StoredNode.Files.end();
		while (StoredFilesIterator != StoredFilesEnd && LocalFilesIterator != LocalFilesEnd)
		{
			if (*LocalFilesIterator < StoredFilesIterator->FileName)
			{
				//More files
				OutResult.AddedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
				LocalFilesIterator++;
			}
			else if (*LocalFilesIterator > StoredFilesIterator->FileName)
			{
				OutResult.RemovedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
				StoredFilesIterator++;
			}
			else
			{
				//Equal
				if (Filesystem.Exists(CurrentDirectoryPath +"/"+ * LocalFilesIterator) == MBUtility::FilesystemError::Ok)
				{
					MBUtility::FilesystemError FSError;
					MBUtility::FSObjectInfo Info = Filesystem.GetInfo(CurrentDirectoryPath + "/" + *LocalFilesIterator,&FSError);
					if (!FSError)
					{
						StoredFilesIterator++;
						LocalFilesIterator++;
						continue;
					}
					uint64_t LocalLastWriteTime = Info.LastWriteTime;
					uint64_t StoredLastWriteTime = StoredFilesIterator->LastWriteTime;
					if (!(StoredLastWriteTime == 0 || LocalLastWriteTime == 0))
					{
						if (LocalLastWriteTime > StoredLastWriteTime)
						{
							OutResult.UpdatedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
						}
					}
					else
					{
						//std::string LocalHash = MBCrypto::GetFileHash(CurrentDirectoryPath + "/" + *LocalFilesIterator, MBCrypto::HashFunction::SHA1);
						MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
						std::unique_ptr<MBUtility::MBOctetInputStream> FileInput = Filesystem.GetFileInputStream(CurrentDirectoryPath + "/" + *LocalFilesIterator, &FSError);
						if (!FSError)
						{
							OutResult.RemovedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
							StoredFilesIterator++;
							LocalFilesIterator++;
							continue;
						}
						std::string LocalHash = MBCrypto::GetHash(FileInput.get(), MBCrypto::HashFunction::SHA1);
						if (LocalHash != StoredFilesIterator->FileHash)
						{
							OutResult.UpdatedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
						}
					}
				}
				StoredFilesIterator++;
				LocalFilesIterator++;
			}
		}
		while (LocalFilesIterator != LocalFilesEnd)
		{
			OutResult.AddedFiles.push_back(CurrentIndexPath + *LocalFilesIterator);
			LocalFilesIterator++;
		}
		while (StoredFilesIterator != StoredFilesEnd)
		{
			OutResult.RemovedFiles.push_back(CurrentIndexPath + StoredFilesIterator->FileName);
			StoredFilesIterator++;
		}
		auto LocalDirectoriesIterator = DirectoryPaths.begin();
		auto LocalDirectoriesEnd = DirectoryPaths.end();
		auto StoredDirectoriesIterator = StoredNode.Directories.begin();
		auto StoredDirectoriesEnd = StoredNode.Directories.end();
		while (LocalDirectoriesIterator != LocalDirectoriesEnd && StoredDirectoriesIterator != StoredDirectoriesEnd)
		{
			if (*LocalDirectoriesIterator < StoredDirectoriesIterator->DirectoryName)
			{
				OutResult.AddedDirectories.push_back(CurrentIndexPath + *LocalDirectoriesIterator);
				LocalDirectoriesIterator++;
			}
			else if (*LocalDirectoriesIterator > StoredDirectoriesIterator->DirectoryName)
			{
				OutResult.DeletedDirectories.push_back(CurrentIndexPath + *LocalDirectoriesIterator);
				StoredDirectoriesIterator++;
			}
			else
			{
				p_GetLocalInfoDiff_UpdateDirectory(OutResult, Filesystem,Excluder, *StoredDirectoriesIterator, CurrentDirectoryPath +"/"+ StoredDirectoriesIterator->DirectoryName, CurrentIndexPath + StoredDirectoriesIterator->DirectoryName + "/");
				LocalDirectoriesIterator++;
				StoredDirectoriesIterator++;
			}
		}
		while (LocalDirectoriesIterator != LocalDirectoriesEnd)
		{
			OutResult.AddedDirectories.push_back(CurrentIndexPath + *LocalDirectoriesIterator);
			LocalDirectoriesIterator++;
		}
		while (StoredDirectoriesIterator != StoredDirectoriesEnd)
		{
			OutResult.DeletedDirectories.push_back(CurrentIndexPath + StoredDirectoriesIterator->DirectoryName);
			StoredDirectoriesIterator++;
		}
	}


	void CreatePacketFilesData(std::string const& PacketToHashDirectory,std::string const& OutputName)
	{
		//std::ofstream OutputFile = std::ofstream(PacketToHashDirectory +"/"+ OutputName,std::ios::out|std::ios::binary);
		//if (!OutputFile.is_open())
		//{
		//	return;
		//}
		//MBPM_FileInfoExcluder Excluder;
		//if (std::filesystem::exists(PacketToHashDirectory + "/MBPM_FileInfoIgnore"))
		//{
		//	Excluder = MBPM_FileInfoExcluder(PacketToHashDirectory + "/MBPM_FileInfoIgnore");
		//}
		//Excluder.AddExcludeFile("/"+OutputName);
		//MBUtility::MBFileOutputStream OutputStream = MBUtility::MBFileOutputStream(&OutputFile);
		//h_WriteFileInfoHeader(MBPP_FileInfoHeader(), &OutputStream);
		////DEBUG
		////std::cout << bool(OutputFile.is_open() && OutputFile.good()) << std::endl;
		////OutputFile.flush();
		////OutputFile.close();
		////return;
		//h_WriteDirectoryData_Recursive(&OutputStream, PacketToHashDirectory, "/", PacketToHashDirectory, Excluder, MBCrypto::HashFunction::SHA1,nullptr);
		//OutputFile.flush();
		//OutputFile.close();
        MBPP_FileInfoReader::CreateFileInfo(PacketToHashDirectory,OutputName);
	}
	void CreatePacketFilesData(std::string const& PacketToHashDirectory, MBUtility::MBSearchableOutputStream* OutputStream)
	{
		//MBPM_FileInfoExcluder Excluder;
		//if (std::filesystem::exists(PacketToHashDirectory + "/MBPM_FileInfoIgnore"))
		//{
		//	Excluder = MBPM_FileInfoExcluder(PacketToHashDirectory + "/MBPM_FileInfoIgnore");
		//}
		//h_WriteFileInfoHeader(MBPP_FileInfoHeader(), OutputStream);
		//h_WriteDirectoryData_Recursive(OutputStream, PacketToHashDirectory, "/", PacketToHashDirectory, Excluder, MBCrypto::HashFunction::SHA1, nullptr);
        MBPP_FileInfoReader::CreateFileInfo(PacketToHashDirectory,OutputStream);
	}
	MBPP_FileInfoReader CreateFileInfo(std::string const& DirectoryToIterate)
	{
		//otroligt jank l�sning men orkar inte skriva om all kod just nu
		//std::string TemporaryBuffer = "";
		//MBPM_FileInfoExcluder Excluder;
		//if (std::filesystem::exists(DirectoryToIterate + "/MBPM_FileInfoIgnore"))
		//{
		//	Excluder = MBPM_FileInfoExcluder(DirectoryToIterate + "/MBPM_FileInfoIgnore");
		//}
		//MBUtility::MBStringOutputStream OutputStream = MBUtility::MBStringOutputStream(TemporaryBuffer);
		//h_WriteFileInfoHeader(MBPP_FileInfoHeader(), &OutputStream);

		//h_WriteDirectoryData_Recursive(&OutputStream, DirectoryToIterate, "/", DirectoryToIterate, Excluder, MBCrypto::HashFunction::SHA1, nullptr);
		//return(MBPP_FileInfoReader(TemporaryBuffer.data(), TemporaryBuffer.size()));
        MBPP_FileInfoReader ReturnValue;
        MBPP_FileInfoReader::CreateFileInfo(DirectoryToIterate,&ReturnValue);
        return(ReturnValue);
	}
	bool MBPP_PathIsValid(std::string const& PathToCheck)
	{
		if (PathToCheck.find("..") != PathToCheck.npos)
		{
			return(false);
		}
		return(true);
	}
	//MBPP_ComputerInfo GetComputerInfo()
	//{
	//	return(MBPP_ComputerInfo());
	//}
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
	//ANTAGANDE filer �r sorterade i ordning
	//BEGIN MBPM_FileInfoExcluder
	bool MBPM_FileInfoExcluder::p_MatchDirectory(std::string const& Directory, std::string const& StringToMatch) const
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
    bool h_MatchRegex(std::regex const& RegexToCompare,std::string const& StringToMatch)
    {
        bool ReturnValue = false;
        std::sregex_iterator Begin = std::sregex_iterator(StringToMatch.begin(),StringToMatch.end(),RegexToCompare);
        std::sregex_iterator End;
        while(Begin != End)
        {
            if(Begin->position() > 0)
            {
                break; 
            }
            if(Begin->length() == StringToMatch.size())
            {
                ReturnValue = true;
                break;
            }
            ++Begin;
        }
        return(ReturnValue);
    }
    bool MBPM_FileInfoExcluder::p_MatchRegex(std::regex const& RegexToCompare,std::string const& StringToMatch) const
    {
        bool ReturnValue = false;
        if(StringToMatch.size() == 0)
        {
            return(ReturnValue);   
        }
        //TODO Should probably do some form of normalization instead
        if(StringToMatch.back() == '/')
        {
            ReturnValue = h_MatchRegex(RegexToCompare,StringToMatch) || h_MatchRegex(RegexToCompare,StringToMatch.substr(0,StringToMatch.size()-1));   
        }
        else
        {
            ReturnValue = h_MatchRegex(RegexToCompare,StringToMatch) || h_MatchRegex(RegexToCompare,StringToMatch+"/");
        }
        return(ReturnValue);  
    }

	bool MBPM_FileInfoExcluder::p_MatchFile(std::string const& StringToCompare, std::string const& StringToMatch) const
	{
		return(StringToCompare == StringToMatch);
	}
    void MBPM_FileInfoExcluder::AddInclude(std::string const& IncludeString)
    {
        try
        {
            std::regex NewRegex = std::regex(IncludeString);
            m_IncludeRegex.push_back(std::move(NewRegex));
        }
        catch(std::exception const& e)
        {
            m_IncludeStrings.push_back(IncludeString);
        }
    }
	void MBPM_FileInfoExcluder::AddExcludeFile(std::string const& FileToExlude)
	{
        //TODO determining wheter or not it's a valid regex by cathing exceptions is kind of hacky
        try
        {
            std::regex NewRegex = std::regex(FileToExlude);
            m_ExcludeRegex.push_back(std::move(NewRegex));
        }
        catch(std::exception const& e)
        {
            m_ExcludeStrings.push_back(FileToExlude);
        }
	}

	MBPM_FileInfoExcluder::MBPM_FileInfoExcluder(MBUtility::MBOctetInputStream* InputStream)
	{
        p_AddDefaults();
		std::string TotalData;
		constexpr size_t ReadChunkSize = 4096;
		uint8_t ReadBuffer[ReadChunkSize];
		while (true)
		{
			size_t ReadBytes = InputStream->Read(ReadBuffer, ReadChunkSize);
			TotalData.append(ReadBuffer, ReadBuffer + ReadBytes);
			if (ReadBytes < ReadChunkSize)
			{
				break;
			}
		}
		std::stringstream Stream(TotalData);
		std::string CurrentLine = "";
		bool IsInclude = false;
		bool IsExclude = false;
		while (std::getline(Stream, CurrentLine))
		{
			if (CurrentLine.size() == 0)
			{
				continue;
			}
			if (CurrentLine.back() == '\r')
			{
				CurrentLine = CurrentLine.substr(0, CurrentLine.size() - 1);
			}
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
			else if (CurrentLine != "")
			{
				if (IsInclude)
				{
                    AddInclude(CurrentLine);
				}
				else
				{
                    AddExcludeFile(CurrentLine);
				}
			}
		}
	}
    void MBPM_FileInfoExcluder::p_AddDefaults()
    {
		AddExcludeFile("/MBPM_UploadedChanges/");
		AddExcludeFile("/MBPM_BuildFiles/");
		AddExcludeFile("/MBPM_Builds/");
		AddExcludeFile("/\\..*");
    }
    MBPM_FileInfoExcluder::MBPM_FileInfoExcluder(std::string const& PathPosition)
	{
        p_AddDefaults();
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
                if (CurrentLine.size() == 0)
                {
                    continue;
                }
                if (CurrentLine.back() == '\r')
                {
                    CurrentLine = CurrentLine.substr(0, CurrentLine.size() - 1);
                }
                if (IsInclude)
				{
                    AddInclude(CurrentLine);
				}
                else
				{
                    AddExcludeFile(CurrentLine);
				}
			}
		}
	}
	bool MBPM_FileInfoExcluder::Excludes(std::string const& StringToMatch) const
	{
		bool ReturnValue = false;
		for (std::string const& ExcludeString : m_ExcludeStrings)
		{
			if (ExcludeString.front() != '/')
			{
				continue;
			}
			if (ExcludeString.back() == '/')
			{
				ReturnValue = p_MatchDirectory(ExcludeString, StringToMatch);
			}
			else
			{
				ReturnValue = p_MatchFile(ExcludeString, StringToMatch);
			}
			if (ReturnValue)
			{
				break;
			}
		}
        for(std::regex const& ExcludeRegex : m_ExcludeRegex)
        {
            if(p_MatchRegex(ExcludeRegex,StringToMatch))
            {
                ReturnValue = true;
                break;   
            }
        }
		return(ReturnValue);
	}
	bool MBPM_FileInfoExcluder::Includes(std::string const& StringToMatch) const
	{
		bool ReturnValue = false;
		for (std::string const& IncludeString : m_IncludeStrings)
		{
			if (IncludeString.front() != '/')
			{
				continue;
			}
			if (IncludeString.back() == '/')
			{
				ReturnValue = p_MatchDirectory(IncludeString, StringToMatch);
			}
			else
			{
				ReturnValue = p_MatchFile(IncludeString, StringToMatch);
			}
			if (ReturnValue)
			{
				break;
			}
		}
        for(std::regex const& IncludeRegex : m_IncludeRegex)
        {
            if(p_MatchRegex(IncludeRegex,StringToMatch))
            {
                ReturnValue = true;
                break;   
            }
        }
		return(ReturnValue);
	}
	//END MBPM_FileInfoExcluder
	
	//BEGIN MBPP_ComputerDiffInfo
	std::string MBPP_ComputerDiffInfo::GetOSTypeString(MBPP_OSType TypeToConvert)
	{
		std::string ReturnValue = "";
		if (TypeToConvert == MBPP_OSType::Windows)
		{
			ReturnValue = "Windows";
		}
		else if (TypeToConvert == MBPP_OSType::Linux)
		{
			ReturnValue = "Linux";
		}
		else if (TypeToConvert == MBPP_OSType::MacOS)
		{
			ReturnValue = "MacOS";
		}
		else if (TypeToConvert == MBPP_OSType::Null || TypeToConvert == MBPP_OSType::Any)
		{
			ReturnValue = "*";
		}
		return(ReturnValue);
	}
	std::string MBPP_ComputerDiffInfo::GetProcessorTypeString(MBPP_ProcessorType TypeToConvert)
	{
		std::string ReturnValue = "";
		if (TypeToConvert == MBPP_ProcessorType::ARM)
		{
			ReturnValue = "ARM";
		}
		else if (TypeToConvert == MBPP_ProcessorType::ARMv7)
		{
			ReturnValue = "ARMv7";
		}
		else if (TypeToConvert == MBPP_ProcessorType::x86)
		{
			ReturnValue = "x86";
		}
		else if (TypeToConvert == MBPP_ProcessorType::x86_64)
		{
			ReturnValue = "x86-64";
		}
		else if (TypeToConvert == MBPP_ProcessorType::Null || TypeToConvert == MBPP_ProcessorType::Any)
		{
			ReturnValue = "*";
		}
		return(ReturnValue);
	}
	MBPP_OSType MBPP_ComputerDiffInfo::GetStringOSType(std::string const& StringToConvert)
	{
		MBPP_OSType ReturnValue = MBPP_OSType::Null;
		if (StringToConvert == "*")
		{
			ReturnValue = MBPP_OSType::Any;
		}
		else if(StringToConvert == "Linux")
		{
			ReturnValue = MBPP_OSType::Linux;
		}
		else if (StringToConvert == "Windows")
		{
			ReturnValue = MBPP_OSType::Windows;
		}
		return(ReturnValue);
	}
	MBPP_ProcessorType MBPP_ComputerDiffInfo::GetStringProcessorType(std::string const& StringToConvert)
	{
		MBPP_ProcessorType ReturnValue = MBPP_ProcessorType::Null;
		if (StringToConvert == "*")
		{
			ReturnValue = MBPP_ProcessorType::Any;
		}
		else if (StringToConvert == "x86-64")
		{
			ReturnValue = MBPP_ProcessorType::x86_64;
		}
		else if (StringToConvert == "x86")
		{
			ReturnValue = MBPP_ProcessorType::x86;
		}
		else if (StringToConvert == "ARMv7")
		{
			ReturnValue = MBPP_ProcessorType::ARMv7;
		}
		return(ReturnValue);
	}
	MBPP_ComputerDiffRule h_ParseRule(MBParsing::JSONObject const& ObjectToParse, MBError* OutError)
	{
		MBPP_ComputerDiffRule ReturnValue;
		if (!(ObjectToParse.HasAttribute("DataDirectory") && ObjectToParse.GetAttribute("DataDirectory").GetType() == MBParsing::JSONObjectType::String))
		{
			if (OutError != nullptr)
			{
				*OutError = false;
				OutError->ErrorMessage = "DataDirectory attribute isn't of string type";
			}
			return(ReturnValue);
		}
		ReturnValue.InfoDirectory = ObjectToParse.GetAttribute("DataDirectory").GetStringData();
		if (!(ObjectToParse.HasAttribute("InfoMatch") && ObjectToParse.GetAttribute("InfoMatch").GetType() == MBParsing::JSONObjectType::Array))
		{
			if (OutError != nullptr)
			{
				*OutError = false;
				OutError->ErrorMessage = "InfoMatch Doesn't exist or isn't of Array type";
			}
			return(ReturnValue);
		}
		std::vector<MBParsing::JSONObject> const& InfoMatches = ObjectToParse.GetAttribute("InfoMatch").GetArrayData();
		if (!(InfoMatches[0].GetType() == MBParsing::JSONObjectType::Array && InfoMatches.size() >= 2 && InfoMatches[1].GetType() == MBParsing::JSONObjectType::Array))
		{
			if (OutError != nullptr)
			{
				*OutError = false;
				OutError->ErrorMessage = "Invalid infomatch types";
			}
			return(ReturnValue);
		}
		std::vector<MBParsing::JSONObject> const& OSMatches = InfoMatches[0].GetArrayData();
		std::vector<MBParsing::JSONObject> const& ProcessorMatches = InfoMatches[1].GetArrayData();
		for (size_t i = 0; i < OSMatches.size(); i++)
		{
			if (OSMatches[i].GetType() == MBParsing::JSONObjectType::String)
			{
				MBPP_OSType NewOSMatch = MBPP_ComputerDiffInfo::GetStringOSType(OSMatches[i].GetStringData());
				if (NewOSMatch != MBPP_OSType::Null)
				{
					ReturnValue.OSMatches.push_back(NewOSMatch);
				}
				else
				{
					if (OutError != nullptr)
					{
						*OutError = false;
						OutError->ErrorMessage = "OSMatch not string type";
					}
					return(ReturnValue);
				}
			}
		}
		for (size_t i = 0; i < ProcessorMatches.size(); i++)
		{
			if (ProcessorMatches[i].GetType() == MBParsing::JSONObjectType::String)
			{
				MBPP_ProcessorType NewProcessorMatch = MBPP_ComputerDiffInfo::GetStringProcessorType(ProcessorMatches[i].GetStringData());
				if (NewProcessorMatch != MBPP_ProcessorType::Null)
				{
					ReturnValue.ProcessorMatch.push_back(NewProcessorMatch);
				}
				else
				{
					if (OutError != nullptr)
					{
						*OutError = false;
						OutError->ErrorMessage = "Processor match not string type";
					}
					return(ReturnValue);
				}
			}
		}
		return(ReturnValue);
	}
	MBError MBPP_ComputerDiffInfo::ReadInfo(std::string const& InfoPath) 
	{
		MBError ReturnValue = true;
		m_Rules = {};
		if (!std::filesystem::exists(InfoPath))
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Infopath doesn't exist";
			return(ReturnValue);
		}
		std::string FileData = std::string(std::filesystem::file_size(InfoPath),0);
		std::ifstream FileInput = std::ifstream(InfoPath, std::ios::in|std::ios::binary);
		FileInput.read(FileData.data(), FileData.size());
		MBParsing::JSONObject JsonInfo = MBParsing::ParseJSONObject(FileData, 0, nullptr, &ReturnValue);
		if (!ReturnValue)
		{
			return(ReturnValue);
		}
		if (JsonInfo.HasAttribute("Rules") && JsonInfo.GetAttribute("Rules").GetType() == MBParsing::JSONObjectType::Array)
		{
			std::vector<MBParsing::JSONObject> const& Rules = JsonInfo.GetAttribute("Rules").GetArrayData();
			for (size_t i = 0; i < Rules.size(); i++)
			{
				m_Rules.push_back(h_ParseRule(Rules[i],&ReturnValue));
				if (!ReturnValue)
				{
					return(ReturnValue);
				}
			}
		}
		return(ReturnValue);
	}
	std::string MBPP_ComputerDiffInfo::Match(MBPP_ComputerInfo const& InfoToMatch)
	{
		std::string ReturnValue = "";
		for (size_t i = 0; i < m_Rules.size(); i++)
		{
			bool OSMatches = false;
			bool ProccessorMatches = false;
			for (size_t j = 0; j < m_Rules[i].OSMatches.size(); j++)
			{
				if (InfoToMatch.OSType == m_Rules[i].OSMatches[j] || m_Rules[i].OSMatches[j] == MBPP_OSType::Any)
				{
					OSMatches = true;
					break;
				}
			}
			for (size_t j = 0; j < m_Rules[i].ProcessorMatch.size(); j++)
			{
				if (InfoToMatch.ProcessorType == m_Rules[i].ProcessorMatch[j] || m_Rules[i].ProcessorMatch[j] == MBPP_ProcessorType::Any)
				{
					ProccessorMatches = true;
					break;
				}
			}
			if (OSMatches == true && ProccessorMatches == true)
			{
				ReturnValue = m_Rules[i].InfoDirectory;
				break;
			}
		}
		return(ReturnValue);
	}

	//END MBPP_ComputerDiffInfo

	bool h_FileLessThan(MBPP_FileInfo const& Left, MBPP_FileInfo const& Right)
	{
		return(Left.FileName < Right.FileName);
	}
	bool h_DirectoryLessThan(MBPP_DirectoryInfoNode const& Left, MBPP_DirectoryInfoNode const& Right)
	{
		return(Left.DirectoryName < Right.DirectoryName);
	}
	//BEGIN MBPP_FileInfoReader
	MBPP_DirectoryInfoNode MBPP_FileInfoReader::p_ReadDirectoryInfoFromFile(MBUtility::MBOctetInputStream* FileToReadFrom, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData)
	{
		//Skippointer 8 bytes
		//StructureHash
		//ContentHash
		//Namn, 2 bytes l�ngd + l�ngd bytes namn data
		//Number of files 4 bytes, f�ljt av fil bytes
		//number of directories 4 bytes, f�ljt av directoriesen
		//skip poointer pekar till slutet av directory datan
		MBPP_DirectoryInfoNode ReturnValue;
		//const size_t HashSize = MBCrypto::HashObject(Header.HashFunction).GetDigestSize();
		//h_ReadBigEndianInteger(FileToReadFrom, 8);//filepointer
		//std::string DirectoryHash = std::string(HashSize, 0);
		//std::string ContentHash = std::string(HashSize, 0);
		////vi l�ser �ven in storleken
		//FileToReadFrom->Read(DirectoryHash.data(), HashSize);
		//FileToReadFrom->Read(ContentHash.data(), HashSize);
		//uint64_t DirectorySize = h_ReadBigEndianInteger(FileToReadFrom, 8);
		//size_t NameSize = h_ReadBigEndianInteger(FileToReadFrom, 2);
		//std::string DirectoryName = std::string(NameSize, 0);
		//FileToReadFrom->Read(DirectoryName.data(), NameSize);
		
		for (auto const& Property : ExtensionData.DirectoryProperties)
		{
			std::string TotalPropertyData = p_ReadPropertyFromStream(FileToReadFrom, Property);
			if (Property.Type == MBPP_FSEntryPropertyType::ContentHash)
			{
				ReturnValue.ContentHash = std::move(TotalPropertyData);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Name)
			{
				ReturnValue.DirectoryName = std::move(TotalPropertyData);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Size)
			{
				ReturnValue.Size = MBParsing::ParseBigEndianInteger(TotalPropertyData.data(), TotalPropertyData.size(), 0, nullptr);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::LastWrite)
			{
				ReturnValue.LastWriteTime = MBParsing::ParseBigEndianInteger(TotalPropertyData.data(), TotalPropertyData.size(), 0, nullptr);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::StructureHash)
			{
				ReturnValue.StructureHash = std::move(TotalPropertyData);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::SkipPointer)
			{
				//gör inget när vi läser in filen
			}
		}
		
		uint32_t NumberOfFiles = h_ReadBigEndianInteger(FileToReadFrom, 4);
		for (size_t i = 0; i < NumberOfFiles; i++)
		{
			ReturnValue.Files.push_back(p_ReadFileInfoFromFile(FileToReadFrom,Header,ExtensionData));
		}
		if (!std::is_sorted(ReturnValue.Files.begin(), ReturnValue.Files.end(),h_FileLessThan))
		{
			throw std::runtime_error("Invalid FileInfo data: File names not sorted");
		}
		uint32_t NumberOfDirectories = h_ReadBigEndianInteger(FileToReadFrom, 4);
		for (size_t i = 0; i < NumberOfDirectories; i++)
		{
			ReturnValue.Directories.push_back(p_ReadDirectoryInfoFromFile(FileToReadFrom, Header, ExtensionData));
		}
		if (!std::is_sorted(ReturnValue.Directories.begin(), ReturnValue.Directories.end(),h_DirectoryLessThan))
		{
			throw std::runtime_error("Invalid FileInfo data: File names not sorted");
		}
		return(ReturnValue);
	}
	std::string MBPP_FileInfoReader::p_ReadPropertyFromStream(MBUtility::MBOctetInputStream* InputStream, MBPP_FSEntryProperyInfo const& PropertyToRead)
	{
		if (
			(
				PropertyToRead.Type == MBPP_FSEntryPropertyType::LastWrite || 
				PropertyToRead.Type == MBPP_FSEntryPropertyType::Size || 
				PropertyToRead.Type == MBPP_FSEntryPropertyType::SkipPointer
			) 
			&& PropertyToRead.PropertySize != 8)
		{
			throw std::runtime_error("property has to be exactly 8 bytes");
		}
		uint8_t IntBuff[8];
		std::string TotalPropertyData;
		if (PropertyToRead.PropertySize & (1 << 15))
		{
			uint16_t IntSize = PropertyToRead.PropertySize & (~(1 << 15));
			if (IntSize > 8)
			{
				throw std::runtime_error("Dynamic size specifier can't be larget than 8 bytes");
			}
			size_t ReadBytes = InputStream->Read(IntBuff, IntSize);
			if (ReadBytes < IntSize)
			{
				throw std::runtime_error("Unsufficient data when reading FileInfo property length");
			}
			uint64_t PropertySize = MBParsing::ParseBigEndianInteger(IntBuff, IntSize, 0, nullptr);
			TotalPropertyData = std::string(PropertySize, 0);
			uint64_t BytesRead = InputStream->Read(TotalPropertyData.data(), TotalPropertyData.size());
			if (BytesRead < PropertySize)
			{
				throw std::runtime_error("Unsufficient data when reading FileInfo property data");
			}
		}
		else
		{
			TotalPropertyData = std::string(PropertyToRead.PropertySize, 0);
			size_t BytesRead = InputStream->Read(TotalPropertyData.data(), TotalPropertyData.size());
			if (BytesRead < PropertyToRead.PropertySize)
			{
				throw std::runtime_error("Unsufficient data when reading FileInfo properties");
			}
		}
		return(TotalPropertyData);
	}
	MBPP_FileInfo MBPP_FileInfoReader::p_ReadFileInfoFromFile(MBUtility::MBOctetInputStream* InputStream, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData)
	{
		MBPP_FileInfo ReturnValue;
		//const size_t HashSize = MBCrypto::HashObject(Header.HashFunction).GetDigestSize();
		//uint16_t FileNameSize = h_ReadBigEndianInteger(InputStream, 2);
		//ReturnValue.FileName = std::string(FileNameSize, 0);
		//InputStream->Read(ReturnValue.FileName.data(), FileNameSize);
		//ReturnValue.FileHash = std::string(HashSize, 0);
		//InputStream->Read(ReturnValue.FileHash.data(), HashSize);
		//ReturnValue.FileSize = h_ReadBigEndianInteger(InputStream, 8);
		for (auto const& Property : ExtensionData.FileProperties)
		{
			std::string TotalPropertyData = p_ReadPropertyFromStream(InputStream, Property);
			if (Property.Type == MBPP_FSEntryPropertyType::ContentHash)
			{
				ReturnValue.FileHash = std::move(TotalPropertyData);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Name)
			{
				ReturnValue.FileName = std::move(TotalPropertyData);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Size)
			{
				ReturnValue.FileSize = MBParsing::ParseBigEndianInteger(TotalPropertyData.data(), TotalPropertyData.size(), 0, nullptr);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::LastWrite)
			{
				ReturnValue.LastWriteTime = MBParsing::ParseBigEndianInteger(TotalPropertyData.data(), TotalPropertyData.size(), 0, nullptr);
			}
		}
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
		*this = MBPP_FileInfoReader(&FileReader);

	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(MBUtility::MBOctetInputStream* InputStream)
	{
		try
		{
			m_InfoHeader = p_ReadFileInfoHeader(InputStream);
			m_ExtensionData = p_ReadExtensionData(InputStream, m_InfoHeader.ExtensionDataSize);
			if (m_InfoHeader.Version[0] == 0 && m_InfoHeader.Version[1] < 2 || m_InfoHeader.ExtensionDataSize == 0)
			{
				m_ExtensionData.FileProperties = {
					{MBPP_FSEntryPropertyType::Name,uint16_t((1ll << 15ll) + 2)},
					{MBPP_FSEntryPropertyType::ContentHash,20},
					{MBPP_FSEntryPropertyType::Size,8},
				};
				m_ExtensionData.DirectoryProperties = {
					{MBPP_FSEntryPropertyType::SkipPointer,8},
					{MBPP_FSEntryPropertyType::StructureHash,20},
					{MBPP_FSEntryPropertyType::ContentHash,20},
					{MBPP_FSEntryPropertyType::Size,8},
					{MBPP_FSEntryPropertyType::Name,uint16_t((1ll << 15ll) + 2)},
				};
			}
			m_TopNode = p_ReadDirectoryInfoFromFile(InputStream, m_InfoHeader,m_ExtensionData);//sha1 diges size
		}
		catch (const std::exception& e)
		{
			//std::cout << e.what() << std::endl;
			m_TopNode = MBPP_DirectoryInfoNode();
		}
	}
	MBPP_FileInfoReader::MBPP_FileInfoReader(const void* DataToRead, size_t DataSize)
	{
		MBUtility::MBBufferInputStream BufferReader(DataToRead,DataSize);
		*this = MBPP_FileInfoReader(&BufferReader);
	}
	void h_UpdateDiffDirectoryFiles(MBPP_FileInfoDiff& DiffToUpdate, MBPP_DirectoryInfoNode const& ClientDirectory, MBPP_DirectoryInfoNode const& ServerDirectory, std::string const& CurrentPath)
	{
		std::vector<MBPP_FileInfo>::const_iterator ClientEnd = ClientDirectory.Files.end();
		std::vector<MBPP_FileInfo>::const_iterator ServerEnd = ServerDirectory.Files.end();
		std::vector<MBPP_FileInfo>::const_iterator ClientIterator = ClientDirectory.Files.begin();
		std::vector<MBPP_FileInfo>::const_iterator ServerIterator = ServerDirectory.Files.begin();
		assert(std::is_sorted(ClientIterator, ClientEnd));
		assert(std::is_sorted(ServerIterator, ServerEnd));
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
				//dem �r lika
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
		assert(std::is_sorted(ClientIterator, ClientEnd));
		assert(std::is_sorted(ServerIterator, ServerEnd));
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
				//dem �r lika
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
		//assert(std::is_sorted(ReturnValue.AddedDirectories.begin(), ReturnValue.AddedDirectories.end()));
		//assert(std::is_sorted(ReturnValue.DeletedDirectories.begin(), ReturnValue.DeletedDirectories.end()));
		//assert(std::is_sorted(ReturnValue.AddedFiles.begin(), ReturnValue.AddedFiles.end()));
		//assert(std::is_sorted(ReturnValue.UpdatedFiles.begin(), ReturnValue.UpdatedFiles.end()));
		//assert(std::is_sorted(ReturnValue.RemovedFiles.begin(), ReturnValue.RemovedFiles.end()));
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
		const MBPP_DirectoryInfoNode* ReturnValue = nullptr;
		if (PathComponents.size() == 1 || PathComponents.size() == 0)
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

	MBPP_DirectoryInfoNode* MBPP_FileInfoReader::p_GetTargetDirectory(std::vector<std::string> const& PathComponents)
	{
		MBPP_DirectoryInfoNode* ReturnValue = nullptr;
		if (PathComponents.size() == 1 || PathComponents.size() == 0)
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
	MBPP_DirectoryInfoNode* MBPP_FileInfoReader::p_GetTargetDirectory_Impl(std::vector<std::string> const& PathComponents)
	{
		return(nullptr);
	}
	MBPP_FileInfoDiff MBPP_FileInfoReader::GetFileInfoDifference(MBPP_DirectoryInfoNode const& ClientInfo, MBPP_DirectoryInfoNode const& ServerInfo)
	{
		MBPP_FileInfoDiff ReturnValue;
		MBPP_DirectoryInfoNode DefaultClientNode;
		MBPP_DirectoryInfoNode DefaultServerNode;
		MBPP_DirectoryInfoNode const* ClientNodeToUse = &ClientInfo;
		MBPP_DirectoryInfoNode const* ServerNodeToUse = &ServerInfo;
		if (&ClientInfo == nullptr)
		{
			ClientNodeToUse = &DefaultClientNode;
		}
		if (&ServerInfo == nullptr)
		{
			ServerNodeToUse = &DefaultServerNode;
		}
		h_UpdateDiffOverDirectory(ReturnValue, *ClientNodeToUse, *ServerNodeToUse, "/");
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
			if (ObjectDirectory == nullptr)
			{
				return(nullptr);
			}
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
			if (PathComponents.size() == 0)//antar att detta bara kan h�nda om pathen �r p� formen */
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
		//snabb som kanske inte alltid �r sann, men g�r p� hash
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
		std::swap(LeftInfoReader.m_InfoHeader, RightInfoReader.m_InfoHeader);
		std::swap(LeftInfoReader.m_ExtensionData, RightInfoReader.m_ExtensionData);
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
		m_ExtensionData = ReaderToCopy.m_ExtensionData;
		m_InfoHeader = ReaderToCopy.m_InfoHeader;
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
	MBPP_FileInfoExtensionData MBPP_FileInfoReader::p_ReadExtensionData(MBUtility::MBOctetInputStream* InputStream, uint32_t TotalExtensionDataSize)
	{
		MBPP_FileInfoExtensionData ReturnValue;
		size_t ParseOffset = 0;
		uint8_t IntBuff[4];
		while (ParseOffset < TotalExtensionDataSize)
		{
			MBPP_ExtensionType NewType;
			size_t ReadBytes = InputStream->Read(IntBuff, 2);
			if (ReadBytes < 2)
			{
				throw std::runtime_error("Insufficient extension data");
			}
			NewType = MBPP_ExtensionType(MBParsing::ParseBigEndianInteger(IntBuff, 2, 0, nullptr));
			uint32_t ExtensionSize;
			ReadBytes = InputStream->Read(IntBuff, 4);
			if (ReadBytes < 4)
			{
				throw std::runtime_error("Insufficient extension data");
			}
			ExtensionSize = MBParsing::ParseBigEndianInteger(IntBuff, 4, 0, nullptr);
			ParseOffset += 6;
			if (ParseOffset + ExtensionSize > TotalExtensionDataSize)
			{
				throw std::runtime_error("Invalid extension size in ExtensionData block, exceeds total ExtensionDataBlock Size");
			}
			if (NewType == MBPP_ExtensionType::FileAndDirectoryProperties)
			{
				//file properties, then directory properties
				std::string TotalData = std::string(ExtensionSize, 0);
				ReadBytes = InputStream->Read(TotalData.data(), ExtensionSize);
				ParseOffset += ReadBytes;
				if (ReadBytes < ExtensionSize || ExtensionSize < 8)
				{
					throw std::runtime_error("Insufficient extension data");
				}
				size_t ExtensionParseOffset = 0;
				uint16_t NumberOfFileProperties = MBParsing::ParseBigEndianInteger(TotalData.data(),2,ExtensionParseOffset,&ExtensionParseOffset);
				for (size_t i = 0; i < NumberOfFileProperties; i++)
				{
					if (ExtensionParseOffset + 4 > ExtensionSize)
					{
						throw std::runtime_error("Insufficient extension data when parsing FileAndDirectoryProperties extension");
					}
					MBPP_FSEntryProperyInfo NewInfo;
					NewInfo.Type = MBPP_FSEntryPropertyType(MBParsing::ParseBigEndianInteger(TotalData.data(),2,ExtensionParseOffset,&ExtensionParseOffset));
					NewInfo.PropertySize = MBParsing::ParseBigEndianInteger(TotalData.data(), 2, ExtensionParseOffset, &ExtensionParseOffset);
					ReturnValue.FileProperties.push_back(NewInfo);
				}
				if (ExtensionParseOffset + 2 > ExtensionSize)
				{
					throw std::runtime_error("Insufficient extension data when parsing FileAndDirectoryProperties extension");
				}
				uint16_t NumberOfDirectoryProperties = MBParsing::ParseBigEndianInteger(TotalData.data(), 2, ExtensionParseOffset, &ExtensionParseOffset);
				for (size_t i = 0; i < NumberOfDirectoryProperties; i++)
				{
					if (ExtensionParseOffset + 4 > ExtensionSize)
					{
						throw std::runtime_error("Insufficient extension data when parsing FileAndDirectoryProperties extension");
					}
					MBPP_FSEntryProperyInfo NewInfo;
					NewInfo.Type = MBPP_FSEntryPropertyType(MBParsing::ParseBigEndianInteger(TotalData.data(), 2, ExtensionParseOffset, &ExtensionParseOffset));
					NewInfo.PropertySize = MBParsing::ParseBigEndianInteger(TotalData.data(), 2, ExtensionParseOffset, &ExtensionParseOffset);
					ReturnValue.DirectoryProperties.push_back(NewInfo);
				}
			}
			else
			{
				std::string TotalData = std::string(ExtensionSize, 0);
				ReadBytes = InputStream->Read(TotalData.data(), ExtensionSize);
				if (ReadBytes < ExtensionSize)
				{
					throw std::runtime_error("Insufficient extension data");
				}
			}
		}

		return(ReturnValue);
	}
	void MBPP_FileInfoReader::p_WriteExtensionData(MBUtility::MBOctetOutputStream* OutputStream, MBPP_FileInfoExtensionData const& DataToWrite)
	{
		uint8_t IntBuff[4];
		MBPP_ExtensionType TypeToWrite = MBPP_ExtensionType::FileAndDirectoryProperties;
		MBParsing::WriteBigEndianInteger(IntBuff, uint64_t(TypeToWrite), 2);
		OutputStream->Write(IntBuff, 2);
		uint32_t TotalExtensionSize =4+4 * (DataToWrite.FileProperties.size() + DataToWrite.DirectoryProperties.size());
		MBParsing::WriteBigEndianInteger(IntBuff, TotalExtensionSize, 4);
		OutputStream->Write(IntBuff, 4);
		MBParsing::WriteBigEndianInteger(IntBuff, DataToWrite.FileProperties.size(),2);
		OutputStream->Write(IntBuff, 2);
		for (size_t i = 0; i < DataToWrite.FileProperties.size(); i++)
		{
			MBParsing::WriteBigEndianInteger(IntBuff, uint64_t(DataToWrite.FileProperties[i].Type),2);
			OutputStream->Write(IntBuff, 2);
			MBParsing::WriteBigEndianInteger(IntBuff, uint64_t(DataToWrite.FileProperties[i].PropertySize), 2);
			OutputStream->Write(IntBuff, 2);
		}
		MBParsing::WriteBigEndianInteger(IntBuff, DataToWrite.DirectoryProperties.size(), 2);
		OutputStream->Write(IntBuff, 2);
		for (size_t i = 0; i < DataToWrite.DirectoryProperties.size(); i++)
		{
			MBParsing::WriteBigEndianInteger(IntBuff, uint64_t(DataToWrite.DirectoryProperties[i].Type), 2);
			OutputStream->Write(IntBuff, 2);
			MBParsing::WriteBigEndianInteger(IntBuff, uint64_t(DataToWrite.DirectoryProperties[i].PropertySize), 2);
			OutputStream->Write(IntBuff, 2);
		}
	}
	std::string MBPP_FileInfoReader::p_SerializeExtensionData(MBPP_FileInfoExtensionData const& DataToSerialise)
	{
		std::string ReturnValue;
		MBUtility::MBStringOutputStream OutputStream(ReturnValue);
		p_WriteExtensionData(&OutputStream, DataToSerialise);
		return(ReturnValue);
	}
	MBPP_FileInfoExtensionData MBPP_FileInfoReader::p_GetLatestVersionExtensionData()
	{
		MBPP_FileInfoExtensionData ReturnValue;
		ReturnValue.FileProperties = {
			{MBPP_FSEntryPropertyType::Name,uint16_t((1ll << 15ll) + 2)},
			{MBPP_FSEntryPropertyType::ContentHash,20},
			{MBPP_FSEntryPropertyType::Size,8},
			{MBPP_FSEntryPropertyType::LastWrite,8},
		};
		ReturnValue.DirectoryProperties = {
			{MBPP_FSEntryPropertyType::SkipPointer,8},
			{MBPP_FSEntryPropertyType::StructureHash,20},
			{MBPP_FSEntryPropertyType::ContentHash,20},
			{MBPP_FSEntryPropertyType::Size,8},
			{MBPP_FSEntryPropertyType::Name,uint16_t((1ll << 15ll) + 2)},
		};
		return(ReturnValue);
	}
	MBPM_FileInfoExcluder MBPP_FileInfoReader::p_GetDirectoryFileInfoExcluder(std::string const& Directory)
	{
		MBPM_FileInfoExcluder Excluder;
		if (std::filesystem::exists(Directory + "/MBPM_FileInfoIgnore"))
		{
			Excluder = MBPM_FileInfoExcluder(Directory + "/MBPM_FileInfoIgnore");
		}
		return(Excluder);
	}
	void MBPP_FileInfoReader::p_WriteHeader(MBPP_FileInfoHeader const& HeaderToWrite, MBUtility::MBOctetOutputStream* OutputStream)
	{
		h_WriteFileInfoHeader(HeaderToWrite, OutputStream);
	}
	uint64_t MBPP_FileInfoReader::p_GetPathWriteTime(std::filesystem::path const& PathToInspect)
	{
		return(std::filesystem::directory_entry(PathToInspect).last_write_time().time_since_epoch().count());
	}
	MBPP_FileInfo MBPP_FileInfoReader::p_CreateFileInfo(std::string const& FilePath, MBUtility::Filesystem& FS, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData)
	{
		MBPP_FileInfo ReturnValue;
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		MBUtility::FSObjectInfo Info = FS.GetInfo(FilePath,&FSError);
		if (!FSError)
		{
			throw std::exception();
		}
		std::unique_ptr<MBUtility::MBOctetInputStream> Input = FS.GetFileInputStream(FilePath,&FSError);
		if (!FSError)
		{
			throw std::exception();
		}
		ReturnValue.FileName = FilePath.substr(FilePath.find_last_of('/') + 1);
		ReturnValue.FileSize = Info.Size;
		ReturnValue.LastWriteTime = Info.LastWriteTime;
		ReturnValue.FileHash = MBCrypto::GetHash(Input.get(),Header.HashFunction);//MBCrypto::GetFileHash(MBUnicode::PathToUTF8(FilePath), Header.HashFunction);
		return(ReturnValue);
	}
	MBPP_FileInfo MBPP_FileInfoReader::p_CreateFileInfo(std::filesystem::path const& FilePath, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData)
	{
		MBPP_FileInfo ReturnValue;
		std::filesystem::directory_entry AssociatedEntry(FilePath);
		ReturnValue.FileName = MBUnicode::PathToUTF8(FilePath.filename());
		ReturnValue.FileSize = AssociatedEntry.file_size();
		ReturnValue.LastWriteTime = p_GetPathWriteTime(FilePath);
		ReturnValue.FileHash = MBCrypto::GetFileHash(MBUnicode::PathToUTF8(FilePath), Header.HashFunction);
		return(ReturnValue);
	}
	MBPP_DirectoryInfoNode MBPP_FileInfoReader::p_CreateDirectoryInfo(
		std::string const& CurrentPacketPath,
		std::string const& TopDirectoryPath,
		MBUtility::Filesystem& AssociatedFilesystem,
		MBPP_FileInfoHeader const& Header,
		MBPP_FileInfoExtensionData const& ExtensionData,
		MBPM_FileInfoExcluder const& FileExcluder,
		MBPP_FileInfoReader const* ReaderToCompare
	)
	{
		MBPP_DirectoryInfoNode ReturnValue;
		assert(CurrentPacketPath.find('/') != CurrentPacketPath.npos);
		ReturnValue.DirectoryName = CurrentPacketPath.substr(CurrentPacketPath.find_last_of('/')+1);
		if (ReturnValue.DirectoryName == "")
		{
			ReturnValue.DirectoryName = "/";
		}
		ReturnValue.LastWriteTime = -1;//TODO implement correctly
		std::set<std::string> FilePaths;
		std::set<std::string> DirectoryPaths;
		//std::filesystem::directory_iterator DirectoryIterator(DirectoryPath);
		MBUtility::FilesystemError FSError = MBUtility::FilesystemError::Ok;
		std::vector<MBUtility::FSObjectInfo> Contents = AssociatedFilesystem.ListDirectory(TopDirectoryPath+CurrentPacketPath, &FSError);
		for (auto const& Entry : Contents)
		{
			std::string AbsolutePacketPath = CurrentPacketPath + "/" + Entry.Name;
			//TEMP
			if (AbsolutePacketPath.substr(0, 2) == "//")
			{
				AbsolutePacketPath = AbsolutePacketPath.substr(1);
			}
			if (FileExcluder.Excludes(AbsolutePacketPath) && !FileExcluder.Includes(AbsolutePacketPath))
			{
				continue;
			}
			if (Entry.Type == MBUtility::FileSystemType::File)
			{
				if (ReaderToCompare != nullptr)
				{
					if (ReaderToCompare->GetFileInfo(AbsolutePacketPath) != nullptr)
					{
						uint64_t LocalWriteTime = Entry.LastWriteTime;//p_GetPathWriteTime(Entry.path());
						uint64_t CompareWriteTime = ReaderToCompare->GetFileInfo(AbsolutePacketPath)->LastWriteTime;
						if (CompareWriteTime != 0 && CompareWriteTime >= LocalWriteTime)
						{
							continue;
						}
					}
				}
				FilePaths.insert(AbsolutePacketPath.substr(1));
			}
			else if (Entry.Type == MBUtility::FileSystemType::Directory)
			{
				DirectoryPaths.insert(AbsolutePacketPath.substr(1));
			}
		}
		std::string StructureHashData;
		std::vector<std::string> ContentHashData;
		for(auto const& Path : FilePaths)
		{
			MBPP_FileInfo NewInfo = p_CreateFileInfo(TopDirectoryPath+"/"+Path, AssociatedFilesystem, Header, ExtensionData);
			ReturnValue.Files.push_back(NewInfo);
			ContentHashData.push_back(ReturnValue.Files.back().FileHash);
			StructureHashData += ReturnValue.Files.back().FileName + ReturnValue.Files.back().FileHash;
			ReturnValue.Size += ReturnValue.Files.back().FileSize;
		}
		for (auto const& Path : DirectoryPaths)
		{
			ReturnValue.Directories.push_back(p_CreateDirectoryInfo("/"+Path, TopDirectoryPath, AssociatedFilesystem, Header, ExtensionData, FileExcluder, ReaderToCompare));
			ContentHashData.push_back(ReturnValue.Directories.back().ContentHash);
			StructureHashData += ReturnValue.Directories.back().DirectoryName + ReturnValue.Directories.back().StructureHash;
			ReturnValue.Size += ReturnValue.Directories.back().Size;
		}
		std::sort(ContentHashData.begin(), ContentHashData.end());
		std::string TotalContentHash;
		for (size_t i = 0; i < ContentHashData.size(); i++)
		{
			TotalContentHash += ContentHashData[i];
		}
		ReturnValue.ContentHash = MBCrypto::HashData(TotalContentHash, Header.HashFunction);
		ReturnValue.StructureHash = MBCrypto::HashData(StructureHashData, Header.HashFunction);
		return(ReturnValue);
	}

	MBPP_DirectoryInfoNode MBPP_FileInfoReader::p_CreateDirectoryInfo(
		std::filesystem::path const& TopDirectoryPath,
		std::filesystem::path const& DirectoryPath,
		MBPP_FileInfoHeader const& Header,
		MBPP_FileInfoExtensionData const& ExtensionData,
		MBPM_FileInfoExcluder const& FileExcluder,
		MBPP_FileInfoReader const* ReaderToCompare
	)
	{
		MBPP_DirectoryInfoNode ReturnValue;
		ReturnValue.DirectoryName = MBUnicode::PathToUTF8(DirectoryPath.filename());
		if (TopDirectoryPath == DirectoryPath)
		{
			ReturnValue.DirectoryName = "/";
		}
		ReturnValue.LastWriteTime = -1;//TODO implement correctly
		std::set<std::filesystem::path> FilePaths;
		std::set<std::filesystem::path> DirectoryPaths;
		std::filesystem::directory_iterator DirectoryIterator(DirectoryPath);
		for (auto const& Entry : DirectoryIterator)
		{
			std::filesystem::path RelativePath = std::filesystem::relative(Entry.path(), TopDirectoryPath);
			std::string AbsolutePacketPath = "/" + MBUtility::ReplaceAll(MBUnicode::PathToUTF8(RelativePath), "\\", "/");
			if (FileExcluder.Excludes(AbsolutePacketPath) && !FileExcluder.Includes(AbsolutePacketPath))
			{
				continue;
			}
			if (Entry.is_regular_file())
			{
				if (ReaderToCompare != nullptr)
				{
					if (ReaderToCompare->GetFileInfo(AbsolutePacketPath) != nullptr)
					{
						uint64_t LocalWriteTime = p_GetPathWriteTime(Entry.path());
						uint64_t CompareWriteTime = ReaderToCompare->GetFileInfo(AbsolutePacketPath)->LastWriteTime;
						if (CompareWriteTime != 0 && CompareWriteTime >= LocalWriteTime)
						{
							continue;
						}
					}
				}
				FilePaths.insert(Entry.path());
			}
			else if (Entry.is_directory())
			{
				DirectoryPaths.insert(Entry.path());
			}
		}
		std::string StructureHashData;
		std::vector<std::string> ContentHashData;
		for (auto const& Path : FilePaths)
		{
			ReturnValue.Files.push_back(p_CreateFileInfo(Path, Header, ExtensionData));
			ContentHashData.push_back(ReturnValue.Files.back().FileHash);
			StructureHashData += ReturnValue.Files.back().FileName + ReturnValue.Files.back().FileHash;
			ReturnValue.Size += ReturnValue.Files.back().FileSize;
		}
		for (auto const& Path : DirectoryPaths)
		{
			ReturnValue.Directories.push_back(p_CreateDirectoryInfo(TopDirectoryPath, Path, Header, ExtensionData, FileExcluder, ReaderToCompare));
			ContentHashData.push_back(ReturnValue.Directories.back().ContentHash);
			StructureHashData += ReturnValue.Directories.back().DirectoryName + ReturnValue.Directories.back().StructureHash;
			ReturnValue.Size += ReturnValue.Directories.back().Size;
		}
		std::sort(ContentHashData.begin(), ContentHashData.end());
		std::string TotalContentHash;
		for (size_t i = 0; i < ContentHashData.size(); i++)
		{
			TotalContentHash += ContentHashData[i];
		}
		ReturnValue.ContentHash = MBCrypto::HashData(TotalContentHash, Header.HashFunction);
		ReturnValue.StructureHash = MBCrypto::HashData(StructureHashData, Header.HashFunction);
		return(ReturnValue);
	}


	void MBPP_FileInfoReader::p_WriteFileInfo(MBPP_FileInfo const& InfoToWrite, MBUtility::MBOctetOutputStream* OutputStream, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData)
	{
		//h_WriteBigEndianInteger(OutputStream, InfoToWrite.FileName.size(), 2);
		//OutputStream->Write(InfoToWrite.FileName.data(), InfoToWrite.FileName.size());
		//OutputStream->Write(InfoToWrite.FileHash.data(), InfoToWrite.FileHash.size());
		//h_WriteBigEndianInteger(OutputStream, InfoToWrite.FileSize, 8);
		for (auto const& Property : ExtensionData.FileProperties)
		{
			if (Property.Type == MBPP_FSEntryPropertyType::ContentHash)
			{
				OutputStream->Write(InfoToWrite.FileHash.data(), InfoToWrite.FileHash.size());
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Name)
			{
				h_WriteBigEndianInteger(OutputStream, InfoToWrite.FileName.size(), 2);
				OutputStream->Write(InfoToWrite.FileName.data(), InfoToWrite.FileName.size());
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Size)
			{
				h_WriteBigEndianInteger(OutputStream, InfoToWrite.FileSize, 8);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::LastWrite)
			{
				h_WriteBigEndianInteger(OutputStream, InfoToWrite.LastWriteTime, 8);
			}
		}
	}
	void MBPP_FileInfoReader::p_WriteDirectoryInfo(MBPP_DirectoryInfoNode const& InfoToWrite, MBUtility::MBSearchableOutputStream* OutputStream, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData)
	{
		//Skippointer 8 bytes
		//StructureHash
		//ContentHash
		//Namn, 2 bytes l�ngd + l�ngd bytes namn data
		//Number of files 4 bytes, f�ljt av fil bytes
		//number of directories 4 bytes, f�ljt av directoriesen
		//skip poointer pekar till slutet av directory datan
		uint64_t DirectoryHeaderPosition = OutputStream->GetOutputPosition();
		//h_WriteBigEndianInteger(OutputStream, 0, 8);
		//OutputStream->Write(InfoToWrite.StructureHash.data(), InfoToWrite.StructureHash.size());
		//OutputStream->Write(InfoToWrite.ContentHash.data(), InfoToWrite.ContentHash.size());
		//h_WriteBigEndianInteger(OutputStream, InfoToWrite.Size, 8);
		//
		//h_WriteBigEndianInteger(OutputStream, InfoToWrite.DirectoryName.size(), 2);
		//OutputStream->Write(InfoToWrite.DirectoryName.data(), InfoToWrite.DirectoryName.size());
		uint64_t SkipPointerPosition = -1;
		for (auto const& Property : ExtensionData.DirectoryProperties)
		{
			if (Property.Type == MBPP_FSEntryPropertyType::ContentHash)
			{
				OutputStream->Write(InfoToWrite.ContentHash.data(), InfoToWrite.ContentHash.size());
			}
			if (Property.Type == MBPP_FSEntryPropertyType::StructureHash)
			{
				OutputStream->Write(InfoToWrite.StructureHash.data(), InfoToWrite.StructureHash.size());
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Name)
			{
				h_WriteBigEndianInteger(OutputStream, InfoToWrite.DirectoryName.size(), 2);
				OutputStream->Write(InfoToWrite.DirectoryName.data(), InfoToWrite.DirectoryName.size());
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::Size)
			{
				h_WriteBigEndianInteger(OutputStream, InfoToWrite.Size, 8);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::LastWrite)
			{
				h_WriteBigEndianInteger(OutputStream, InfoToWrite.LastWriteTime, 8);
			}
			else if (Property.Type == MBPP_FSEntryPropertyType::SkipPointer)
			{
				SkipPointerPosition = OutputStream->GetOutputPosition();
				h_WriteBigEndianInteger(OutputStream, 0, 8);
			}
		}
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.Files.size(), 4);
		for (size_t i = 0; i < InfoToWrite.Files.size(); i++)
		{
			p_WriteFileInfo(InfoToWrite.Files[i], OutputStream,Header,ExtensionData);
		}
		h_WriteBigEndianInteger(OutputStream, InfoToWrite.Directories.size(), 4);
		for (size_t i = 0; i < InfoToWrite.Directories.size(); i++)
		{
			p_WriteDirectoryInfo(InfoToWrite.Directories[i],OutputStream,Header,ExtensionData);
		}
		if (SkipPointerPosition != -1)
		{
			uint64_t EndSkip = OutputStream->GetOutputPosition();
			OutputStream->SetOutputPosition(DirectoryHeaderPosition);
			h_WriteBigEndianInteger(OutputStream, EndSkip, 8);
			OutputStream->SetOutputPosition(EndSkip);
		}
	}
	void MBPP_FileInfoReader::WriteData(MBUtility::MBSearchableOutputStream* OutputStream)
	{
		MBPP_FileInfoHeader HeaderToWrite = MBPP_FileInfoHeader();

		std::string TotalExtensionData = p_SerializeExtensionData(m_ExtensionData);
		HeaderToWrite.ExtensionDataSize = TotalExtensionData.size();
		p_WriteHeader(HeaderToWrite, OutputStream);
		OutputStream->Write(TotalExtensionData.data(), TotalExtensionData.size());
		p_WriteDirectoryInfo(m_TopNode, OutputStream,m_InfoHeader,m_ExtensionData);
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
				//inneb�r egentligen bara att vi har mer lokala filer �n filer vi uppdaterar, inget problem vi bara forts�tter
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
			NewFileList.push_back(*OtherNodeFiles);
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
				//inneb�r egentligen bara att vi har mer lokala filer �n filer vi uppdaterar, inget problem vi bara forts�tter
				NewDirectoryList.push_back(std::move(*ThisNodeDirectories));
				ThisNodeDirectories++;
			}
			else if (ThisNodeDirectories->DirectoryName > OtherNodeDirectories->DirectoryName)
			{
				//om det �r en ny directory l�gger vi helt enkelt till den
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
		//		//vi ska allts� l�gga till filen
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
			return;
		}
		int FilePosition = MBAlgorithms::BinarySearch(TargetDirectory->Files, TargetComponents.back(), h_GetFileName, std::less<std::string>());
		if (FilePosition != -1)
		{
			TargetDirectory->Files.erase(TargetDirectory->Files.begin() + FilePosition);
			return;
		}
	}
	MBPP_DirectoryHashData MBPP_FileInfoReader::p_UpdateDirectoryStructureHash(MBPP_DirectoryInfoNode& NodeToUpdate)
	{
		//antar att directoryn �r sorterad
		std::vector<std::string> ContentHashes = {};
		std::string StructureHashData = "";
		uint64_t TotalSize = 0;
		for(size_t i = 0; i < NodeToUpdate.Files.size();i++)
		{
			TotalSize += NodeToUpdate.Files[i].FileSize;
			StructureHashData += NodeToUpdate.Files[i].FileName+ NodeToUpdate.Files[i].FileHash;
			ContentHashes.push_back(NodeToUpdate.Files[i].FileHash);
		}
		for (size_t i = 0; i < NodeToUpdate.Directories.size(); i++)
		{
			MBPP_DirectoryHashData DirectoryHash = p_UpdateDirectoryStructureHash(NodeToUpdate.Directories[i]);
			TotalSize += NodeToUpdate.Directories[i].Size;
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
		NodeToUpdate.Size = TotalSize;
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::p_UpdateHashes()
	{
		p_UpdateDirectoryStructureHash(m_TopNode);
	}
	bool MBPP_FileInfoReader::p_ObjectIsFile(std::string const& ObjectToCheck)
	{
		std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToCheck);
		MBPP_DirectoryInfoNode* AssociatedDirectory = p_GetTargetDirectory(PathComponents);
		int TargetIndex = MBAlgorithms::BinarySearch(AssociatedDirectory->Directories, PathComponents.back(), h_GetDirectoryName, std::less<std::string>());
		if (TargetIndex != -1)
		{
			return(false);
		}
		int FilePosition = MBAlgorithms::BinarySearch(AssociatedDirectory->Files, PathComponents.back(), h_GetFileName, std::less<std::string>());
		if (TargetIndex != -1)
		{
			return(true);
		}
		//existerar inte, borde throwa exception
		return(false);
	}
	MBPP_FileInfo* MBPP_FileInfoReader::p_GetObjectFileData(std::string const& ObjectToCheck)
	{
		MBPP_FileInfo* ReturnValue = nullptr;
		std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToCheck);
		MBPP_DirectoryInfoNode* AssociatedDirectory = p_GetTargetDirectory(PathComponents);
		int FilePosition = MBAlgorithms::BinarySearch(AssociatedDirectory->Files, PathComponents.back(), h_GetFileName, std::less<std::string>());
		if (FilePosition != -1)
		{
			ReturnValue = &AssociatedDirectory->Files[FilePosition];
		}
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::UpdateInfo(MBPP_DirectoryInfoNode const& NewInfo)
	{
		p_UpdateDirectoryInfo(m_TopNode, NewInfo);
		p_UpdateHashes();
		p_UpdateDirectoriesParents(&m_TopNode);
	}
	void MBPP_FileInfoReader::UpdateInfo(MBPP_DirectoryInfoNode const& NewInfo, std::string const& MountPoint)
	{
		if (!ObjectExists(MountPoint))
		{
			std::vector<std::string> PathComponents = sp_GetPathComponents(MountPoint);
			std::string NewDirectoryName = PathComponents.back();
			MBPP_DirectoryInfoNode* TargetDirectory = p_GetTargetDirectory(PathComponents);
			size_t FirstLesserDirectoryPosition = 0;
			for (size_t i = 0; i < TargetDirectory->Directories.size(); i++)
			{
				if (TargetDirectory->Directories[i].DirectoryName < NewDirectoryName)
				{
					FirstLesserDirectoryPosition += 1;
				}
				else
				{
					break;
				}
			}
			TargetDirectory->Directories.insert(TargetDirectory->Directories.begin() + FirstLesserDirectoryPosition, NewInfo);
			TargetDirectory->Directories[FirstLesserDirectoryPosition] = NewInfo;
			TargetDirectory->Directories[FirstLesserDirectoryPosition].DirectoryName = NewDirectoryName;
		}
		else if (p_ObjectIsFile(MountPoint) == false)
		{
			//om vi mountar �r det helt enkelt att bara l�gga p� den
			MBPP_DirectoryInfoNode* Data = p_GetObjectDirectoryData(MountPoint);
			std::string DirectoryName = Data->DirectoryName;
			*Data = NewInfo;
			Data->DirectoryName = DirectoryName;
		}
		else
		{
			//g�r inget, throwa exception?
		}
		p_UpdateDirectoriesParents(&m_TopNode);
		p_UpdateHashes();
	}
	void MBPP_FileInfoReader::UpdateInfo(MBPP_FileInfoReader const& NewInfo)
	{
		//hej
		UpdateInfo(NewInfo.m_TopNode);
	}
	MBPP_DirectoryInfoNode* MBPP_FileInfoReader::p_GetObjectDirectoryData(std::string const& ObjectToCheck)
	{
		MBPP_DirectoryInfoNode* ReturnValue = nullptr;
		std::vector<std::string> PathComponents = sp_GetPathComponents(ObjectToCheck);
		MBPP_DirectoryInfoNode* AssociatedDirectory = p_GetTargetDirectory(PathComponents);
		int TargetIndex = MBAlgorithms::BinarySearch(AssociatedDirectory->Directories, PathComponents.back(), h_GetDirectoryName, std::less<std::string>());
		if (TargetIndex != -1)
		{
			ReturnValue = &AssociatedDirectory->Directories[TargetIndex];
		}
		return(ReturnValue);
	}
	void MBPP_FileInfoReader::UpdateInfo(MBPP_FileInfoReader const& NewInfo, std::string const& MountPoint)
	{
		if (!ObjectExists(MountPoint))
		{
			std::vector<std::string> PathComponents = sp_GetPathComponents(MountPoint);
			std::string NewDirectoryName = PathComponents.back();
			MBPP_DirectoryInfoNode* TargetDirectory = p_GetTargetDirectory(PathComponents);
			size_t FirstLesserDirectoryPosition = 0;
			for (size_t i = 0; i < TargetDirectory->Directories.size(); i++)
			{
				if (TargetDirectory->Directories[i].DirectoryName < NewDirectoryName)
				{
					FirstLesserDirectoryPosition += 1;
				}
				else
				{
					break;
				}
			}
			TargetDirectory->Directories.insert(TargetDirectory->Directories.begin() + FirstLesserDirectoryPosition, NewInfo.m_TopNode);
			TargetDirectory->Directories[FirstLesserDirectoryPosition] = NewInfo.m_TopNode;
			TargetDirectory->Directories[FirstLesserDirectoryPosition].DirectoryName = NewDirectoryName;
		}
		else if(p_ObjectIsFile(MountPoint) == false)
		{
			//om vi mountar �r det helt enkelt att bara l�gga p� den
			MBPP_DirectoryInfoNode* Data = p_GetObjectDirectoryData(MountPoint);
			std::string DirectoryName = Data->DirectoryName;
			*Data = NewInfo.m_TopNode;
			Data->DirectoryName = DirectoryName;
		}
		else
		{
			//g�r inget, throwa exception?
		}
		p_UpdateDirectoriesParents(&m_TopNode);
		p_UpdateHashes();
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
				//ska l�gga till en ny directory p� stacken, eller g� ner f�r den
				m_IterationInfo.top().CurrentDirectoryOffset += 1;
				if (m_IterationInfo.top().CurrentDirectoryOffset < m_IterationInfo.top().AssociatedDirectory->Directories.size())
				{
					//h�nder enbart d� vi ska l�gga till en ny
					DirectoryIterationInfo NewInfo;
					NewInfo.CurrentDirectoryOffset = -1;
					NewInfo.CurrentFileOffset = -1;
					NewInfo.AssociatedDirectory = &(m_IterationInfo.top().AssociatedDirectory->Directories[m_IterationInfo.top().CurrentDirectoryOffset]);
					m_IterationInfo.push(NewInfo);
					m_CurrentDirectoryPath += NewInfo.AssociatedDirectory->DirectoryName + "/";
				}
				else
				{
					//vi har iterat �ver alla directories och ska d� poppa
					if (m_IterationInfo.size() > 1)
					{
						m_CurrentDirectoryPath.resize(m_CurrentDirectoryPath.size() - (m_IterationInfo.top().AssociatedDirectory->DirectoryName.size() + 1));
					}
					m_IterationInfo.pop();
				}
			}
			else
			{
				//den nuvarande offseten �r giltig, vi breaker
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
		m_IterationInfo.push(NewInfo); //blir relativt till den h�r nodens grejer
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
	const MBPP_FileInfo* MBPP_DirectoryInfoNode_ConstIterator::operator->()
	{
		return(&(**this));
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
	
	//MBError MBPP_FileListDownloadHandler::Open(std::string const& FileToDownloadName)
	//{
	//	assert(false);
	//	return(false);
	//}
	//MBError MBPP_FileListDownloadHandler::InsertData(const void* Data, size_t DataSize)
	//{
	//	assert(false);
	//	return(false);
	//}
	//MBError MBPP_FileListDownloadHandler::Close()
	//{
	//	assert(false);
	//	return(false);
	//}
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
	MBError	MBPP_ClientHTTPConverter::Connect(MBPP_PacketHost const& HostData)
	{
		MBError ReturnValue = true;
		std::string PortToUse = "";
		std::string Adress = HostData.URL.substr(0, std::min(HostData.URL.find('/'), HostData.URL.size()));
		m_MBPP_ResourceLocation = HostData.URL.substr(std::min(HostData.URL.find('/'), HostData.URL.size()));
		m_RemoteHost = Adress;
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
		MBSockets::TCPClient* Client = new MBSockets::TCPClient(Adress, PortToUse);
		Client->Connect();
		if (Client->IsConnected())
		{
			m_InternalHTTPSocket = std::unique_ptr<MBSockets::TLSConnectSocket>(new MBSockets::TLSConnectSocket(
				std::unique_ptr<MBSockets::TCPClient>(Client)));
		}
		return(ReturnValue);
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
	bool MBPP_ClientHTTPConverter::IsConnected()
	{
		return(m_InternalHTTPSocket->IsConnected());
	}
	bool MBPP_ClientHTTPConverter::IsValid()
	{
		return(m_InternalHTTPSocket->IsValid());
	}
	void MBPP_ClientHTTPConverter::Close()
	{
		if (m_InternalHTTPSocket != nullptr)
		{
			m_InternalHTTPSocket->Close();
		}
	}
	MBError MBPP_ClientHTTPConverter::EstablishTLSConnection()
	{
		return(m_InternalHTTPSocket->EstablishClientTLSConnection(m_RemoteHost));
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
	MBError MBPP_ClientHTTPConverter::SendData(const void* DataToSend, size_t DataSize)
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
	MBError MBPP_ClientHTTPConverter::SendData(std::string const& StringToSend)
	{
		return(MBPP_ClientHTTPConverter::SendData(StringToSend.data(), StringToSend.size()));
	}
	//END MBPP_ClientHTTPConverter

	//BEGIN MBPP_Client
	void MBPP_Client::SetLogTerminal(MBCLI::MBTerminal* NewLogTerminal)
	{
		m_LogTerminal = NewLogTerminal;
	}
	MBError MBPP_Client::Connect(MBPP_TransferProtocol TransferProtocol, std::string const& Domain, std::string const& Port)
	{
		return(Connect({ Domain,TransferProtocol,(uint16_t) std::stoi(Port) }));
	}
	MBError MBPP_Client::Connect(MBPP_PacketHost const& PacketHost)
	{
		MBError ReturnValue = true;
		if (PacketHost.TransferProtocol == MBPP_TransferProtocol::TCP || PacketHost.TransferProtocol == MBPP_TransferProtocol::Null)
		{
			m_ServerConnection = std::unique_ptr<MBSockets::ConnectSocket>(new MBSockets::TCPClient(PacketHost.URL, std::to_string(PacketHost.Port)));
			MBSockets::TCPClient* ClientSocket = (MBSockets::TCPClient*)m_ServerConnection.get();
			ClientSocket->Connect();
		}
		else if(PacketHost.TransferProtocol == MBPP_TransferProtocol::HTTP || PacketHost.TransferProtocol == MBPP_TransferProtocol::HTTPS)
		{
			m_ServerConnection = std::unique_ptr<MBSockets::ConnectSocket>(new MBPP_ClientHTTPConverter());
			MBPP_ClientHTTPConverter* ClientSocket = (MBPP_ClientHTTPConverter*)m_ServerConnection.get();
			ClientSocket->Connect(PacketHost);
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
	MBError MBPP_Client::p_GetFiles(std::string const& PacketName, std::vector<std::string> const& FilesToGet, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		std::string PacketNameData = MBPP_EncodeString(PacketName);
		RecordToSend.ComputerInfo = p_GetComputerInfo();
		RecordToSend.Type = MBPP_RecordType::GetFiles;
		RecordToSend.RecordData = MBPP_EncodeString(PacketName) + MBPP_GenerateFileList(FilesToGet);
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
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), DownloadHandler);
			//ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), OutputDirectory, FilesToGet);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_GetDirectory(std::string const& PacketName, std::string const& DirectoryToGet, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetDirectory;
		RecordToSend.ComputerInfo = p_GetComputerInfo();
		RecordToSend.RecordData = MBPP_EncodeString(PacketName) + MBPP_GenerateFileList({ DirectoryToGet });
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		std::string RecievedData = m_ServerConnection->RecieveData();
		while (m_ServerConnection->IsConnected() && RecievedData.size() < MBPP_GenericRecordHeaderSize)
		{
			RecievedData += m_ServerConnection->RecieveData();
		}
		if (RecievedData.size() < MBPP_GenericRecordHeaderSize + 2)
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Server didn't send enough data for a MBPPGenericHeader";
		}
		else
		{
			ReturnValue = p_DownloadFileList(RecievedData, MBPP_GenericRecordHeaderSize, m_ServerConnection.get(), DownloadHandler);
		}
		return(ReturnValue);
	}
	MBError MBPP_Client::p_GetFiles(std::string const& PacketName,std::vector<std::string> const& FilesToGet, std::string const& OutputDirectory)
	{
		MBPP_FileListDownloader DownloaderToUse(OutputDirectory);
		return(p_GetFiles(PacketName, FilesToGet, &DownloaderToUse));
	}
	MBError MBPP_Client::p_GetDirectory(std::string const& Packet,std::string const& DirectoryToGet, std::string const& OutputDirectory) 
	{
		MBPP_FileListDownloader DownloaderToUse(OutputDirectory);
		return(p_GetDirectory(Packet, DirectoryToGet, &DownloaderToUse));
	}
	MBError MBPP_Client::p_DownloadServerFilesInfo(std::string const& PacketName, std::string const& OutputDirectory,std::vector<std::string> const& OutputFileNames)
	{
		MBError ReturnValue = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetPacketInfo;
		RecordToSend.ComputerInfo = p_GetComputerInfo();
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
	void MBPP_Client::SetComputerInfo(MBPP_ComputerInfo NewComputerInfo)
	{
		m_CurrentComputerInfo = NewComputerInfo;
	}
	MBPP_ComputerInfo MBPP_Client::GetSystemComputerInfo()
	{
		MBPP_ComputerInfo ReturnValue;
		if (MBUtility::IsWindows())
		{
			ReturnValue.OSType = MBPP_OSType::Windows;
		}
		else
		{
			ReturnValue.OSType = MBPP_OSType::Linux;
		}
#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64)
		ReturnValue.ProcessorType = MBPP_ProcessorType::x86_64;
#elif defined(_M_IX86) || defined(__i386__)
		ReturnValue.ProcessorType = MBPP_ProcessorType::x86;
#elif defined(__ARM_ARCH_7__) || defined(_M_ARM)
		ReturnValue.ProcessorType = MBPP_ProcessorType::ARMv7;
#endif //
		return(ReturnValue);
	}
	MBPP_ComputerInfo MBPP_Client::p_GetComputerInfo()
	{
		return(m_CurrentComputerInfo);
	}
	MBPP_UploadRequest_Response MBPP_Client::p_GetLoginResult(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, MBError* OutError)
	{
		MBPP_UploadRequest_Response ReturnValue;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::UploadRequest;
		RecordToSend.ComputerInfo = p_GetComputerInfo();

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
		//reciever all data vi kan, vet att det h�r kommer f� plats i ram
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
		MBPP_FileListDownloader DownloaderToUse = MBPP_FileListDownloader(OutputDirectory);
		return(p_DownloadFileList(InitialData, InitialDataOffset, SocketToUse, &DownloaderToUse));
	}
	 MBError MBPP_Client::p_DownloadFileList(std::string const& InitialData, size_t DataOffset, MBSockets::ConnectSocket* SocketToUse, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		size_t MaxRecieveSize = 300000000;

		std::string ResponseData = InitialData; //TODO borde kanske mova intial datan ist�llet

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
		MBCLI::MBTerminalLoadingbar Loadingbar;
		if (m_LogTerminal != nullptr && m_LoggingEnabled)
		{
			m_LogTerminal->PrintLine("Files to download:");
			for (size_t i = 0; i < FilesToDownload.size(); i++)
			{
				m_LogTerminal->PrintLine(FilesToDownload[i]);
			}
		}
		while (CurrentFileIndex < FilesToDownload.size())
		{
			if (NewFile)
			{
				//vi m�ste f� info om den fil vi ska skriva till nu
				while (ResponseData.size() - ResponseDataOffset < 8)
				{
					ResponseData += SocketToUse->RecieveData(MaxRecieveSize);
				}
				CurrentFileSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 8, ResponseDataOffset, &ResponseDataOffset);
				CurrentFileParsedBytes = 0;
				//OBS!!! egentligen en security risk om vi inte kollar att filen �r inom directoryn
				DownloadHandler->Open(FilesToDownload[CurrentFileIndex]);
				NewFile = false;
				if (m_LogTerminal != nullptr && m_LoggingEnabled)
				{
					Loadingbar = m_LogTerminal->CreateLineLoadingBar(FilesToDownload[CurrentFileIndex] + " (" + MBCLI::MBTerminal::GetByteSizeString(CurrentFileSize) + ")", 0);
				}
			}
			else
			{
				//all ny data vi f�r nu ska tillh�ra filen
				//antar ocks� att response data �r tom h�r
				if (ResponseData.size() == ResponseDataOffset)
				{
					ResponseData = SocketToUse->RecieveData(MaxRecieveSize);
					ResponseDataOffset = 0;
				}
				uint64_t FileBytes = std::min((uint64_t)ResponseData.size() - ResponseDataOffset, CurrentFileSize - CurrentFileParsedBytes);
				DownloadHandler->InsertData(ResponseData.data() + ResponseDataOffset, FileBytes);
				CurrentFileParsedBytes += FileBytes;
				ResponseDataOffset += FileBytes;
				if (m_LogTerminal != nullptr && m_LoggingEnabled)
				{
					Loadingbar.UpdateProgress(double(CurrentFileParsedBytes) / double(CurrentFileSize));
				}
				if (CurrentFileParsedBytes == CurrentFileSize)
				{
					ReturnValue = DownloadHandler->Close();
					NewFile = true;
					if (m_LogTerminal != nullptr && m_LoggingEnabled)
					{
						Loadingbar.Finalize();
					}
					CurrentFileIndex += 1;
				}
			}
		}
		//DownloadHandler->Close();
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
		//kollar om att ladda ner packet infon �r tillr�ckligt litet och skapar en diff fil, eller g�r det genom att manuellt be om directory info f�r varje fil och se om hashen skiljer
		MBError UpdateError = true;
		MBPP_FileInfoReader ServerFiles = GetPacketFileInfo(PacketName, &UpdateError);
		if(!UpdateError)
		{
			return(UpdateError);
		}
		//ANTAGANDE vi antar h�r att PacketFilesData �r uppdaterad
		//if (!std::filesystem::exists(OutputDirectory + "/MBPM_FileInfo"))
		//{
			MBPM::CreatePacketFilesData(OutputDirectory);
		//}
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
		//deletar sedan filerna som �r �ver
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
						//std::cout << e.what() << std::endl;
					}
				}
			}
		}
		MBPM::CreatePacketFilesData(OutputDirectory);
		return(UpdateError);
	}
	MBError MBPP_Client::p_UploadPacket(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, std::string const& PacketDirectory, std::vector<std::string> const& FilesToUpload,std::vector<std::string> const& FilesToDelete)
	{
		MBError UploadError = true;
		MBPP_GenericRecord RecordToSend;
		RecordToSend.ComputerInfo = p_GetComputerInfo();
		RecordToSend.Type = MBPP_RecordType::UploadChanges;
		RecordToSend.VerificationData = MBPP_GetUploadCredentialsData(PacketName, CredentialsType, CredentialsData);
		RecordToSend.VerificationDataSize = RecordToSend.VerificationData.size();
		
		RecordToSend.RecordSize = 0;
		RecordToSend.RecordData += MBPP_EncodeString(PacketName);
		RecordToSend.RecordData += MBPP_GenerateFileList(FilesToDelete);
		RecordToSend.RecordData += MBPP_GenerateFileList(FilesToUpload);
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		uint64_t TotalUploadSize = 0;
		if (m_LogTerminal != nullptr)
		{
			m_LogTerminal->PrintLine("Files To Send:");
		}
		for (size_t i = 0; i < FilesToUpload.size(); i++)
		{
			uint64_t CurrentFileSize = MBGetFileSize(PacketDirectory + FilesToUpload[i]);
			RecordToSend.RecordSize += 8 + CurrentFileSize;
			if (m_LogTerminal != nullptr)
			{
				m_LogTerminal->PrintLine(PacketDirectory+FilesToUpload[i]);
			}
		}
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));

		for (size_t i = 0; i < FilesToUpload.size(); i++)
		{
			std::string DataToSend = std::string(8, 0);
			uint64_t FileSize = MBGetFileSize(PacketDirectory + FilesToUpload[i]);
		
			MBCLI::MBTerminalLoadingbar LoadingBar;
			if (m_LogTerminal != nullptr)
			{
				LoadingBar = m_LogTerminal->CreateLineLoadingBar(FilesToUpload[i] + " ("+MBCLI::MBTerminal::GetByteSizeString(FileSize)+")",0);
			}
			h_WriteBigEndianInteger(DataToSend.data(), FileSize, 8);
			std::ifstream FileInputStream = std::ifstream(PacketDirectory + FilesToUpload[i], std::ios::in | std::ios::binary);
			m_ServerConnection->SendData(DataToSend);
			const size_t BlockSize = 4096 * 4;
			std::string Buffer = std::string(BlockSize, 0);
			uint64_t CurrentSentBytes = 0;
			while (true)
			{
				FileInputStream.read(Buffer.data(), BlockSize);
				size_t BytesRead = FileInputStream.gcount();
				CurrentSentBytes += BytesRead;
				if (m_LogTerminal != nullptr)
				{
					LoadingBar.UpdateProgress(double(CurrentSentBytes) / double(FileSize));
				}
				m_ServerConnection->SendData(Buffer.data(), BytesRead);
				if (BytesRead < BlockSize)
				{
					if (m_LogTerminal != nullptr)
					{
						LoadingBar.Finalize();
					}
					break;
				}
			}
		}

		std::string ResponseData = m_ServerConnection->RecieveData();
		while (ResponseData.size() < (MBPP_GenericRecordHeaderSize + 2) && m_ServerConnection->IsConnected() && m_ServerConnection->IsValid())
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
					MBPP_GetErrorCodeString(MBPP_ErrorCode(MBParsing::ParseBigEndianInteger(ResponseData.data(), 2, MBPP_GenericRecordHeaderSize, nullptr)));
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
				UploadError.ErrorMessage = "Error result in UploadChanges: " + MBPP_GetErrorCodeString(Result);
			}
		}
		return(UploadError);
		
	}
	MBError MBPP_Client::UploadPacket(std::string const& PacketDirectory, std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, std::vector<std::string> const& FilesToUpload, std::vector<std::string> const& FilesToDelete, MBPP_UploadRequest_Response* OutResponse)
	{
		MBError UploadError = true;
		MBPP_UploadRequest_Response UploadRequestResponse = p_GetLoginResult(PacketName, CredentialsType, CredentialsData, &UploadError);
		*OutResponse = UploadRequestResponse;
		if (UploadRequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
		{
			//update out struct
			UploadError = false;
			return(UploadError);
		}

		UploadError = p_UploadPacket(PacketName, CredentialsType, CredentialsData, PacketDirectory, FilesToUpload, FilesToDelete);
		return(UploadError);
	}
	MBError MBPP_Client::UploadPacket(std::string const& PacketDirectory, std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, 
		MBPP_UploadRequest_Response* OutResponse)
	{
		//kollar om att ladda ner packet infon �r tillr�ckligt litet och skapar en diff fil, eller g�r det genom att manuellt be om directory info f�r varje fil och se om hashen skiljer
		MBError UploadError = true;
		MBPP_UploadRequest_Response UploadRequestResponse = p_GetLoginResult(PacketName, CredentialsType, CredentialsData,&UploadError);
		*OutResponse = UploadRequestResponse;
		if (UploadRequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
		{
			//update out struct
			UploadError = false;
			return(UploadError);
		}
		//TODO finns det n�got smidigt s�tt att undvika mycket redundant server nedladdande?
		MBPP_FileInfoReader  ServerFileInfo = GetPacketFileInfo(PacketName, &UploadError);
		

		if (!UploadError)
		{
			return(UploadError);
		}
		//ANTAGANDE vi antar h�r att PacketFilesData �r uppdaterad
		//byter plats p� client och server s� att client tolkas som den uppdaterade q
		
		MBPP_FileInfoReader LocalFileInfo = MBPP_FileInfoReader(PacketDirectory + "/MBPM_FileInfo");
		MBPP_FileInfoDiff FileInfoDifferance = GetFileInfoDifference(ServerFileInfo, LocalFileInfo);
		//DEBUG 
		//std::cout << "Local files: " << std::endl;
		//MBPP_DirectoryInfoNode_ConstIterator Iterator = LocalFileInfo.GetDirectoryInfo("/")->begin();
		//while (!(Iterator == LocalFileInfo.GetDirectoryInfo("/")->end()))
		//{
		//	std::cout << Iterator.GetCurrentDirectory() + (*Iterator).FileName << std::endl;
		//	Iterator++;
		//}

		std::vector<std::string> FilesToSend = {};
		std::vector<std::string> ObjectsToDelete = {};
		for (auto const& UpdatedFile : FileInfoDifferance.UpdatedFiles)
		{
			FilesToSend.push_back(UpdatedFile);
		}
		for (auto const& NewFiles : FileInfoDifferance.AddedFiles)
		{
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
				FilesToSend.push_back(PacketPath);
				DirectoryIterator++;
			}

		}
		for (auto const& DeletedFiles : FileInfoDifferance.RemovedFiles)
		{
			ObjectsToDelete.push_back(DeletedFiles);
		}
		for (auto const& DeletedDirectoris : FileInfoDifferance.DeletedDirectories)
		{
			ObjectsToDelete.push_back(DeletedDirectoris);
		}
		UploadError = p_UploadPacket(PacketName, CredentialsType, CredentialsData, PacketDirectory, FilesToSend, ObjectsToDelete);
		return(UploadError);
	}
	MBError MBPP_Client::DownloadPacketFiles(std::string const& OutputDirectory, std::string const& PacketName, std::vector<std::string> const& FilesToGet)
	{
		MBError ReturnValue = p_GetFiles(PacketName, FilesToGet, OutputDirectory);
		return(ReturnValue);
	}
	MBError MBPP_Client::DownloadPacketDirectories(std::string const& OutputDirectory, std::string const& PacketName, std::vector<std::string> const& DirectoriesToGet)
	{
		//TODO kanske bara kan ers�ttas med en del av protocollet som �r
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
	MBError MBPP_Client::DownloadPacketFiles(std::string const& PacketName,std::vector<std::string> const& FilesToDownload, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		ReturnValue = p_GetFiles(PacketName,FilesToDownload,DownloadHandler);
		return(ReturnValue);
	}
	MBError MBPP_Client::DownloadPacketDirectories(std::string const& PacketName, std::vector<std::string> const& DirectoriesToDownload, MBPP_FileListDownloadHandler* DownloadHandler)
	{
		MBError ReturnValue = true;
		for (size_t i = 0; i < DirectoriesToDownload.size(); i++)
		{
			ReturnValue = p_GetDirectory(PacketName, DirectoriesToDownload[i], DownloadHandler);
		}
		return(ReturnValue);
	}
	MBPP_FileInfoReader MBPP_Client::GetPacketFileInfo(std::string const& PacketName, MBError* OutError)
	{
		bool PreviousEnableLoggin = m_LoggingEnabled;
		MBPP_FileListMemoryMapper MemoryMapper = MBPP_FileListMemoryMapper();
		MBPP_FileInfoReader ReturnValue;
		MBError GetPacketError = true;

		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetPacketInfo;
		RecordToSend.ComputerInfo = p_GetComputerInfo();
		RecordToSend.RecordData = MBPP_EncodeString(PacketName);
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));
		if (!m_ServerConnection->IsConnected())
		{
			*OutError = false;
			OutError->ErrorMessage = "Not connected to client";
			return(ReturnValue);
		}
		DisableLogging();
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
		if (PreviousEnableLoggin)
		{
			EnableLoggin();
		}
		return(ReturnValue);
	}
	//END MBPP_Client

	//BEGIN MBPP_Server
	MBPP_Server::MBPP_Server(std::string const& PacketDirectory,MBPP_UploadChangesIncorporator* UploadChangesHandler)
	{
		m_PacketChangesIncorporator = UploadChangesHandler;
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
	//top level, garanterat att det �r klart efter, inte garanterat att funka om inte l�g level under �r klara
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

	//om stora saker skickas kan inte allt sparas i minnet, s� detta api s� insertas dem rakt in i klassen, 
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
	MBError MBPP_Server::p_Handle_GetFiles() //manuell parsing av datan som inte v�ntar in att allt kommit
	{
		MBError ReturnValue = true;
		if (m_RequestResponseData == nullptr)
		{
			//f�rsta g�ngen funktionen callas, vi m�ste se till att 
			m_RequestResponseData = new MBPP_GetFileList_ResponseData();
			MBPP_GetFileList_ResponseData& CurrentData = p_GetResponseData<MBPP_GetFileList_ResponseData>();
			CurrentData.ComputerInfo = m_CurrentRequestHeader.ComputerInfo;
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
			//f�rsta g�ngen funktionen callas, vi m�ste se till att 
			m_RequestResponseData = new MBPP_GetDirectories_ResponseData();
			MBPP_GetDirectories_ResponseData& CurrentResponseData = p_GetResponseData<MBPP_GetDirectories_ResponseData>();
			CurrentResponseData.ComputerInfo = m_CurrentRequestHeader.ComputerInfo;
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
			//f�rsta g�ngen funktionen callas, vi m�ste se till att 
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
				CurrentResponseData.ResponseComputerInfo = m_CurrentRequestHeader.ComputerInfo;
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
	void MBPP_Server::p_CreateEmptyComputerDiffDirectory(std::string const& Packetname, MBPP_ComputerInfo NewComputerInfo)
	{
		std::string PacketDirectory = p_GetPacketDirectory(Packetname);
		if (!std::filesystem::exists(PacketDirectory + "/.mbpm/ComputerDiff/") || !std::filesystem::exists(PacketDirectory + "/.mbpm/ComputerDiff/MBPM_ComputerDiffInfo.json"))
		{
			p_CreateEmptyComputerDiffDirectory(Packetname);
		}
		std::string ComputerDiffDirectory = PacketDirectory + "/.mbpm/ComputerDiff/";
		std::string OSString = MBPP_ComputerDiffInfo::GetOSTypeString(NewComputerInfo.OSType);
		std::string ProcessorString = MBPP_ComputerDiffInfo::GetProcessorTypeString(NewComputerInfo.ProcessorType);
		std::string NewDirectoryName = OSString +"_"+ ProcessorString;
		std::filesystem::create_directories(ComputerDiffDirectory + "/" + NewDirectoryName);
		//uppdaterar JSON filen
		MBParsing::JSONObject NewRule = MBParsing::JSONObject(std::map<std::string, MBParsing::JSONObject>({}));
		MBParsing::JSONObject NewRuleInfoMatch = std::vector<MBParsing::JSONObject>({ std::vector<MBParsing::JSONObject>(1,MBParsing::JSONObject(std::move(OSString))),
			std::vector<MBParsing::JSONObject>(1,MBParsing::JSONObject(std::move(ProcessorString))) });
		NewRule["InfoMatch"] = std::move(NewRuleInfoMatch);
		NewRule["DataDirectory"] = MBParsing::JSONObject(std::move(NewDirectoryName));
		//MBParsing::JSONObject ObjectToWrite = std::map<std::string, MBParsing::JSONObject>({ { "Rules",MBParsing::JSONObject(std::vector<MBParsing::JSONObject>({std::move(NewRule)})) } });
		MBError ParseError = true;

		std::string JsonData = std::string(std::filesystem::file_size(ComputerDiffDirectory + "/MBPM_ComputerDiffInfo.json"), 0);
		std::ifstream JsonFile = std::ifstream(ComputerDiffDirectory + "/MBPM_ComputerDiffInfo.json", std::ios::in | std::ios::binary);
		JsonFile.read(JsonData.data(), std::filesystem::file_size(ComputerDiffDirectory + "/MBPM_ComputerDiffInfo.json"));
		JsonFile.close();
		MBParsing::JSONObject CurrentJSONObject = MBParsing::ParseJSONObject(JsonData,0,nullptr,&ParseError);
		if (ParseError && CurrentJSONObject.GetType() == MBParsing::JSONObjectType::Aggregate && CurrentJSONObject.HasAttribute("Rules"))
		{
			if (CurrentJSONObject.GetAttribute("Rules").GetType() == MBParsing::JSONObjectType::Array)
			{
				std::vector<MBParsing::JSONObject>& Rules = CurrentJSONObject.GetAttribute("Rules").GetArrayData();
				Rules.push_back(std::move(NewRule));
			}
		}
		std::ofstream NewJsonObject = std::ofstream(ComputerDiffDirectory + "/MBPM_ComputerDiffInfo.json",std::ios::out);
		std::string NewJsonString = CurrentJSONObject.ToString();
		NewJsonObject << NewJsonString << std::endl;

		std::string NewDirectory = ComputerDiffDirectory + "/" + NewDirectoryName + "/";
		std::ofstream DeletedObjects = std::ofstream(NewDirectory + "MBPM_RemovedObjects.txt", std::ios::out);
		DeletedObjects << "";
		std::filesystem::create_directories(NewDirectory + "/Data");
		std::ofstream EmptyFileInfo = std::ofstream(NewDirectory + "MBPM_UpdatedFileInfo");
		MBUtility::MBFileOutputStream OutputStream = MBUtility::MBFileOutputStream(&EmptyFileInfo);
		CreatePacketFilesData(NewDirectory + "/Data/", &OutputStream);
		EmptyFileInfo.flush();
		EmptyFileInfo.close();
		NewJsonObject.flush();
		NewJsonObject.close();
	}
	void MBPP_Server::p_CreateEmptyComputerDiffDirectory(std::string const& Packetname)
	{
		std::string PacketDirectory = p_GetPacketDirectory(Packetname);
		std::filesystem::create_directories(PacketDirectory + "/.mbpm/ComputerDiff/");
		std::string ComputerDiffDirectory = PacketDirectory + "/.mbpm/ComputerDiff/";
		if (!std::filesystem::exists(ComputerDiffDirectory + "/MBPM_ComputerDiffInfo.json"))
		{
			std::ofstream NewFile = std::ofstream(ComputerDiffDirectory + "/MBPM_ComputerDiffInfo.json",std::ios::out);
			NewFile << "{\"Rules\": []}";
		}
	}
	std::string MBPP_Server::p_GetPacketDirectory(std::string const& PacketName)
	{
		//g�r igenom packet search directorisen och ser om packeten existerar
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
		//vi vet att denna data f�r plats i minnet
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
	MBError MBPP_Server::p_DownloadClientFileList(MBPP_FileListDownloadState* DownloadState, MBPP_FileListDownloadHandler* DownloadHandler,bool* DownloadFinished)
	{
		MBError ReturnValue = true;
		while (m_RecievedDataOffset < m_RecievedData.size() && ReturnValue)
		{
			if (DownloadState->CurrentFileIndex >= DownloadState->FileNames.size())
			{
				*DownloadFinished = true;
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
			*DownloadFinished = true;
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
		//beh�ver packet namnet f�r att kunna verifiera
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
				//den h�r sparar allt nytt i MBPM_UploadedChanges, om computerdiffen g�r skillnad s� tas det hand om p� IncroproratePacketChanges sidan
				CurrentResponseData.Downloader = std::unique_ptr<MBPP_FileListDownloader>(new MBPP_FileListDownloader(PacketDirectory + "/MBPM_UploadedChanges/"));
				CurrentResponseData.DownloadState.FileNames = CurrentResponseData.FilesToDownload.Files;
			}
		}
		if (CurrentResponseData.PacketName != "" && CurrentResponseData.FilesToDownloadParsed && CurrentResponseData.ObjectsToDeleteParsed)
		{
			bool DownloadFinished = false;
			ReturnValue = p_DownloadClientFileList(&CurrentResponseData.DownloadState, CurrentResponseData.Downloader.get(),&DownloadFinished);
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
			if (DownloadFinished)
			{
				m_RequestFinished = true;
				m_PacketChangesIncorporator->IncorporatePacketChanges(CurrentResponseData.PacketName, m_CurrentRequestHeader.ComputerInfo, CurrentResponseData.ObjectsToDelete.Files);
			}
		}
		return(ReturnValue);
	}
	void MBPP_Server::AddPacketSearchDirectory(std::string const& PacketSearchDirectoryToAdd)
	{
		m_PacketSearchDirectories.push_back(PacketSearchDirectoryToAdd);
	}
	//Upload/Remove metoder
	//bool MBPP_Server::PacketUpdated()
	//{
	//	return(m_PacketUpdated);
	//}
	//std::string MBPP_Server::GetUpdatedPacket()
	//{
	//	return(m_UpdatedPacket);
	//}
	//std::vector<std::string> MBPP_Server::GetPacketRemovedFiles()
	//{
	//	return(m_UpdateRemovedObjects);
	//}
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
					//resettar tidigare packet data vi f�tt
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
			//tom, kr�ver ingen extra data
			else if (m_CurrentRequestHeader.Type == MBPP_RecordType::GetPacketInfo)
			{
				p_Handle_GetPacketInfo();
			}
			else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadRequest)
			{
				//vi vill bara skicka request datan servern hantarerar, s� �r finished redan h�r 
				m_RequestResponseData = new MBPP_UploadRequest_ResponseData();
				MBPP_UploadRequest_ResponseData& CurrentData = p_GetResponseData<MBPP_UploadRequest_ResponseData>();
				CurrentData.Response.Result = m_VerificationResult;
				CurrentData.Response.DomainToLogin = p_GetPacketDomain(m_RequestVerificationData.PacketName);
				CurrentData.Response.SupportedLoginTypes = m_SupportedCredentials;
				m_RequestFinished = true;
			}
			else if (m_CurrentRequestHeader.Type == MBPP_RecordType::UploadChanges)
			{
				//om det misslyckas med autentiseringen s� �r det inte n�got vi svarar p�, s� vi bara avbryter
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
		//l�gger alltid 
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
	MBPP_ServerTotalFileInfo MBPP_ServerResponseIterator::p_GetFileInfo(std::string const& PacketName, MBPP_ComputerInfo ComputerInfo)
	{
		MBPP_ServerTotalFileInfo ReturnValue;
		std::string PacketTopDirectory = p_GetPacketDirectory(PacketName);
		ReturnValue.DefaultTopDirectory = PacketTopDirectory;
		ReturnValue.DefaultInfo = MBPM::MBPP_FileInfoReader(PacketTopDirectory + "/MBPM_FileInfo");
		//skapar directoryn om den ej existerar
		if (!std::filesystem::exists(PacketTopDirectory + "/.mbpm/ComputerDiff/") || !std::filesystem::exists(PacketTopDirectory + "/.mbpm/ComputerDiff/MBPM_ComputerDiffInfo.json"))
		{
			m_AssociatedServer->p_CreateEmptyComputerDiffDirectory(PacketName);
		}
		if (ComputerInfo.OSType != MBPM::MBPP_OSType::Null || ComputerInfo.ProcessorType != MBPM::MBPP_ProcessorType::Null)
		{
			std::string ComputerDiffInfoDirecory = PacketTopDirectory + "/.mbpm/ComputerDiff/";
			MBPM::MBPP_ComputerDiffInfo ComputerDiffInfo;
			MBError ParseResult = ComputerDiffInfo.ReadInfo(ComputerDiffInfoDirecory + "/MBPM_ComputerDiffInfo.json");
			if (ParseResult)
			{
				std::string ComputerDiffDirectory = ComputerDiffInfo.Match(ComputerInfo);
				if (ComputerDiffDirectory == "")
				{
					//om den ej existerar skapar vi en ny
					m_AssociatedServer->p_CreateEmptyComputerDiffDirectory(PacketName, ComputerInfo);
					ComputerDiffInfo.ReadInfo(ComputerDiffInfoDirecory + "/MBPM_ComputerDiffInfo.json");
					ComputerDiffDirectory = ComputerDiffInfo.Match(ComputerInfo);
				}
				assert(ComputerDiffDirectory != "");
				if (ComputerDiffDirectory != "")
				{
					std::string ConfigurationInfoDirectory = ComputerDiffInfoDirecory + ComputerDiffDirectory+"/";
					MBPM::MBPP_FileInfoReader UpdatedFileInfo = MBPM::MBPP_FileInfoReader(ConfigurationInfoDirectory + "/MBPM_UpdatedFileInfo");
					ReturnValue.ComputerDiffInfo = std::move(UpdatedFileInfo);
					ReturnValue.ComputerDiffTopDirectory = ConfigurationInfoDirectory + "/Data/";
					std::set<std::string> DeletedObjects = {};
					std::ifstream DeletedObjectsFile = std::ifstream(ConfigurationInfoDirectory + "/MBPM_RemovedObjects.txt");
					if (DeletedObjectsFile.is_open())
					{
						std::string CurrentLine;
						while (std::getline(DeletedObjectsFile, CurrentLine))
						{
							if (CurrentLine.back() == '\r')
							{
								CurrentLine.resize(CurrentLine.size() - 1);
							}
							DeletedObjects.insert(std::move(CurrentLine));
						}
					}
					ReturnValue.RemovedObjects = std::move(DeletedObjects);
				}
			}
		}
		return(ReturnValue);
	}
	std::vector<std::string> MBPP_ServerResponseIterator::p_GetFilesystemPaths(MBPP_ServerTotalFileInfo const& TotalFileInfo, std::vector<std::string> const& ClientFileList,MBError* OutError)
	{
		std::vector<std::string> ReturnValue = {};
		for (size_t i = 0; i < ClientFileList.size(); i++)
		{
			std::string CurrentFile = ClientFileList[i];
			if (TotalFileInfo.RemovedObjects.find(CurrentFile) != TotalFileInfo.RemovedObjects.end())
			{
				//inngetning? throwa exception?
				*OutError = false;
				OutError->ErrorMessage = CurrentFile+" doesn't exists";
				break;
			}
			else if (TotalFileInfo.ComputerDiffInfo.ObjectExists(CurrentFile))
			{
				ReturnValue.push_back(TotalFileInfo.ComputerDiffTopDirectory + CurrentFile);
			}
			else if (TotalFileInfo.DefaultInfo.ObjectExists(CurrentFile))
			{
				ReturnValue.push_back(TotalFileInfo.DefaultTopDirectory + CurrentFile);
			}
			else
			{
				//ingenting, indikera att filen som s�kes inte existerar
				*OutError = false;
				OutError->ErrorMessage = CurrentFile + " doesn't exists";
				break;
			}
		}
		return(ReturnValue);
	}
	//END MBPP_ServerResponseIterator
	
	//Specifika iterators
	uint64_t h_CalculateFileListResponseSize(std::vector<std::string> const& FilesToSend, std::string const& PacketDirectory)
	{
		uint64_t ReturnValue = 4; //storleken av pointer av FileList
		for (size_t i = 0; i < FilesToSend.size(); i++)
		{
			ReturnValue += MBPP_StringLengthSize+FilesToSend[i].size();//f�r filelisten
			ReturnValue += 8+MBGetFileSize(PacketDirectory + FilesToSend[i]);
		}
		return(ReturnValue);
	}
	uint64_t h_CalculateFileListResponseSize(std::vector<std::string> const& FilesToSend,std::vector<std::string> const& FilesToSendName)
	{
		uint64_t ReturnValue = 4; //storleken av pointer av FileList
		for (size_t i = 0; i < FilesToSend.size(); i++)
		{
			ReturnValue += MBPP_StringLengthSize + FilesToSendName[i].size();//f�r filelisten
			ReturnValue += 8 + MBGetFileSize(FilesToSend[i]);
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
	bool h_UpdateFileListIterator(std::ifstream& FileHandle, std::vector<std::string> const& FileList, size_t& CurrentFileIndex, std::string& ResponseBuffer)
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
			FileHandle.open(FileList[CurrentFileIndex], std::ios::in | std::ios::binary);
			uint64_t FileSize = MBGetFileSize(FileList[CurrentFileIndex]);
			ResponseBuffer += std::string(8, 0);
			h_WriteBigEndianInteger(ResponseBuffer.data(), FileSize, 8);
			BufferOffset = 8;
		}
		size_t ReadBlockSize = 4096 * 2;//godtyckligt
		ResponseBuffer += std::string(ReadBlockSize, 0);
		FileHandle.read(ResponseBuffer.data() + BufferOffset, ReadBlockSize);
		size_t ReadBytes = FileHandle.gcount();
		ResponseBuffer.resize(ReadBytes + BufferOffset);
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
		MBPP_ServerTotalFileInfo TotalFileInfo = p_GetFileInfo(ResponseData.PacketName, ResponseData.ComputerInfo);
		MBError FileListError = true;
		m_FilesToSendPaths = p_GetFilesystemPaths(TotalFileInfo, ResponseData.FilesToGet, &FileListError);
		//om n�got error h�nder h�r ska vi egentligen bara krascha
		assert(FileListError);
		m_HeaderToSend.Type = MBPP_RecordType::FilesData;
		m_HeaderToSend.RecordSize = h_CalculateFileListResponseSize(m_FilesToSendPaths,ResponseData.FilesToGet);
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
			m_Finished = h_UpdateFileListIterator(m_FileHandle, m_FilesToSendPaths, m_CurrentFileIndex, m_CurrentResponse);
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
		MBPP_ServerTotalFileInfo TotalFileInfo = p_GetFileInfo(ResponseData.PacketName, ResponseData.ComputerInfo);
		TotalFileInfo.DefaultInfo.UpdateInfo(TotalFileInfo.ComputerDiffInfo);
		MBPP_FileInfoReader const& PacketFileInfo = TotalFileInfo.DefaultInfo;

		m_FilesToSendNames = {};
		for (size_t i = 0; i < ResponseData.DirectoriesToGet.size(); i++)
		{
			//std::filesystem::recursive_directory_iterator DirectoryIterator = std::filesystem::recursive_directory_iterator(PacketDirectory + ResponseData.DirectoriesToGet[i]);
			const MBPP_DirectoryInfoNode* DirectoryToIterate = PacketFileInfo.GetDirectoryInfo(ResponseData.DirectoriesToGet[i]);
			if (DirectoryToIterate == nullptr)
			{
				continue;
			}
			MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = DirectoryToIterate->begin();
			while(!(DirectoryIterator == DirectoryToIterate->end()))
			{
				std::string CurrentFilePath = ResponseData.DirectoriesToGet[i] +DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName;
				m_FilesToSendNames.push_back(CurrentFilePath);
				DirectoryIterator++;
			}
		}
		MBError FileListError = true;
		m_FilesToSendPaths = p_GetFilesystemPaths(TotalFileInfo, m_FilesToSendNames, &FileListError);
		m_HeaderToSend.RecordSize = h_CalculateFileListResponseSize(m_FilesToSendPaths, m_FilesToSendNames);
		assert(FileListError);
		Increment();
	}
	void MBPP_GetDirectories_ResponseIterator::Increment()
	{
		MBPP_GetDirectories_ResponseData& DataToUse = p_GetResponseData<MBPP_GetDirectories_ResponseData>();
		if (!m_FileListSent)
		{
			m_CurrentResponse = MBPP_GetRecordHeader(m_HeaderToSend) + MBPP_GenerateFileList(m_FilesToSendNames);
			m_FileListSent = true;
		}
		else
		{
			m_Finished = h_UpdateFileListIterator(m_FileHandle, m_FilesToSendPaths, m_CurrentFileIndex, m_CurrentResponse);
		}
	}
	//END MBPP_GetDirectories_ResponseIterator

	//BEGIN MBPP_GetPacketInfo_ResponseIterator
	MBPP_GetPacketInfo_ResponseIterator::MBPP_GetPacketInfo_ResponseIterator(MBPP_GetPacketInfo_ResponseData const& ResponseData, MBPP_Server* AssociatedServer)
	{
		m_AssociatedServer = AssociatedServer;
		m_HeaderToSend.Type = MBPP_RecordType::FilesData;
		//f�r att supporta att man upploadar till packets som inte �n existerar, vilket vi kanske inte vill supporta men �r ju bara f�r mina grejer, s� skapar vi en tom directory n�r man
		//beg�r fr�n ett packet som ej existerar
		//h�r beh�ver vi ocks� ta fram FileInfon
		
		if (p_GetPacketDirectory(ResponseData.PacketName) == "")
		{
			//packetet finns inte, vi skapar det, och checkar f�rst att namnet �r valid, annars bara skiter vi i
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
		MBPP_ServerTotalFileInfo PacketFileInfo = p_GetFileInfo(ResponseData.PacketName, ResponseData.ResponseComputerInfo);
		MBPM::MBPP_FileInfoReader FileInfoReader;
		if (PacketFileInfo.ComputerDiffTopDirectory != "")
		{
			PacketFileInfo.DefaultInfo.UpdateInfo(PacketFileInfo.ComputerDiffInfo);
			FileInfoReader = std::move(PacketFileInfo.DefaultInfo);
		}
		else
		{
			FileInfoReader = std::move(PacketFileInfo.DefaultInfo);
		}
		MBUtility::MBStringOutputStream StringStream(m_FileInfoBytes);
		FileInfoReader.WriteData(&StringStream);
		m_HeaderToSend.RecordSize = h_CalculateFileListResponseSize(m_FilesToSend, p_GetPacketDirectory(ResponseData.PacketName));
		m_HeaderToSend.RecordSize += MBPP_StringLengthSize + m_FileList[1].size() + 8 + m_FileInfoBytes.size();
		Increment();
	}
	void MBPP_GetPacketInfo_ResponseIterator::Increment()
	{
		MBPP_GetPacketInfo_ResponseData& DataToUse = p_GetResponseData<MBPP_GetPacketInfo_ResponseData>();
		if (!m_FileListSent)
		{
			m_CurrentResponse =MBPP_GetRecordHeader(m_HeaderToSend)+ MBPP_GenerateFileList(m_FileList);
			m_FileListSent = true;
		}
		else
		{
			m_Finished = h_UpdateFileListIterator(m_FileHandle, m_FilesToSend, m_CurrentFileIndex, p_GetPacketDirectory(DataToUse.PacketName), m_CurrentResponse);
			if (m_Finished && !m_FileInfoSent)
			{
				m_Finished = false;
				std::string FileInfoData(8, 0);
				h_WriteBigEndianInteger(FileInfoData.data(), m_FileInfoBytes.size(), 8);
				m_CurrentResponse = FileInfoData + m_FileInfoBytes;
				m_FileInfoSent = true;
			}
		}
	}
	//END MBPP_GetPacketInfo_ResponseIterator

	//BEGIN MBPP_GetPacketInfo_UploadRequestResponseIterator
	MBPP_UploadRequest_ResponseIterator::MBPP_UploadRequest_ResponseIterator(MBPP_UploadRequest_ResponseData const& ResponseData, MBPP_Server* AssociatedServer)
	{
		m_RequestResponse = ResponseData.Response;
		m_HeaderToSend.Type = MBPP_RecordType::UploadRequestResponse;
		m_HeaderToSend.ComputerInfo = MBPP_ComputerInfo();
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
		RecordToSend.ComputerInfo = MBPP_ComputerInfo();
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
