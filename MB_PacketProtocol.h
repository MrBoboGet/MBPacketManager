#include <string>
#include <MBErrorHandling.h>
#include <cstdint>
#include <MrBoboSockets/MrBoboSockets.h>
#include <memory>
#include <vector>
namespace MBPM
{

	//protocol antagande: Request Response, varje client skick måste efterväntas av ett svar 
	//
	enum class MBPP_TransferProtocol
	{
		TCP,
		HTTP,
		HTTPS,
	};
	enum class MBPP_RecordType : uint8_t
	{
		Error,
		GetDirectory, //ger alla files i directoryn
		GetFiles,
		GetPacketInfo, //ladder ner MBPM_PacketInfo, och MBPM_FileInfo

		UploadFiles,
		RemoveFiles,
		RemoveDirectories,

		//FileInfo,
		FilesData,
		DirectoryData,
		PacketInfo,
		UploadResponse,
		RemoveResponse,
		Null,
	};
	enum class MBPP_ErrorCode : uint16_t
	{

	};
	struct MBPP_FileData
	{
		std::string FileName = "";//kan vara relativt till en directory för att spara plats / vara mer intuitivt
		uint64_t FileSize = 0;
		std::string FileData = "";//binär data
	};
	constexpr uint8_t MBPP_GenericRecordHeaderSize = 9;
	struct MBPP_GenericRecord
	{
		MBPP_RecordType Type = MBPP_RecordType::Null;
		uint64_t RecordSize = 0;
		std::string RecordData = "";
	};
	struct MBPP_GetDirectoryRecord
	{
		std::string DirectoryToGet = "";
	};
	struct MBPP_GetDirectoryResponseRecord
	{
		std::vector<MBPP_FileData> Files = {};//filerna har en total filepath
	};
	struct MBPP_GetFilesRecord
	{
		std::vector<std::string> FilesToGet = {};
	};
	struct MBPP_GetFilesResponseRecord
	{
		std::vector<MBPP_FileData> FilesToSend = {};
	};
	struct MBPP_GetDirectoryInfoRecord
	{
		std::string DirectoryToQuery = "";
	};
	struct MBPP_GetDirectoryInfoResponseRecord
	{
		
	};
	struct MBPP_GetPacketHeaderRecord
	{

	};
	struct MBPP_GetPacketHeaderResponseRecord
	{

	};
	struct MBPP_GetPacketInfoRecord
	{

	};
	struct MBPP_GetPacketInfoResponseRecord
	{

	};
	std::string MBPP_EncodeString(std::string const& StringToEncode);
	std::string MBPP_GetRecordData(MBPP_GenericRecord const& RecordToConvert);

	struct MBPP_FileInfoDiff
	{
		std::vector<std::string> UpdatedFiles = {};
		std::vector<std::string> AddedFiles = {};
		std::vector<std::string> RemovedFiles = {};
		std::vector<std::string> DeletedDirectories = {};
		std::vector<std::string> AddedDirectories = {};
	};

	struct MBPP_FileInfo
	{
		std::string FileName = "";
		std::string FileHash = "";
		bool operator<(MBPP_FileInfo const& OtherFileInfo) const
		{
			return(FileName < OtherFileInfo.FileName);
		}
	};
	struct MBPP_DirectoryInfoNode
	{
		std::string DirectoryName = "";
		std::string DirectoryHash = "";
		std::vector<MBPP_FileInfo> Files = {};
		std::vector<MBPP_DirectoryInfoNode> Directories = {};
		bool operator<(MBPP_DirectoryInfoNode const& OtherDirectory) const
		{
			return(DirectoryName < OtherDirectory.DirectoryName);
		}
	};

	class MBPP_FileInfoReader
	{
	private:
		friend MBPP_FileInfoDiff GetFileInfoDifference(MBPP_FileInfoReader const& ClientInfo, MBPP_FileInfoReader const& ServerInfo);
		MBPP_DirectoryInfoNode m_TopNode = MBPP_DirectoryInfoNode();
		static std::vector<std::string> sp_GetPathComponents(std::string const& PathToDecompose);
		MBPP_DirectoryInfoNode p_ReadDirectoryInfoFromFile(std::ifstream& FileToReadFrom,size_t HashSize);
		MBPP_FileInfo p_ReadFileInfoFromDFile(std::ifstream& FileToReadFrom,size_t HashSize);
		const MBPP_DirectoryInfoNode* p_GetTargetDirectory(std::vector<std::string> const& PathComponents);
	public:
		MBPP_FileInfoReader(std::string const& FileInfoPath);
		bool ObjectExists(std::string const& ObjectToSearch);
		const MBPP_FileInfo* GeFileInfo(std::string const& ObjectToSearch);
		const MBPP_DirectoryInfoNode * GetDirectoryInfo(std::string const& ObjectToSearch);
	};

