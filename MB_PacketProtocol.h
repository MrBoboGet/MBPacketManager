#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <MBUtility/MBErrorHandling.h>
#include <MrBoboSockets/MrBoboSockets.h>
#include <MBUtility/MBInterfaces.h>
#include <MBCLI/MBCLI.h>

namespace MBPM
{
	
	//protocol antagande: Request Response, varje client skick m�ste efterv�ntas av ett svar 
	//
	enum class MBPP_TransferProtocol
	{
		TCP,
		HTTP,
		HTTPS,
		Null,
	};
	enum class MBPP_RecordType : uint8_t
	{
		Error,
		GetDirectory, //ger alla files i directoryn
		GetFiles,
		GetPacketInfo, //ladder ner MBPM_PacketInfo, och MBPM_FileInfo

		UploadChanges,
		UploadRequest,

		//FileInfo,
		FilesData,
		DirectoryData,
		PacketInfo,
		UploadRequestResponse,
		UploadChangesResponse,
		Null,
	};
	enum class MBPP_ErrorCode : uint16_t
	{
		Ok,
		InvalidCredentials,
		LoginRequired,
		ParseError,
		NoVerification,
		InvalidVerificationType,
		Denied,
		FilesystemObjectNotFound,
		Null,
	};
	struct MBPP_FileData
	{
		std::string FileName = "";//kan vara relativt till en directory f�r att spara plats / vara mer intuitivt
		uint64_t FileSize = 0;
		std::string FileData = "";//bin�r data
	};

	enum class MBPP_ComputerInfoType : uint8_t
	{
		Standard,
		Null
	};
	enum class MBPP_ProcessorType : uint32_t
	{
		x86,
		x86_64,
		ARM,
		ARMv7,
		Any,
		Null,
	};
	enum class MBPP_OSType : uint32_t
	{
		Linux,
		Windows,
		Windows_10,
		Windows_7,
		MacOS,
		Any,
		Null,
	};
	struct MBPP_ComputerInfo
	{
		MBPP_ComputerInfoType Type = MBPP_ComputerInfoType::Standard;
		MBPP_OSType OSType = MBPP_OSType::Null;
		MBPP_ProcessorType ProcessorType = MBPP_ProcessorType::Null;
	};
	constexpr uint8_t MBPP_GenericRecordHeaderSize = 22;
	constexpr uint8_t MBPP_StringLengthSize = 2;
	constexpr uint8_t MBPP_FileListLengthSize = 4;
	struct MBPP_GenericRecord
	{
		MBPP_RecordType Type = MBPP_RecordType::Null;
		MBPP_ComputerInfo ComputerInfo = MBPP_ComputerInfo();
		uint32_t VerificationDataSize = 0;
		uint64_t RecordSize = 0;
		std::string VerificationData = "";
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
	struct MBPP_ErrorRecord
	{
		MBPP_ErrorCode ErrorCode = MBPP_ErrorCode::Null;
		std::string ErrorMessage = "";
	};
	enum class MBPP_UserCredentialsType : uint8_t
	{
		Request,
		Plain,
		Null,
	};
	struct MBPP_VerificationData
	{
		MBPP_UserCredentialsType CredentialsType = MBPP_UserCredentialsType::Null;//structuren beror p� denna
		uint16_t PacketNameStringLength = -1;
		std::string PacketName = "";
		uint32_t CredentialsDataLength = -1;//till�ter
		std::string CredentialsData = "";
		//
		//std::string Password = "";
		///std::string Username = "";
	};
	struct MBPP_UploadRequest_Response
	{
		MBPP_ErrorCode Result = MBPP_ErrorCode::Null;
		std::string DomainToLogin = "";
		std::vector<MBPP_UserCredentialsType> SupportedLoginTypes = {};//storleken av UserCredentialsType avg�r storleken av vetkor size headern
	};
	struct MBPP_UploadChanges_Response
	{
		MBPP_ErrorCode Result = MBPP_ErrorCode::Null;
	};
	struct MBPP_UploadChangesRecord
	{
		std::string const& PacketToUpdate = "";
		uint32_t ObjectsToDeleteListSize = 0;
		std::vector<std::string> ObjectsToDeleteList = {};
		uint32_t FilesToUploadSize = 0;// kan nu tolkas som en vanlig download files
		std::vector<std::string> FilesToUpload = {};
	};
	bool MBPP_PathIsValid(std::string const& PathToCheck);
	//MBPP_ComputerInfo GetComputerInfo();
	MBPP_GenericRecord MBPP_ParseRecordHeader(const void* Data, size_t DataSize, size_t InOffset, size_t* OutOffset);
	std::string MBPP_EncodeString(std::string const& StringToEncode);
	std::string MBPP_GetRecordHeader(MBPP_GenericRecord const& RecordToConvert);
	std::string MBPP_GetUploadCredentialsData(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData);
	std::string MBPP_GetRecordData(MBPP_GenericRecord const& RecordToConvert);
	std::string MBPP_GenerateFileList(std::vector<std::string> const& FilesToList);
	std::string MBPP_GetErrorCodeString(MBPP_ErrorCode ErrorToConvert);
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
		uint64_t FileSize = 0;
		//
		uint64_t LastWriteTime = 0;
		bool operator<(MBPP_FileInfo const& OtherFileInfo) const
		{
			return(FileName < OtherFileInfo.FileName);
		}
	};
	class MBPP_FileInfoReader;
	class MBPP_DirectoryInfoNode;
	class MBPP_DirectoryInfoNode_ConstIterator
	{
	private:
		struct DirectoryIterationInfo
		{
			const MBPP_DirectoryInfoNode* AssociatedDirectory = nullptr;
			size_t CurrentFileOffset = 0;
			size_t CurrentDirectoryOffset = 0;
		};
		std::stack<DirectoryIterationInfo> m_IterationInfo = {};
		std::string m_CurrentDirectoryPath = "/";
		bool m_Finished = true;
		void p_Increment();
	public:
		MBPP_DirectoryInfoNode_ConstIterator() {};
		MBPP_DirectoryInfoNode_ConstIterator(const MBPP_DirectoryInfoNode* InitialNode);
		
