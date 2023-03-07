#include "MBBuild.h"
#include "../MBPacketManager.h"
#include "../MB_PacketProtocol.h"

#include <assert.h>
#include <chrono>
#include <filesystem>
#include <ios>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <stdint.h>

#include <MBSystem/MBSystem.h>
#include <MBSystem/MBSystem.h>
#include <MBUnicode/MBUnicode.h>
#include <MBParsing/MBParsing.h>
#include <MBUtility/MBFiles.h>
#include <MBUtility/MBStrings.h>
#include <MBUtility/MBErrorHandling.h>
#include <MBUtility/MBInterfaces.h>
#include <limits>
namespace MBPM
{
    namespace MBBuild
{

    ExtraLanguageInfo_Cpp ParseExtraLanguageInfo_Cpp(MBParsing::JSONObject const& ObjectToParse)
    {
        ExtraLanguageInfo_Cpp ReturnValue;        
        if(ObjectToParse.HasAttribute("Headers"))
        {
            for(auto const& Header : ObjectToParse["Headers"].GetArrayData())
            {
                ReturnValue.Headers.push_back(Header.GetStringData());
            }
        }
        return(ReturnValue);
    }
    ExtraLanguageInfo_Cpp ParseExtraLanguageInfo_Cpp(MBParsing::JSONObject const& ObjectToParse,MBError& OutError)
    {
        ExtraLanguageInfo_Cpp ReturnValue;       
        try
        {
            ReturnValue = ParseExtraLanguageInfo_Cpp(ObjectToParse);        
        }
        catch(std::exception const& e)
        {
            OutError = false;       
            OutError.ErrorMessage = e.what();
        }
        return(ReturnValue);
    }


