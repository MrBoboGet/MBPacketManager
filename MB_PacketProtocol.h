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
		GetDirectory, //ger alla files i directoryn
		GetFiles,
		//GetFileInfo
		GetDirectoryInfo,//ger file info på varje fil i directoryn i bokstavs ordning och sedan på dess 
		GetPacketHeader,
		GetPacketInfo,

		UploadFiles,
		RemoveFiles,

		//FileInfo,
		FilesData,
		DirectoryData,
		DirectoryInfo,
		PacketHeader,
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
	constexpr uint8_t MBPP_GenericRecordHeaderSize = 8;
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

	void CreatePacketFilesData(std::string const& PacketToHashDirectory);
	//generella
	class MBPP_Client
	{
	private:
		std::unique_ptr<MBSockets::ConnectSocket> m_ServerConnection = nullptr;
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
	class MBPP_Server
	{
	private:
		
	public:
		MBPP_Server(std::string const& PacketDirectory);

		//top level, garanterat att det är klart efter, inte garanterat att funka om inte låg level under är klara
		std::string GenerateResponse(const void* RequestData, size_t RequestSize,MBError* OutError);
		std::string GenerateResponse(std::string const& RequestData,MBError* OutError);
		
		MBError SendResponse(MBSockets::ConnectSocket* SocketToUse,const void* RequestData,size_t RequestSize);
		MBError SendResponse(MBSockets::ConnectSocket* SocketToUse,std::string const& CompleteRequestData);

		//om stora saker skickas kan inte allt sparas i minnet, så detta api så insertas dem rakt in i klassen, 
		MBError InsertClientData(const void* ClientData, size_t DataSize);
		MBError InsertClientData(std::string const& ClientData);
		bool ClientRequestFinished();
		MBError SendResponse(MBSockets::ConnectSocket* SocketToUse);
	};
}