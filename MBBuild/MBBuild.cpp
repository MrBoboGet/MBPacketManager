#include "MBBuild.h"
#include "MBSystem/MBSystem.h"
#include "../MBPacketManager.h"
#include "MBUtility/MBErrorHandling.h"
#include "MBUtility/MBInterfaces.h"
#include <assert.h>
#include <filesystem>
#include <stdexcept>
#include <MBUnicode/MBUnicode.h>
#include <MBParsing/MBParsing.h>
#include <MBUtility/MBFiles.h>
#include <MBUtility/MBStrings.h>
#include <unordered_map>
#include <MBSystem/MBSystem.h>
namespace MBPM
{
    namespace MBBuild
{
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
            auto const& ExtraIncludes = JSONData["ExtraIncludes"].GetArrayData();
            for(MBParsing::JSONObject const& Include : ExtraIncludes)
            {
                Result.ExtraIncludes.push_back(Include.GetStringData()); 
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
                for(auto const& Source : TargetData.second["Sources"].GetArrayData())
                {
                    TargetInfo.SourceFiles.push_back(Source.GetStringData());   
                }
                Result.Targets[TargetData.first] = std::move(TargetInfo);
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






    
    uint32_t h_GetEntryWriteTimestamp(std::filesystem::directory_entry const& Entry)
    {
        return(0);
    }

    //BEGIN DependancyInfo
    bool DependancyInfo::p_FileIsUpdated(std::filesystem::path const& SourceDirectory ,FileID Index)
    {
        bool ReturnValue = false;
        assert(Index <= m_Dependancies.size());
        DependancyStruct& CurrentStruct = m_Dependancies[Index];
        if(CurrentStruct.Flags.IsUpdated)
        {
            return(true);   
        }
        std::filesystem::directory_entry FileStatus = std::filesystem::directory_entry(SourceDirectory/CurrentStruct.Path);
        if(!FileStatus.exists())
        {
            throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Update dependancies?");
        }
        uint32_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
        if(CurrentTimestamp != CurrentStruct.WriteFlag)
        {
            CurrentStruct.Flags.IsUpdated = true;       
            CurrentStruct.WriteFlag = CurrentTimestamp;
            return(true);
        }
        for(FileID Dependancy : CurrentStruct.Dependancies)
        {
            if(p_FileIsUpdated(SourceDirectory,Dependancy))
            {
                ReturnValue = true;
                break;
            } 
        }
        return(ReturnValue);  
    }
    void DependancyInfo::p_CreateStringToIDMap()
    {
        for(size_t i = 0; i < m_Dependancies.size();i++)
        {
            auto const& Source = m_Dependancies[i];
            if(Source.Flags.IsSource == false)
            {
                break;   
            }
            std::string SourceName = MBUnicode::PathToUTF8(Source.Path);
            if (m_StringToIDMap.find(SourceName) != m_StringToIDMap.end())
            {
                throw std::runtime_error("Multiple sources with the same name found");
            }
            m_StringToIDMap[SourceName] = FileID(i);
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
        std::vector<std::string> Dependancies = MBUtility::Split(LineToParse.substr(FirstColonPosition+1)," ");
        for(std::string const& Dependancy : Dependancies)
        {
            if (Dependancy == "")
            {
                continue;
            }
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
    std::vector<DependancyInfo::DependancyStruct> DependancyInfo::p_NormalizeDependancies(std::vector<MakeDependancyInfo>  const& InfoToConvert)
    {
        std::vector<DependancyInfo::DependancyStruct> ReturnValue;
        std::unordered_map<std::string, FileID> FileIDMap;
        ReturnValue.resize(InfoToConvert.size());
        size_t CurrentSourceOffset = 0;
        FileID CurrentFileID = InfoToConvert.size();
        for(MakeDependancyInfo const& SourceFile : InfoToConvert)
        {
            DependancyStruct NewInfo;
            NewInfo.Flags.IsSource = true;
            NewInfo.Flags.IsUpdated = true;
            NewInfo.Path = SourceFile.FileName;
            NewInfo.WriteFlag = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(NewInfo.Path));
            for(std::string const& Dependancy : SourceFile.Dependancies)
            {
                FileID& DependancyID = FileIDMap[Dependancy];
                if(DependancyID != 0)
                {
                    NewInfo.Dependancies.push_back(DependancyID);   
                    continue;
                }
                DependancyStruct NewDependancy;  
                NewDependancy.Path = Dependancy;
                NewDependancy.Flags.IsUpdated = false;
                NewDependancy.Flags.IsSource = false;
                NewDependancy.WriteFlag = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(NewDependancy.Path));
                DependancyID = CurrentFileID;
                NewInfo.Dependancies.push_back(CurrentFileID);
                ReturnValue.push_back(NewDependancy);
                CurrentFileID++;
            }
            ReturnValue[CurrentSourceOffset] = NewInfo;
            CurrentSourceOffset++;
        }  
        return(ReturnValue);
    }
    std::vector<std::string> h_GetAllSources(SourceInfo const& BuildToInspect)
    {
        std::vector<std::string> ReturnValue = BuildToInspect.Targets.at("MBBuild").SourceFiles;
        return(ReturnValue);
    }
    MBError DependancyInfo::CreateDependancyInfo(SourceInfo const& CurrentBuild,std::filesystem::path const& BuildRoot,DependancyInfo* OutInfo)
    {
        MBError ReturnValue = true;    
        DependancyInfo Result;
        //Test, assumes same working directory as build root
        std::string GCCDependancyCommand = "gcc -MM -MF - -I"+GetSystemPacketsDirectory()+" ";
        std::vector<std::string> AllSources = h_GetAllSources(CurrentBuild);
        for(std::string const& SourceFile : AllSources)
        {
            GCCDependancyCommand += MBUnicode::PathToUTF8(BuildRoot)+ SourceFile;
            GCCDependancyCommand += " ";
        }
        //for (std::string const& SourceFile : AllSources)
        //{
        //    GCCDependancyCommand += "-MT " + SourceFile;
        //    GCCDependancyCommand += " ";
        //}
        MBSystem::UniDirectionalSubProcess SubProcess(GCCDependancyCommand,true);
        Result.m_Dependancies = p_NormalizeDependancies(p_ParseMakeDependancyInfo(SubProcess));
        Result.p_CreateStringToIDMap();
        *OutInfo = std::move(Result);
        return(ReturnValue);
    }
    MBError DependancyInfo::UpdateDependancyInfo(SourceInfo const& CurrentBuild, std::filesystem::path const& BuildRoot, DependancyInfo* OutInfo)
    {
        MBError ReturnValue = true;

        return(ReturnValue);
    }
    MBError DependancyInfo::ParseDependancyInfo(MBUtility::MBOctetInputStream& InputStream, DependancyInfo* OutInfo)
    {
        MBError ReturnValue = true;

        return(ReturnValue);
    }
    void DependancyInfo::WriteDependancyInfo(MBUtility::MBOctetOutputStream& OutStream) const
    {

    }
    MBError DependancyInfo::GetUpdatedFiles(std::filesystem::path const& SourceDirectory,std::vector<std::string> const& InputFiles,std::vector<std::string>* OutFilesToCompile) 
    {
        MBError ReturnValue = true;
        std::vector<std::string> Result;
        try
        {
            for(std::string const& Source : InputFiles)
            {
                size_t LastSlashPosition = Source.find_last_of('/');
                if (LastSlashPosition == Source.npos)
                {
                    LastSlashPosition = 0;
                }
                auto const& It = m_StringToIDMap.find(Source.substr(LastSlashPosition+1));  
                if(It != m_StringToIDMap.end())
                {
                    if(p_FileIsUpdated(SourceDirectory,It->second))
                    {
                        Result.push_back(Source);   
                    }
                }
                else
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Invalid source file provided: not in dependancy info. Update dependancies?";       
                    break;
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
    //END DependancyInfo
  
    //BEGIN MBBuildCLI 
    
    UserConfigurationsInfo MBBuildCLI::p_GetGlobalCompileConfigurations()
    {
       UserConfigurationsInfo ReturnValue;
       std::filesystem::path ConfigPath = MBPM::GetSystemPacketsDirectory()+"MBCompileConfigurations.json";
       MBError Result = ParseUserConfigurationInfo(ConfigPath,ReturnValue);
       if(!Result)
       {
            throw std::runtime_error("Error parsing global compile configurations: "+Result.ErrorMessage);   
       }
       return(ReturnValue);
    }
    SourceInfo MBBuildCLI::p_GetPacketSourceInfo(std::filesystem::path const& PacketPath)
    {
        SourceInfo ReturnValue;
        MBError Result = ParseSourceInfo(PacketPath/"MBSourceInfo.json",ReturnValue);
        if(!Result)
        {
            throw std::runtime_error("Error parsing  source info: "+Result.ErrorMessage);   
        }
        return(ReturnValue); 
    }

    MBPM_PacketInfo MBBuildCLI::p_GetPacketInfo(std::filesystem::path const& PacketPath)
    {
        MBPM_PacketInfo ReturnValue = ParseMBPM_PacketInfo(MBUnicode::PathToUTF8(PacketPath/"MBPM_PacketInfo")); 
        if(ReturnValue.PacketName == "")
        {
            throw std::runtime_error("Error parsing MBPM_PacketInfo");   
        }
        return(ReturnValue);  
    }
    DependancyConfigSpecification MBBuildCLI::p_GetConfigSpec(std::filesystem::path const& PacketPath)
    {
        DependancyConfigSpecification  ReturnValue;

        return(ReturnValue); 
    }

    bool MBBuildCLI::p_VerifyConfigs(LanguageConfiguration const& Config,std::vector<std::string> const& ConfigNames)
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
    bool MBBuildCLI::p_VerifyTargets(SourceInfo const& InfoToCompile,std::vector<std::string> const& Targests)
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


    void MBBuildCLI::p_Compile_GCC(CompileConfiguration const& CompileConf,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        std::string CompileCommand = "g++ -c ";    
        for(std::string const& Flag : CompileConf.CompileFlags)
        {
            CompileCommand += Flag;
            CompileCommand += ' ';
        }
        for(std::string const& ExtraInclude : ExtraIncludeDirectories)
        {
            CompileCommand += "-I"+ExtraInclude;
            CompileCommand += ' ';
        }
        for(std::string const& Source : SourcesToCompile)
        {
            CompileCommand += Source+' ';
        }

        int Result = std::system(CompileCommand.c_str());
        if(Result != 0)
        {
            throw std::runtime_error("Failed compiling sources");   
        }
    }
    void MBBuildCLI::p_Link_GCC(CompileConfiguration const& CompileConfig,Target const& TargetToLink,std::string const& TargetName,std::vector<std::string> ExtraLibraries)
    {
        
        std::vector<std::string> ObjectFiles;
        for(std::string const& SourceToLink : TargetToLink.SourceFiles)
        {
            std::string ObjectFile = SourceToLink.substr(1);
            size_t LastDotPosition = ObjectFile.find_last_of('.');
            if(LastDotPosition == ObjectFile.npos)
            {
                LastDotPosition = ObjectFile.size();
            }
            ObjectFile.replace(LastDotPosition,2,".o");
            ObjectFiles.push_back(ObjectFile);
        }
        if(TargetToLink.Type == TargetType::Executable)
        {
            std::string LinkCommand = "g++ ";
            for(std::string const& LinkFlag : CompileConfig.LinkFlags)
            {
                LinkCommand += LinkFlag;   
                LinkCommand += ' ';
            }
            for(std::string const& ObjectFile : ObjectFiles)
            {
                LinkCommand += ObjectFile;
                LinkCommand += ' ';
            }
            //extra libraries stuff

        }
        else if(TargetToLink.Type == TargetType::Library)
        {
            std::string LibraryName = TargetName;
            if constexpr( MBUtility::IsWindows())
            {
                LibraryName += TargetName+".lib";
            }
            else
            {
                LibraryName = "lib"+TargetName+".a";       
            }
            std::string LinkCommand = "ar rcs "+LibraryName +" ";
            for (std::string const& ObjectFile : ObjectFiles)
            {
                LinkCommand += ObjectFile + " ";
            }
            //Dependancy stuff
            int Result = std::system(LinkCommand.c_str());
            if(!Result)
            {
                throw std::runtime_error("Failed linking target: "+TargetName); 
            }
        }
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
    void MBBuildCLI::p_BuildLanguageConfig(std::filesystem::path const& PacketPath,MBPM_PacketInfo const& PacketInfo,DependancyConfigSpecification const& DependancySpec,CompileConfiguration const& CompileConf,std::string const& CompileConfName, SourceInfo const& InfoToCompile,std::vector<std::string> const& Targets)
    {
        //Create build info   
        DependancyInfo SourceDependancies; 
        std::vector<std::string> TotalSources = h_GetAllSources(InfoToCompile);
        std::filesystem::path BuildFilesDirectory = PacketPath/("MBPM_BuildFiles/"+CompileConfName);
        if(!std::filesystem::exists(BuildFilesDirectory))
        {
            std::filesystem::create_directories(BuildFilesDirectory);
        }
        std::filesystem::path DependancyInfoPath = BuildFilesDirectory/"MBBuildDependancyInfo";
        MBError DependancyInfoResult = true;
        if(!std::filesystem::exists(DependancyInfoPath))
        {
            DependancyInfoResult = DependancyInfo::CreateDependancyInfo(InfoToCompile,PacketPath,&SourceDependancies);
        }
        else
        {
            DependancyInfoResult = DependancyInfo::UpdateDependancyInfo(InfoToCompile,PacketPath,&SourceDependancies);
        }
        if (!DependancyInfoResult)
        {
            throw std::runtime_error("Error creatign dependancy info when compiling packet for configuration \"" + CompileConfName + "\": " + DependancyInfoResult.ErrorMessage);
        }
        std::vector<std::string> UpdatedSources;
        DependancyInfoResult = SourceDependancies.GetUpdatedFiles(PacketPath, TotalSources, &UpdatedSources);
        if(!DependancyInfoResult)
        {
            throw std::runtime_error("Error reading dependancies for \"" +CompileConfName+"\": "+DependancyInfoResult.ErrorMessage);
        }
        std::filesystem::path PreviousWD = std::filesystem::current_path();
        std::filesystem::current_path(BuildFilesDirectory);
        for(std::string& Source : TotalSources)
        {
            Source = "../../"+Source;   
        }
        //Dependacies
        std::string InstallDirectory = GetSystemPacketsDirectory();
        std::vector<std::string> ExtraIncludes = {InstallDirectory};
        for(std::string const& Include : PacketInfo.ExtraIncludeDirectories)
        {
            ExtraIncludes.push_back(Include);   
        }
        PacketIdentifier LocalPacket;
        LocalPacket.PacketName = PacketInfo.PacketName;
        LocalPacket.PacketURI = "../../";
        LocalPacket.PacketLocation = PacketLocationType::Local;
        std::vector<PacketIdentifier> TotalDependancies = m_AssociatedRetriever->GetPacketDependancies(LocalPacket);
        std::vector<MBPM_PacketInfo> DependancyInfos;
        for(PacketIdentifier const& Packet : TotalDependancies)
        {
            MBPM_PacketInfo DependancyInfo = m_AssociatedRetriever->GetPacketInfo(Packet);
            for(std::string const& IncludePath : DependancyInfo.ExtraIncludeDirectories)
            {
                ExtraIncludes.push_back(InstallDirectory+IncludePath);
            }
            DependancyInfos.push_back(std::move(DependancyInfo));
        }
        p_Compile_GCC(CompileConf,TotalSources,ExtraIncludes); 

        std::vector<std::string> ExtraLibraries = {};
        std::reverse(DependancyInfos.begin(),DependancyInfos.end());
        for(MBPM_PacketInfo const& DependancyInfo : DependancyInfos)
        {
            std::string PacketConfig = DependancySpec.GetDependancyConfig(DependancyInfo,CompileConfName);
            if(DependancyInfo.Attributes.find(MBPM_PacketAttribute::SubOnly) == DependancyInfo.Attributes.end())
            {
                ExtraLibraries.push_back(InstallDirectory+"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(DependancyInfo.PacketName));
            } 
            for(auto const& SubLib : DependancyInfo.SubLibraries)
            {
                ExtraLibraries.push_back(InstallDirectory+"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(SubLib.LibraryName));
            }
        }
        for(std::string const& TargetName : Targets)
        {
            Target const& CurrentTarget =InfoToCompile.Targets.at(TargetName);
            p_Link_GCC(CompileConf,InfoToCompile.Targets.at(TargetName),TargetName,ExtraLibraries);
            if(CurrentTarget.Type == TargetType::Library)
            {
                //copy
            }
        } 
        std::filesystem::current_path(PreviousWD);
    }
    MBBuildCLI::MBBuildCLI(PacketRetriever* Retriever)
    {
        m_AssociatedRetriever = Retriever;    
    }
    MBError MBBuildCLI::BuildPacket(std::filesystem::path const& PacketPath,std::vector<std::string> Configs,std::vector<std::string> Targets)
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
            auto const& ConfIt = CompileConfigurations.Configurations.find("C++");
            if(ConfIt == CompileConfigurations.Configurations.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "MBBuild currently only supports C++ builds";   
                return(ReturnValue);
            }
            LanguageConfiguration const& LanguageInfo = ConfIt->second;
            TargetsAreValid = p_VerifyConfigs(LanguageInfo,Targets);
            if(!TargetsAreValid)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid config: Configuration not found in C++ language configurations";
                return(ReturnValue);
            }
            DependancyConfigSpecification DepConf;
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
                p_BuildLanguageConfig(PacketPath,PacketInfo,DepConf,LanguageInfo.Configurations.at(Config),Config,BuildInfo,Targets);
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what(); 
        } 
        return(ReturnValue);    
    }
    MBError MBBuildCLI::ExportTarget(std::filesystem::path const& PacketPath,std::vector<std::string> const& TargetsToExport,std::string const& ExportConfig)
    {
        MBError ReturnValue = true;

        return(ReturnValue);    
    }
    int MBBuildCLI::Run(int argc,const char** argv,MBCLI::MBTerminal* TerminalToUse)
    {
        return(0);        
    }
    //END MBBuildCLI 

    //BEGIN DependancyConfigSpecification
    std::string DependancyConfigSpecification::GetDependancyConfig(MBPM_PacketInfo const& InfoToInspect,std::string const& CompileConfig) const
    {
        return(CompileConfig);       
    }
    //END DependancyConfigSpecification
}   
}