		std::string GetCurrentDirectory() { return(m_CurrentDirectoryPath); };

		bool operator==(MBPP_DirectoryInfoNode_ConstIterator const& IteratorToCompare) const;
		bool operator!=(MBPP_DirectoryInfoNode_ConstIterator const& IteratorToCompare) const;
		MBPP_DirectoryInfoNode_ConstIterator& operator++();
		MBPP_DirectoryInfoNode_ConstIterator& operator++(int);
		MBPP_FileInfo const& operator*();
		const MBPP_FileInfo * operator->();
	};
	
	struct MBPP_DirectoryInfoNode
	{
	private:
		friend class MBPP_FileInfoReader;
		MBPP_DirectoryInfoNode* m_ParentDirectory = nullptr;//enbart till f�r att kunna iterera enkelt �ver filler/directories
	public:
		std::string DirectoryName = "";
		std::string StructureHash = "";
		std::string ContentHash = "";
		uint64_t Size = 0;
		//
		uint64_t LastWriteTime = 0;
		std::vector<MBPP_FileInfo> Files = {};
		std::vector<MBPP_DirectoryInfoNode> Directories = {};
		bool operator<(MBPP_DirectoryInfoNode const& OtherDirectory) const
		{
			return(DirectoryName < OtherDirectory.DirectoryName);
		}
		MBPP_DirectoryInfoNode_ConstIterator begin() const;
		MBPP_DirectoryInfoNode_ConstIterator end() const;
	};

	struct MBPP_FileList
	{
		uint32_t FileListSize = -1;
		std::vector<std::string> Files = {};
	};
	MBPP_FileInfoDiff GetFileInfoDifference(MBPP_FileInfoReader const& ClientInfo, MBPP_FileInfoReader const& ServerInfo);

	struct MBPP_ComputerDiffRule
	{
		std::vector<MBPP_OSType> OSMatches = {};
		std::vector<MBPP_ProcessorType> ProcessorMatch = {};
		std::string InfoDirectory = "";
	};
	class MBPP_ComputerDiffInfo
	{
	private:
		std::vector<MBPP_ComputerDiffRule> m_Rules = {};
		//MBPP_ComputerDiffRule p_ParseRule(MBParsing::JSONObject const& ObjectToParse,MBError* OutError);
	public:
		static MBPP_OSType GetStringOSType(std::string const& StringToConvert);
		static MBPP_ProcessorType GetStringProcessorType(std::string const& StringToConvert);
		static std::string GetOSTypeString(MBPP_OSType TypeToConvert);
		static std::string GetProcessorTypeString(MBPP_ProcessorType TypeToConvert);
		MBError ReadInfo(std::string const& InfoPath);
		std::string Match(MBPP_ComputerInfo const& InfoToMatch);
	};

	constexpr uint16_t MBPP_FileInfoHeaderStaticSize = 23 + 6 + 4 + 4;
	struct MBPP_FileInfoHeader
	{
	private:
	public:
		std::string ArbitrarySniffData = "\x07\xEB\x13\x37MBG_MBPP_FileInfo\x13\x37";
		//static length part
		uint16_t Version[3] = { 0,2,0 };
		MBCrypto::HashFunction HashFunction = MBCrypto::HashFunction::SHA1;//4 bytes
		//
		uint32_t ExtensionDataSize = 0;
	};
	struct MBPP_DirectoryHashData
	{
		std::string StructureHash = "";
		std::string ContentHash = "";
	};
	enum class MBPP_ExtensionType : uint16_t
	{
		FileAndDirectoryProperties,
	};
	struct MBPP_ExtensionInfo
	{
		MBPP_ExtensionType Type;
		uint32_t Size;
	};
	enum class MBPP_FSEntryPropertyType : uint16_t
	{
		Null,

