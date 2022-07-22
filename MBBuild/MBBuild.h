#include "../MBPacketManager.h"
#include <MBUtility/MBInterfaces.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <MBUtility/MBErrorHandling.h>
#include <filesystem>
#include <MBCLI/MBCLI.h>

namespace MBPM
{
    namespace MBBuild
{

    typedef uint32_t FileID;
  
     
    enum class TargetType
    {
        Null,
        Library,//Implies that it can be both dynamic or static
        StaticLibrary,
        DynamicLibrary,
        Executable,
    };
    struct Target
    {
        TargetType Type = TargetType::Null;
        std::vector<std::string> SourceFiles;   
    };
    struct SourceInfo
    {
        std::string Language;
        std::vector<std::string> ExtraIncludes;
        std::vector<std::string> ExternalDependancies;
        std::unordered_map<std::string, Target> Targets;
    };
    struct CompileConfiguration
    {
        std::string Toolchain;   
        std::vector<std::string> CompileFlags;
        std::vector<std::string> LinkFlags;
    };
    struct LanguageConfiguration
    {
		std::string DefaultExportConfig;
        std::vector<std::string> DefaultConfigs;
        std::unordered_map<std::string,CompileConfiguration> Configurations;   
    };
    struct UserConfigurationsInfo
    {
        std::unordered_map<std::string,LanguageConfiguration> Configurations;
    };

    class DependancyConfigSpecification
    {
    private:

    public:
        std::string GetDependancyConfig(MBPM_PacketInfo const& InfoToInspect,std::string const& CompileConfig) const;   
    };
    
    //[[SemanticallyAuthoritative]]
    MBError ParseUserConfigurationInfo(const void* Data,size_t DataSize,UserConfigurationsInfo& OutInfo);
    MBError ParseUserConfigurationInfo(std::filesystem::path const& FilePath,UserConfigurationsInfo &OutInfo);
    //[[SemanticallyAuthoritative]]
    MBError ParseSourceInfo(const void* Data,size_t DataSize,SourceInfo& OutInfo);
    MBError ParseSourceInfo(std::filesystem::path const& FilePath,SourceInfo& OutInfo);

    enum class MBBuildCompileFlags
    {
        CompleteRecompile, 
    };
     MBError CompileMBBuild(
            std::filesystem::path const& PacketDirectory,
            std::string const& PacketInstallDirectory,
            MBBuildCompileFlags Flags,
            LanguageConfiguration const& LanguageConf,
            SourceInfo const& PacketSource,
            std::vector<std::string> const& TargetsToCompile,
            std::vector<std::string> const& ConfigurationsToCompile);
    MBError CompileMBBuild(std::filesystem::path const& PacketPath, std::vector<std::string> Targets, std::vector<std::string> Configurations,
		std::string const& PacketInstallDirectory,MBBuildCompileFlags Flags);


    

    
    class DependancyInfo
    {
    public:
        struct MakeDependancyInfo
        {
            std::string FileName;
            std::vector<std::string> Dependancies;
        };
    private:
        struct DependancyFlags
        {
            bool IsSource : 1;      
            bool IsUpdated : 1;
            DependancyFlags()
            {
                IsSource = false;       
                IsUpdated = false;       
            }
        };
        struct DependancyStruct
        {
            DependancyFlags Flags;
            uint32_t WriteFlag = 0;
            std::filesystem::path Path;
            std::vector<FileID> Dependancies; 
        };
        
        
        //A FileID as an Index to the depenancyies
        static std::vector<MakeDependancyInfo> p_ParseMakeDependancyInfo(MBUtility::MBOctetInputStream& InStream);
        static std::vector<DependancyStruct> p_NormalizeDependancies(std::vector<MakeDependancyInfo>  const& InfoToConvert);
        //Convenience part 
        std::unordered_map<std::string,FileID> m_StringToIDMap;
        //Serialzied partsk
        std::vector<DependancyStruct> m_Dependancies;

        bool p_FileIsUpdated(std::filesystem::path const& SourceDirectory ,FileID Index);
        void p_CreateStringToIDMap();
    public:
        //uint32_t 
        //MBError CreateDependancyInfo('
        static MBError CreateDependancyInfo(SourceInfo const& CurrentBuild,std::filesystem::path const& BuildRoot,DependancyInfo* OutInfo);
        static MBError UpdateDependancyInfo(SourceInfo const& CurrentBuild,std::filesystem::path const& BuildRoot,DependancyInfo* OutInfo);
        static MBError ParseDependancyInfo(MBUtility::MBOctetInputStream& InputStream,DependancyInfo* OutInfo);
        void WriteDependancyInfo(MBUtility::MBOctetOutputStream& OutStream) const;
        MBError GetUpdatedFiles(std::filesystem::path const& SourceDirectory,std::vector<std::string> const& FilesToCheck,std::vector<std::string>* OutFilesToCompile) ;

    };

    class MBBuildCLI
    {
    private:
        PacketRetriever* m_AssociatedRetriever = nullptr; 
        //[[Throws]] 
        UserConfigurationsInfo p_GetGlobalCompileConfigurations();
        SourceInfo p_GetPacketSourceInfo(std::filesystem::path const& PacketPath);        
        MBPM_PacketInfo p_GetPacketInfo(std::filesystem::path const& PacketPath);
        DependancyConfigSpecification p_GetConfigSpec(std::filesystem::path const& PacketPath);

        bool p_VerifyConfigs(LanguageConfiguration const& Config,std::vector<std::string> const& ConfigNames);
        bool p_VerifyTargets(SourceInfo const& InfoToCompile,std::vector<std::string> const& Targests);
        
        void p_Compile_GCC(CompileConfiguration const& CompileConf,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories);
        void p_Link_GCC(CompileConfiguration const& CompileConfig,Target const& TargetToLink,std::string const& TargetName,std::vector<std::string> ExtraLibraries);
        
        void p_BuildLanguageConfig(std::filesystem::path const& PacketPath,MBPM_PacketInfo const& PacketInfo,DependancyConfigSpecification const& Dependancies,CompileConfiguration const& CompileConf,std::string const& CompileConfName, SourceInfo const& InfoToCompile,std::vector<std::string> const& Targets);
    public:
        MBBuildCLI(PacketRetriever* Retriever);
        MBError BuildPacket(std::filesystem::path const& PacketPath,std::vector<std::string> Configs,std::vector<std::string> Targets);
        MBError ExportTarget(std::filesystem::path const& PacketPath,std::vector<std::string> const& TargetsToExport,std::string const& ExportConfig);
        int Run(int argc,const char** argv,MBCLI::MBTerminal* TerminalToUse);
    };
}
}
