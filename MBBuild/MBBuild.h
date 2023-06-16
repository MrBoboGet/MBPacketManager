#pragma once
#include "../MBPacketManager.h"
#include <MBUtility/MBInterfaces.h>
#include <mutex>
#include <stdint.h>
#include <unordered_set>
#include <vector>
#include <string>
#include <unordered_map>
#include <MBUtility/MBErrorHandling.h>
#include <filesystem>
#include <MBCLI/MBCLI.h>
#include <MBUtility/ThreadPool.h>

#include "../MBPM_CLI.h"


#include <MBUtility/Optional.h>
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

    struct ExtraLanguageInfo_Cpp
    {
        std::vector<std::string> Headers;
    };
    ExtraLanguageInfo_Cpp ParseExtraLanguageInfo_Cpp(MBParsing::JSONObject const& ObjectToParse);
    ExtraLanguageInfo_Cpp ParseExtraLanguageInfo_Cpp(MBParsing::JSONObject const& ObjectToParse,MBError& OutError);

    struct SourceInfo
    {
        std::string Language;
        std::string Standard;
        MBParsing::JSONObject ExtraLanguageInfo;
        std::vector<std::string> ExtraIncludes;
        std::vector<std::string> ExternalDependancies;
        std::unordered_map<std::string, Target> Targets;
    };

    std::vector<std::string> GetAllSources(SourceInfo const& BuildToInspect);

    struct CompileConfiguration
    {
        std::string Name;
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

    struct MBBuildPacketInfo
    {
        std::string Name;
        std::unordered_set<std::string> Attributes;
        std::vector<std::string> ExtraIncludeDirectories;
        //empty means that the linked targets defaults to the packet name, which is transparantly handled by the parser
        std::vector<std::string> DefaultLinkTargets;
    };

    MBError ParseMBBuildPacketInfo(MBPM_PacketInfo const& PacketInfo,MBBuildPacketInfo& OutInfo);
    
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
        IsUpdated = 1<<1,
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
        struct DependancyStruct
        {
            //32
            std::string Path;
            std::vector<FileID> Dependancies; 

            //Could be used with heirarchical dependancies,
            //which isn't provided by gcc
            //to determine the dependancies dependancies newest timestamp, which could in a sense 
            //cache the calculation of determining wheter or not the dependancy is updated or not.
            //This is not implemented at the moment.
            uint64_t HighestParentTime = 0;
            
            
            //not serialized
            uint64_t LastWriteTime = 0;
            bool operator==(DependancyStruct const& OtherInfo) const
            {
                bool ReturnValue = true;
                ReturnValue &= LastWriteTime == OtherInfo.LastWriteTime;
                ReturnValue &= Path == OtherInfo.Path;
                ReturnValue &= Dependancies == OtherInfo.Dependancies;
                return(ReturnValue); 
            }
            bool operator!=(DependancyStruct const& OtherInfo) const
            {
                return(!(*this  == OtherInfo));       
            }
        };
        //assumption: files only
        struct SourceDependancyInfo
        {
            uint64_t LastCompileTime = 0;   
            std::string Path = "";
            std::string CompileString = "";
            std::vector<FileID> Dependancies = {};

            bool Updated = false;
        };
        struct LinkedTargetInfo
        {
            std::string TargetName;
            uint16_t TargetType;
            std::string OutputName;
            std::string LinkOptionsString;
            uint64_t LastLinkTime = 0;
            std::vector<std::string> NeededSources;
        };
        static std::string GetGCCDependancyCommand(std::filesystem::path const& BuildRoot,std::vector<std::string> const& SourcesToCheck,std::vector<std::string> const& ExtraIncludeDirectories);
        static std::vector<MakeDependancyInfo> p_ParseMakeDependancyInfo(MBUtility::MBOctetInputStream& InStream);
        static std::vector<SourceDependancyInfo> p_NormalizeDependancies(std::vector<MakeDependancyInfo>  const& InfoToConvert,
            std::filesystem::path const& SourceRoot,std::unordered_map<std::string,FileID>& IDMap,std::vector<DependancyStruct>& OutDeps,std::vector<std::string> const& OriginalSources);
        //Convenience part 
        std::unordered_map<std::string,FileID> m_PathToDependancyIndex;
        //std::unordered_map<std::string,size_t> m_PathToSourceIndex;
        
        //Serialzied parts
        //std::vector<SourceDependancyInfo> m_Sources;
        std::unordered_map<std::string,SourceDependancyInfo> m_Sources;
        std::vector<DependancyStruct> m_Dependancies;
        
        std::unordered_map<std::string,LinkedTargetInfo> m_LinkedTargetsInfo;

        bool p_DependancyIsUpdated(uint64_t LastCompileTime,FileID Index);
        bool p_FileIsUpdated(std::filesystem::path const& SourceDirectory ,SourceDependancyInfo& Source);

        //void p_CreateSourceMap();
        void p_CreateDependancyMap();
        
        void p_WriteSourceDependancyStruct(MBUtility::MBOctetOutputStream& OutStream,SourceDependancyInfo const& StructToWrite) const;
        static SourceDependancyInfo p_ParseSourceDependancyStruct(MBUtility::MBOctetInputStream& OutStream);
        void p_WriteDependancyStruct(MBUtility::MBOctetOutputStream& OutStream, DependancyStruct const& StructToWrite) const;
        static DependancyStruct p_ParseDependancyStruct(MBUtility::MBOctetInputStream& OutStream);
        void p_WriteTargetInfo(MBUtility::MBOctetOutputStream& OutStream, LinkedTargetInfo const& StructToWrite) const;
        static LinkedTargetInfo p_ParseTargetInfo(MBUtility::MBOctetInputStream& OutStream);
    public:
        //uint32_t 
        //MBError CreateDependancyInfo('
        static MBError CreateDependancyInfo(SourceInfo const& CurrentBuild,CompileConfiguration const& CompileConfig,std::filesystem::path const& BuildRoot, std::vector<std::string> const& ExtraIncludes,DependancyInfo* OutInfo);
        static MBError ParseDependancyInfo(MBUtility::MBOctetInputStream& InputStream,DependancyInfo* OutInfo);
        void WriteDependancyInfo(MBUtility::MBOctetOutputStream& OutStream) const;
    
        bool operator==(DependancyInfo const& OtherInfo) const;
        bool operator!=(DependancyInfo const& OtherInfo) const;
       
        //TODO void RemoveRedundantDependancies();
        //TODO void GarbageCollect(SourceInfo const& SInfO);
        
        //Updated interface
        bool IsSourceOutOfDate(std::filesystem::path const& SourceDirectory,std::string const& SourceToCheck,std::string const& CompileString,MBError& OutError);
        void UpdateSource(std::string const& FileToUpdate,std::string const& CompileString); 
        bool IsTargetOutOfDate(std::string const& TargetName,uint64_t LatestDependancyTimestamp,Target const& TargetInfo,std::string const& LinkString);
        void UpdateTarget(std::string const& TargetToUpdate,Target const& TargetInfo,std::string const& LinkString); 

        MBError UpdateUpdatedFilesDependancies(std::filesystem::path const& SourceDirectory,std::vector<std::string> const& ExtraIncludes);
    };
    
    class Compiler
    {
    public:
        virtual MBError SupportsLanguage(SourceInfo const& SourcesToCheck) = 0;
        virtual bool CompileSource( CompileConfiguration const& CompileConfig,
                                    SourceInfo const& SInfo,
                                    std::string const& SourceAbsolutePath,
                                    std::string const& SourceRelativePath,
                                    std::string const& OutTopDir,
                                    std::vector<std::string> const& ExtraIncludes) const = 0;
        virtual bool LinkTarget(CompileConfiguration const& CompileConfig,
                                SourceInfo const& SInfo,
                                Target const& TargetToLink,
                                std::string const& SourceDir,
                                std::string const& OutDir,
                                std::vector<std::string> const& ExtraLinkLibraries) const = 0;
    };
    //assumes here that clang and gcc works the same way, not entirely true, and that they support the same versions.
    //TODO is there a way to dynamically check wheter or not a compiler supports C++20 etc?
    class Compiler_GCCSyntax : public Compiler
    {
        std::string m_CompilerBase = "";
        std::string p_GetCompilerName(SourceInfo const& SInfo) const;
    public:
        Compiler_GCCSyntax(std::string const& CompilerBase)
        {
            m_CompilerBase = CompilerBase;
        }
        virtual MBError SupportsLanguage(SourceInfo const& SourcesToCheck) override;
        virtual bool CompileSource( CompileConfiguration const& CompileConfig,
                                    SourceInfo const& SInfo,
                                    std::string const& SourceAbsolutePath,
                                    std::string const& SourceRelativePath,
                                    std::string const& OutTopDir,
                                    std::vector<std::string> const& ExtraIncludes) const override;
        virtual bool LinkTarget(CompileConfiguration const& CompileConfig,
                                SourceInfo const& SInfo,
                                Target const& TargetToLink,
                                std::string const& SourceDir,
                                std::string const& OutDir,
                                std::vector<std::string> const& ExtraLinkLibraries) const override;
    };
    class Compiler_MSVC : public Compiler
    {
    public: 
        virtual MBError SupportsLanguage(SourceInfo const& SourcesToCheck) override;
        virtual bool CompileSource( CompileConfiguration const& CompileConfig,
                                    SourceInfo const& SInfo,
                                    std::string const& SourceAbsolutePath, 
                                    std::string const& SourceRelativePath,
                                    std::string const& OutTopDir,
                                    std::vector<std::string> const& ExtraIncludes) const override;
        virtual bool LinkTarget(CompileConfiguration const& CompileConfig,
                                SourceInfo const& SInfo,
                                Target const& TargetToLink,
                                std::string const& SourceDir,
                                std::string const& OutDir,
                                std::vector<std::string> const& ExtraLinkLibraries) const override;
    };
   
    template<typename T>
    class AsyncStore
    {
        private:
            std::unique_ptr<std::mutex> m_Lock = std::make_unique<std::mutex>();
            std::condition_variable m_WaitCond;
            T m_Value;
            bool m_ValueStored = false;
        public:
        T const& Get()
        {
            std::unique_lock<std::mutex> Lock(*m_Lock);
            while(!m_ValueStored)
            {
                m_WaitCond.wait(Lock);
            }
            return(m_Value);
        }
        void Store(T&& ValueToStore)
        {
            std::lock_guard<std::mutex> Lock(*m_Lock);
            m_ValueStored = true;
            m_Value = ValueToStore;
            m_WaitCond.notify_all();
        }
    };
    
    template<typename KeyType,typename ValueType >
    class i_ConcurrentDict
    {
        std::mutex m_ContentMutex;
        std::unordered_map<KeyType,ValueType> m_Contents;
    public:
        bool Contains(KeyType const& Key)
        {
            std::lock_guard<std::mutex> Lock(m_ContentMutex);
            return(m_Contents.find(Key) != m_Contents.end());
        }
        void Add(KeyType const& Key)
        {
            std::lock_guard<std::mutex> Lock(m_ContentMutex);
            m_Contents[Key];
        }
        ValueType& operator[](KeyType const& Key)
        {
            std::lock_guard<std::mutex> Lock(m_ContentMutex);
            return(m_Contents[Key]);
        }
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
     

        mutable std::mutex m_ThreadPoolMutex;
        mutable std::unique_ptr<MBUtility::ThreadPool> m_ThreadPool;
        mutable i_ConcurrentDict<std::string,AsyncStore<MBError>> m_CompiledDependancies;

        //Cached, constant in regards to p_BuildLangaugeConfig
        std::unordered_map<std::string,std::unique_ptr<Compiler>> m_SupportedCompilers;
        UserConfigurationsInfo m_UserCompilationConfigs;
        DependancyConfigSpecification m_UserDependancyConfigs;
       


        struct CompilationDependancy
        {
            std::filesystem::path PacketPath;
            MBPM_PacketInfo PacketInfo;     
            MBBuildPacketInfo BuildInfo;
            SourceInfo Sources;
        };

        static std::vector<CompilationDependancy> GetCompilationDependancies(std::string const& PacketPath, MBPM_PacketInfo const& PacketInfo, PacketRetriever const& AssociatedRetriever);


        MBError p_BuildDependancy(CompilationDependancy const& DependancyToBuild,bool RebuildRecursive,std::string const& Config,std::promise<bool>& VariableToSignal) const;
        //cachable, paralellised
        //
        //Probably reqquires complete rework
        MBError p_BuildLanguageConfig(
                    std::filesystem::path const& PacketPath,
                    MBPM_PacketInfo const& PacketInfo,
                    DependancyConfigSpecification const& Dependancies,
                    CompileConfiguration const& CompileConf,
                    std::string const& CompileConfName, 
                    SourceInfo const& InfoToCompile,
                    std::vector<std::string> const& Targets,
                    bool Rebuild,
                    bool RebuildRecursive,
                    std::promise<bool>& VariableToSignal,
                    Compiler const& CompilerToUse) const;


        MBError p_Handle_Compile(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever & RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Export(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Retract(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Create(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_Handle_Verify(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);

        int p_Handle_Embedd(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
    public:
        MBBuild_Extension();
        virtual const char* GetName() override; 
        virtual CustomCommandInfo GetCustomCommands() override;
        virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) override;
        virtual MBError HandleCommand(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
        virtual int HandleTotalCommand(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
        virtual void HandleHelp(CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) override;

        MBError BuildPacket(std::filesystem::path const& PacketPath,std::vector<std::string> Configs,std::vector<std::string> Targets,CommandInfo const& Command);
    };
}
}