		Name,
		ContentHash,
		StructureHash,
		Size,
		LastWrite,
		SkipPointer
	};
	struct MBPP_FSEntryProperyInfo
	{
		MBPP_FSEntryPropertyType Type;
		uint16_t PropertySize;//last byte set means that the size can vary, and that the rest of the bytes indicates how large the integer that denotes it's size is
	};
	struct MBPP_FileInfoExtensionData
	{
		//FileAndDirectoryProperties
		std::vector<MBPP_FSEntryProperyInfo> FileProperties;
		std::vector<MBPP_FSEntryProperyInfo> DirectoryProperties;
	};
	class MBPM_FileInfoExcluder
	{
	private:
		std::vector<std::string> m_ExcludeStrings = {};
		std::vector<std::string> m_IncludeStrings = {};
		bool p_MatchDirectory(std::string const& Directory, std::string const& StringToMatch) const;
		bool p_MatchFile(std::string const& StringToCompare, std::string const& StringToMatch) const;
	public:
		MBPM_FileInfoExcluder(std::string const& PathPosition);
		MBPM_FileInfoExcluder() {};
		void AddExcludeFile(std::string const& FileToExlude);

		bool Includes(std::string const& StringToMatch) const;
		bool Excludes(std::string const& StringToMatch) const;
	};


	class MBPP_FileInfoReader
	{

	private:
		MBPP_DirectoryInfoNode m_TopNode = MBPP_DirectoryInfoNode();
		MBPP_FileInfoHeader m_InfoHeader;
		MBPP_FileInfoExtensionData m_ExtensionData;

		friend MBPP_FileInfoDiff GetFileInfoDifference(MBPP_FileInfoReader const& ClientInfo, MBPP_FileInfoReader const& ServerInfo);
		static std::vector<std::string> sp_GetPathComponents(std::string const& PathToDecompose);

		const MBPP_DirectoryInfoNode* p_GetTargetDirectory(std::vector<std::string> const& PathComponents) const;
		MBPP_DirectoryInfoNode* p_GetTargetDirectory(std::vector<std::string> const& PathComponents);
		MBPP_DirectoryInfoNode* p_GetTargetDirectory_Impl(std::vector<std::string> const& PathComponents);
		
		void p_UpdateDirectoriesParents(MBPP_DirectoryInfoNode* DirectoryToUpdate);
		void p_UpdateDirectoryInfo(MBPP_DirectoryInfoNode& NodeToUpdate, MBPP_DirectoryInfoNode const& UpdatedNode);
		void p_DeleteObject(std::string const& ObjectPath);
		void p_UpdateHashes();
		MBPP_DirectoryHashData p_UpdateDirectoryStructureHash(MBPP_DirectoryInfoNode& NodeToUpdate);

		bool p_ObjectIsFile(std::string const& ObjectToCheck);
		MBPP_FileInfo* p_GetObjectFileData(std::string const& ObjectToCheck);
		MBPP_DirectoryInfoNode* p_GetObjectDirectoryData(std::string const& ObjectToCheck);

		static uint64_t p_GetPathWriteTime(std::filesystem::path const& PathToInspect);

		static MBPP_FileInfo p_CreateFileInfo(std::filesystem::path const& FilePath, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData);
		static MBPP_DirectoryInfoNode p_CreateDirectoryInfo(
			std::filesystem::path const& TopDirectoryPath,
			std::filesystem::path const& DirectoryPath,
			MBPP_FileInfoHeader const& Header,
			MBPP_FileInfoExtensionData const& ExtensionData,
			MBPM_FileInfoExcluder const& FileExcluder,
			MBPP_FileInfoReader const* ReaderToCompare
		);
	
		static MBPP_FileInfoHeader p_ReadFileInfoHeader(MBUtility::MBOctetInputStream* StreamToReadFrom);
		static MBPP_FileInfoExtensionData p_ReadExtensionData(MBUtility::MBOctetInputStream* InputStream,uint32_t ExtensionSize);
		static MBPP_FileInfo p_ReadFileInfoFromFile(MBUtility::MBOctetInputStream* FileToReadFrom,MBPP_FileInfoHeader const& Header,MBPP_FileInfoExtensionData const& ExtensionData);
		static MBPP_DirectoryInfoNode p_ReadDirectoryInfoFromFile(MBUtility::MBOctetInputStream* FileToReadFrom, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData);
	
		static std::string p_ReadPropertyFromStream(MBUtility::MBOctetInputStream* InputStream, MBPP_FSEntryProperyInfo const& PropertyToRead);

