#include "MBBuild.h"
#include "MBSystem/MBSystem.h"
#include "../MBPacketManager.h"
#include "MBUtility/MBErrorHandling.h"
#include "MBUtility/MBInterfaces.h"
#include <assert.h>
#include <filesystem>
#include <ios>
#include <stdexcept>
#include <MBUnicode/MBUnicode.h>
#include <MBParsing/MBParsing.h>
#include <MBUtility/MBFiles.h>
#include <MBUtility/MBStrings.h>
#include <stdint.h>
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
        uint32_t ReturnValue = 0;
        ReturnValue = Entry.last_write_time().time_since_epoch().count();
        return(ReturnValue);
    }

    //BEGIN DependancyInfo
    bool DependancyInfo::p_DependancyIsUpdated(FileID Index)
    {
        bool ReturnValue = false;
        assert(Index - 1 < m_Dependancies.size());
        DependancyStruct& CurrentStruct = m_Dependancies[Index-1];
        if((CurrentStruct.Flags & DependancyFlag::FileChecked) != DependancyFlag::Null)
        {
            return((CurrentStruct.Flags & DependancyFlag::IsUpdated) != DependancyFlag::Null);      
        }
        std::filesystem::directory_entry FileStatus = std::filesystem::directory_entry(CurrentStruct.Path);
        if(!FileStatus.exists())
        {
            throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Update dependancies?");
        }
        CurrentStruct.Flags |= DependancyFlag::FileChecked;       
        uint32_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
        if(CurrentTimestamp != CurrentStruct.WriteFlag)
        {
            CurrentStruct.Flags |= DependancyFlag::IsUpdated;       
            CurrentStruct.WriteFlag = CurrentTimestamp;
            return(true);
        }
        for(FileID Dependancy : CurrentStruct.Dependancies)
        {
            if(p_DependancyIsUpdated(Dependancy))
            {
                ReturnValue = true;
                break;
            } 
        }
        return(ReturnValue);     
    }
    bool DependancyInfo::p_FileIsUpdated(std::filesystem::path const& SourceDirectory ,FileID Index)
    {
        bool ReturnValue = false;
        assert(Index <= m_Sources.size());
        DependancyStruct& CurrentStruct = m_Sources[Index];
        if((CurrentStruct.Flags & DependancyFlag::FileChecked) != DependancyFlag::Null)
        {
            return((CurrentStruct.Flags & DependancyFlag::IsUpdated) != DependancyFlag::Null);      
        }
        std::filesystem::directory_entry FileStatus = std::filesystem::directory_entry(SourceDirectory/CurrentStruct.Path);
        if(!FileStatus.exists())
        {
            throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Update dependancies?");
        }
        CurrentStruct.Flags |= DependancyFlag::FileChecked;       
        uint32_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
        if(CurrentTimestamp != CurrentStruct.WriteFlag)
        {
            CurrentStruct.Flags |= DependancyFlag::IsUpdated;       
            CurrentStruct.WriteFlag = CurrentTimestamp;
            return(true);
        }
        for(FileID Dependancy : CurrentStruct.Dependancies)
        {
            if(p_DependancyIsUpdated(Dependancy))
            {
                ReturnValue = true;
                break;
            } 
        }
        return(ReturnValue);     
    }
    void DependancyInfo::p_CreateSourceMap()
    {
        m_PathToSourceIndex.clear();
        m_PathToSourceIndex.reserve(m_Sources.size());
        FileID i = 0;
        for(auto const& Source : m_Sources)
        {
            m_PathToSourceIndex[Source.Path] = i;
            i++;
        } 
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
    std::vector<DependancyInfo::DependancyStruct> DependancyInfo::p_NormalizeDependancies(std::vector<MakeDependancyInfo>  const& InfoToConvert, 
        std::filesystem::path const& SourceRoot,std::unordered_map<std::string,FileID>& IDMap,std::vector<DependancyStruct>& OutDependancies, std::vector<std::string> const& OriginalSources)
    {
        std::vector<DependancyStruct> ReturnValue;
        ReturnValue.resize(InfoToConvert.size());
        size_t CurrentSourceOffset = 0;
        FileID CurrentFileID = 1;
        size_t i = 0;
        for(MakeDependancyInfo const& SourceFile : InfoToConvert)
        {
            DependancyStruct NewInfo;
            NewInfo.Flags |= DependancyFlag::FileChecked;
            NewInfo.Flags |= DependancyFlag::IsUpdated;
            NewInfo.Path = OriginalSources[i];
            NewInfo.WriteFlag = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(SourceRoot/NewInfo.Path));

            for(std::string const& Dependancy : SourceFile.Dependancies)
            {
                FileID& DependancyID = IDMap[Dependancy];
                if(DependancyID != 0)
                {
                    NewInfo.Dependancies.push_back(DependancyID);   
                    continue;
                }
                DependancyStruct NewDependancy;  
                NewDependancy.Path = Dependancy;
                NewDependancy.Flags &= ~DependancyFlag::IsUpdated;
                NewDependancy.Flags &= ~DependancyFlag::FileChecked;
                NewDependancy.WriteFlag = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(NewDependancy.Path));
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
    std::vector<std::string> h_GetAllSources(SourceInfo const& BuildToInspect)
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
        ReturnValue = "gcc -MM -MF - -I"+GetSystemPacketsDirectory()+" ";
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
    MBError DependancyInfo::CreateDependancyInfo(SourceInfo const& CurrentBuild,CompileConfiguration const& CompileConfig,std::filesystem::path const& BuildRoot,DependancyInfo* OutInfo)
    {
        MBError ReturnValue = true;    
        DependancyInfo Result;
        //Test, assumes same working directory as build root
        Result.m_CompileOptions = CompileConfig.CompileFlags;
        Result.m_ToolChain = CompileConfig.Toolchain;
        std::vector<std::string> AllSources = h_GetAllSources(CurrentBuild);
        for (std::string& Source : AllSources)
        {
            Source = Source.substr(1);
        }
        std::string GCCCommand = GetGCCDependancyCommand(BuildRoot,AllSources,{});
        //for (std::string const& SourceFile : AllSources)
        //{
        //    GCCDependancyCommand += "-MT " + SourceFile;
        //    GCCDependancyCommand += " ";
        //}
        MBSystem::UniDirectionalSubProcess SubProcess(GCCCommand,true);
        Result.m_Sources = p_NormalizeDependancies(p_ParseMakeDependancyInfo(SubProcess),BuildRoot,Result.m_PathToDependancyIndex,Result.m_Dependancies,AllSources);
        Result.p_CreateSourceMap();
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
    void DependancyInfo::p_WriteDependancyStruct(MBUtility::MBOctetOutputStream& OutStream, DependancyInfo::DependancyStruct const& StructToWrite) const
    {
        MBParsing::WriteBigEndianInteger(OutStream, StructToWrite.WriteFlag, 4);
        std::string FilePath = MBUnicode::PathToUTF8(StructToWrite.Path);
        MBParsing::WriteBigEndianInteger(OutStream, FilePath.size(), 2);
        OutStream.Write(FilePath.data(), FilePath.size());
        MBParsing::WriteBigEndianInteger(OutStream, StructToWrite.Dependancies.size(), 4);
        for (FileID IDToWrite : StructToWrite.Dependancies)
        {
            MBParsing::WriteBigEndianInteger(OutStream, IDToWrite, 4);
        }
    }
    DependancyInfo::DependancyStruct DependancyInfo::p_ParseDependancyStruct(MBUtility::MBOctetInputStream& InputStream)
    {
        DependancyStruct NewStruct;
        NewStruct.WriteFlag = MBParsing::ParseBigEndianInteger(InputStream, 4);
        std::string SerializedString;
        uint16_t StringSize = MBParsing::ParseBigEndianInteger(InputStream, 2);
        SerializedString.resize(StringSize);
        InputStream.Read(SerializedString.data(), StringSize);
        NewStruct.Path = SerializedString;
        uint32_t DependanciesCount = MBParsing::ParseBigEndianInteger(InputStream, 4);
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
            uint16_t ToolchainSize = uint16_t(MBParsing::ParseBigEndianInteger(InputStream, 2));
            Result.m_ToolChain = std::string(ToolchainSize, 0);
            size_t ReadBytes = InputStream.Read(Result.m_ToolChain.data(), Result.m_ToolChain.size());
            if (ReadBytes < ToolchainSize)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Not enough bytes provided for toolchain name";
                return(ReturnValue);
            }
            uint32_t CompileOptionsCount = uint32_t(MBParsing::ParseBigEndianInteger(InputStream, 4));
            if (CompileOptionsCount > 1000000)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid number of compile options: compile options assumed to be under 1 million";
                return(ReturnValue);
            }
            Result.m_CompileOptions.reserve(CompileOptionsCount);
            for (size_t i = 0; i < CompileOptionsCount; i++)
            {
                uint16_t CompileCommandSize = uint16_t(MBParsing::ParseBigEndianInteger(InputStream, 2));
                std::string CompileCommand = std::string(CompileCommandSize, 0);
                size_t ReadBytes = InputStream.Read(CompileCommand.data(), CompileCommand.size());
                if (ReadBytes < CompileCommandSize)
                {
                    ReturnValue = false;
                    ReturnValue.ErrorMessage = "Not enough bytes provided for compile option";
                    return(ReturnValue);
                }
                Result.m_CompileOptions.push_back(std::move(CompileCommand));
            }

            uint32_t NumberOfSources = uint32_t(MBParsing::ParseBigEndianInteger(InputStream, 4));
            if (NumberOfSources >= 1000000)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid number of source files: assumed to be under 1 million";
            }
            Result.m_Sources.reserve(NumberOfSources);
            for (uint32_t i = 0; i < NumberOfSources; i++)
            {
                DependancyStruct NewStruct = p_ParseDependancyStruct(InputStream);
                Result.m_Sources.push_back(std::move(NewStruct));
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


            Result.p_CreateDependancyMap();
            Result.p_CreateSourceMap();
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
        //string
        MBParsing::WriteBigEndianInteger(OutStream, m_ToolChain.size(), 2);
        OutStream.Write(m_ToolChain.data(), m_ToolChain.size());
        MBParsing::WriteBigEndianInteger(OutStream, m_CompileOptions.size(), 4); 
        for (std::string const& Option : m_CompileOptions)
        {
            MBParsing::WriteBigEndianInteger(OutStream, Option.size(), 2);
            OutStream.Write(Option.data(), Option.size());
        }

        MBParsing::WriteBigEndianInteger(OutStream,m_Sources.size(),4);
        for(DependancyStruct const& Dependancy : m_Sources)
        {
            p_WriteDependancyStruct(OutStream, Dependancy);
        } 
        MBParsing::WriteBigEndianInteger(OutStream, m_Dependancies.size(), 4);
        for (DependancyStruct const& Dependancy : m_Dependancies)
        {
            p_WriteDependancyStruct(OutStream, Dependancy);
        }
    }
    MBError DependancyInfo::UpdateUpdatedFilesDependancies(std::filesystem::path const& SourceDirectory)
    {
        MBError ReturnValue = true;
        std::vector<std::string> AddedFiles = std::move(m_AddedSources);
        m_AddedSources.clear();
        std::vector<std::string> UpdatedFiles;
        for(auto const& Source : m_Sources)
        {
            if((Source.Flags & DependancyFlag::IsUpdated) != DependancyFlag::Null)
            {
                UpdatedFiles.push_back(Source.Path);
            }
        }
        std::vector<std::string> FilesToUpdate;
        for(auto const& Path : AddedFiles)
        {
            FilesToUpdate.push_back(MBUnicode::PathToUTF8(Path));   
        }
        for(auto const& Path : UpdatedFiles)
        {
            FilesToUpdate.push_back(MBUnicode::PathToUTF8(Path));   
        }
        std::string GCCComand = GetGCCDependancyCommand(SourceDirectory,FilesToUpdate,{});
        MBSystem::UniDirectionalSubProcess GCCProccess(GCCComand,true); 
       
        size_t CurrentDepCount = m_Dependancies.size(); 
        std::vector<DependancyStruct> UpdatedInfo = p_NormalizeDependancies(p_ParseMakeDependancyInfo(GCCProccess),SourceDirectory,m_PathToDependancyIndex,m_Dependancies,FilesToUpdate);
        for(size_t i = 0; i < AddedFiles.size();i++)
        {
            m_PathToSourceIndex[AddedFiles[i]] = m_Sources.size();
            m_Sources.push_back(std::move(UpdatedInfo[i]));
        }
        for(size_t i = 0; i < UpdatedFiles.size();i++)
        {
            m_Sources[m_PathToSourceIndex.at(UpdatedFiles[i])] = std::move(UpdatedInfo[i+AddedFiles.size()]);                
        }
        for(size_t i = CurrentDepCount; i < m_Dependancies.size();i++)
        {
            m_PathToDependancyIndex[m_Dependancies[i].Path] = i;   
        }
        return(ReturnValue);
    }
    MBError DependancyInfo::GetUpdatedFiles(std::filesystem::path const& SourceDirectory,CompileConfiguration const& CompileConfig,std::vector<std::string> const& InputFiles,std::vector<std::string>* OutFilesToCompile) 
    {
        MBError ReturnValue = true;
        std::vector<std::string> Result;
        bool CompileCommandsMatch = m_CompileOptions == CompileConfig.CompileFlags && CompileConfig.Toolchain == m_ToolChain;
        try
        {
            for(std::string const& Source : InputFiles)
            {
                //size_t LastSlashPosition = Source.find_last_of('/');
                //if (LastSlashPosition == Source.npos)
                //{
                //    LastSlashPosition = -1;
                //}
                auto const& It = m_PathToSourceIndex.find(Source.substr(1));  
                if(It != m_PathToSourceIndex.end())
                {
                    if(!CompileCommandsMatch || p_FileIsUpdated(SourceDirectory,It->second))
                    {
                        Result.push_back(Source);   
                    }
                }
                else
                {
                    m_AddedSources.push_back(Source);
                    Result.push_back(Source);
                }
            } 
        } 
        catch(std::exception const& e)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what();   
            return(ReturnValue);
        }
        *OutFilesToCompile = std::move(Result);
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
    DependancyConfigSpecification MBBuildCLI::p_GetGlobalDependancySpecification()
    {
        DependancyConfigSpecification ReturnValue;
        std::filesystem::path ConfigPath = MBPM::GetSystemPacketsDirectory()+"MBDependancySpecification.json";
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
    
    bool MBBuildCLI::p_VerifyToolchain(SourceInfo const& InfoToCompile,CompileConfiguration const& CompileConfigToVerify)
    {
        bool ToolchainSupported = false; 
        for(auto const& Toolchain : {"msvc","gcc","clang"})
        {
            if(CompileConfigToVerify.Toolchain == Toolchain)
            {
                ToolchainSupported = true;
                break;   
            }
        }
        if(!ToolchainSupported)
        {
            throw std::runtime_error("Unsupported toolchain: "+CompileConfigToVerify.Toolchain);   
        }
        //TODO per toolchain verify that booth the language and standard is supported

    }

    std::vector<std::string> h_SourcesToObjectFiles(std::vector<std::string> const& Sources,std::string const& Extension)
    {
        std::vector<std::string> ReturnValue;
        for(std::string const& Source : Sources)
        {
            std::string ObjectFile = Source.substr(1);
            size_t LastDotPosition = ObjectFile.find_last_of('.');
            if(LastDotPosition == ObjectFile.npos)
            {
                LastDotPosition = ObjectFile.size();
            }
            ObjectFile.replace(LastDotPosition,Extension.size(),Extension.c_str());
            ObjectFile.resize(LastDotPosition + Extension.size());
            size_t LastSlash = ObjectFile.find_last_of('/');
            if (LastSlash == ObjectFile.npos)
            {
                LastSlash = 0;
            }
            else
            {
                LastSlash = LastSlash + 1;
            }
            ObjectFile = ObjectFile.substr(LastSlash);
            ReturnValue.push_back(std::move(ObjectFile));
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
        ReturnValue += " ";
        return(ReturnValue);
    }

    void MBBuildCLI::p_Compile_MSVC(CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        //std::string CompileCommand = "vcvars64.bat & cl /c ";
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
    void MBBuildCLI::p_Link_MSVC(CompileConfiguration const& CompileConfig,SourceInfo const& SInfo, std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries)
    {
        std::vector<std::string> ObjectFiles = h_SourcesToObjectFiles(TargetToLink.SourceFiles,".obj");
        
        std::string OutputDirectory = "../../MBPM_Builds/"+ConfigName+"/";
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
            for(std::string const& Flag : CompileConfig.LinkFlags)
            {
                LinkCommand += Flag;
                LinkCommand += ' ';
            }
            LinkCommand += ObjectFilesList;
            for(std::string const& Library : ExtraLibraries)
            {
                LinkCommand += Library;   
                LinkCommand += ' ';
            }
            LinkCommand += "/OUT:"+ OutputDirectory+ TargetToLink.OutputName + ".exe";
            //LinkCommand += "/OUT:"+TargetToLink.OutputName + ".exe & move "+TargetToLink.OutputName + ".exe "+OutputDirectory+TargetToLink.OutputName+".exe";
            int Result = std::system(LinkCommand.c_str());     
            if(Result != 0)
            {
                throw std::runtime_error("Failed linking executable");   
            }
        }
        else if(TargetToLink.Type == TargetType::Library)
        {
            std::string LinkCommand =  "vcvarsall.bat x86_x64 & lib ";
            LinkCommand += ObjectFilesList;
            LinkCommand += "/OUT:"+OutputDirectory+TargetToLink.OutputName+".lib";
            int Result = std::system(LinkCommand.c_str());
            if(Result != 0)
            {
                throw std::runtime_error("Failed creating library");   
            }
        }
        else 
        {
            throw std::runtime_error("Unsupported target type");   
        }
       
        
          
    }
    std::string h_MBStandardToGCCStandard(std::string const& Language,std::string const& Standard)
    {
        std::string ReturnValue = " -std=";
        if(Standard == "C++17")
        {
            ReturnValue += "c++17";  
        } 
        else if(Standard == "c99")
        {
            ReturnValue += "c99";
        }
        ReturnValue += " ";
        return(ReturnValue); 
    }
    void MBBuildCLI::p_Compile_GCC(std::string const& CompilerName,CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        std::string CompileCommand =  CompilerName+" -c ";    
        CompileCommand += h_MBStandardToGCCStandard(SInfo.Language,SInfo.Standard);
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
    
    void MBBuildCLI::p_Link_GCC(std::string const& CompilerName,CompileConfiguration const& CompileConfig,SourceInfo const& SInfo,std::string const& ConfigName,Target const& TargetToLink,std::vector<std::string> ExtraLibraries)
    {
        
        std::vector<std::string> ObjectFiles = h_SourcesToObjectFiles(TargetToLink.SourceFiles,".o");
        std::string OutputDirectory = "../../MBPM_Builds/"+ConfigName+"/";
        if(!std::filesystem::exists(OutputDirectory))
        {
            std::filesystem::create_directories(OutputDirectory);    
        }
        if(TargetToLink.Type == TargetType::Executable)
        {
            std::string LinkCommand = "g++ ";
            LinkCommand += "-o "+ MBUnicode::PathToUTF8(OutputDirectory+TargetToLink.OutputName);
            if constexpr(MBUtility::IsWindows())
            {
                LinkCommand += ".exe";   
            }
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
            for (std::string const& LibraryPath : ExtraLibraries)
            {
                size_t LastSlash = LibraryPath.find_last_of('/');
                if (LastSlash == LibraryPath.npos) 
                {
                    LastSlash = 0;
                }
                else
                {
                }
                std::string Directory = LibraryPath.substr(0, LastSlash);
                std::string Name = LibraryPath.substr(LastSlash+1);
                LinkCommand += "-L" + Directory+ " ";
                LinkCommand += "-l:" + Name+" ";
            }
            int LinkResult = std::system(LinkCommand.c_str());
            //if (LinkResult != 0)
            //{
            //    throw std::runtime_error("Error linking executable");
            //}
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
            if(Result != 0)
            {
                throw std::runtime_error("Failed linking target: "+TargetToLink.OutputName); 
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
    void MBBuildCLI::p_Compile(CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::vector<std::string> const& SourcesToCompile,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        if(CompileConf.Toolchain == "gcc")
        {
            std::string CompilerName = "g++";
            if (SInfo.Language == "C")
            {
                CompilerName = "gcc";
            }
            p_Compile_GCC(CompilerName,CompileConf,SInfo,SourcesToCompile,ExtraIncludeDirectories);
        }
        else if(CompileConf.Toolchain == "clang")
        {
            std::string CompilerName = "clang++";
            if (SInfo.Language == "C")
            {
                CompilerName = "clang";
            }
            p_Compile_GCC(CompilerName,CompileConf,SInfo,SourcesToCompile,ExtraIncludeDirectories);
        }
        else if(CompileConf.Toolchain == "msvc")
        {
            p_Compile_MSVC(CompileConf,SInfo,SourcesToCompile,ExtraIncludeDirectories);
        }
    }
    void MBBuildCLI::p_Link(CompileConfiguration const& CompileConfig,SourceInfo const& SInfo, std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries)
    {
        if(CompileConfig.Toolchain == "gcc")
        {
            std::string CompilerName = "g++";
            if(SInfo.Language == "C")
            {
                CompilerName = "gcc";   
            }
            p_Link_GCC(CompilerName,CompileConfig, SInfo,ConfigName,TargetToLink,ExtraLibraries);
        }
        else if(CompileConfig.Toolchain == "clang")
        {
            std::string CompilerName = "clang++";
            if(SInfo.Language == "C")
            {
                CompilerName = "clang";   
            }
            p_Link_GCC(CompilerName,CompileConfig, SInfo,ConfigName,TargetToLink,ExtraLibraries);
        }
        else if(CompileConfig.Toolchain == "msvc")
        {
            p_Link_MSVC(CompileConfig, SInfo,ConfigName,TargetToLink,ExtraLibraries);
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
            DependancyInfoResult = DependancyInfo::CreateDependancyInfo(InfoToCompile,CompileConf,PacketPath,&SourceDependancies);
        }
        else
        {
            std::ifstream FileInput = std::ifstream(DependancyInfoPath,std::ios::in|std::ios::binary);
            MBUtility::MBFileInputStream  InputStream(&FileInput);
            DependancyInfoResult = DependancyInfo::ParseDependancyInfo(InputStream,&SourceDependancies);
            //DependancyInfoResult = DependancyInfo::UpdateDependancyInfo(InfoToCompile,PacketPath,&SourceDependancies);
        }
        if (!DependancyInfoResult)
        {
            throw std::runtime_error("Error creatign dependancy info when compiling packet for configuration \"" + CompileConfName + "\": " + DependancyInfoResult.ErrorMessage);
        }
        std::vector<std::string> UpdatedSources; 
        DependancyInfoResult = SourceDependancies.GetUpdatedFiles(PacketPath,CompileConf, TotalSources, &UpdatedSources);
        if (UpdatedSources.size() == 0)
        {
            return;
        }
        if(!DependancyInfoResult)
        {
            throw std::runtime_error("Error reading dependancies for \"" +CompileConfName+"\": "+DependancyInfoResult.ErrorMessage);
        }
        std::filesystem::path PreviousWD = std::filesystem::current_path();
        std::filesystem::current_path(BuildFilesDirectory);
        for(std::string& Source : UpdatedSources)
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
        p_Compile(CompileConf,InfoToCompile, UpdatedSources,ExtraIncludes);

        std::vector<std::string> ExtraLibraries = {};
        std::reverse(DependancyInfos.begin(),DependancyInfos.end());
        for(MBPM_PacketInfo const& CurrentInfo : DependancyInfos)
        {
            std::string PacketConfig = DependancySpec.GetDependancyConfig(CurrentInfo,CompileConfName);
            if(CurrentInfo.Attributes.find(MBPM_PacketAttribute::SubOnly) == CurrentInfo.Attributes.end())
            {
                ExtraLibraries.push_back(InstallDirectory+ CurrentInfo.PacketName+"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(CurrentInfo.PacketName));
            } 
            for(auto const& SubLib : CurrentInfo.SubLibraries)
            {
                ExtraLibraries.push_back(InstallDirectory+ CurrentInfo.PacketName +"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(SubLib.LibraryName));
            }
        }
        for(std::string const& TargetName : Targets)
        {
            Target const& CurrentTarget =InfoToCompile.Targets.at(TargetName);
            p_Link(CompileConf, InfoToCompile,CompileConfName,InfoToCompile.Targets.at(TargetName),ExtraLibraries);
            if(CurrentTarget.Type == TargetType::Library)
            {
                //copy
            }
        } 
        std::filesystem::current_path(PreviousWD);

        MBError UpdateDependanciesResult = SourceDependancies.UpdateUpdatedFilesDependancies(PacketPath);
        if(!UpdateDependanciesResult)
        {
            throw std::runtime_error("Error updating dependancy info after compilation: "+UpdateDependanciesResult.ErrorMessage); 
        }
        std::ofstream DependancyFile = std::ofstream(DependancyInfoPath,std::ios::out|std::ios::binary);
        MBUtility::MBFileOutputStream DependancyOutputStream(&DependancyFile);
        SourceDependancies.WriteDependancyInfo(DependancyOutputStream);
        DependancyOutputStream.Flush();
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
                ReturnValue.Attribute.push_back(MBPM::StringToPacketAttribute(String));           
                if(ReturnValue.Attribute.back() == MBPM_PacketAttribute::Null)
                {
                    throw std::runtime_error("Invalid attribute found: "+String);   
                }
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
        for (MBPM_PacketAttribute Attribute : DependancyPacket.Attributes)
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
}   
}