	void CreatePacketFilesData(std::string const& PacketToHashDirectory,std::string const& FileName = "MBPM_FileInfo");
	//generella
	class MBPP_Client
	{
	private:
		std::unique_ptr<MBSockets::ConnectSocket> m_ServerConnection = nullptr;

		MBError p_DownloadFileList(std::string const& InitialData,size_t DataOffset,MBSockets::ConnectSocket* SocketToUse,std::string const& OutputTopDirectory);
		MBError p_GetFiles(std::string const& PacketName,std::vector<std::string> const& FilesToGet,std::string const& OutputDirectory);
		MBError p_GetDirectory(std::string const& PacketName,std::string const& DirectoryToGet,std::string const& OutputDirectory);
		MBError p_DownloadServerFilesInfo(std::string const& PacketName,std::string const& OutputFilepath, std::string const& OutputDirectory);
	public:
		MBError Connect(MBPP_TransferProtocol TransferProtocol, std::string const& Domain, std::string const& Port);
		bool IsConnected();
		MBError DownloadPacket(std::string const& OutputDirectory, std::string const& PacketName); //semantiken av denna funktion är att den laddar ner totalt nytt, medans update tar diffen
		MBError UpdatePacket(std::string const& OutputDirectory, std::string const& PacketName);
		MBError UploadPacket(std::string const& PacketDirectory, std::string const& PacketName);
	};
	//client grejer

	//server grejer



	//Implicit antagande här är att servern aldrig riktigt kommer få en stor request
	class MBPP_Server;
	enum class MBPP_ServerResponseIterator_Type
	{
		SendListOfFiles,
		Null,
	};
	class MBPP_ServerResponseIterator
	{
	private:
		friend class MBPP_Server;
		MBPP_Server* m_AssociatedServer = nullptr;
		void* m_IterationData = nullptr; //ägs inte av iteratorn
		std::string m_CurrentResponse = "";
		bool m_HasEnded = true;

		MBPP_ServerResponseIterator(MBPP_Server* m_AssociatedServer);
		MBPP_ServerResponseIterator();
	public:
		std::string& operator*()
		{
			return(m_CurrentResponse);
		}
		std::string& operator->()
		{
			return(m_CurrentResponse);
		}
		bool operator=(MBPP_ServerResponseIterator const& OtherIterator)
		{
			if (m_HasEnded == true && OtherIterator.m_HasEnded == true)
			{
				return(true);
			}
			else
			{
				//borde gör mer checks
				return(false);
			}
		}
		MBPP_ServerResponseIterator& operator++();
		MBPP_ServerResponseIterator& operator++(int); //postfix
		bool IsFinished() { return(m_HasEnded);  }
		~MBPP_ServerResponseIterator();
	};
	class MBPP_Server
	{
		friend class MBPP_ServerResponseIterator;
	private:
		std::vector<std::string> m_PacketSearchDirectories = {};
		bool m_RequestFinished = false;

		MBPP_RecordType m_CurrentRequestType = MBPP_RecordType::Null;
		void* m_RequestResponseData = nullptr;

		std::string m_RecievedData = "";
		size_t m_RecievedDataOffset = 0;
		size_t m_CurrentRequestSize = -1;
		
		template<typename T> T& p_GetRequestArgument()
		{
			return(*(T*)m_CurrentRequestData)
		}
		void p_FreeRequestResponseData();
		void p_ResetRequestResponseState();

		//Get metoder
		MBError p_Handle_GetFiles();
		MBError p_Handle_GetDirectories();
		MBError p_Handle_GetPacketInfo();

		//Upload/Remove metoder
		MBError p_Handle_UploadFiles();
	public:
		MBPP_Server(std::string const& PacketDirectory);

		//top level, garanterat att det är klart efter, inte garanterat att funka om inte låg level under är klara
		//std::string GenerateResponse(const void* RequestData, size_t RequestSize,MBError* OutError);
		//std::string GenerateResponse(std::string const& RequestData,MBError* OutError);
		//
		//MBError SendResponse(MBSockets::ConnectSocket* SocketToUse,const void* RequestData,size_t RequestSize);
		//MBError SendResponse(MBSockets::ConnectSocket* SocketToUse,std::string const& CompleteRequestData);

		//om stora saker skickas kan inte allt sparas i minnet, så detta api så insertas dem rakt in i klassen, 
		MBError InsertClientData(const void* ClientData, size_t DataSize);
		MBError InsertClientData(std::string const& ClientData);
		bool ClientRequestFinished();
		MBPP_ServerResponseIterator GetResponseIterator();
		//MBError SendResponse(MBSockets::ConnectSocket* SocketToUse);
	};
}