		static void p_WriteHeader(MBPP_FileInfoHeader const& HeaderToWrite, MBUtility::MBOctetOutputStream* OutputStream);
		static void p_WriteExtensionData(MBUtility::MBOctetOutputStream* OutputStream,MBPP_FileInfoExtensionData const& DataToWrite); 
		static void p_WriteFileInfo(MBPP_FileInfo const& InfoToWrite, MBUtility::MBOctetOutputStream* OutputStream, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData);
		static void p_WriteDirectoryInfo(MBPP_DirectoryInfoNode const& InfoToWrite, MBUtility::MBSearchableOutputStream* OutputStream, MBPP_FileInfoHeader const& Header, MBPP_FileInfoExtensionData const& ExtensionData);
		static std::string p_SerializeExtensionData(MBPP_FileInfoExtensionData const& DataToSerialise);

		static void p_GetLocalInfoDiff_UpdateDirectory(MBPP_FileInfoDiff& OutResult,MBPM_FileInfoExcluder const& Excluder,MBPP_DirectoryInfoNode const& StoredNode,std::filesystem::path const& LocalDirectory,std::string const& CurrentIndexPath);

		static MBPP_FileInfoExtensionData p_GetLatestVersionExtensionData();
		static MBPM_FileInfoExcluder p_GetDirectoryFileInfoExcluder(std::string const& Directory);
	public:
		static MBPP_FileInfoDiff GetFileInfoDifference(MBPP_DirectoryInfoNode const& ClientInfo, MBPP_DirectoryInfoNode const& ServerInfo);
		
		friend void swap(MBPP_FileInfoReader& LeftInfoReader, MBPP_FileInfoReader& RightInfoReader) noexcept;
		MBPP_FileInfoReader() {};
		MBPP_FileInfoReader(std::string const& FileInfoPath);
		MBPP_FileInfoReader(const void* DataToRead, size_t DataSize);
		[[SemanticallyAuthoritative]]
		MBPP_FileInfoReader(MBUtility::MBOctetInputStream* InputStream);
		MBPP_FileInfoReader(MBPP_FileInfoReader&& ReaderToSteal) noexcept;
		MBPP_FileInfoReader(MBPP_FileInfoReader const& ReaderToCopy);
		MBPP_FileInfoReader& operator=(MBPP_FileInfoReader ReaderToCopy);
		
		[[SemanticallyAuthoritative]]
		static void CreateFileInfo(std::string const& PacketToHashDirectory, MBPM_FileInfoExcluder const& Excluder, MBPP_FileInfoReader* OutReader);
		
		static void CreateFileInfo(std::string const& PacketToHashDirectory, MBPP_FileInfoReader* OutReader);
		static void CreateFileInfo(std::string const& PacketToHashDirectory, std::string const& FileName = "MBPM_FileInfo");
		static void CreateFileInfo(std::string const& PacketToHashDirectory, MBUtility::MBSearchableOutputStream* OutputStream);

		[[SemanticallyAuthoritative]]
		static void UpdateFileInfo(std::string const& FileInfoTopDirectory, MBPM_FileInfoExcluder const& Excluder, MBPP_FileInfoReader& ReaderToUpdate);
		
		static void UpdateFileInfo(std::string const& FileInfoTopDirectory,MBPP_FileInfoReader& ReaderToUpdate);
		static void UpdateFileInfo(std::string const& DirectoryToIndex, std::string const& FileName = "MBPM_FileInfo");

		static MBPP_FileInfoReader GetLocalUpdatedFiles(std::string const& DirectoryToIndex,std::string const& IndexName = "MBPM_FileInfo");
		[[SemanticallyAuthoritative]]
		static MBPP_FileInfoDiff GetLocalInfoDiff(std::string const& DirectoryToIndex,MBPM_FileInfoExcluder const& Excluder,MBPP_FileInfoReader const& InfoToCompare);
		
		static MBPP_FileInfoDiff GetLocalInfoDiff(std::string const& DirectoryToIndex,std::string const& IndexName = "MBPM_FileInfo");

		bool ObjectExists(std::string const& ObjectToSearch) const;
		const MBPP_FileInfo* GetFileInfo(std::string const& ObjectToSearch) const;
		const MBPP_DirectoryInfoNode * GetDirectoryInfo(std::string const& ObjectToSearch) const;
		bool operator==(MBPP_FileInfoReader const& OtherReader) const;
		
		//copy grejer

		//output grejer
		[[SemanticallyAuthoritative]]
		void WriteData(MBUtility::MBSearchableOutputStream* OutputStream);
		
		std::string GetBinaryString();

		//static grejer 
		void UpdateInfo(MBPP_DirectoryInfoNode const& NewInfo);
		void UpdateInfo(MBPP_DirectoryInfoNode const& NewInfo,std::string const& MountPoint);
		void UpdateInfo(MBPP_FileInfoReader const& NewInfo);
		void UpdateInfo(MBPP_FileInfoReader const& NewInfo,std::string const& MountPoint);
		template<typename T>
		void DeleteObjects(T const& ObjectsToDelete)
		{
			for (std::string const& CurrentObject : ObjectsToDelete)
			{
				p_DeleteObject(CurrentObject);
			}
			p_UpdateDirectoriesParents(&m_TopNode);
			p_UpdateHashes();
		}
	};

