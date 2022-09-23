#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <MBUtility/MBErrorHandling.h>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_set>
#include <MBParsing/MBParsing.h>
#include <unordered_map>

#include "MBPacketManager.h"
#include "MB_PacketProtocol.h"
namespace MBPM
{
	struct MBPM_SubLibrary
	{
		std::string LibraryName = "";
	};

	//enum class PacketType
	//{
	//	CPP,
	//	MBDoc,
	//	Unkown,
	//	Null
	//};
	struct MBPM_PacketInfo
	{
		std::string PacketName = "";
        std::string Type = "C++";
		std::unordered_set<std::string> Attributes = {};
		//std::vector<MBPM_CompileOutputConfiguration> SupportedOutputConfigurations = {};
		std::vector<std::string> AdditionalSystemLibraryDependancies = {};
		std::vector<std::string> PacketDependancies = {};
		std::vector<std::string> ExportedTargets = {}; 
		std::vector<std::string> ExtraIncludeDirectories = {};
		std::vector<MBPM_SubLibrary> SubLibraries = {};
	};
	MBPM_PacketInfo ParseMBPM_PacketInfo(std::string const& PacketPath);
	MBError WriteMBPM_PacketInfo(MBPM_PacketInfo const& PacketToWrite);

    enum class PacketLocationType
    {
        Null,
        User,
        Installed,
        Local,
        Remote
    };
    struct MBPM_PacketDependancyRankInfo
	{
		uint32_t DependancyDepth = -1;
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
    struct PacketIdentifier
    {
        std::string PacketName = "";
        std::string PacketURI = "";//Default/"" implicerar att man anv�nder default remoten
        PacketLocationType PacketLocation = PacketLocationType::Null;
        bool operator==(PacketIdentifier const& rhs) const
        {
            bool ReturnValue = true;
            if (PacketName != rhs.PacketName)
            {
                ReturnValue = false;
            }
            if (PacketURI != rhs.PacketURI)
            {
                ReturnValue = false;
            }
            if (PacketLocation != rhs.PacketLocation)
            {
                ReturnValue = false;
            }
            return(ReturnValue);
        }
    };
    class PacketRetriever
    {
    private:
        //std::unordered_map<std::filesystem::path, std::pair<PacketIdentifier,MBPM_PacketInfo>> m_CachedPackets;

        std::vector<PacketIdentifier> p_GetUserPackets();
        
        MBPM::MBPM_PacketInfo p_GetPacketInfo(PacketIdentifier const& PacketToInspect, MBError* OutError);
		std::map<std::string, MBPM_PacketDependancyRankInfo> p_GetPacketDependancieInfo(
			std::vector<PacketIdentifier> const& InPacketsToCheck,
			MBError& OutError,
			std::vector<std::string>* OutMissing);
        std::vector<PacketIdentifier> p_GetPacketDependancies_DependancyOrder(std::vector<PacketIdentifier> const& InPacketsToCheck,MBError& OutError, std::vector<std::string>* MissingPackets);
        PacketIdentifier p_GetUserPacket(std::string const& PacketName);
        PacketIdentifier p_GetInstalledPacket(std::string const& PacketName);
        PacketIdentifier p_GetLocalPacket(std::string const& PacketName);
        PacketIdentifier p_GetRemotePacketIdentifier(std::string const& PacketName);
    public:
        PacketIdentifier GetInstalledPacket(std::string const& PacketName);
        PacketIdentifier GetUserPacket(std::string const& PacketName);
        std::vector<PacketIdentifier> GetPacketDependancies(PacketIdentifier const& PacketToInspect);
        std::vector<PacketIdentifier> GetTotalDependancies(std::vector<std::string> const& DependancyNames,MBError& OutError);
        std::vector<PacketIdentifier> GetPacketDependees(std::string const& PacketName);
        MBPM_PacketInfo GetPacketInfo(PacketIdentifier const& PacketToRetrieve);
    };



    
    //Self contained
	
    
    
    //std::string GetMBPMCmakeFunctions();

	//MBError WriteCMakeProjectToFile(MBPM_CmakeProject const& ProjectToWrite, std::string const& OutputFilePath);

	MBError UpdateCmakeMBPMVariables(std::string const& PacketPath,std::vector<std::string>const&  TotalPacketDependancies,std::string const& StaticMBPMData);

	//MBError GenerateCmakeFile(MBPM_PacketInfo const& PacketToConvert, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& FileName);
	//MBError GenerateCmakeFile(std::string const& PacketPath, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& FileName);
	MBError GenerateCmakeFile(std::string const& PacketPath, std::string const& CmakeName = "CMakeLists.txt");


	//MBError EmbeddDependancies(std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath);
	//MBError EmbeddDependancies(MBPM_PacketInfo const& PacketInfo,std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath);

    
    
	//MBError CompilePacket(std::string const& PacketDirectory);
	//MBError InstallCompiledPacket(std::string const& PacketDirectory);

	//MBError CompileMBCmake(std::string const& PacketDirectory, std::string const& Configuration, std::vector<std::string> const& Targets);

	//[[deprecated]]
	//MBError CompileAndInstallPacket(std::string const& PacketToCompileDirectory);

	//bool PacketIsPrecompiled(std::string const& PacketDirectoryToCheck, MBError* OutError);

	std::string GetSystemPacketsDirectory();
	//MBError SetSystemPacketsDirectory(std::string const& DirectoryPath);
};