    std::string h_ReplaceExtension(std::string OriginalString,std::string const& NewExtension)
    {
        size_t LastDot = OriginalString.find_last_of('.');    
        if(LastDot == OriginalString.npos)
        {
            LastDot = OriginalString.size();   
        }
        OriginalString.resize(LastDot);
        OriginalString += '.';
        OriginalString += NewExtension;
        return(OriginalString);   
    }
    std::string h_CreateCompileString(CompileConfiguration const& CompileInfo)
    {
        std::string ReturnValue;
        //we cant assume that compile options are commutative
        ReturnValue += CompileInfo.Toolchain;
        ReturnValue += ' ';
        for(std::string const& Source : CompileInfo.CompileFlags)
        {
            ReturnValue += Source;
            ReturnValue += ' ';
        }
        return(ReturnValue);   
    }
    std::string h_CreateLinkString(CompileConfiguration const& CompileInfo)
    {
        std::string ReturnValue;
        for(std::string const& Flag : CompileInfo.CompileFlags)
        {
            ReturnValue += Flag;
            ReturnValue += ' ';
        } 
        return(ReturnValue);   
    }
    template<typename ClockType>
    std::chrono::time_point<std::chrono::system_clock> h_ClockToSystemClock(ClockType tp)
    {
        std::chrono::time_point<std::chrono::system_clock> ReturnValue;
        ReturnValue = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp - ClockType::clock::now() + 
                std::chrono::system_clock::now());
        return(ReturnValue);
    }
    uint64_t h_GetCurrentTime()
    {
        uint64_t ReturnValue = 0;
        const auto p1 = std::chrono::system_clock::now();
        ReturnValue = p1.time_since_epoch().count();
        return(ReturnValue);
    }
    uint64_t h_GetEntryWriteTimestamp(std::filesystem::directory_entry const& Entry)
    {
        uint64_t ReturnValue = 0;
        ReturnValue = h_ClockToSystemClock(Entry.last_write_time()).time_since_epoch().count();
        return(ReturnValue);
    }
    //MBPM Build
    CompileConfiguration ParseCompileConfiguration(MBParsing::JSONObject const& ObjectToParse)
    {
        CompileConfiguration ReturnValue;
        ReturnValue.Toolchain = ObjectToParse["Toolchain"].GetStringData();  
        if(ObjectToParse.HasAttribute("CompileFlags"))
        {
            for(MBParsing::JSONObject const& Flag : ObjectToParse["CompileFlags"].GetArrayData())
            {
                ReturnValue.CompileFlags.push_back(Flag.GetStringData());
            }   
        }
        if(ObjectToParse.HasAttribute("LinkFlags"))
        {
            for(MBParsing::JSONObject const& Flag : ObjectToParse["LinkFlags"].GetArrayData())
            {
                ReturnValue.LinkFlags.push_back(Flag.GetStringData());
            } 
        }
        return(ReturnValue);   
    }
    LanguageConfiguration ParseLanguageConfiguration(MBParsing::JSONObject const& ObjectToParse)
    {
        LanguageConfiguration ReturnValue;
		ReturnValue.DefaultExportConfig = ObjectToParse["DefaultExportConfig"].GetStringData();
        auto const& DefaultConfigs = ObjectToParse["DefaultConfigs"].GetArrayData(); 
        for(MBParsing::JSONObject const& ConfigName : DefaultConfigs)
        {
            ReturnValue.DefaultConfigs.push_back(ConfigName.GetStringData());   
        }
        auto const& Configs = ObjectToParse["CompileConfigurations"].GetMapData(); 
        for(auto const& CompileConfiguration  : Configs)
        {
            ReturnValue.Configurations[CompileConfiguration.first] = ParseCompileConfiguration(CompileConfiguration.second);
        }
        return(ReturnValue);
    }
    MBError ParseUserConfigurationInfo(const void* Data,size_t DataSize,UserConfigurationsInfo& OutInfo)
    {
        MBError ReturnValue = true;
        UserConfigurationsInfo Result;
        try 
        {
            MBParsing::JSONObject JSONData = MBParsing::ParseJSONObject(Data,DataSize,0,nullptr,&ReturnValue);
            if(!ReturnValue)
            {
                return(ReturnValue);   
            }
            auto const& ConfigMap = JSONData["LanguageConfigs"].GetMapData();
            for(auto const& Object : ConfigMap)
            {
                Result.Configurations[Object.first] = ParseLanguageConfiguration(Object.second);
            }
        }
        catch(std::exception const& Exception)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = Exception.what();    
        }
        if(ReturnValue)
        {
            OutInfo = std::move(Result);   
        }
        return(ReturnValue);        
    }
    MBError ParseUserConfigurationInfo(std::filesystem::path const& FilePath,UserConfigurationsInfo &OutInfo)
    {
        MBError ReturnValue = true;
        std::string TotalData = MBUtility::ReadWholeFile(MBUnicode::PathToUTF8(FilePath));
        ReturnValue = ParseUserConfigurationInfo(TotalData.data(),TotalData.size(),OutInfo);
        return(ReturnValue);        
    }
    //
    TargetType h_StringToTargetType(std::string const& StringToConvert)
    {
        TargetType ReturnValue = TargetType::Null;
        if(StringToConvert == "Executable")
        {
            ReturnValue = TargetType::Executable;   
        }
        else if(StringToConvert == "Library")
        {
            ReturnValue = TargetType::Library;   
        }
        else if(StringToConvert == "StaticLibrary")
        {
            ReturnValue = TargetType::StaticLibrary;
        }
        else if(StringToConvert == "DynamicLibrary")
        {
            ReturnValue = TargetType::DynamicLibrary;
        }
        else
        {
            throw std::runtime_error("Invalid target type: "+StringToConvert);
        }
        return(ReturnValue);       
    }
    MBError ParseSourceInfo(const void* Data,size_t DataSize,SourceInfo& OutInfo)
    {
        MBError ReturnValue = true;
        SourceInfo Result;
        try
        {
            MBParsing::JSONObject JSONData = MBParsing::ParseJSONObject(Data,DataSize,0,nullptr,&ReturnValue);
            if(!ReturnValue)
            {
                return(ReturnValue);   
            }
            Result.Language = JSONData["Language"].GetStringData();
            Result.Standard = JSONData["Standard"].GetStringData();
            if(JSONData.HasAttribute("ExtraIncludes"))
            {
                auto const& ExtraIncludes = JSONData["ExtraIncludes"].GetArrayData();
                for(MBParsing::JSONObject const& Include : ExtraIncludes)
                {
                    Result.ExtraIncludes.push_back(Include.GetStringData()); 
                }
            }
            auto const& Dependancies = JSONData["Dependancies"].GetArrayData();
            for(MBParsing::JSONObject const& Dependancy : Dependancies)
            {
                Result.ExternalDependancies.push_back(Dependancy.GetStringData());   
            }
            auto const& Targets = JSONData["Targets"].GetMapData();
            for(auto const& TargetData : Targets)
            {
                Target TargetInfo;
                TargetInfo.Type = h_StringToTargetType(TargetData.second["TargetType"].GetStringData());
                TargetInfo.OutputName = TargetData.second["OutputName"].GetStringData();
                for(auto const& Source : TargetData.second["Sources"].GetArrayData())
                {
                    TargetInfo.SourceFiles.push_back(Source.GetStringData());   
                }
                //normalizing sources, mainly for use in order to determine wheter or not source list are identical
                std::sort(TargetInfo.SourceFiles.begin(),TargetInfo.SourceFiles.end());
                Result.Targets[TargetData.first] = std::move(TargetInfo);
            }
            if(JSONData.HasAttribute("ExtraLanguageInfo"))
            {
                Result.ExtraLanguageInfo = JSONData["ExtraLanguageInfo"]; 
            }  
        }
        catch(std::exception const& Exception)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = Exception.what();    
        }
        if(ReturnValue)
		{
		OutInfo = std::move(Result);
		}
		return(ReturnValue);
	}
	MBError ParseSourceInfo(std::filesystem::path const& FilePath, SourceInfo& OutInfo)
	{
		MBError ReturnValue = true;
		std::string TotalData = MBUtility::ReadWholeFile(MBUnicode::PathToUTF8(FilePath));
		ReturnValue = ParseSourceInfo(TotalData.data(), TotalData.size(), OutInfo);
		return(ReturnValue);
	}
    MBError ParseMBBuildPacketInfo(MBPM_PacketInfo const& TotalPacketInfo,MBBuildPacketInfo& OutInfo)
    {
        MBError ReturnValue = true;    
        MBBuildPacketInfo Result;
        Result.Name = TotalPacketInfo.PacketName;
        if(TotalPacketInfo.TypeInfo.HasAttribute("Attributes"))
        {
            MBParsing::JSONObject const& AttributeObject = TotalPacketInfo.TypeInfo["Attributes"];
            if(AttributeObject.GetType() != MBParsing::JSONObjectType::Array)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Attributes field has to be an array of strings";
                return(ReturnValue);
            }
            auto const& VectorData = AttributeObject.GetArrayData();
            for(auto const& Attribute : VectorData)
            {
                if(Attribute.GetType() != MBParsing::JSONObjectType::String)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Attributes field has to be an array of strings";
                    return(ReturnValue);
                } 
                Result.Attributes.insert(Attribute.GetStringData());  
            }
        }
        if(TotalPacketInfo.TypeInfo.HasAttribute("DefaultLinkTargets"))
        {
            MBParsing::JSONObject const& AttributeObject = TotalPacketInfo.TypeInfo["DefaultLinkTargets"];
            if(AttributeObject.GetType() != MBParsing::JSONObjectType::Array)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "DefaultLinkTargets field has to be an array of strings";
                return(ReturnValue);
            }
            auto const& VectorData = AttributeObject.GetArrayData();
            for(auto const& Attribute : VectorData)
            {
                if(Attribute.GetType() != MBParsing::JSONObjectType::String)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "DefaultLinkTargets field has to be an array of strings";
                    return(ReturnValue);
                } 
                Result.DefaultLinkTargets.push_back(Attribute.GetStringData());  
            }
        }
        else
        {
            Result.DefaultLinkTargets.push_back(TotalPacketInfo.PacketName);   
        }
        if(TotalPacketInfo.TypeInfo.HasAttribute("ExtraIncludeDirectories"))
        {
            MBParsing::JSONObject const& AttributeObject = TotalPacketInfo.TypeInfo["ExtraIncludeDirectories"];
            if(AttributeObject.GetType() != MBParsing::JSONObjectType::Array)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "ExtraIncludeDirectories field has to be an array of strings";
                return(ReturnValue);
            }
            auto const& VectorData = AttributeObject.GetArrayData();
            for(auto const& Attribute : VectorData)
            {
                if(Attribute.GetType() != MBParsing::JSONObjectType::String)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "ExtraIncludeDirectories field has to be an array of strings";
                    return(ReturnValue);
                } 
                Result.ExtraIncludeDirectories.push_back(Attribute.GetStringData());  
            }
        }
        OutInfo = std::move(Result);
        return(ReturnValue);
    }
	//MBPM Build

    MBError CompileMBBuild_Cpp_GCC(
            std::filesystem::path const& PacketDirectory,
            std::string const& PacketInstallDirectory,
            MBBuildCompileFlags Flags,
            SourceInfo const& PacketSource,
            std::vector<std::string> const& TargetsToCompile,
            CompileConfiguration const& CompileConf,
            std::string const& ConfName)
    {
        MBError ReturnValue = true;
        std::error_code FSError;
        std::vector<std::string> TotalSources;
        std::filesystem::path PreviousDir = std::filesystem::current_path();
        for(std::string const& TargetName : TargetsToCompile)
        {
            auto const& TargetPosition = PacketSource.Targets.find(TargetName);
            if(TargetPosition == PacketSource.Targets.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid target for source info: "+TargetName;
                return(ReturnValue);
            }
            Target const& TargetInfo = TargetPosition->second;
            TotalSources.insert(TotalSources.end(),TargetInfo.SourceFiles.begin(),TargetInfo.SourceFiles.end());
        }
        std::sort(TotalSources.begin(),TotalSources.end());
        size_t NewSize = std::unique(TotalSources.begin(),TotalSources.end())-TotalSources.begin();
        std::string CompileSourcesCommand = "g++ -c -I"+PacketInstallDirectory+" ";
        //NOTE allow this part to throw, filesystem error att this point could potentially destroy the current working directory
        bool CreateResult = std::filesystem::directory_entry(PacketDirectory / ("MBPM_Builds/" + ConfName)).is_directory()||std::filesystem::create_directories(PacketDirectory / ("MBPM_Builds/" + ConfName));
        if (!CreateResult)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Failed to create direcory for packet";
            return(ReturnValue);
        }
        std::filesystem::current_path(PacketDirectory/("MBPM_Builds/"+ConfName));
        for(size_t i = 0; i < NewSize;i++)
        {
            CompileSourcesCommand += "../../"+TotalSources[i]+" "; 
        }
        for(std::string const& ExtraInclude : PacketSource.ExtraIncludes)
        {
            CompileSourcesCommand += "-I"+ExtraInclude;   
        }
        for (std::string const& ExtraArgument : CompileConf.CompileFlags)
        {
            CompileSourcesCommand += ExtraArgument+" ";
        }
        int CompileResult = std::system(CompileSourcesCommand.c_str());
        if(CompileResult != 0)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Error compiling sources for packet with configuration \""+ConfName+"\"";
            std::filesystem::current_path(PreviousDir);
            return(ReturnValue);
        }
        for(std::string const& TargetName : TargetsToCompile)
        {
            Target const& TargetInfo = PacketSource.Targets.at(TargetName);
            if(TargetInfo.Type == TargetType::Executable)
            {
                std::string CompileExecutableString = "g++ -o "+TargetName+".exe ";
                for(std::string const& SourceFile : TargetInfo.SourceFiles)
                {
                    //Extremely innefecient
                    CompileExecutableString += MBUnicode::PathToUTF8(std::filesystem::path(SourceFile.substr(1)).replace_extension("o")) + " ";
                }
                for(std::string const& Dependancy : PacketSource.ExternalDependancies)
                {
                    CompileExecutableString += "-L"+PacketInstallDirectory+"/"+Dependancy+"/MBPM_Builds/"+ConfName+" ";
                    CompileExecutableString += "-l"+Dependancy+" ";
                }
                for (std::string const& ExtraArgument : CompileConf.LinkFlags)
                {
                    CompileExecutableString += ExtraArgument + " ";
                }
                int LinkResult = std::system(CompileExecutableString.c_str());
                if(LinkResult != 0)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Error linking target: "+TargetName;   
                }
            }
            else if(TargetInfo.Type == TargetType::Library || TargetInfo.Type == TargetType::StaticLibrary)
            {
                std::string CreateLibraryCommand = "ar rcs lib"+TargetName + ".a ";
                for (std::string const& Source : TargetInfo.SourceFiles)
                {
                    //assumes that Source starts with /, canonical packet path
                    std::filesystem::path AbsolutePath = Source.substr(1);
                    AbsolutePath.replace_extension("o");
                    CreateLibraryCommand += MBUnicode::PathToUTF8(AbsolutePath)+" ";
                }
                int CreateStaticLibraryResult = std::system(CreateLibraryCommand.c_str());
                if (CreateStaticLibraryResult != 0)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Failed creating archive for target " + TargetName;
                }
            }
            else if(TargetInfo.Type == TargetType::DynamicLibrary)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid target type, dynamic library not supported yet";   
            }
            if(!ReturnValue)
            {
                std::filesystem::current_path(PreviousDir);
                return ReturnValue;
            }
        }
        std::filesystem::current_path(PreviousDir);
        return(ReturnValue);       
    }

    MBError CompileMBBuild_Cpp(
            std::filesystem::path const& PacketDirectory,
            std::string const& PacketInstallDirectory,
            MBBuildCompileFlags Flags,
            LanguageConfiguration const& LanguageConf,
            SourceInfo const& PacketSource,
            std::vector<std::string> const& TargetsToCompile,
            std::vector<std::string> const& ConfigurationsToCompile)
    {
        MBError ReturnValue = true;
        for(std::string const& Configuration : ConfigurationsToCompile)
        {
            auto const& ConfigLocation = LanguageConf.Configurations.find(Configuration);
            if(ConfigLocation == LanguageConf.Configurations.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Cannot find configuration \""+Configuration+"\""; 
                return(ReturnValue);
            } 
            CompileConfiguration const& CurrentConfig = ConfigLocation->second;
            if(CurrentConfig.Toolchain == "gcc")
            {
                ReturnValue = CompileMBBuild_Cpp_GCC(PacketDirectory,PacketInstallDirectory,Flags,PacketSource,TargetsToCompile,CurrentConfig,Configuration);
            }
            else if(CurrentConfig.Toolchain == "mbpm_cmake")
            {
                //Kinda hacky to retain backwards compatability, but redundant to compile multiple passes of mbpm_cmake as it implicitly compiles both debug and release
                //ReturnValue = MBPM::CompileMBCmake(MBUnicode::PathToUTF8(PacketDirectory), Configuration, TargetsToCompile);
            }
            else
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Unsupported toolchain value: "+CurrentConfig.Toolchain;   
            }
            if(!ReturnValue)
            {
                return(ReturnValue);
            }

        }
        return(ReturnValue);    
    }


    MBError CompileMBBuild(
            std::filesystem::path const& PacketDirectory,
            std::string const& PacketInstallDirectory,
            MBBuildCompileFlags Flags,
            LanguageConfiguration const& LanguageConf,
            SourceInfo const& PacketSource,
            std::vector<std::string> const& TargetsToCompile,
            std::vector<std::string> const& ConfigurationsToCompile)
    {
        MBError ReturnValue = true;
        if(PacketSource.Language == "C++")
        {
            ReturnValue = CompileMBBuild_Cpp(PacketDirectory,PacketInstallDirectory,Flags,LanguageConf,PacketSource,TargetsToCompile,ConfigurationsToCompile);  
        } 
        else
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Unsupported language: "+PacketSource.Language;   
        }
        return(ReturnValue);       
    }
    MBError CompileMBBuild(std::filesystem::path const& PacketPath,std::vector<std::string> Targets,std::vector<std::string> Configurations,std::string const& PacketInstallDirectory,MBBuildCompileFlags Flags)
    {
        MBError ReturnValue = true;
        if(!std::filesystem::exists(PacketInstallDirectory+"/MBCompileConfigurations.json"))
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Couldn't find MBCompileConfigurations.json in MBPM_PACKTETS_INSTALL_DIRECTORY";
            return(ReturnValue);
        }
        UserConfigurationsInfo UserConfigurations;
        ReturnValue = ParseUserConfigurationInfo(PacketInstallDirectory+"/MBCompileConfigurations.json",UserConfigurations);
        if(!ReturnValue)
        {
            return(ReturnValue);   
        }
        if(!std::filesystem::exists(PacketPath/"MBSourceInfo.json"))
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Couldn't find source MBSourceInfo.json in packet directory";
            return(ReturnValue);
        }
        SourceInfo PacketSourceInfo;
        ReturnValue = ParseSourceInfo(PacketPath/"MBSourceInfo.json",PacketSourceInfo);
        if(!ReturnValue)
        {
            return(ReturnValue);   
        }
        if(UserConfigurations.Configurations.find(PacketSourceInfo.Language) == UserConfigurations.Configurations.end())
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "No configuration for lanuage in MBSourceInfo.json: "+PacketSourceInfo.Language; 
            return(ReturnValue);
        }
        if (Targets.size() == 0)
        {
            for (auto const& CurrentTarget : PacketSourceInfo.Targets)
            {
                Targets.push_back(CurrentTarget.first);
            }
        }
        LanguageConfiguration const& PacketLanguage = UserConfigurations.Configurations.at(PacketSourceInfo.Language);
        if (Configurations.size() == 0)
        {
            Configurations = PacketLanguage.DefaultConfigs;
        }
        ReturnValue = CompileMBBuild(PacketPath,PacketInstallDirectory,Flags,PacketLanguage,PacketSourceInfo,Targets, Configurations);
        return(ReturnValue);       
    }






    

    //BEGIN DependancyInfo
    bool DependancyInfo::p_DependancyIsUpdated(uint64_t LastCompileTime,FileID Index)
    {
        bool ReturnValue = false;
        assert(Index - 1 < m_Dependancies.size());
        DependancyStruct& CurrentStruct = m_Dependancies[Index-1];
        if(CurrentStruct.LastWriteTime == 0)
        {
            std::filesystem::directory_entry FileStatus = std::filesystem::directory_entry(CurrentStruct.Path);
            if(!FileStatus.exists())
            {
                //throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Update dependancies?");
                ReturnValue = true; 
                CurrentStruct.LastWriteTime = std::numeric_limits<uint64_t>::max();
            }
            else
            {
                uint64_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
                CurrentStruct.LastWriteTime = CurrentTimestamp;
            }
        }
        if(CurrentStruct.LastWriteTime > LastCompileTime)
        {
            return(true);   
        }
        for(FileID Dependancy : CurrentStruct.Dependancies)
        {
            //potential for cache:ing
            if(p_DependancyIsUpdated(LastCompileTime,Dependancy))
            {
                ReturnValue = true;
                break;
            } 
        }
        return(ReturnValue);     
    }
    //ASSUMPTION only called once per source file for maximal efficiency
    bool DependancyInfo::p_FileIsUpdated(std::filesystem::path const& SourceDirectory ,SourceDependancyInfo& Source)
    {
        bool ReturnValue = false;
        SourceDependancyInfo& CurrentStruct = Source;
        std::filesystem::directory_entry FileStatus = std::filesystem::directory_entry(SourceDirectory/CurrentStruct.Path);
        if(!FileStatus.exists())
        {
            throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Missing source file");
        }
        uint64_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
        if(CurrentTimestamp > CurrentStruct.LastCompileTime)
        {
            CurrentStruct.LastCompileTime = CurrentTimestamp;
            return(true);
        }
        for(FileID Dependancy : CurrentStruct.Dependancies)
        {
            if(p_DependancyIsUpdated(CurrentStruct.LastCompileTime,Dependancy))
            {
                ReturnValue = true;
                break;
            } 
        }
        return(ReturnValue);     
    }
    void DependancyInfo::p_CreateDependancyMap()
    {
        m_PathToDependancyIndex.clear();
        m_PathToDependancyIndex.reserve(m_Sources.size());
        FileID i = 0;
        for(auto const& Source : m_Dependancies)
        {
            m_PathToDependancyIndex[Source.Path] = i;
            i++;
        } 
    }
    DependancyInfo::MakeDependancyInfo h_ParseMakeDependancyLine(std::string const& LineToParse)
    {
        DependancyInfo::MakeDependancyInfo ReturnValue;
        size_t FirstColonPosition = LineToParse.find(':'); 
        if(FirstColonPosition == LineToParse.npos)
        {
            throw std::runtime_error("Invalid Make rule: no colon found on target");   
        }
        ReturnValue.FileName =LineToParse.substr(0, FirstColonPosition);
        size_t FirstDotPosition = ReturnValue.FileName.find_last_of('.');
        if(FirstDotPosition == LineToParse.npos)
        {
            throw std::runtime_error("Invalid filename: No dot");
        }
        ReturnValue.FileName.replace(FirstDotPosition,4,".cpp");
        std::vector<std::string> PreDependancies = MBUtility::Split(LineToParse.substr(FirstColonPosition+1)," ");
        std::vector<std::string> Dependancies;
        Dependancies.reserve(PreDependancies.size());
        for (size_t i = 0; i < PreDependancies.size(); i++)
        {
            if (PreDependancies[i] != "")
            {
                Dependancies.push_back(PreDependancies[i]);
            }
        }

        for(size_t i = 1; i< Dependancies.size();i++)
        {
            std::string& Dependancy = Dependancies[i];
            if(Dependancy != ReturnValue.FileName)
            {
                ReturnValue.Dependancies.push_back(Dependancy);
            }
        }
        return(ReturnValue);
    }
    std::vector<DependancyInfo::MakeDependancyInfo> DependancyInfo::p_ParseMakeDependancyInfo(MBUtility::MBOctetInputStream& InputStream)
    {
        std::vector<DependancyInfo::MakeDependancyInfo> ReturnValue;
        MBUtility::LineRetriever LineRetriever(&InputStream);
        std::string CurrentLine;
        std::string CurrentDependancyInfo;
        while(LineRetriever.GetLine(CurrentLine))
        {
            if(CurrentLine.size() == 0)
            {
                continue;
            }     
            if(CurrentLine.back() == '\\')
            {
                CurrentLine.resize(CurrentLine.size()-1); 
                CurrentDependancyInfo += CurrentLine;
                continue;
            }
            if(CurrentDependancyInfo.size() == 0)
            {
                ReturnValue.push_back(h_ParseMakeDependancyLine(CurrentLine));    
            }
            else
            {
                ReturnValue.push_back(h_ParseMakeDependancyLine(CurrentDependancyInfo)); 
                CurrentDependancyInfo.resize(0);
            }
        }
        if(CurrentDependancyInfo.size() != 0)
        {
            ReturnValue.push_back(h_ParseMakeDependancyLine(CurrentDependancyInfo));   
        }
        return(ReturnValue);
    }
    std::vector<DependancyInfo::SourceDependancyInfo> DependancyInfo::p_NormalizeDependancies(std::vector<MakeDependancyInfo>  const& InfoToConvert, 
        std::filesystem::path const& SourceRoot,std::unordered_map<std::string,FileID>& IDMap,std::vector<DependancyStruct>& OutDependancies, std::vector<std::string> const& OriginalSources)
    {
        std::vector<SourceDependancyInfo> ReturnValue;
        ReturnValue.resize(InfoToConvert.size());
        size_t CurrentSourceOffset = 0;
        FileID CurrentFileID = 1;
        size_t i = 0;
        for(MakeDependancyInfo const& SourceFile : InfoToConvert)
        {
            SourceDependancyInfo NewInfo;
            NewInfo.Path = OriginalSources[i];
            //NewInfo.LastCompileTime = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(SourceRoot/ OriginalSources[i]));
            //
            //
            //
            NewInfo.LastCompileTime = 0;

            for(std::string const& Dependancy : SourceFile.Dependancies)
            {
                FileID& DependancyID = IDMap[Dependancy];
                if(DependancyID != 0)
                {
                    NewInfo.Dependancies.push_back(DependancyID);   
                    continue;
                }
                DependancyStruct NewDependancy;  
                NewDependancy.Path = MBUnicode::PathToUTF8(std::filesystem::canonical(Dependancy));
                NewDependancy.LastWriteTime = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(NewDependancy.Path));
                DependancyID = CurrentFileID;
                NewInfo.Dependancies.push_back(CurrentFileID);
                //ReturnValue.push_back(NewDependancy);
                OutDependancies.push_back(NewDependancy);
                CurrentFileID++;
            }
            ReturnValue[CurrentSourceOffset] = NewInfo;
            CurrentSourceOffset++;
            i++;
        }  
        return(ReturnValue);
    }
    std::vector<std::string> GetAllSources(SourceInfo const& BuildToInspect)
    {
        //std::vector<std::string> ReturnValue = BuildToInspect.Targets.at("MBBuild").SourceFiles;
        std::unordered_set<std::string> Sources;
        for (auto const& Target : BuildToInspect.Targets)
        {
            for (std::string const& Source : Target.second.SourceFiles)
            {
                Sources.insert(Source);
            }
        }
        std::vector<std::string> ReturnValue;
        ReturnValue.insert(ReturnValue.begin(), Sources.begin(), Sources.end());
        return(ReturnValue);
    }
    std::string DependancyInfo::GetGCCDependancyCommand(std::filesystem::path const& BuildRoot,std::vector<std::string> const& SourcesToCheck,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        std::string ReturnValue;
        //Hack
        //maby add -std=c++17?
        ReturnValue = "gcc -MM -MF - ";
        for(std::string const& String : SourcesToCheck)
        {
            ReturnValue += MBUnicode::PathToUTF8(BuildRoot)+"/"+String;
            ReturnValue += ' ';
        }
        for(std::string const& Include : ExtraIncludeDirectories)
        {
            ReturnValue += "-I";
            ReturnValue += Include;
            ReturnValue += ' ';
        }
        return(ReturnValue); 
    }
    MBError DependancyInfo::CreateDependancyInfo(SourceInfo const& CurrentBuild,CompileConfiguration const& CompileConfig,std::filesystem::path const& BuildRoot, std::vector<std::string> const& ExtraIncludes,DependancyInfo* OutInfo)
    {
        MBError ReturnValue = true;    
        DependancyInfo Result;
        //Test, assumes same working directory as build root
        //Result. = CompileConfig.CompileFlags;
        //Result.m_ToolChain = CompileConfig.Toolchain;
        std::vector<std::string> AllSources = GetAllSources(CurrentBuild);
        for (std::string& Source : AllSources)
        {
            Source = Source.substr(1);
        }
        std::string GCCCommand = GetGCCDependancyCommand(BuildRoot,AllSources,ExtraIncludes);
        //for (std::string const& SourceFile : AllSources)
        //{
        //    GCCDependancyCommand += "-MT " + SourceFile;
        //    GCCDependancyCommand += " ";
        //}
        MBSystem::UniDirectionalSubProcess SubProcess(GCCCommand,true);
        //Result.m_Sources = p_NormalizeDependancies(p_ParseMakeDependancyInfo(SubProcess),BuildRoot,Result.m_PathToDependancyIndex,Result.m_Dependancies,AllSources);
        std::vector<SourceDependancyInfo> NewSources = p_NormalizeDependancies(p_ParseMakeDependancyInfo(SubProcess),BuildRoot,Result.m_PathToDependancyIndex,Result.m_Dependancies,AllSources);
        for(SourceDependancyInfo& Source : NewSources)
        {
            std::string SourceName = Source.Path;
            //TODO is the left hand side guaranteed to be evaluated before the right hand side?
            Result.m_Sources[SourceName] = std::move(Source);   
        }
        *OutInfo = std::move(Result);
        return(ReturnValue);
    }
    bool DependancyInfo::operator==(DependancyInfo const& OtherInfo) const
    {
        return(m_Dependancies == OtherInfo.m_Dependancies);            
    }
    bool DependancyInfo::operator!=(DependancyInfo const& OtherInfo) const
    {
        return(m_Dependancies != OtherInfo.m_Dependancies);            
    }
    void h_WriteString(MBUtility::MBOctetOutputStream& OutStream,std::string const& StringToWrite)
    {
        MBParsing::WriteBigEndianInteger(OutStream,StringToWrite.size(),2); 
        OutStream.Write(StringToWrite.data(),StringToWrite.size());
    }
    std::string h_ParseString(MBUtility::MBOctetInputStream& InStream)
    {
        std::string ReturnValue;
        uint16_t StringSize = uint16_t(MBParsing::ParseBigEndianInteger(InStream,2));
        ReturnValue.resize(StringSize);
        size_t ReadBytes = InStream.Read(ReturnValue.data(),StringSize);
        if(ReadBytes < StringSize)
        {
            throw std::runtime_error("Failed to parse string: insufficient bytes left in stream");   
        }
        return(ReturnValue);    
    }
    void DependancyInfo::p_WriteSourceDependancyStruct(MBUtility::MBOctetOutputStream& OutStream,SourceDependancyInfo const& StructToWrite) const
    {
        MBParsing::WriteBigEndianInteger(OutStream,StructToWrite.LastCompileTime,8);
        h_WriteString(OutStream,StructToWrite.Path);
        h_WriteString(OutStream,StructToWrite.CompileString);
        MBParsing::WriteBigEndianInteger(OutStream,StructToWrite.Dependancies.size(),4);
        for(FileID ID : StructToWrite.Dependancies)
        {
            MBParsing::WriteBigEndianInteger(OutStream,ID,4);
        }
    }
    DependancyInfo::SourceDependancyInfo DependancyInfo::p_ParseSourceDependancyStruct(MBUtility::MBOctetInputStream& InStream)
    {
        SourceDependancyInfo ReturnValue;
        ReturnValue.LastCompileTime = MBParsing::ParseBigEndianInteger(InStream,8);
        ReturnValue.Path = h_ParseString(InStream);
        ReturnValue.CompileString = h_ParseString(InStream);
        uint32_t DependancyCount = MBParsing::ParseBigEndianInteger(InStream,4);
        if(DependancyCount > 1000000)
        {
            throw std::runtime_error("Error parsing dependancy struct: Dependancies count assumed to be under 1000000");
        }
        ReturnValue.Dependancies.reserve(DependancyCount);
        for(uint32_t i = 0; i < DependancyCount;i++)
        {
            ReturnValue.Dependancies.push_back(MBParsing::ParseBigEndianInteger(InStream,4));
        }
        return(ReturnValue);
    }
    void DependancyInfo::p_WriteTargetInfo(MBUtility::MBOctetOutputStream& OutStream, LinkedTargetInfo const& StructToWrite) const
    {
        h_WriteString(OutStream,StructToWrite.TargetName);
        MBParsing::WriteBigEndianInteger(OutStream,StructToWrite.TargetType,2);
        h_WriteString(OutStream,StructToWrite.OutputName);
        h_WriteString(OutStream,StructToWrite.LinkOptionsString);  
        MBParsing::WriteBigEndianInteger(OutStream,StructToWrite.LastLinkTime,8);
        MBParsing::WriteBigEndianInteger(OutStream,StructToWrite.NeededSources.size(),4);
        for(size_t i = 0; i < StructToWrite.NeededSources.size();i++)
        {
            h_WriteString(OutStream,StructToWrite.NeededSources[i]);
        }
    }
    DependancyInfo::LinkedTargetInfo DependancyInfo::p_ParseTargetInfo(MBUtility::MBOctetInputStream& InStream)
    {
        LinkedTargetInfo ReturnValue;
        ReturnValue.TargetName = h_ParseString(InStream); 
        ReturnValue.TargetType = uint16_t(MBParsing::ParseBigEndianInteger(InStream,2));
        ReturnValue.OutputName = h_ParseString(InStream); 
        ReturnValue.LinkOptionsString = h_ParseString(InStream); 
        ReturnValue.LastLinkTime = MBParsing::ParseBigEndianInteger(InStream,8);
        uint32_t SourcesCount = MBParsing::ParseBigEndianInteger(InStream,4);
        if(SourcesCount > 1000000)
        {
            throw std::runtime_error("Source count for target assumed to be under 1000000");   
        }
        ReturnValue.NeededSources.reserve(SourcesCount);
        for(uint32_t i = 0; i < SourcesCount;i++)
        {
            ReturnValue.NeededSources.push_back(h_ParseString(InStream));   
        }
        return(ReturnValue);
    }
    void DependancyInfo::p_WriteDependancyStruct(MBUtility::MBOctetOutputStream& OutStream, DependancyInfo::DependancyStruct const& StructToWrite) const
    {
        std::string FilePath = MBUnicode::PathToUTF8(StructToWrite.Path);
        h_WriteString(OutStream,FilePath);
        MBParsing::WriteBigEndianInteger(OutStream, StructToWrite.Dependancies.size(), 4);
        for (FileID IDToWrite : StructToWrite.Dependancies)
        {
            MBParsing::WriteBigEndianInteger(OutStream, IDToWrite, 4);
        }
    }
    DependancyInfo::DependancyStruct DependancyInfo::p_ParseDependancyStruct(MBUtility::MBOctetInputStream& InputStream)
    {
        DependancyStruct NewStruct;
        NewStruct.Path = h_ParseString(InputStream);
        uint64_t DependanciesCount = MBParsing::ParseBigEndianInteger(InputStream, 4);
        if (DependanciesCount >= 1000000)
        {
            throw std::runtime_error("Error parsing dependancy struct: Dependancies count assumed to be under 1000000");
        }
        NewStruct.Dependancies.resize(DependanciesCount);
        for (uint32_t j = 0; j < DependanciesCount; j++)
        {
            NewStruct.Dependancies[j] = FileID(MBParsing::ParseBigEndianInteger(InputStream, 4));
        }
        return(NewStruct);
    }
    MBError DependancyInfo::ParseDependancyInfo(MBUtility::MBOctetInputStream& InputStream, DependancyInfo* OutInfo)
    {
        MBError ReturnValue = true;
        DependancyInfo Result;
        try
        {
            uint32_t NumberOfSources = uint32_t(MBParsing::ParseBigEndianInteger(InputStream, 4));
            if (NumberOfSources >= 1000000)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid number of source files: assumed to be under 1 million";
            }
            Result.m_Sources.reserve(NumberOfSources);
            for (uint32_t i = 0; i < NumberOfSources; i++)
            {
                SourceDependancyInfo NewStruct = p_ParseSourceDependancyStruct(InputStream);
                Result.m_Sources[NewStruct.Path] = NewStruct;
                //Result.m_Sources.push_back(std::move(NewStruct));
            }
            uint32_t NumberOfDependancies = uint32_t(MBParsing::ParseBigEndianInteger(InputStream,4));
            if(NumberOfDependancies >= 1000000)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid number of dependancies: assumed to be under 1 million";  
            } 
            Result.m_Dependancies.reserve(NumberOfDependancies);
            for(uint32_t i = 0; i < NumberOfDependancies;i++)
            {
                DependancyStruct NewStruct = p_ParseDependancyStruct(InputStream);
                Result.m_Dependancies.push_back(std::move(NewStruct));
            }
            uint32_t TargetsCount = uint32_t(MBParsing::ParseBigEndianInteger(InputStream,4));
            if(TargetsCount >= 1000000)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid number of targets: assumed to be under 1 million";  
            } 
            for(uint32_t i = 0; i < TargetsCount;i++)
            {
                LinkedTargetInfo NewStruct = p_ParseTargetInfo(InputStream);
                Result.m_LinkedTargetsInfo[NewStruct.TargetName] = NewStruct;
            }
            Result.p_CreateDependancyMap();
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Error parsing data: ";
            ReturnValue.ErrorMessage += e.what(); 
            return(ReturnValue);
        }
        *OutInfo = std::move(Result);
        return(ReturnValue);
    }
    void DependancyInfo::WriteDependancyInfo(MBUtility::MBOctetOutputStream& OutStream) const
    {
        MBParsing::WriteBigEndianInteger(OutStream,m_Sources.size(),4);
        for(auto const& Source : m_Sources)
        {
            p_WriteSourceDependancyStruct(OutStream, Source.second);
        } 
        MBParsing::WriteBigEndianInteger(OutStream, m_Dependancies.size(), 4);
        for (DependancyStruct const& Dependancy : m_Dependancies)
        {
            p_WriteDependancyStruct(OutStream, Dependancy);
        }
        MBParsing::WriteBigEndianInteger(OutStream, m_LinkedTargetsInfo.size(), 4);
        for (auto const& Target : m_LinkedTargetsInfo)
        {
            p_WriteTargetInfo(OutStream, Target.second);
        }
    }
    MBError DependancyInfo::UpdateUpdatedFilesDependancies(std::filesystem::path const& SourceDirectory, std::vector<std::string> const& ExtraIncludes)
    {
        MBError ReturnValue = true;
        std::vector<std::string> UpdatedFiles;
        for(auto const& Source : m_Sources)
        {
            if(Source.second.Updated)
            {
                UpdatedFiles.push_back(Source.second.Path);
            }
        }
        if (UpdatedFiles.size() == 0)
        {
            return ReturnValue;
        }
        std::string GCCComand = GetGCCDependancyCommand(SourceDirectory, UpdatedFiles,ExtraIncludes);
        MBSystem::UniDirectionalSubProcess GCCProccess(GCCComand,true); 
       
        size_t CurrentDepCount = m_Dependancies.size(); 
        std::vector<SourceDependancyInfo> UpdatedInfo = p_NormalizeDependancies(p_ParseMakeDependancyInfo(GCCProccess),SourceDirectory,m_PathToDependancyIndex,m_Dependancies, UpdatedFiles);
        //for(size_t i = 0; i < AddedFiles.size();i++)
        //{
        //    //m_PathToSourceIndex[AddedFiles[i]] = m_Sources.size();
        //    //m_Sources.push_back(std::move(UpdatedInfo[i]));
        //    
        //}
        //for(size_t i = 0; i < UpdatedFiles.size();i++)
        //{
        //    m_Sources[m_PathToSourceIndex.at(UpdatedFiles[i])] = std::move(UpdatedInfo[i+AddedFiles.size()]);                
        //}
        for(SourceDependancyInfo const& Source : UpdatedInfo)
        {
            m_Sources[Source.Path].Dependancies = Source.Dependancies;   
        }
        for(size_t i = CurrentDepCount; i < m_Dependancies.size();i++)
        {
            m_PathToDependancyIndex[m_Dependancies[i].Path] = i;   
        }
        return(ReturnValue);
    }
    bool DependancyInfo::IsSourceOutOfDate(std::filesystem::path const& SourceDirectory,std::string const& SourceToCheck,std::string const& CompileString,MBError& OutError)
    {
        bool ReturnValue = false;
        //dependancy not existing can't be considred an exception, it simply means that the file has to be compiled to determine wheter or not it works
        SourceDependancyInfo& DepInfo = m_Sources[SourceToCheck.substr(1)];
        if(DepInfo.CompileString != CompileString)
        {
            return(true);       
        }
        //dependancy not existing can't be considred an exception, it simply means that the file has to be compiled to determine wheter or not it works
        ReturnValue = p_FileIsUpdated(SourceDirectory, DepInfo);
        return(ReturnValue);
    }
    void DependancyInfo::UpdateSource(std::string const& FileToUpdate,std::string const& CompileString)
    {
        SourceDependancyInfo& DependancyInfo = m_Sources[FileToUpdate.substr(1)];  
        DependancyInfo.Path = FileToUpdate.substr(1);
        DependancyInfo.Updated = true;
        DependancyInfo.CompileString = CompileString;
        DependancyInfo.LastCompileTime = h_GetCurrentTime();
        //dependancies are determined later
    }
    bool DependancyInfo::IsTargetOutOfDate(std::string const& TargetName, uint64_t LatestDependancyTimestamp,Target const& TargetInfo,std::string const& LinkString)
    {
        bool ReturnValue = false;
        auto It = m_LinkedTargetsInfo.find(TargetName);
        if(It != m_LinkedTargetsInfo.end())
        {
            LinkedTargetInfo const& StoredInfo = It->second;
            if(StoredInfo.LastLinkTime < LatestDependancyTimestamp)
            {
                return(true);
            }
            if(StoredInfo.LinkOptionsString != LinkString)
            {
                return(true);   
            }
            if(StoredInfo.NeededSources != TargetInfo.SourceFiles)
            {
                return(true);     
            }
            if(StoredInfo.OutputName != TargetInfo.OutputName)
            {
                return(true);   
            }
            if(StoredInfo.TargetType != uint16_t(TargetInfo.Type))
            {
                return(true);   
            }
            //check if the sources are present. Compiliation of sources are always done before linkage
            //so only last compilation time need to be checked
            for(auto const& Source : TargetInfo.SourceFiles)
            {
                auto const& SourceIt = m_Sources.find(Source.substr(1)); 
                if(SourceIt == m_Sources.end() || SourceIt->second.LastCompileTime > StoredInfo.LastLinkTime) 
                {
                    return(true);
                }
            }
        }
        else
        {
            ReturnValue = true;   
        }
        return(ReturnValue);
    }
    void DependancyInfo::UpdateTarget(std::string const& TargetToUpdate,Target const& TargetInfo,std::string const& LinkString)
    {
        LinkedTargetInfo& StoredTargetInfo = m_LinkedTargetsInfo[TargetToUpdate]; 
        //Could check for equivalence before assigning
        StoredTargetInfo.OutputName = TargetInfo.OutputName;
        StoredTargetInfo.TargetName = TargetToUpdate;
        StoredTargetInfo.NeededSources = TargetInfo.SourceFiles;
        StoredTargetInfo.LinkOptionsString = LinkString;
        StoredTargetInfo.TargetType = uint16_t(TargetInfo.Type);
        StoredTargetInfo.LastLinkTime = h_GetCurrentTime();
    }
    //END DependancyInfo
  
    //BEGIN MBBuildCLI 
    
    MBBuild_Extension::MBBuild_Extension()
    {
        m_SupportedCompilers["clang"] = std::make_unique<Compiler_GCCSyntax>("clang");    
        m_SupportedCompilers["gcc"] = std::make_unique<Compiler_GCCSyntax>("gcc");    
        m_SupportedCompilers["msvc"] = std::make_unique<Compiler_MSVC>();    
    }
    UserConfigurationsInfo MBBuild_Extension::p_GetGlobalCompileConfigurations()
    {
       UserConfigurationsInfo ReturnValue;
       std::filesystem::path ConfigPath = m_ConfigDirectory/"MBCompileConfigurations.json";
       MBError Result = ParseUserConfigurationInfo(ConfigPath,ReturnValue);
       if(!Result)
       {
            throw std::runtime_error("Error parsing global compile configurations: "+Result.ErrorMessage);   
       }
       return(ReturnValue);
    }
    DependancyConfigSpecification MBBuild_Extension::p_GetGlobalDependancySpecification()
    {
        DependancyConfigSpecification ReturnValue;
        std::filesystem::path ConfigPath =m_ConfigDirectory/"MBDependancySpecification.json";
        if(std::filesystem::exists(ConfigPath))
        {
            std::ifstream FileInput(ConfigPath);    
            MBUtility::MBFileInputStream InputStream(&FileInput);
            MBError ParseResult = DependancyConfigSpecification::ParseDependancyConfigSpecification(InputStream,&ReturnValue);
            if(!ParseResult)
            {
                throw std::runtime_error("Error parsing dependancy config: "+ParseResult.ErrorMessage);
            }
        }
        return(ReturnValue);    
    }
    SourceInfo MBBuild_Extension::p_GetPacketSourceInfo(std::filesystem::path const& PacketPath)
    {
        SourceInfo ReturnValue;
        MBError Result = ParseSourceInfo(PacketPath/"MBSourceInfo.json",ReturnValue);
        if(!Result)
        {
            throw std::runtime_error("Error parsing  source info: "+Result.ErrorMessage);   
        }
        return(ReturnValue); 
    }

    MBPM_PacketInfo MBBuild_Extension::p_GetPacketInfo(std::filesystem::path const& PacketPath)
    {
        MBPM_PacketInfo ReturnValue = ParseMBPM_PacketInfo(MBUnicode::PathToUTF8(PacketPath/"MBPM_PacketInfo")); 
        if(ReturnValue.PacketName == "")
        {
            throw std::runtime_error("Error parsing MBPM_PacketInfo");   
        }
        return(ReturnValue);  
    }
    DependancyConfigSpecification MBBuild_Extension::p_GetConfigSpec(std::filesystem::path const& PacketPath)
    {
        DependancyConfigSpecification  ReturnValue;
         
        return(ReturnValue); 
    }

    bool MBBuild_Extension::p_VerifyConfigs(LanguageConfiguration const& Config,std::vector<std::string> const& ConfigNames)
    {
        bool ReturnValue = true;
        for(std::string const& ConfigName : ConfigNames)
        {
            if(Config.Configurations.find(ConfigName) == Config.Configurations.end())
            {
                ReturnValue = false;
            }   
        }
        return(ReturnValue);      
    }
    bool MBBuild_Extension::p_VerifyTargets(SourceInfo const& InfoToCompile,std::vector<std::string> const& Targests)
    {
        bool ReturnValue = true;
        for(std::string const& TargetName : Targests)
        {
            if(InfoToCompile.Targets.find(TargetName)==InfoToCompile.Targets.end())
            {
                ReturnValue = false;
            }   
        }
        return(ReturnValue);
    }
    
    bool MBBuild_Extension::p_VerifyToolchain(SourceInfo const& InfoToCompile,CompileConfiguration const& CompileConfigToVerify)
    {
        bool ToolchainSupported = false; 
        auto const& ItToolchain = m_SupportedCompilers.find(CompileConfigToVerify.Toolchain);
        if(ItToolchain == m_SupportedCompilers.end())
        {
            throw std::runtime_error("Unsupported toolchain: "+CompileConfigToVerify.Toolchain);   
        }
        MBError SupportedSources = ItToolchain->second->SupportsLanguage(InfoToCompile);
        if(!SupportedSources)
        {
            throw std::runtime_error("Unsupported SourceInfo: "+SupportedSources.ErrorMessage);
        }
        return(ToolchainSupported);
    }

    std::vector<std::string> h_SourcesToObjectFiles(std::string const& SourceDir, std::vector<std::string> const& Sources, std::string const& Extension)
    {
        std::vector<std::string> ReturnValue;
        for(std::string const& Source : Sources)
        {
            ReturnValue.push_back(SourceDir+"/"+h_ReplaceExtension(Source.substr(1), Extension));
        }
        return(ReturnValue);    
    }
    std::string h_MBStandardToMSVCStandard(std::string const& Langauge,std::string const& Standard)
    {
        std::string ReturnValue = " /std:";
        if(Standard == "C++17")
        {
            ReturnValue += "c++17";   
        }
        else if(Standard == "C11")
        {
            ReturnValue += "c11";   
        }
        else if(Standard == "C99")
        {
            //a little bit bruh, but msvc doesn't seem to support c99
            ReturnValue += "c11";
        }
        else if(Standard == "C89")
        {
            //default is c89
            ReturnValue = "";
        }
        ReturnValue += " ";
        return(ReturnValue);
    }

    std::string h_MBStandardToGCCStandard(std::string const& Language,std::string const& Standard)
    {
        std::string ReturnValue = " -std=";
        if(Standard == "C++17")
        {
            ReturnValue += "c++17";  
        } 
        else if(Standard == "C99")
        {
            ReturnValue += "c99";
        }
        else if(Standard == "C89")
        {
            ReturnValue += "c89";   
        }
        ReturnValue += " ";
        return(ReturnValue); 
    }
    std::string h_GetLibraryName(std::string const& TargetName)
    {
        if constexpr(MBUtility::IsWindows())
        {
            return(TargetName+".lib");
        }
        else
        {
            return("lib"+TargetName+".a");       
        }
    }
    std::string h_GetExecutableName(std::string const& TargetName)
    {
        std::string ReturnValue = TargetName;    
        if constexpr(MBUtility::IsWindows())
        {
            ReturnValue += ".exe";   
        }
        return(ReturnValue);
    }

    MBError MBBuild_Extension::p_BuildLanguageConfig(std::filesystem::path const& PacketPath,MBPM_PacketInfo const& PacketInfo,DependancyConfigSpecification const& DependancySpec,CompileConfiguration const& CompileConf,std::string const& CompileConfName, SourceInfo const& InfoToCompile,std::vector<std::string> const& Targets,CommandInfo const& Command,Compiler& CompilerToUse)
    {
        MBError ReturnValue = true;
        MBBuildPacketInfo BuildPacketInfo;
        ReturnValue = ParseMBBuildPacketInfo(PacketInfo,BuildPacketInfo);
        if(!ReturnValue)
        {
            ReturnValue.ErrorMessage = "Error parsing build packet info: "+ReturnValue.ErrorMessage;   
            return(ReturnValue);
        }
        //Create build info   
        DependancyInfo SourceDependancies; 
        std::vector<std::string> TotalSources = GetAllSources(InfoToCompile);
        std::filesystem::path BuildFilesDirectory = PacketPath/("MBPM_BuildFiles/"+CompileConfName);
        std::filesystem::path PreviousWD = std::filesystem::current_path();
        std::filesystem::current_path(PacketPath);
        bool RebuildAll = Command.Flags.find("rebuild") != Command.Flags.end();
        if(!std::filesystem::exists(BuildFilesDirectory))
        {
            std::filesystem::create_directories(BuildFilesDirectory);
        }
        std::filesystem::path DependancyInfoPath = BuildFilesDirectory/"MBBuildDependancyInfo";
        try
        {
            std::string InstallDirectory = GetSystemPacketsDirectory();
            std::vector<std::string> ExtraIncludes = {InstallDirectory};
            for(std::string const& Include : BuildPacketInfo.ExtraIncludeDirectories)
            {
                ExtraIncludes.push_back("."+Include);   
            }
            PacketIdentifier LocalPacket;
            LocalPacket.PacketName = PacketInfo.PacketName;
            LocalPacket.PacketURI = "./";
            LocalPacket.PacketLocation = PacketLocationType::Local;
            //TODO use the source info instead of the MBPM_PacketInfo
            std::vector<PacketIdentifier> TotalDependancies = m_AssociatedRetriever->GetPacketDependancies(LocalPacket,ReturnValue);
            if(!ReturnValue)
            {
                throw std::runtime_error("Failed retrieving packet dependancies: "+ReturnValue.ErrorMessage);
            }
            std::vector<MBPM_PacketInfo> DependancyInfos;
            std::vector<MBBuildPacketInfo> BuildInfos;
            for(PacketIdentifier const& Packet : TotalDependancies)
            {
                //std::cout<<"Dependancies "<<Packet.PacketName<<std::endl;
                MBPM_PacketInfo DependancyInfo = m_AssociatedRetriever->GetPacketInfo(Packet);
                MBBuildPacketInfo DependancyBuildInfo;
                ReturnValue = ParseMBBuildPacketInfo(DependancyInfo,DependancyBuildInfo);
                if(!ReturnValue)
                {
                    throw std::runtime_error("Error parsing dependancy packet build info: "+ReturnValue.ErrorMessage);
                }
                for(std::string const& IncludePath : DependancyBuildInfo.ExtraIncludeDirectories)
                {
                    ExtraIncludes.push_back(InstallDirectory+"/"+DependancyInfo.PacketName+"/"+IncludePath);
                }
                DependancyInfos.push_back(std::move(DependancyInfo));
                BuildInfos.push_back(std::move(DependancyBuildInfo));
            }

            MBError DependancyInfoResult = true;
            if (!std::filesystem::exists(DependancyInfoPath) || RebuildAll)
            {
                DependancyInfoResult = DependancyInfo::CreateDependancyInfo(InfoToCompile, CompileConf, PacketPath,ExtraIncludes, &SourceDependancies);
            }
            else
            {
                std::ifstream FileInput = std::ifstream(DependancyInfoPath, std::ios::in | std::ios::binary);
                MBUtility::MBFileInputStream  InputStream(&FileInput);
                DependancyInfoResult = DependancyInfo::ParseDependancyInfo(InputStream, &SourceDependancies);
                //DependancyInfoResult = DependancyInfo::UpdateDependancyInfo(InfoToCompile,PacketPath,&SourceDependancies);
            }
            if (!DependancyInfoResult)
            {
                throw std::runtime_error("Error creating dependancy info when compiling packet for configuration \"" + CompileConfName + "\": " + DependancyInfoResult.ErrorMessage);
            }
            //TODO fix
            std::string CompileString = h_CreateCompileString(CompileConf);
            std::string OutSourceDir = MBUnicode::PathToUTF8(PacketPath/"MBPM_BuildFiles"/CompileConfName)+"/";
            bool CompilationResult = true;
            bool CompilationNeeded = false;
            for (std::string const& Source : TotalSources)
            {
                if(!SourceDependancies.IsSourceOutOfDate(PacketPath,Source,CompileString,DependancyInfoResult) && !RebuildAll)
                {
                    continue;    
                }
                CompilationNeeded = true;
                CompilationResult = CompilerToUse.CompileSource(CompileConf,InfoToCompile,Source,OutSourceDir,ExtraIncludes);
                if(!CompilationResult)
                {
                    break;   
                }
                SourceDependancies.UpdateSource(Source,CompileString);
            }
            if(!CompilationResult)
            {
                //kinda ugly
                throw std::runtime_error("Error compiling sources");
            }
            //Link step
            std::vector<std::string> ExtraLibraries = {};
            std::reverse(DependancyInfos.begin(),DependancyInfos.end());
            std::reverse(BuildInfos.begin(),BuildInfos.end());
            std::string LinkString = h_CreateLinkString(CompileConf);
            std::string DependancyString;
            for(int i = 0; i < DependancyInfos.size();i++)
            {
                MBPM_PacketInfo& CurrentInfo = DependancyInfos[i];
                MBBuildPacketInfo& CurrentBuildInfo = BuildInfos[i];
                DependancyString += CurrentInfo.PacketName;
                DependancyString += ' ';
                std::string PacketConfig = DependancySpec.GetDependancyConfig(CurrentInfo,CompileConfName);
                for(auto const& LinkTarget : CurrentBuildInfo.DefaultLinkTargets)
                {
                    ExtraLibraries.push_back(InstallDirectory+ CurrentInfo.PacketName +"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(LinkTarget));
                }
            }
            uint64_t LatestDependancyUpdate = 0;
            for(std::string const& DependantLibrary : ExtraLibraries)
            {
                std::filesystem::directory_entry Entry = std::filesystem::directory_entry(DependantLibrary);     
                if(Entry.exists() == false)
                {
                    throw std::runtime_error("Error finding library dependancy: "+DependantLibrary);
                }
                uint64_t NewTimestamp = h_GetEntryWriteTimestamp(Entry);
                if(NewTimestamp > LatestDependancyUpdate)
                {
                    LatestDependancyUpdate = NewTimestamp;
                }
            }
            for(std::string const& TargetName : Targets)
            {
                bool ShouldLink = false;
                Target const& CurrentTarget =InfoToCompile.Targets.at(TargetName);
                if(CompilationNeeded || SourceDependancies.IsTargetOutOfDate(TargetName,LatestDependancyUpdate,CurrentTarget,LinkString) || RebuildAll)
                {
                    std::string OutDir = "MBPM_Builds/"+CompileConfName+"/";
                    bool Result = CompilerToUse.LinkTarget(CompileConf, InfoToCompile,CurrentTarget,OutSourceDir,OutDir,ExtraLibraries);
                    if(!Result)
                    {
                        throw std::runtime_error("Error linking library");
                    }
                    else
                    {
                        SourceDependancies.UpdateTarget(TargetName,CurrentTarget, LinkString);
                    }
                }
            } 
            MBError UpdateDependanciesResult = SourceDependancies.UpdateUpdatedFilesDependancies(PacketPath,ExtraIncludes);
            if(!UpdateDependanciesResult)
            {
                throw std::runtime_error("Error updating dependancy info after compilation: "+UpdateDependanciesResult.ErrorMessage); 
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Error compiling packet: "+std::string(e.what());        
        }
        std::filesystem::current_path(PreviousWD);
        std::ofstream DependancyFile = std::ofstream(DependancyInfoPath,std::ios::out|std::ios::binary);
        MBUtility::MBFileOutputStream DependancyOutputStream(&DependancyFile);
        SourceDependancies.WriteDependancyInfo(DependancyOutputStream);
        DependancyOutputStream.Flush();
        return(ReturnValue);
    }
    MBError MBBuild_Extension::BuildPacket(std::filesystem::path const& PacketPath,std::vector<std::string> Configs,std::vector<std::string> Targets,CommandInfo const& Command)
    {
        MBError ReturnValue = true;
        try
        {
            SourceInfo BuildInfo = p_GetPacketSourceInfo(PacketPath);
            UserConfigurationsInfo CompileConfigurations = p_GetGlobalCompileConfigurations();
            bool TargetsAreValid = p_VerifyTargets(BuildInfo,Targets);
            MBPM_PacketInfo PacketInfo = p_GetPacketInfo(PacketPath);
            if(!TargetsAreValid)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid targets for compilation: Target not found in MBSourceInfo.json";
                return(ReturnValue);
            }
            auto const& ConfIt = CompileConfigurations.Configurations.find(BuildInfo.Language);
            if(ConfIt == CompileConfigurations.Configurations.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "No compile configurations exist for language: "+BuildInfo.Language;
                return(ReturnValue);
            }
            LanguageConfiguration const& LanguageInfo = ConfIt->second;
            bool ConfigsAreValid = p_VerifyConfigs(LanguageInfo,Configs);
            if(!ConfigsAreValid)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid config: Configuration not found in language configurations";
                return(ReturnValue);
            }
            DependancyConfigSpecification DepConf = p_GetGlobalDependancySpecification();
            if(Configs.size() == 0)
            {
                Configs = LanguageInfo.DefaultConfigs;
            }
            if(Targets.size() == 0)
            {
                Targets.reserve(BuildInfo.Targets.size());
                for(auto const& Target : BuildInfo.Targets)
                {
                    Targets.push_back(Target.first);   
                }
            }
            for(std::string const& Config : Configs)
            {
                ReturnValue = p_BuildLanguageConfig(PacketPath,PacketInfo,DepConf,LanguageInfo.Configurations.at(Config),Config,BuildInfo,Targets,Command
                        , *m_SupportedCompilers[LanguageInfo.Configurations.at(Config).Toolchain]);
                if (!ReturnValue)
                {
                    return(ReturnValue);
                }
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what(); 
        } 
        return(ReturnValue);    
    }
    //ugly as it is more or less a carbon copy of the function above
    const char* MBBuild_Extension::GetName()
    {
        return("MBBuild"); 
    }
    CustomCommandInfo MBBuild_Extension::GetCustomCommands()
    {
        CustomCommandInfo ReturnValue; 
        CustomCommand CompileCommand;
        CompileCommand.Name = "compile";
        CompileCommand.Type = CommandType::TopCommand;
        CompileCommand.SupportedTypes = {"C","C++"};
        CustomCommand CreateCommand;
        CreateCommand.Name = "create";
        CreateCommand.Type = CommandType::SubCommand;
        CreateCommand.SupportedTypes = {"C","C++"};
        CustomCommand ExportCommand;
        ExportCommand.Name = "export";
        ExportCommand.Type = CommandType::TopCommand;
        ExportCommand.SupportedTypes = {"C","C++"};
        CustomCommand RetractCommand;
        RetractCommand.Name = "retract";
        RetractCommand.Type = CommandType::TopCommand;
        RetractCommand.SupportedTypes = {"C","C++"};
        CustomCommand VerifyCommand;
        RetractCommand.Name = "verify";
        RetractCommand.Type = CommandType::TopCommand;
        RetractCommand.SupportedTypes = {"C","C++"};
        CustomCommand MBBuildCommand;
        MBBuildCommand.Type = CommandType::TotalCommand;
        MBBuildCommand.Name = "mbbuild";//should maybe standardize so that every extension has an appropriate top command with it's name?

        ReturnValue.Commands = {CompileCommand,CreateCommand,ExportCommand,RetractCommand,VerifyCommand,MBBuildCommand};
        return(ReturnValue);
    }
    void MBBuild_Extension::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError)
    {
        m_ConfigDirectory =  ConfigurationDirectory; 
        //Should maybe read the contents of the config to make sure it's valid, a question of wheter or not
        //failure should be noticed at runtime for specific packets or not
    }
    int MBBuild_Extension::p_Handle_Embedd(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        int ReturnValue = 0;
        if(CommandToHandle.Arguments.size() == 0)
        {
            AssociatedTerminal.PrintLine("embedd requires file to embedd as the first argument");
            return(1);
        }
        std::string VariableName;
        std::string OutName;
        auto const& OutNameIt = CommandToHandle.SingleValueOptions.find("o");
        auto const& VariableNameIt = CommandToHandle.SingleValueOptions.find("v");
        if(!std::filesystem::exists(CommandToHandle.Arguments[0]))
        {
            AssociatedTerminal.PrintLine("File to embedd doesn't exist"); 
            return(1);
        }
        std::ifstream InputFile = std::ifstream(CommandToHandle.Arguments[0],std::ios::in|std::ios::binary);
        if(!InputFile.is_open())
        {
            AssociatedTerminal.PrintLine("Error opening input file");
            return(1);
        }
        if(OutNameIt == CommandToHandle.SingleValueOptions.end() || OutNameIt->second.size() > 1)
        {
            AssociatedTerminal.PrintLine("embedd requires exactly 1 output filename as a single value option");   
            return(1);
        }
        OutName = OutNameIt->second[0];
        if(VariableNameIt == CommandToHandle.SingleValueOptions.end() || VariableNameIt->second.size() > 1)
        {
            AssociatedTerminal.PrintLine("embedd requires exactly 1 variable name given as a single value option");   
            return(1);
        }
        VariableName = VariableNameIt->second[0];

        std::ofstream OutHeader = std::ofstream(OutName+".h");
        if(!OutHeader.is_open())
        {
            AssociatedTerminal.PrintLine("Failed opening file \""+OutName+".h\"");
            return(1);
        }
        OutHeader<<"#pragma once\n#include <cstddef>\nextern const char* "+VariableName+";\n extern size_t "+VariableName+"_size;";

        std::ofstream OutSource = std::ofstream(OutName+".cpp",std::ios::out|std::ios::binary);
        //TODO make algorithm acutally able guarantee embedding, this only works most of the time
        //NOTE while std::fileystem filesize will not work for files >4g on for example rasberry pi so is this fine,
        //no one wants to load a 4g executable into memory
        //TODO calculate the odds for 16 bytes string a appearing in file of size x where we assume the file contents to be 
        //completely randomiz
        OutSource<<"#include <cstddef>\n";
        OutSource<<"const char* "+VariableName+" = \"\"\n";
        uintmax_t TotalSize = 0;
        constexpr size_t ReadChunkSize = 400;
        char CharBuffer[ReadChunkSize];
        while(true)
        {
            OutSource<<"\"";
            size_t ReadBytes = InputFile.read(CharBuffer,ReadChunkSize).gcount();
            TotalSize += ReadBytes;
            for(size_t i = 0; i < ReadBytes;i++)
            {
                uint8_t CurrentByte = uint8_t(CharBuffer[i]);
                OutSource<<"\\x"+MBUtility::HexEncodeByte(CurrentByte);
                //if(CurrentByte < 32 || CurrentByte == '"' || CurrentByte == '\\' || CurrentByte > 127)
                //{
                //    std::string OctalString = std::to_string(CurrentByte);
                //    OctalString = std::string(3-OctalString.size(),'0')+OctalString;
                //    OutSource << "\\"+OctalString;
                //}
                //else
                //{
                //    OutSource<<char(CurrentByte);
                //}
            }
            OutSource<<"\"\n";
            if(ReadBytes < ReadChunkSize)
            {
                break;
            }
        }
        OutSource<<";\n";
        OutSource<<"size_t "+VariableName+"_size="+std::to_string(TotalSize)+";\n";
        return(ReturnValue);
    }
    int MBBuild_Extension::HandleTotalCommand(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        int ReturnValue = 0;       
        //only mbbuild registered
        if(CommandToHandle.Arguments.size() == 0)
        {
            AssociatedTerminal.PrintLine("mbbuild requires top command");
            return(1);
        }
        if(CommandToHandle.Arguments[0] == "embedd")
        {
            return(p_Handle_Embedd(CommandToHandle.GetSubCommand(),RetrieverToUse,AssociatedTerminal));
        }
        else 
        {
            AssociatedTerminal.PrintLine("Invalid top command for mbbuild: \""+CommandToHandle.Arguments[0]+"\"");
            return(1);
        }
        return(ReturnValue);
    }
    MBError MBBuild_Extension::HandleCommand(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        if(CommandToHandle.CommandName == "compile")
        {
            ReturnValue = p_Handle_Compile(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal); 
        }       
        else if(CommandToHandle.CommandName == "export")
        {
            ReturnValue = p_Handle_Export(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal);
        }
        else if(CommandToHandle.CommandName == "create")
        {
            ReturnValue = p_Handle_Create(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal);
        }
        else if(CommandToHandle.CommandName == "retract")
        {
            ReturnValue = p_Handle_Retract(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal);
        }
        else if(CommandToHandle.CommandName == "verify")
        {
            ReturnValue = p_Handle_Verify(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal);
        }
        return(ReturnValue);
    }
    void MBBuild_Extension::HandleHelp(CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal)
    {
        AssociatedTerminal.PrintLine("MBBuild: compile code the good MBG way");
    }
    MBError MBBuild_Extension::p_Handle_Compile(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever & RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        std::vector<std::string> Targets; 
        std::vector<std::string> Configurations;
        auto TargetsIt = CommandToHandle.SingleValueOptions.find("t");
        if(TargetsIt != CommandToHandle.SingleValueOptions.end())
        {
            for(std::string const& Target : TargetsIt->second)
            {
                Targets.push_back(Target);   
            } 
        }
        auto ConfigurationsIt = CommandToHandle.SingleValueOptions.find("c");
        if(ConfigurationsIt != CommandToHandle.SingleValueOptions.end())
        {
            for(std::string const& Configuration : ConfigurationsIt->second)
            {
                Configurations.push_back(Configuration);   
            } 
        }
        ReturnValue =  BuildPacket(PacketToHandle.PacketURI,Configurations,Targets,CommandToHandle);
        return(ReturnValue);    
    }
    MBError MBBuild_Extension::p_Handle_Export(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        try
        {
            SourceInfo BuildInfo = p_GetPacketSourceInfo(PacketToHandle.PacketURI);
            UserConfigurationsInfo CompileConfigurations = p_GetGlobalCompileConfigurations();
            std::string ExecutablesDirectory = MBPM::GetSystemPacketsDirectory()+"/MBPM_ExportedExecutables/";
            std::string LibraryDirectory = MBPM::GetSystemPacketsDirectory()+"/MBPM_CompiledLibraries/";
            auto const& ConfigIt = CompileConfigurations.Configurations.find(BuildInfo.Language);
            if(ConfigIt == CompileConfigurations.Configurations.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Language not supported in compile config: "+BuildInfo.Language;
                return(ReturnValue);
            }
            LanguageConfiguration const& LanguageConfig = ConfigIt->second;
            std::string ExportConfiguration = LanguageConfig.DefaultExportConfig;
            auto const& ItExportOverride = CommandToHandle.SingleValueOptions.find("c");
            if(ItExportOverride != CommandToHandle.SingleValueOptions.end())
            {
                if(ItExportOverride->second.size() > 1)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Only one config can be exported at a time";   
                    return(ReturnValue);
                }
                ExportConfiguration = ItExportOverride->second[0];
            }
            for(auto const& Target : BuildInfo.Targets)
            {
                std::string TargetFilename;
                std::string OutDir;
                if(Target.second.Type == TargetType::Executable)
                {
                    OutDir = ExecutablesDirectory;
                    TargetFilename = h_GetExecutableName(Target.second.OutputName);   
                }
                else if(Target.second.Type == TargetType::StaticLibrary || Target.second.Type == TargetType::Library)
                {
                    OutDir = LibraryDirectory;
                    TargetFilename = h_GetLibraryName(Target.second.OutputName);        
                }
                else
                {
                    throw std::runtime_error("Unsupported target type: DynamicLibrary");
                }
                std::filesystem::path PacketFile = std::filesystem::canonical(PacketToHandle.PacketURI+"/MBPM_Builds/"+ExportConfiguration+"/"+TargetFilename);
                if(!std::filesystem::exists(PacketFile))
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Can't find target to export: "+MBUnicode::PathToUTF8(PacketFile);
                    return(ReturnValue);
                }
                std::filesystem::path LinkPath = OutDir+TargetFilename;
                if(std::filesystem::exists(LinkPath) || std::filesystem::is_symlink(LinkPath))
                {
                    std::filesystem::remove(LinkPath);   
                }
                std::filesystem::create_symlink(PacketFile,LinkPath);
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what(); 
        } 
        return(ReturnValue);    
   }
    MBError MBBuild_Extension::p_Handle_Retract(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        try
        {
            SourceInfo BuildInfo = p_GetPacketSourceInfo(PacketToHandle.PacketURI);
            UserConfigurationsInfo CompileConfigurations = p_GetGlobalCompileConfigurations();
            std::string ExecutablesDirectory = MBPM::GetSystemPacketsDirectory()+"/MBPM_ExportedExecutables/";
            std::string LibraryDirectory = MBPM::GetSystemPacketsDirectory()+"/MBPM_CompiledLibraries/";
            auto const& ConfigIt = CompileConfigurations.Configurations.find(BuildInfo.Language);
            if(ConfigIt == CompileConfigurations.Configurations.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Language not supported in compile config: "+BuildInfo.Language;
                return(ReturnValue);
            }
            LanguageConfiguration const& LanguageConfig = ConfigIt->second;
            for(auto const& Target : BuildInfo.Targets)
            {
                std::string TargetFilename;
                std::string OutDir;
                if(Target.second.Type == TargetType::Executable)
                {
                    OutDir = ExecutablesDirectory;
                    TargetFilename = h_GetExecutableName(Target.second.OutputName);   
                }
                else if(Target.second.Type == TargetType::StaticLibrary || Target.second.Type == TargetType::Library)
                {

                    OutDir = LibraryDirectory;
                    TargetFilename = h_GetLibraryName(Target.second.OutputName);        
                }
                else
                {
                    throw std::runtime_error("Unsupported target type: DynamicLibrary");
                }
                std::filesystem::path LinkPath = OutDir+TargetFilename;
                if(std::filesystem::exists(LinkPath) || std::filesystem::is_symlink(LinkPath))
                {
                    std::filesystem::remove(LinkPath);   
                }
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what(); 
        } 
        return(ReturnValue);
    }
    MBError MBBuild_Extension::p_Handle_Verify(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        SourceInfo LocalSourceInfo;
        ReturnValue = ParseSourceInfo(PacketToHandle.PacketURI+"/MBSourceInfo.json",LocalSourceInfo);
        if(!ReturnValue)
        {
            return(ReturnValue);
        }
        std::vector<std::string> AllSources = GetAllSources(LocalSourceInfo);
        try
        {
            for(std::string const& Source : AllSources)
            {
                if(!std::filesystem::exists(PacketToHandle.PacketURI+"/"+Source))
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Source \""+Source+"\" doesn't exists";
                    return(ReturnValue);
                }
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Error checking sources: "+std::string(e.what());
        }
        return(ReturnValue);    
    }
    MBError MBBuild_Extension::p_Handle_Create(CommandInfo const& CommandToHandle,PacketIdentifier const& Packet,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        if(CommandToHandle.Arguments[0] == "sourceinfo")
        {
            MBPM::MBPM_FileInfoExcluder Excluder(Packet.PacketURI+"/MBPM_FileInfoIgnore");
            MBPM_PacketInfo PacketInfo = RetrieverToUse.GetPacketInfo(Packet);
            MBBuildPacketInfo BuildPacketInfo;
            ReturnValue = ParseMBBuildPacketInfo(PacketInfo,BuildPacketInfo);
            if(!ReturnValue) return(ReturnValue);
            //ASSUMPTION only comes here on existing packets, file system dataraces are reported as exceptions
            std::filesystem::recursive_directory_iterator PacketDirectoryIterator(Packet.PacketURI);
            MBParsing::JSONObject SourceInfo(MBParsing::JSONObjectType::Aggregate);
            SourceInfo["Language"] = "C++";
            SourceInfo["Standard"] = "C++17";
            SourceInfo["ExtraIncludes"] = BuildPacketInfo.ExtraIncludeDirectories; 
            //SourceInfo["Dependancies"] = PacketInfo.PacketDependancies;
            SourceInfo["Dependancies"] = PacketInfo.PacketDependancies;
            std::vector<std::string> AllSources;
            for(auto const& Entry : PacketDirectoryIterator)
            {
                std::string PacketPath ="/"+MBUtility::ReplaceAll(MBUnicode::PathToUTF8(std::filesystem::relative(Entry.path(),Packet.PacketURI)),"\\","/");
                if(Excluder.Excludes(PacketPath) && !Excluder.Includes(PacketPath))
                {
                    if (Entry.is_directory()) 
                    {
                        PacketDirectoryIterator.disable_recursion_pending();
                    }
                    continue;   
                }
                if(Entry.path().extension() == ".cpp" || Entry.path().extension() == "c")
                {
                    AllSources.push_back(PacketPath);   
                }
            }
            MBParsing::JSONObject Targets(MBParsing::JSONObjectType::Aggregate);
            MBParsing::JSONObject LibTarget(MBParsing::JSONObjectType::Aggregate);
            LibTarget["TargetType"] = "Library";
            LibTarget["Sources"] = AllSources;
            LibTarget["OutputName"] = PacketInfo.PacketName;
            Targets[Packet.PacketName] = std::move(LibTarget);
            SourceInfo["Targets"] = std::move(Targets);

            bool FileExist = std::filesystem::exists(Packet.PacketURI + "/MBSourceInfo.json");

            if(FileExist && CommandToHandle.Flags.find("newonly") != CommandToHandle.Flags.end())
            {
                return(ReturnValue);
            }
            bool OverrideAll = CommandToHandle.Flags.find("override") != CommandToHandle.Flags.end();
            if (!OverrideAll && FileExist)
            {
                std::string Response;
                bool ContinueTop = false;
                while (true)
                {
                    AssociatedTerminal.PrintLine("MBSourceInfo.json already exists for " + Packet.PacketURI + ". Do you want to override it? [y/n/all]");
                    AssociatedTerminal.GetLine(Response);
                    if (Response == "y")
                    {
                        break;
                    }
                    else if (Response == "n")
                    {
                        ContinueTop = true;
                        break;
                    }
                    else if (Response == "all")
                    {
                        OverrideAll = true;
                        break;
                    }
                }
                if (ContinueTop)
                {
                    return(ReturnValue);
                }
            }
            std::ofstream OutFile = std::ofstream(Packet.PacketURI+"/MBSourceInfo.json");
            OutFile<<SourceInfo.ToPrettyString();

        }
        else if(CommandToHandle.Arguments[0] == "makefile")
        {
            MBPM::MBPM_FileInfoExcluder Excluder(Packet.PacketURI+"/MBPM_FileInfoIgnore");

        }
        else if(CommandToHandle.Arguments[0] == "cmake")
        {
            std::filesystem::path SourceInfoPath= Packet.PacketURI+"/MBSourceInfo.json";
            if(!std::filesystem::exists(SourceInfoPath))
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "create cmake for packet requries existing MBSourceInfo.json"; 
                return(ReturnValue);
            }
            SourceInfo LocalSourceInfo;
            ReturnValue = ParseSourceInfo(SourceInfoPath,LocalSourceInfo);
            if(!ReturnValue) return(ReturnValue);
            MBPM_PacketInfo CurrentPacketInfo = RetrieverToUse.GetPacketInfo(Packet);
            std::string DependanciesConfig = "Debug";
            std::filesystem::path OutFilePath = Packet.PacketURI+"/CMakeLists.txt";
            if(CommandToHandle.Flags.find("override") == CommandToHandle.Flags.end() && std::filesystem::exists(OutFilePath))
            {
                if(CommandToHandle.Flags.find("newonly") != CommandToHandle.Flags.end())
                {
                    return(ReturnValue);   
                }
                AssociatedTerminal.PrintLine("CMakeLists.txt already exists for packet. Do you want to override it? (y/n)");
                while(true)
                {
                    std::string Input;
                    AssociatedTerminal.GetLine(Input);
                    if(Input == "y")
                    {
                        break;
                    }
                    else if(Input == "n")
                    {
                        return(ReturnValue);   
                    }
                }
            }
            std::vector<PacketIdentifier> TotalDependancies = RetrieverToUse.GetPacketDependancies(Packet,ReturnValue);
            if(!ReturnValue) return(ReturnValue);
            std::vector<MBBuildPacketInfo> DependancyBuildInfo;
            std::string MBPM_IMPORTED_INCLUDE_DIRS = "set(MBPM_IMPORTED_INCLUDE_DIRS\n";
            for(auto const& Dependancy : TotalDependancies)
            {
                MBPM_PacketInfo DependancyInfo = RetrieverToUse.GetPacketInfo(Dependancy);
                MBBuildPacketInfo DependancyBuild;
                ReturnValue = ParseMBBuildPacketInfo(DependancyInfo,DependancyBuild);
                if(!ReturnValue)
                {
                    return(ReturnValue);   
                }
                for(auto const& LinkDir : DependancyBuild.ExtraIncludeDirectories)
                {
                     MBPM_IMPORTED_INCLUDE_DIRS += "    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"+DependancyInfo.PacketName+"/"+LinkDir+"\n";
                }
                DependancyBuildInfo.push_back(std::move(DependancyBuild));
            }
            MBPM_IMPORTED_INCLUDE_DIRS += ")\n";
            std::reverse(DependancyBuildInfo.begin(),DependancyBuildInfo.end());
            std::string TotalCmakeData = "";
            TotalCmakeData += "project("+CurrentPacketInfo.PacketName+")\n";
            TotalCmakeData += "set(TOTAL_MBPM_PACKET_LIBRARIES\n";
            for(MBBuildPacketInfo const& BuildInfo : DependancyBuildInfo)
            {
                //4 spaces
                for(std::string const& LinkTarget : BuildInfo.DefaultLinkTargets)
                {
                    TotalCmakeData += "    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"+BuildInfo.Name+"/MBPM_Builds/"+DependanciesConfig+"/"+h_GetLibraryName(LinkTarget)+"\n";
                }
            }
            TotalCmakeData += ")\n";
            TotalCmakeData += MBPM_IMPORTED_INCLUDE_DIRS;
            TotalCmakeData += "set(SYSTEM_LIBRARIES\n";
            if constexpr(MBUtility::IsWindows())
            {
                TotalCmakeData += "Ws2_32.lib\n"
                    "Secur32.lib\n"
                    "Bcrypt.lib\n"
                    "Mfplat.lib\n"
                    "opengl32\n"
                    "Mfuuid.lib\n	"
                    "Strmiids.lib\n";
            }
            //ASSUMPTION !MBWindows -> IsPOSIX
            else
            {
                TotalCmakeData += "    dl\n    m    \n    pthread\n    stdc++fs\n";       
            }
            TotalCmakeData += ")\n";
            
            //creating sources
            for(auto const& Target : LocalSourceInfo.Targets)
            {
                TotalCmakeData += "set(SOURCES_"+Target.first+"\n";    
                for(std::string const& Source : Target.second.SourceFiles)
                {
                    TotalCmakeData += std::string(4,' ')+Source.substr(1)+"\n";
                }
                TotalCmakeData += ")\n";
            }
            //creating cmake targets
            for(auto const& Target : LocalSourceInfo.Targets)
            {

                if(Target.second.Type == TargetType::Executable)
                {
                    TotalCmakeData += "add_executable("+Target.first+" ${SOURCES_"+Target.first+"})\n";
                }
                //TODO add support for multiple library types
                else if(Target.second.Type == TargetType::Library)
                {
                    TotalCmakeData += "add_library("+Target.first+" STATIC ${SOURCES_"+Target.first+"})\n";
                }
                else
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "unsuported library type for cmake";   
                    return(ReturnValue);
                }
                TotalCmakeData += "target_compile_features("+Target.first+" PRIVATE ";
                if(LocalSourceInfo.Language == "C++")
                {
                    TotalCmakeData += "cxx";   
                }
                else
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "unsupported language type for target: "+LocalSourceInfo.Language;   
                    return(ReturnValue);
                }
                TotalCmakeData += "_std";
                if(LocalSourceInfo.Standard == "C++17")
                {
                    TotalCmakeData += "_17";   
                }
                else
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "unsupported standard for target: "+LocalSourceInfo.Standard;   
                    return(ReturnValue);
                }
                TotalCmakeData += ")\n";
                //TODO add "extra include directories" for packets
                TotalCmakeData += "target_include_directories("+Target.first+" PRIVATE $ENV{MBPM_PACKETS_INSTALL_DIRECTORY} ${MBPM_IMPORTED_INCLUDE_DIRS})\n";
                TotalCmakeData += "target_link_libraries("+Target.first+" PRIVATE ${TOTAL_MBPM_PACKET_LIBRARIES} ${SYSTEM_LIBRARIES})\n";
                TotalCmakeData += "set_target_properties(" + Target.first + " PROPERTIES \n";
                TotalCmakeData += "ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>\n";
                TotalCmakeData += "RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>\n";
                TotalCmakeData += "LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${CMAKE_BUILD_TYPE}/>\n)\n\n";
                /*
                	set_target_properties(${TargetToUpdate} 
	PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	)
                */
            }
            std::ofstream OutFile = std::ofstream(OutFilePath, std::ios::out);
            if (!OutFile.is_open())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "error creating cmake: failed to open CMakeLists.txt";
                return(ReturnValue);
            }
            OutFile<<TotalCmakeData;
        }
        else if(CommandToHandle.Arguments[0] == "compilecommands")
        {
            std::filesystem::path SourceInfoPath= Packet.PacketURI+"/MBSourceInfo.json";
            if(!std::filesystem::exists(SourceInfoPath))
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "creating compile_commands.json for packet requries existing MBSourceInfo.json"; 
                return(ReturnValue);
            }
            SourceInfo LocalSourceInfo;
            std::vector<std::string> ExtraIncludeDirectories = { MBPM::GetSystemPacketsDirectory() };
            ReturnValue = ParseSourceInfo(SourceInfoPath,LocalSourceInfo);
            if(!ReturnValue) return(ReturnValue);
            std::vector<PacketIdentifier> TotalDependancies = RetrieverToUse.GetTotalDependancies(LocalSourceInfo.ExternalDependancies, ReturnValue);
            if(!ReturnValue) return(ReturnValue);
            for(auto const& Dependancy : TotalDependancies)
            {
                MBPM_PacketInfo DependancyInfo = RetrieverToUse.GetPacketInfo(Dependancy);
                MBBuildPacketInfo DependancyBuildInfo;
                ReturnValue = ParseMBBuildPacketInfo(DependancyInfo,DependancyBuildInfo);
                if(!ReturnValue) return(ReturnValue);
                for(std::string const& IncludeDir : DependancyBuildInfo.ExtraIncludeDirectories)
                {
                    ExtraIncludeDirectories.push_back(MBPM::GetSystemPacketsDirectory()+Dependancy.PacketName+"/"+IncludeDir);
                }
            }
            std::filesystem::path NewFilePath = Packet.PacketURI+"/compile_commands.json";
            if(std::filesystem::exists(NewFilePath))
            {
                AssociatedTerminal.PrintLine("compile_commands.json already exists for packet \""+Packet.PacketName+"\"");
                while(true)
                {
                    AssociatedTerminal.PrintLine("do you want to overrride it? (y/n)");
                    std::string Response;
                    AssociatedTerminal.GetLine(Response);
                    if(Response == "y")
                    {
                        break;    
                    }
                    else if(Response  == "n")
                    {
                        return(ReturnValue);   
                    }
                }
            }
            std::ofstream OutStream = std::ofstream(NewFilePath,std::ios::out);
            if(OutStream.is_open() == false)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Failed opening output file";   
                return(ReturnValue);
            }
            std::vector<MBParsing::JSONObject> ObjectsToWrite;
            std::vector<std::string> Sources = GetAllSources(LocalSourceInfo);
            for(std::string const& Source : Sources)
            {
                MBParsing::JSONObject NewSource(MBParsing::JSONObjectType::Aggregate);   
                NewSource["file"] = Source.substr(1);
                NewSource["directory"] = MBUnicode::PathToUTF8(std::filesystem::absolute(Packet.PacketURI));
                std::vector<std::string> Arguments; 
                //TODO change to actually match the specified language/standard in the file
                Arguments.push_back("clang++");
                Arguments.push_back("-std=c++17");
                Arguments.push_back("-c");
                Arguments.push_back(Source.substr(1));
                //Arguments.push_back("--include-directory="+MBPM::GetSystemPacketsDirectory());
                for (std::string const& IncludeDirectory : ExtraIncludeDirectories)
                {
                    Arguments.push_back("--include-directory=" + IncludeDirectory);
                }
                NewSource["arguments"] = Arguments;
                ObjectsToWrite.push_back(std::move(NewSource));
            }
            OutStream<<MBParsing::JSONObject(std::move(ObjectsToWrite)).ToPrettyString();
            OutStream.flush();
        }
        return(ReturnValue);
    }
    //END MBBuildCLI 

    //BEGIN DependancyConfigSpecification
    std::vector<std::string> DependancyConfigSpecification::p_ParseStringArray(MBParsing::JSONObject const& ObjectToParse)
    {
        std::vector<std::string> ReturnValue;
        auto const& StringArray = ObjectToParse.GetArrayData();
        ReturnValue.reserve(StringArray.size());
        for(MBParsing::JSONObject const& String : StringArray)
        {
            ReturnValue.push_back(String.GetStringData());
        }
        return(ReturnValue);
    }
    DependancyConfigSpecification::Predicate DependancyConfigSpecification::p_ParsePredicate(MBParsing::JSONObject const& ObjectToParse)
    {
        Predicate ReturnValue; 
        if(ObjectToParse.HasAttribute("Negated"))
        {
            ReturnValue.Negated = ObjectToParse["Negated"].GetBooleanData();   
        }
        if(ObjectToParse.HasAttribute("PacketName"))
        {
            ReturnValue.PacketName = p_ParseStringArray(ObjectToParse["PacketName"]); 
        }
        if(ObjectToParse.HasAttribute("Attribute"))
        {
            for(std::string const& String : p_ParseStringArray(ObjectToParse["Attribute"]))
            {
                ReturnValue.Attribute.push_back(String);           
            }
        }
        if(ObjectToParse.HasAttribute("ConfigurationName"))
        {
            ReturnValue.ConfigurationName = p_ParseStringArray(ObjectToParse["ConfigurationName"]); 
        }
        return(ReturnValue);
    }
    DependancyConfigSpecification::Specification DependancyConfigSpecification::p_ParseSpecification(MBParsing::JSONObject const& ObjectToParse)
    {
        Specification ReturnValue;
        ReturnValue.ResultConfiguration = ObjectToParse.GetAttribute("ResultConfiguration").GetStringData();
        auto const& PredicateArray = ObjectToParse.GetAttribute("Predicates").GetArrayData();
        for(MBParsing::JSONObject const& Predicate : PredicateArray)
        {
            ReturnValue.Predicates.push_back(p_ParsePredicate(Predicate));   
        }
        return(ReturnValue);
    }
    MBError DependancyConfigSpecification::ParseDependancyConfigSpecification(MBUtility::MBOctetInputStream& InputStream,DependancyConfigSpecification* OutConfig)
    {
        MBError ReturnValue = true;
        DependancyConfigSpecification Result;
        try
        {
            std::string TotalData;
            size_t ReadChunkSize = 4096;
            size_t MaxFileSize = 1000000;
            while(true)
            {
                if(TotalData.size() > MaxFileSize)
                {
                    throw std::runtime_error("Error parsing config: DependancyConfigSpecification assumed to be under 1MB");
                }
                size_t PreviousSize = TotalData.size();
                TotalData.resize(TotalData.size()+ReadChunkSize);
                size_t ReadBytes = InputStream.Read(TotalData.data()+PreviousSize,ReadChunkSize);
                TotalData.resize(PreviousSize+ReadBytes);
                if(ReadBytes < ReadChunkSize)
                {
                    break;   
                }
            } 
            MBError JSONParseResult = true;
            MBParsing::JSONObject JsonInfo = MBParsing::ParseJSONObject(TotalData,0,nullptr,&JSONParseResult);
            if(!JSONParseResult)
            {
                JSONParseResult.ErrorMessage = "Error parsing JSON data: "+JSONParseResult.ErrorMessage;   
                return(ReturnValue);
            }
            auto const& SpecArray = JsonInfo.GetAttribute("Specifications").GetArrayData();
            for(MBParsing::JSONObject const& JsonObject : SpecArray)
            {
                Result.Specifications.push_back(p_ParseSpecification(JsonObject));
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage ="Error parsing config: "+std::string(e.what());    
            return(ReturnValue);
        }
        *OutConfig = std::move(Result);
        return(ReturnValue);        
    }
    bool DependancyConfigSpecification::p_SpecIsTrue(Specification const& SpecToVerify,MBPM_PacketInfo const& DependancyPacket,std::string const& CompileConfig)
    {
        bool ReturnValue = true;
        for(Predicate const& PredicateToVerify : SpecToVerify.Predicates)
        {
            if(!p_PredicateIsTrue(PredicateToVerify,DependancyPacket,CompileConfig))
            {
                ReturnValue = false;
                break;
            }    
        }
        return(ReturnValue);    
    }
    bool DependancyConfigSpecification::p_PredicateIsTrue(Predicate const& PredicateToVerify,MBPM_PacketInfo const& DependancyPacket,std::string const& CompileConfig)
    {
        bool ReturnValue = false;
        ReturnValue |= std::find(PredicateToVerify.PacketName.begin(),PredicateToVerify.PacketName.end(),DependancyPacket.PacketName) != PredicateToVerify.PacketName.end();
        ReturnValue |= std::find(PredicateToVerify.ConfigurationName.begin(),PredicateToVerify.ConfigurationName.end(),CompileConfig) != PredicateToVerify.ConfigurationName.end();
        for (std::string const& Attribute : DependancyPacket.Attributes)
        {
            ReturnValue |= std::find(PredicateToVerify.Attribute.begin(), PredicateToVerify.Attribute.end(), Attribute) != PredicateToVerify.Attribute.end();
            if (ReturnValue)
            {
                break;
            }
        }
        if(PredicateToVerify.Negated)
        {
            ReturnValue = !ReturnValue;   
        }
        return(ReturnValue); 
    }
    std::string DependancyConfigSpecification::GetDependancyConfig(MBPM_PacketInfo const& InfoToInspect,std::string const& CompileConfig) const
    {
        std::string ReturnValue = CompileConfig;
        for(Specification const& Spec : Specifications)
        {
            if(p_SpecIsTrue(Spec,InfoToInspect,CompileConfig))
            {
                ReturnValue = Spec.ResultConfiguration;   
                break;
            }
        }
        return(ReturnValue);
    }
    //END DependancyConfigSpecification
    MBError Compiler_GCCSyntax::SupportsLanguage(SourceInfo const& SourcesToCheck) 
    {
        MBError ReturnValue = true;
        if(SourcesToCheck.Language != "C++" && SourcesToCheck.Language != "C")
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Only C/C++ supported for "+m_CompilerBase;
            return(ReturnValue);
        }
        std::string Standard = h_MBStandardToGCCStandard(SourcesToCheck.Language,SourcesToCheck.Language);
        if(Standard == "")
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Unknown standard for "+SourcesToCheck.Language+": "+SourcesToCheck.Standard;   
        }
        return(ReturnValue);
    }
    std::string Compiler_GCCSyntax::p_GetCompilerName(SourceInfo const& SInfo)
    {
        std::string ReturnValue = m_CompilerBase; 
        if(SInfo.Language == "C++")
        {
            if(ReturnValue == "gcc")
            {
                ReturnValue = "g++";   
            }
            else if(ReturnValue == "clang")
            {
                ReturnValue = "clang";   
            }
        }
        ReturnValue += ' ';
        return(ReturnValue);
    }
    bool Compiler_GCCSyntax::CompileSource( CompileConfiguration const& CompileConfig,
                                    SourceInfo const& SInfo,
                                    std::string const& SourcePath,
                                    std::string const& OutTopDir,
                                    std::vector<std::string> const& ExtraIncludeDirectories)
    {

        bool ReturnValue = true;
        std::string CompilerName = p_GetCompilerName(SInfo);
        std::string CompileCommand =  CompilerName+" -c ";    
        CompileCommand += h_MBStandardToGCCStandard(SInfo.Language,SInfo.Standard);
        for(std::string const& Flag : CompileConfig.CompileFlags)
        {
            CompileCommand += Flag;
            CompileCommand += ' ';
        }
        for(std::string const& ExtraInclude : ExtraIncludeDirectories)
        {
            CompileCommand += "-I"+ExtraInclude;
            CompileCommand += ' ';
        }
        CompileCommand += SourcePath.substr(1)+' ';
        std::string OutSourcePath = OutTopDir+"/"+h_ReplaceExtension(SourcePath,"o");
        std::filesystem::path ParentDirectory = std::filesystem::path(OutSourcePath).parent_path();
        if(!std::filesystem::exists(ParentDirectory))
        {
            std::filesystem::create_directories(ParentDirectory);
        }
        CompileCommand += "-o "+OutSourcePath;
        int Result = std::system(CompileCommand.c_str());
        ReturnValue = Result == 0;
        return(ReturnValue);
    }
    bool Compiler_GCCSyntax::LinkTarget(CompileConfiguration const& CompileConfig,
                                SourceInfo const& SInfo,
                                Target const& TargetToLink,
                                std::string const& SourceDir,
                                std::string const& OutDir,
                                std::vector<std::string> const& ExtraLinkLibraries) 
    {
        bool ReturnValue = true;
        std::vector<std::string> ObjectFiles = h_SourcesToObjectFiles(SourceDir,TargetToLink.SourceFiles,"o");
        std::string OutputDirectory = OutDir;
        if(!std::filesystem::exists(OutputDirectory))
        {
            std::filesystem::create_directories(OutputDirectory);    
        }
        if(TargetToLink.Type == TargetType::Executable)
        {
            std::string LinkCommand = p_GetCompilerName(SInfo);
            LinkCommand += "-o "+ MBUnicode::PathToUTF8(OutputDirectory+TargetToLink.OutputName);
            if constexpr(MBUtility::IsWindows())
            {
                LinkCommand += ".exe";   
            }
            LinkCommand += " ";
            for (std::string const& ObjectFile : ObjectFiles)
            {
                LinkCommand += ObjectFile;
                LinkCommand += ' ';
            }
            //extra libraries stuff
            for (std::string const& LibraryPath : ExtraLinkLibraries)
            {
                LinkCommand += " ";
                LinkCommand += LibraryPath;
                LinkCommand += " ";
                //size_t LastSlash = LibraryPath.find_last_of('/');
                //if (LastSlash == LibraryPath.npos) 
                //{
                //    LastSlash = 0;
                //}
                //else
                //{
                //}
                //std::string Directory = LibraryPath.substr(0, LastSlash);
                //std::string Name = LibraryPath.substr(LastSlash+1);
                //LinkCommand += "-L" + Directory+ " ";
                //LinkCommand += "-l:" + Name+" ";
            }
            for (std::string const& LinkFlag : CompileConfig.LinkFlags)
            {
                LinkCommand += LinkFlag;
                LinkCommand += ' ';
            }
            int LinkResult = std::system(LinkCommand.c_str());
            ReturnValue = LinkResult == 0;
        }
        else if(TargetToLink.Type == TargetType::Library)
        {
            std::string LibraryName = TargetToLink.OutputName;
            if constexpr( MBUtility::IsWindows())
            {
                LibraryName = TargetToLink.OutputName+".lib";
            }
            else
            {
                LibraryName = "lib"+TargetToLink.OutputName+".a";       
            }
            std::string LinkCommand = "ar rcs "+OutputDirectory+LibraryName +" ";
            for (std::string const& ObjectFile : ObjectFiles)
            {
                LinkCommand += ObjectFile + " ";
            }
            //Dependancy stuff
            int Result = std::system(LinkCommand.c_str());
            ReturnValue = Result == 0;
        }
        return(ReturnValue);
    }
    
    MBError Compiler_MSVC::SupportsLanguage(SourceInfo const& SourcesToCheck) 
    {
        MBError ReturnValue = true;
        if(SourcesToCheck.Language == "C" || SourcesToCheck.Language == "C++")
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Only C and C++ supported for msvc"; 
            return(ReturnValue);
        }
        std::string Config = h_MBStandardToMSVCStandard(SourcesToCheck.Language,SourcesToCheck.Standard);
        if(Config == "")
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Unkown standard: "+SourcesToCheck.Standard;   
            return(ReturnValue);
        }
        return(ReturnValue);
    }
    bool Compiler_MSVC::CompileSource( CompileConfiguration const& CompileConf,
                                SourceInfo const& SInfo,
                                std::string const& SourceToCompile,
                                std::string const& OutDir,
                                std::vector<std::string> const& ExtraIncludeDirectories)
    {
        bool ReturnValue = true;
        std::string CompileCommand = "vcvarsall.bat x86_x64 & cl /c ";
        CompileCommand += h_MBStandardToMSVCStandard(SInfo.Language, SInfo.Standard);
        for(std::string const& Flag : CompileConf.CompileFlags)
        {
            CompileCommand += Flag;
            CompileCommand += ' ';
        }
        for(std::string const& ExtraInclude : ExtraIncludeDirectories)
        {
            CompileCommand += "/I \""+ExtraInclude+"\"";
            CompileCommand += ' ';
        }
        CompileCommand += SourceToCompile.substr(1)+' ';
        //output path
        CompileCommand += "/Fo\"";
        std::string OutSourcePath = OutDir+"/"+h_ReplaceExtension(SourceToCompile,"obj");
        CompileCommand += OutSourcePath;
        CompileCommand += "\"";
        CompileCommand += " ";
        CompileCommand += "/Fd"+OutDir+"/"+h_ReplaceExtension(SourceToCompile,"pdb");
        std::filesystem::path ParentDirectory = std::filesystem::path(OutSourcePath).parent_path();
        if(!std::filesystem::exists(ParentDirectory))
        {
            std::filesystem::create_directories(ParentDirectory);
        }

        int Result = std::system(CompileCommand.c_str());    
        ReturnValue = Result == 0;
        return(ReturnValue);
    }
    bool Compiler_MSVC::LinkTarget(CompileConfiguration const& CompileConfig,
                            SourceInfo const& SInfo,
                            Target const& TargetToLink,
                            std::string const& SourceDir,
                            std::string const& OutDir,
                            std::vector<std::string> const& ExtraLibraries) 
    {
        bool ReturnValue = true;
        std::vector<std::string> ObjectFiles = h_SourcesToObjectFiles(SourceDir,TargetToLink.SourceFiles,"obj");
        std::string OutputDirectory = OutDir;
        if(!std::filesystem::exists(OutputDirectory))
        {
            std::filesystem::create_directories(OutputDirectory);
        }
        std::string ObjectFilesList = " "; 
        for(auto const& ObjectFile : ObjectFiles)
        {
            ObjectFilesList += ObjectFile;   
            ObjectFilesList += ' ';   
        }
        if(TargetToLink.Type == TargetType::Executable)
        {
            std::string LinkCommand = "vcvarsall.bat x86_x64 & link ";    
            LinkCommand += ObjectFilesList;
            for(std::string const& Library : ExtraLibraries)
            {
                LinkCommand += Library;   
                LinkCommand += ' ';
            }
            for (std::string const& Flag : CompileConfig.LinkFlags)
            {
                LinkCommand += Flag;
                LinkCommand += ' ';
            }
            LinkCommand += "/OUT:"+ OutputDirectory+ TargetToLink.OutputName + ".exe";
            //LinkCommand += "/OUT:"+TargetToLink.OutputName + ".exe & move "+TargetToLink.OutputName + ".exe "+OutputDirectory+TargetToLink.OutputName+".exe";
            int Result = std::system(LinkCommand.c_str());     
            ReturnValue = Result == 0;
        }
        else if(TargetToLink.Type == TargetType::Library)
        {
            std::string LinkCommand =  "vcvarsall.bat x86_x64 & lib ";
            LinkCommand += ObjectFilesList;
            LinkCommand += "/OUT:"+OutputDirectory+TargetToLink.OutputName+".lib";
            int Result = std::system(LinkCommand.c_str());
            ReturnValue = Result == 0;
        }
        else 
        {
            throw std::runtime_error("Unsupported target type");   
        }
        return(ReturnValue);   
    }
}   
}