	struct MBPP_PacketHost
	{
		std::string URL = "";
		MBPP_TransferProtocol TransferProtocol = MBPP_TransferProtocol::Null;
		uint16_t Port = -1; //-1 st�r f�r default port i f�rh�llande till en transfer protocol
	};
	void CreatePacketFilesData(std::string const& PacketToHashDirectory,std::string const& FileName = "MBPM_FileInfo");
	void CreatePacketFilesData(std::string const& PacketToHashDirectory,MBUtility::MBSearchableOutputStream* OutputStream);
	void UpdateFileInfo(std::string const& PacketToIndex, std::string const& FileName = "MBPM_FileInfo");
	void UpdateFileInfo(std::string const& PacketToIndex, MBUtility::MBSearchableOutputStream* OutputStream);
	MBPP_FileInfoReader CreateFileInfo(std::string const& DirectoryToIterate);
	//generella
	class MBPP_FileListDownloadHandler
	{
	private:
	public:
		virtual MBError NotifyFiles(std::vector<std::string> const& FileToNotify) { return(MBError(true)); };
		virtual MBError Open(std::string const& FileToDownloadName) = 0;
		virtual MBError InsertData(const void* Data, size_t DataSize) = 0;
		virtual MBError Close() = 0;
		MBError InsertData(std::string const& DataToInsert)
		{
			return(InsertData(DataToInsert.data(), DataToInsert.size()));
		};
		virtual ~MBPP_FileListDownloadHandler()
		{

		}
	};
	//laddar ner tills den f�tt alla filer, *inte* efter att den 
	class MBPP_FileListDownloader : public MBPP_FileListDownloadHandler
	{
	private:
		std::string m_OutputDirectory = "";
		size_t m_CurrentFileIndex = 0;
		std::ofstream m_FileHandle;
	public:
		MBPP_FileListDownloader(std::string const& OutputDirectory);
		MBError Open(std::string const& FileToDownloadName) override;
		MBError InsertData(const void* Data, size_t DataSize) override;
		MBError Close() override;
	};
	class MBPP_FileListMemoryMapper : public MBPP_FileListDownloadHandler
	{
	private:
		std::string m_CurrentFile = "";
		std::map<std::string, std::string> m_DownloadedFiles = {};
	public:
		MBPP_FileListMemoryMapper();
		virtual MBError Open(std::string const& FileToDownloadName) override;
		virtual MBError InsertData(const void* Data, size_t DataSize) override;
		virtual MBError Close() override;
		std::map<std::string, std::string>& GetDownloadedFiles() { return(m_DownloadedFiles); };
	};
	class MBPP_ClientHTTPConverter : public MBSockets::ConnectSocket
	{
	private:
		std::unique_ptr<MBSockets::TLSConnectSocket> m_InternalHTTPSocket = nullptr;
		std::string m_RemoteHost = "";

		std::string m_MBPP_ResourceLocation = "";

		std::string m_MBPP_CurrentHeaderData = "";
		MBPP_GenericRecord m_MBPP_CurrentHeader;
		bool m_MBPP_HeaderSent = false;
		uint64_t m_TotalSentRecordData = 0;

		std::string m_MBPP_ResponseData = "";
		bool m_MBPP_HTTPHeaderRecieved = false;

		void p_ResetSendState();
		void p_ResetRecieveState();
		std::string p_GenerateHTTPHeader(MBPP_GenericRecord const& MBPPHeader);
	public:
		MBPP_ClientHTTPConverter() {};
		MBError Connect(MBPP_PacketHost const& HostData);
		MBError EstablishTLSConnection();
		//int Connect() override;
		bool IsConnected() override;
		bool IsValid() override;
		void Close() override;
		std::string RecieveData(size_t MaxDataToRecieve) override;
		MBError SendData(const void* DataToSend, size_t DataSize) override;
		MBError SendData(std::string const& StringToSend) override;
	};
	class MBPP_Client
	{
	private:
		std::unique_ptr<MBSockets::ConnectSocket> m_ServerConnection = nullptr;
		MBPP_ComputerInfo m_CurrentComputerInfo = MBPP_ComputerInfo();
		MBCLI::MBTerminal* m_LogTerminal = nullptr;
		bool m_LoggingEnabled = true;

		MBError p_DownloadFileList(std::string const& InitialData, size_t DataOffset, MBSockets::ConnectSocket* SocketToUse, std::string const& OutputTopDirectory
			, std::vector<std::string> const& OutputFileNames = {});
		MBError p_DownloadFileList(std::string const& InitialData, size_t DataOffset, MBSockets::ConnectSocket* SocketToUse, MBPP_FileListDownloadHandler* DownloadHandler);
		
