#pragma once
#include <string>
#include <vector>
#include <MBErrorHandling.h>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_set>
#include <MBParsing/MBParsing.h>

#include "MB_PacketProtocol.h"
namespace MBPM
{
	enum class MBPM_CompileOutputConfiguration
	{
		StaticDebug,
		StaticRelease,
		DynamicDebug,
		DynamicRelease,
		Embedded,
		Null,
	};
	enum class MBPM_PacketAttribute : uint64_t
	{
		TriviallyCompilable,
		TriviallyConstructable,
		Embedabble,
		TriviallyEmbedabble,
		NoSource,
		Precompiled,
		Null
	};
	struct MBPM_PacketInfo
	{
		std::string PacketName = "";
		std::string UsedCompiler = "";
		std::unordered_set<MBPM_PacketAttribute> Attributes = {};
		std::set<MBPM_CompileOutputConfiguration> SupportedOutputConfigurations = {};
		//std::vector<MBPM_CompileOutputConfiguration> SupportedOutputConfigurations = {};
		std::vector<std::string> AdditionalSystemLibraryDependancies = {};
		std::vector<std::string> PacketDependancies = {};
		std::map<MBPM_CompileOutputConfiguration, std::string> OutputConfigurationTargetNames = {};
		std::vector<std::string> ExportedTargets = {}; //Targets som ska komma till pathen, library targets inkluderas ej
	};
	MBPM_PacketInfo ParseMBPM_PacketInfo(std::string const& PacketPath);
	MBError WriteMBPM_PacketInfo(MBPM_PacketInfo const& PacketToWrite);

	enum class MBPM_CompileOption : uint64_t
	{
		Null = 0,
		Debug = 1,
		Release = 1 << 1,
		Static = 1 << 2,
		Dynamic = 1 << 3,
		AllConfigurations = 1 << 4,
		EmbeddAll = 1 << 5,
		DynamicDependancyLinkage = 1<<6,
		StaticDependancyLinkage = 1<<7,
	};
	struct MBPM_CompleteCompileConfiguration
	{

		std::map<MBPM_CompileOption, bool> CompilationOptions = {};
	};



	struct MBPM_MakefileGenerationOptions
	{
		std::set<MBPM_CompileOutputConfiguration> SupportedLibraryConfigurations = {
			MBPM_CompileOutputConfiguration::StaticRelease,
			MBPM_CompileOutputConfiguration::StaticDebug,
			MBPM_CompileOutputConfiguration::DynamicRelease,
			MBPM_CompileOutputConfiguration::DynamicDebug };
	};
	struct MBPM_CmakeProject_TargetData
	{
		std::vector<std::string> ExplicitHeaders = {};
		std::vector<std::string> ExplicitSourceFiles = {};
		std::vector<std::string> StaticLibrarysNeeded = {};
		std::vector<std::string> DynamicLibrarysNeeded = {};
	};
	struct MBPM_CmakeProject
	{
		std::string ProjectName = "";
		std::vector<std::string> CommonHeaders = {};
		std::vector<std::string> CommonSources = {};
		std::vector<std::string> CommonStaticLibrarysNeeded = {};
		std::vector<std::string> CommonDynamicLibrarysNeeded = {};
		std::map<MBPM_CompileOutputConfiguration, MBPM_CmakeProject_TargetData> TargetsData = {};
		std::unordered_set<std::string> ProjectPacketsDepandancies;
	};

	std::string GetMBPMCmakeFunctions();

	MBError WriteCMakeProjectToFile(MBPM_CmakeProject const& ProjectToWrite, std::string const& OutputFilePath);

	MBError UpdateCmakeMBPMVariables(std::string const& PacketPath);

	MBError GenerateCmakeFile(MBPM_PacketInfo const& PacketToConvert, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& FileName);
	MBError GenerateCmakeFile(std::string const& PacketPath, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& FileName);
	MBError GenerateCmakeFile(std::string const& PacketPath, std::string const& CmakeName = "CMakeLists.txt");


	MBError EmbeddDependancies(std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath);
	MBError EmbeddDependancies(MBPM_PacketInfo const& PacketInfo,std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath);

	MBError CompilePacket(std::string const& PacketDirectory);
	MBError InstallCompiledPacket(std::string const& PacketDirectory);

	MBError CompileAndInstallPacket(std::string const& PacketToCompileDirectory);

	std::string GetSystemPacketsDirectory();
	MBError SetSystemPacketsDirectory(std::string const& DirectoryPath);
};