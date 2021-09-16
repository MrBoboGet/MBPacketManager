#include "MB_PacketProtocol.h"
#include <MBParsing/MBParsing.h>
#include <set>
#include <MBCrypto/MBCrypto.h>
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
			if (Entries.is_regular_file())
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
		std::string DirectoryName = std::filesystem::path(SubPathToIterate).filename().generic_u8string();
		h_WriteBigEndianInteger(FileToWriteTo, DirectoryName.size(), 2);
		FileToWriteTo.write(DirectoryName.data(), DirectoryName.size());
		std::string DataToHash = "";
		for (auto const& Files : FilesToWrite)
		{
			DataToHash += h_WriteFileData(FileToWriteTo, Files, HashFunctionToUse);
		}
		for (auto const& Directories : DirectoriesToWrite)
		{
			DataToHash += h_WriteDirectoryData_Recursive(FileToWriteTo, std::filesystem::path(Directories).generic_u8string(), Directories, ExcludedFiles, HashFunctionToUse);
		}
		uint64_t DirectoryEndPosition = FileToWriteTo.tellp();
		FileToWriteTo.seekp(SkipPointerPosition);
		h_WriteBigEndianInteger(FileToWriteTo, DirectoryEndPosition, 8);
		std::string DirectoryHash = MBCrypto::HashData(DataToHash, HashFunctionToUse);
		FileToWriteTo.write(DirectoryHash.data(), DirectoryHash.size());
		FileToWriteTo.seekp(DirectoryEndPosition);
		return(DirectoryHash);
	}
	void CreatePacketFilesData(std::string const& PacketToHashDirectory)
	{
		std::ofstream OutputFile = std::ofstream(PacketToHashDirectory + "MBPM_FileInfo");
		std::set<std::string> ExcludedFiles = {};
		h_WriteDirectoryData_Recursive(OutputFile, "/", PacketToHashDirectory, ExcludedFiles, MBCrypto::HashFunction::SHA1);
		OutputFile.flush();
		OutputFile.close();
	}

	//BEGIN MBPP_Client
	MBError MBPP_Client::Connect(MBPP_TransferProtocol TransferProtocol, std::string const& Domain, std::string const& Port)
	{

	}
	bool MBPP_Client::IsConnected()
	{
		return(m_ServerConnection->IsConnected());
	}
	MBError MBPP_Client::DownloadPacket(std::string const& OutputDirectory, std::string const& PacketName)
	{
		MBError ReturnValue = true;
		size_t MaxRecieveSize = 300000000;

		MBPP_GenericRecord RecordToSend;
		RecordToSend.Type = MBPP_RecordType::GetDirectory;
		RecordToSend.RecordData = MBPP_EncodeString("/");
		RecordToSend.RecordSize = RecordToSend.RecordData.size();
		m_ServerConnection->SendData(MBPP_GetRecordData(RecordToSend));

		std::ofstream FileHandle;
		uint64_t TotalResponseSize = -1;
		uint64_t TotalRecievedBytes = 0;
		uint64_t TotalNumberOfFiles = -1;
		std::string ResponseData = m_ServerConnection->RecieveData(MaxRecieveSize); // 300 MB
		std::string CurrentFile = "";
		size_t CurrentFileSize = -1;
		size_t CurrentFileParsedBytes = -1;

		//parsa grundläggande Data
		while (ResponseData.size() < MBPP_GenericRecordHeaderSize+4)//vi behöver också veta hur många saker som skickas
		{
			ResponseData += m_ServerConnection->RecieveData(MaxRecieveSize);
		}
		TotalResponseSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 8, 1, nullptr);
		TotalNumberOfFiles = MBParsing::ParseBigEndianInteger(ResponseData.data(), 8, MBPP_GenericRecordHeaderSize, nullptr);
		size_t ResponseDataOffset = MBPP_GenericRecordHeaderSize + 8;
		while (TotalRecievedBytes < TotalResponseSize)
		{
			if (CurrentFile == "")
			{
				//vi måste få info om den fil vi ska skriva till nu
				while(ResponseData.size()-ResponseDataOffset < 2)
				{
					size_t PreviousResponseDataSize = ResponseData.size();
					ResponseData += m_ServerConnection->RecieveData(MaxRecieveSize);
					TotalRecievedBytes += ResponseData.size() - PreviousResponseDataSize;
				}
				uint16_t FileNameSize = MBParsing::ParseBigEndianInteger(ResponseData.data(), 2, ResponseDataOffset, &ResponseDataOffset);
				while (ResponseData.size()-ResponseDataOffset < uint64_t(FileNameSize)+8)
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
				uint64_t FileBytes = std::min((uint64_t)ResponseData.size()-ResponseDataOffset, CurrentFileSize - CurrentFileParsedBytes);
				FileHandle.write(ResponseData.data()+ResponseDataOffset, FileBytes);
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

		CreatePacketFilesData(OutputDirectory);
		return(ReturnValue);
	}
	MBError MBPP_Client::UpdatePacket(std::string const& OutputDirectory, std::string const& PacketName)
	{
		//kollar om att ladda ner packet infon är tillräckligt litet och skapar en diff fil, eller gör det genom att manuellt be om directory info för varje fil och se om hashen skiljer
		
		//ANTAGANDE vi antar här att PacketFilesData är uppdaterad


	}
	MBError MBPP_Client::UploadPacket(std::string const& PacketDirectory, std::string const& PacketName)
	{

	}
	//END MBPP_Client
}