		MBError p_GetFiles(std::string const& PacketName, std::vector<std::string> const& FilesToGet, MBPP_FileListDownloadHandler*);
		MBError p_GetDirectory(std::string const& PacketName, std::string const& FilesToGet, MBPP_FileListDownloadHandler*);
		
		MBError p_GetFiles(std::string const& PacketName,std::vector<std::string> const& FilesToGet,std::string const& OutputDirectory);
		MBError p_GetDirectory(std::string const& PacketName,std::string const& DirectoryToGet,std::string const& OutputDirectory);
		
		MBError p_DownloadServerFilesInfo(std::string const& PacketName, std::string const& OutputDirectory, std::vector<std::string> const& OutputFileNames);
		MBPP_UploadRequest_Response p_GetLoginResult(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData,MBError* OutError);
		MBPP_ComputerInfo p_GetComputerInfo();

		MBError p_UploadPacket(std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, std::string const& PacketDirectory, std::vector<std::string> const& FilesToUpload,std::vector<std::string> const& FilesToDelete);
	public:
		void SetLogTerminal(MBCLI::MBTerminal* NewLogTerminal);
		void EnableLoggin() { m_LoggingEnabled = true; };
		void DisableLogging() { m_LoggingEnabled = false; };

		MBError Connect(MBPP_TransferProtocol TransferProtocol, std::string const& Domain, std::string const& Port);
		MBError Connect(MBPP_PacketHost const& PacketHost);
		bool IsConnected();

		void SetComputerInfo(MBPP_ComputerInfo NewComputerInfo);
		MBPP_ComputerInfo GetComputerInfo() { return(m_CurrentComputerInfo); }
		static MBPP_ComputerInfo GetSystemComputerInfo();


		MBError DownloadPacket(std::string const& OutputDirectory, std::string const& PacketName); //semantiken av denna funktion �r att den laddar ner totalt nytt, medans update tar diffen
		
		MBError UpdatePacket(std::string const& OutputDirectory, std::string const& PacketName);
		
		MBError UploadPacket(std::string const& PacketDirectory, std::string const& PacketName,MBPP_UserCredentialsType CredentialsType,std::string const& CredentialsData
		,MBPP_UploadRequest_Response* OutResponse);
		MBError UploadPacket(std::string const& PacketDirectory, std::string const& PacketName, MBPP_UserCredentialsType CredentialsType, std::string const& CredentialsData, std::vector<std::string> const& FilesToUpload, std::vector<std::string> const& FilesToDelete ,MBPP_UploadRequest_Response* OutResponse);
		
		MBError DownloadPacketFiles(std::string const& OutputDirectory, std::string const& PacketName,std::vector<std::string> const& FilesToGet);
		MBError DownloadPacketDirectories(std::string const& OutputDirectory, std::string const& PacketName,std::vector<std::string> const& DirectoriesToGet);
		
		MBError DownloadPacketFiles(std::string const& PacketName,std::vector<std::string> const& FilesToDownload, MBPP_FileListDownloadHandler* DownloadHandler);
		MBError DownloadPacketDirectories(std::string const& PacketName, std::vector<std::string> const& DirectorieToDownload, MBPP_FileListDownloadHandler* DownloadHandler);
		
		MBPP_FileInfoReader GetPacketFileInfo(std::string const& PacketName,MBError* OutError);
	};
	//client grejer

	//server grejer



	//Implicit antagande h�r �r att servern aldrig riktigt kommer f� en stor request
	class MBPP_Server;
	enum class MBPP_ServerResponseIterator_Type
	{
		SendListOfFiles,
		Null,
	};
	struct MBPP_ServerTotalFileInfo
	{
		std::string DefaultTopDirectory = "";
		MBPP_FileInfoReader DefaultInfo;

