#pragma once
#include "../MBPacketManager.h"
#include <MBUtility/MBInterfaces.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <MBUtility/MBErrorHandling.h>
#include <filesystem>
#include <MBCLI/MBCLI.h>

#include "../MBPM_CLI.h"

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
        std::string OutputName;
        std::vector<std::string> SourceFiles;   
    };
    struct SourceInfo
    {
        std::string Language;
        std::string Standard;
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
        struct Predicate
        {
            bool Negated = false;
            std::vector<std::string> PacketName;      
            std::vector<std::string> Attribute;      
            std::vector<std::string> ConfigurationName;      
        };
        struct Specification
        {
            std::vector<Predicate> Predicates;
            std::string ResultConfiguration;
        };

        std::vector<Specification> Specifications;
        
        static std::vector<std::string> p_ParseStringArray(MBParsing::JSONObject const& ObjectToParse);
        static Predicate p_ParsePredicate(MBParsing::JSONObject const& ObjectToParse);
        static Specification p_ParseSpecification(MBParsing::JSONObject const& ObjectToParse);

        static bool p_SpecIsTrue(Specification const& SpecToVerify,MBPM_PacketInfo const& DependancyPacket,std::string const& CompileConfig);
        static bool p_PredicateIsTrue(Predicate const& SpecToVerify,MBPM_PacketInfo const& DependancyPacket,std::string const& CompileConfig);
    public:
        static MBError ParseDependancyConfigSpecification(MBUtility::MBOctetInputStream& InputStream,DependancyConfigSpecification* OutConfig);
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


    enum class DependancyFlag
    {
        Null = 0,
        FileChecked = 1,
        IsUpdated = 1 <<1,
    };

    inline DependancyFlag operator&(DependancyFlag Left,DependancyFlag Right)
    {
        return(DependancyFlag(uintmax_t(Left)&uintmax_t(Right)));   
    }
    inline DependancyFlag operator|(DependancyFlag Left,DependancyFlag Right)
    {
        return(DependancyFlag(uintmax_t(Left)|uintmax_t(Right)));   
    }
    inline void operator&=(DependancyFlag& lhs,DependancyFlag rhs)
    {
        lhs = (lhs & rhs); 
    } 
    inline void operator|=(DependancyFlag& lhs,DependancyFlag rhs)
    {
        lhs = (lhs | rhs); 
    } 
    inline DependancyFlag operator~(DependancyFlag rhs)
    {
        return(DependancyFlag(~uintmax_t(rhs)));
    }
    class DependancyInfo
    {
    public:
        struct MakeDependancyInfo
        {
            std::string FileName;
            std::vector<std::string> Dependancies;
        };
    private:
        //struct DependancyFlags
        //{
        //    bool IsSource : 1; 
        //    bool IsUpdated : 1;
        //    DependancyFlags()
        //    {
        //        IsSource = false;       
        //        IsUpdated = false;       
        //    }
        //};
        struct DependancyStruct
        {
            //32
            DependancyFlag Flags = DependancyFlag::Null;
            uint32_t WriteFlag = 0;
            std::string Path;
            std::vector<FileID> Dependancies; 
            bool operator==(DependancyStruct const& OtherInfo) const
            {
                bool ReturnValue = true;
                ReturnValue &= WriteFlag == OtherInfo.WriteFlag;
                ReturnValue &= Path == OtherInfo.Path;
                ReturnValue &= Dependancies == OtherInfo.Dependancies;
                return(ReturnValue); 
            }
            bool operator!=(DependancyStruct const& OtherInfo) const
            {
                return(!(*this  == OtherInfo));       
            }
        };
        //struct SourceStruct
        //{
        //    DependancyFlag Flags;
        //    uint32_t WriteFlag = 0;
        //    std::filesystem::path Path;
        //    std::vector<FileID> Dependancies; 
        //    bool operator==(DependancyStruct const& OtherInfo) const
        //    {
        //        bool ReturnValue = true;
        //        ReturnValue &= Flags == OtherInfo.Flags;
        //        ReturnValue &= WriteFlag == OtherInfo.WriteFlag;
        //        ReturnValue &= Path == OtherInfo.Path;
        //        ReturnValue &= Dependancies == OtherInfo.Dependancies;
        //        return(ReturnValue); 
        //    }
        //    bool operator!=(DependancyStruct const& OtherInfo) const
        //    {
        //        return(!(*this  == OtherInfo));       
        //    }
        //        
        //};
        //A FileID as an Index to the depenancyies
        static std::string GetGCCDependancyCommand(std::filesystem::path const& BuildRoot,std::vector<std::string> const& SourcesToCheck,std::vector<std::string> const& ExtraIncludeDirectories);
        static std::vector<MakeDependancyInfo> p_ParseMakeDependancyInfo(MBUtility::MBOctetInputStream& InStream);
        static std::vector<DependancyStruct> p_NormalizeDependancies(std::vector<MakeDependancyInfo>  const& InfoToConvert,
            std::filesystem::path const& SourceRoot,std::unordered_map<std::string,FileID>& IDMap,std::vector<DependancyStruct>& OutDeps,std::vector<std::string> const& OriginalSources);
        //Convenience part 
        std::vector<std::string> m_AddedSources = {};
        std::unordered_map<std::string,FileID> m_PathToDependancyIndex;
        std::unordered_map<std::string,size_t> m_PathToSourceIndex;
        //Serialzied partsk
        std::vector<std::string> m_CompileOptions;
        std::string m_ToolChain;
        std::vector<DependancyStruct> m_Sources;
        std::vector<DependancyStruct> m_Dependancies;

        bool p_DependancyIsUpdated(FileID Index);
        bool p_FileIsUpdated(std::filesystem::path const& SourceDirectory ,FileID Index);
        void p_CreateSourceMap();
        void p_CreateDependancyMap();

        void p_WriteDependancyStruct(MBUtility::MBOctetOutputStream& OutStream, DependancyStruct const& StructToWrite) const;
        static DependancyStruct p_ParseDependancyStruct(MBUtility::MBOctetInputStream& OutStream);
    public:
        //uint32_t 
        //MBError CreateDependancyInfo('
        static MBError CreateDependancyInfo(SourceInfo const& CurrentBuild,CompileConfiguration const& CompileConfig,std::filesystem::path const& BuildRoot,DependancyInfo* OutInfo);
        static MBError ParseDependancyInfo(MBUtility::MBOctetInputStream& InputStream,DependancyInfo* OutInfo);
        void WriteDependancyInfo(MBUtility::MBOctetOutputStream& OutStream) const;
    
        bool operator==(DependancyInfo const& OtherInfo) const;
        bool operator!=(DependancyInfo const& OtherInfo) const;
       
        //TODO void RemoveRedundantDependancies();
        
        MBError UpdateUpdatedFilesDependancies(std::filesystem::path const& SourceDirectory);
        MBError GetUpdatedFiles(std::filesystem::path const& SourceDirectory,CompileConfiguration const& CompileConfig,std::vector<std::string> const& FilesToCheck,std::vector<std::string>* OutFilesToCompile) ;

    };

    class MBBuild_Extension : public CLI_Extension
    {
    private:
        PacketRetriever* m_AssociatedRetriever = nullptr; 
        std::filesystem::path m_ConfigDirectory;
        //[[Throws]] 
        UserConfigurationsInfo p_GetGlobalCompileConfigurations();
        DependancyConfigSpecification p_GetGlobalDependancySpecification();
        SourceInfo p_GetPacketSourceInfo(std::filesystem::path const& PacketPath);        
        MBPM_PacketInfo p_GetPacketInfo(std::filesystem::path const& PacketPath);
        DependancyConfigSpecification p_GetConfigSpec(std::filesystem::path const& PacketPath);

       
        bool p_VerifyConfigs(LanguageConfiguration const& Config,std::vector<std::string> const& ConfigNames);
        bool p_VerifyTargets(SourceInfo const& InfoToCompile,std::vector<std::string> const& Targests);
        //Responsible both for verifying that the toolchain is supported, and that the standard is supported by the toolchain 
        bool p_VerifyToolchain(SourceInfo const& InfoToCompile,CompileConfiguration const& TargetToVerify);
       
        void p_Compile(CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories);
        void p_Link(CompileConfiguration const& CompileConfig,SourceInfo const& SInfo, std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries);

        void p_Compile_MSVC(CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories);
        void p_Link_MSVC(CompileConfiguration const& CompileConfig, SourceInfo const& SInfo,std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries);
        
        void p_Compile_GCC(std::string const& CompilerName,CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories);
        void p_Link_GCC( std::string const& CompilerName,CompileConfiguration const& CompileConfig, SourceInfo const& SInfo,std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries);
        
        void p_BuildLanguageConfig(std::filesystem::path const& PacketPath,MBPM_PacketInfo const& PacketInfo,DependancyConfigSpecification const& Dependancies,CompileConfiguration const& CompileConf,std::string const& CompileConfName, SourceInfo const& InfoToCompile,std::vector<std::string> const& Targets);


        MBError p_Handle_Compile(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever & RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Export(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Retract(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Create(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
    public:
        virtual const char* GetName() override; 
        virtual CustomCommandInfo GetCustomCommands() override;
        virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) override;
        virtual MBError HandleCommand(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
        virtual void HandleHelp(CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) override;

        MBError BuildPacket(std::filesystem::path const& PacketPath,std::vector<std::string> Configs,std::vector<std::string> Targets);
        MBError ExportPacket(std::filesystem::path const& PacketPath);
        MBError RetractPacket(std::filesystem::path const& PacketPath);
    };
}
}