		std::string ComputerDiffTopDirectory = "";
		MBPP_FileInfoReader ComputerDiffInfo;
		std::set<std::string> RemovedObjects;
	};
	class MBPP_ServerResponseIterator
	{
	protected:
		friend class MBPP_Server;
		size_t m_MaxBytesInMemory = 300000000; //300mb, helt godtyckligt just nu
		MBPP_Server* m_AssociatedServer = nullptr;
		//void* m_IterationData = nullptr; //�gs inte av iteratorn
		std::string m_CurrentResponse = "";
		bool m_Finished = false;
		MBPP_GenericRecord m_HeaderToSend;
		//MBPP_ServerResponseIterator(MBPP_Server* m_AssociatedServer);
		//MBPP_ServerResponseIterator();
		//template<typename T> T& p_GetResponseData()
		template<typename T> T& p_GetResponseData();
		//{
		//	return(m_AssociatedServer->p_GetResponseData<T>());
		//}
		std::string p_GetPacketDirectory(std::string const& PacketName);
		std::string p_GetTopPacketsDirectory();
		MBPP_ServerTotalFileInfo p_GetFileInfo(std::string const& PacketName,MBPP_ComputerInfo ComputerInfo);
		std::vector<std::string> p_GetFilesystemPaths(MBPP_ServerTotalFileInfo const& TotalFileInfo, std::vector<std::string> const& ClientFileList,MBError* OutError);
	public:
		std::string& operator*()
		{
			return(m_CurrentResponse);
		}
		std::string& operator->()
		{
			return(m_CurrentResponse);
		}
		bool operator==(MBPP_ServerResponseIterator const& OtherIterator)
		{
			if (m_Finished == true && OtherIterator.m_Finished == true)
			{
				return(true);
			}
			else
			{
				//borde g�r mer checks
				return(false);
			}
		}
		uint64_t GetResponseSize() { return(m_HeaderToSend.RecordSize+MBPP_GenericRecordHeaderSize); };
		MBPP_ServerResponseIterator& operator++();
		MBPP_ServerResponseIterator& operator++(int); //postfix
		virtual void Increment();
		bool IsFinished() { return(m_Finished);  }
		virtual ~MBPP_ServerResponseIterator() {};
	};
	struct MBPP_GetFileList_ResponseData
	{
		MBPP_ComputerInfo ComputerInfo;
		uint32_t PacketNameSize = -1; //totala sizen �r egentligen 2, men utifall att
		std::string PacketName = "";

		uint32_t TotalFileListSize = -1;
		uint32_t TotalParsedFileListData = -1;

		std::string CurrentString = "";
		size_t CurrentStringSize = -1;
		size_t CurrentStringParsedData = 0;

		std::vector<std::string>  FilesToGet = {};
	};
	struct MBPP_GetDirectories_ResponseData
	{
		MBPP_ComputerInfo ComputerInfo;
		uint32_t PacketNameSize = -1;
		std::string PacketName = "";
		uint32_t DirectoryListSize = -1;
		std::vector<std::string> DirectoriesToGet = {};
	};
	struct MBPP_GetPacketInfo_ResponseData
	{
		uint32_t PacketNameSize = -1;
		MBPP_ComputerInfo ResponseComputerInfo;
		std::string PacketName = "";
	};
	struct MBPP_UploadRequest_ResponseData
	{
		MBPP_UploadRequest_Response Response;
	};
	struct MBPP_FileListDownloadState
	{
		bool NewFile = true;
		size_t CurrentFileIndex = 0;
		uint64_t CurrentFileSize = -1;
		uint64_t CurrentFileParsedData = 0;
		std::vector<std::string> FileNames = {};
	};
	struct MBPP_UploadChanges_ResponseData
	{
		uint16_t PacketNameSize = -1;
		std::string PacketName = "";
		bool ObjectsToDeleteParsed = false;
		MBPP_FileList ObjectsToDelete;
		bool FilesToDownloadParsed = false;
		MBPP_FileList FilesToDownload;
		
		MBPP_FileListDownloadState DownloadState;
		std::unique_ptr<MBPP_FileListDownloader> Downloader = nullptr;
		MBError DownloadResult = true;
	};
	class MBPP_GetFileList_ResponseIterator : public MBPP_ServerResponseIterator
	{
	private:
		bool m_FileListSent = false;

		std::vector<std::string> m_FilesToSendPaths = {};
		size_t m_CurrentFileIndex = 0;
		std::ifstream m_FileHandle;
	public:
		MBPP_GetFileList_ResponseIterator(MBPP_GetFileList_ResponseData const& ResponseData, MBPP_Server* AssociatedServer);
		virtual void Increment() override;
	};
	class MBPP_GetDirectories_ResponseIterator : public MBPP_ServerResponseIterator
	{
	private:
		bool m_FileListSent = false;
		std::vector<std::string> m_FilesToSendPaths = {};
		std::vector<std::string> m_FilesToSendNames = {};
		size_t m_CurrentFileIndex = 0;
		std::ifstream m_FileHandle;
	public:
		MBPP_GetDirectories_ResponseIterator(MBPP_GetDirectories_ResponseData const& ResponseData, MBPP_Server* AssociatedServer);
		virtual void Increment() override;
	};
	class MBPP_GetPacketInfo_ResponseIterator : public MBPP_ServerResponseIterator
	{
	private:
		bool m_FileListSent = false;
		bool m_FileInfoSent = false;
		size_t m_CurrentFileIndex = 0;
		std::vector<std::string> m_FileList = { "MBPM_PacketInfo","MBPM_FileInfo" };
		std::vector<std::string> m_FilesToSend = { "MBPM_PacketInfo"};
		std::string m_FileInfoBytes = "";
		size_t m_SentFileInfoBytes = 0;
		std::ifstream m_FileHandle;
	public:
		MBPP_GetPacketInfo_ResponseIterator(MBPP_GetPacketInfo_ResponseData const& ResponseData,MBPP_Server* AssociatedServer);
		virtual void Increment() override;
	};
	class MBPP_UploadRequest_ResponseIterator : public MBPP_ServerResponseIterator
	{
	private:
		bool m_ResponseSent = false;
		MBPP_UploadRequest_Response m_RequestResponse;
	public:
		MBPP_UploadRequest_ResponseIterator(MBPP_UploadRequest_ResponseData const& ResponseData, MBPP_Server* AssociatedServer);
		virtual void Increment() override;
	};
	class MBPP_UploadChanges_ResponseIterator : public MBPP_ServerResponseIterator
	{
	private:
	public:
		MBPP_UploadChanges_ResponseIterator();
		virtual void Increment() override;
	};
	class MBPP_UploadChangesIncorporator
	{
	private:
	public:
		virtual void IncorporatePacketChanges(std::string const& ChangedPacket,MBPP_ComputerInfo ComputerInfo, std::vector<std::string> const& DeletedObjects) = 0;
		virtual ~MBPP_UploadChangesIncorporator()
		{

		};
	};
	class MBPP_Server
	{
		friend class MBPP_ServerResponseIterator;
	private:
		std::vector<std::string> m_PacketSearchDirectories = {};
		bool m_RequestFinished = false;
		
		MBPP_GenericRecord m_CurrentRequestHeader;
		void* m_RequestResponseData = nullptr;

		std::string m_RecievedData = "";
		size_t m_RecievedDataOffset = 0;

		bool m_VerificationDataParsed = false;
		MBPP_VerificationData m_RequestVerificationData;
		MBPP_ErrorCode m_VerificationResult = MBPP_ErrorCode::Null;
		//authentication
		MBUtility::MBBasicUserAuthenticator* m_UserAuthenticator = nullptr;
		MBPP_UploadChangesIncorporator* m_PacketChangesIncorporator = nullptr;
		
		std::vector<MBPP_UserCredentialsType> m_SupportedCredentials = { MBPP_UserCredentialsType::Plain };

		bool m_PacketUpdated = false;
		std::string m_UpdatedPacket = "";
		std::vector<std::string> m_UpdateRemovedObjects = {};

		template<typename T> T& p_GetResponseData()
		{
			return(*(T*)m_RequestResponseData);
		}
		void p_FreeRequestResponseData();
		void p_ResetRequestResponseState();


		std::string m_ServerDomain = "TestDomain";
		//Get metoder
		MBError p_Handle_GetFiles();
		MBError p_Handle_GetDirectories();
		MBError p_Handle_GetPacketInfo();
		MBError p_Handle_UploadChanges();

		MBError p_ParseRequestVerificationData();
		MBError p_ParseFileList(MBPP_FileList& FileListToUpdate,bool* BoolToUpdate);
		MBError p_DownloadClientFileList(MBPP_FileListDownloadState* DownloadState ,MBPP_FileListDownloadHandler* DownloadHandler,bool* DownloadFinished);//antar att fillistan redan �r parsad

		std::string p_GetPacketDirectory(std::string const& PacketName);
		std::string p_GetPacketDomain(std::string const& PacketName);
		std::string p_GetTopPacketsDirectory();

		MBPP_ErrorCode p_VerifyPlainLogin(std::string const& PacketName);
		MBPP_ErrorCode p_VerifyRequest(std::string const& PacketName);

		void p_CreateEmptyComputerDiffDirectory(std::string const& Packetname,MBPP_ComputerInfo NewComputerInfo);//skapar den om den inte existerar
		void p_CreateEmptyComputerDiffDirectory(std::string const& Packetname);//skapar den helt nytt om den inte �n existerar
	public:
		MBPP_Server(std::string const& PacketDirectory, MBPP_UploadChangesIncorporator* UploadChangesHandler);
		void SetUserAuthenticator(MBUtility::MBBasicUserAuthenticator* AuthenticatorToSet);
		void SetTopDomain(std::string const& NewTopDomain);

		void AddPacketSearchDirectory(std::string const& PacketSearchDirectoryToAdd);

		//om stora saker skickas kan inte allt sparas i minnet, s� detta api s� insertas dem rakt in i klassen, 
		MBError InsertClientData(const void* ClientData, size_t DataSize);
		MBError InsertClientData(std::string const& ClientData);
		bool ClientRequestFinished();
		MBPP_ServerResponseIterator* GetResponseIterator();
		void FreeResponseIterator(MBPP_ServerResponseIterator* IteratorToFree);
		MBPP_FileInfoReader GetPacketFileInfo(std::string const& PacketName);

		//bool PacketUpdated();
		//std::string GetUpdatedPacket();
		//std::vector<std::string> GetPacketRemovedFiles();
		//MBError SendResponse(MBSockets::ConnectSocket* SocketToUse);
	};


	//TODO extremt fult, mest lat
	template<typename T> T& MBPP_ServerResponseIterator::p_GetResponseData()
	{
		return(m_AssociatedServer->p_GetResponseData<T>());
	}
}