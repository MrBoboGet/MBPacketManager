#include "MBBuild.h"
#include "../MBPacketManager.h"
#include "../MB_PacketProtocol.h"

#include <assert.h>
#include <filesystem>
#include <ios>
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
namespace MBPM
{
    namespace MBBuild
{
    std::string h_ReplaceExtension(std::string OriginalString,std::string const& NewExtension)
    {
        size_t LastDot = OriginalString.find_last_of(LastDot);    
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

        return(ReturnValue);   
    }
    std::string h_CreateLinkString(CompileConfiguration const& CompileInfo)
    {
        std::string ReturnValue;
        
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






    
    uint64_t h_GetEntryWriteTimestamp(std::filesystem::directory_entry const& Entry)
    {
        uint64_t ReturnValue = 0;
        ReturnValue = Entry.last_write_time().time_since_epoch().count();
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
                throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Update dependancies?");
            }
            uint32_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
            CurrentStruct.LastWriteTime = CurrentTimestamp;
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
            throw std::runtime_error("Failed retrieving status for: \""+MBUnicode::PathToUTF8(CurrentStruct.Path)+"\". Update dependancies?");
        }
        uint32_t CurrentTimestamp = h_GetEntryWriteTimestamp(FileStatus);  
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
            NewInfo.LastCompileTime = h_GetEntryWriteTimestamp(std::filesystem::directory_entry(SourceRoot/NewInfo.Path));

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
        //Result. = CompileConfig.CompileFlags;
        //Result.m_ToolChain = CompileConfig.Toolchain;
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
            throw std::exception("Failed to parse string: insufficient bytes left in stream");   
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
    MBError DependancyInfo::UpdateUpdatedFilesDependancies(std::filesystem::path const& SourceDirectory)
    {
        MBError ReturnValue = true;
        std::vector<std::string> AddedFiles = std::move(m_AddedSources);
        m_AddedSources.clear();
        std::vector<std::string> UpdatedFiles;
        for(auto const& Source : m_Sources)
        {
            if(Source.second.Updated)
            {
                UpdatedFiles.push_back(Source.second.Path);
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

        if (FilesToUpdate.size() == 0)
        {
            return ReturnValue;
        }
        std::string GCCComand = GetGCCDependancyCommand(SourceDirectory,FilesToUpdate,{});
        MBSystem::UniDirectionalSubProcess GCCProccess(GCCComand,true); 
       
        size_t CurrentDepCount = m_Dependancies.size(); 
        std::vector<SourceDependancyInfo> UpdatedInfo = p_NormalizeDependancies(p_ParseMakeDependancyInfo(GCCProccess),SourceDirectory,m_PathToDependancyIndex,m_Dependancies,FilesToUpdate);
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
            m_Sources[Source.Path] = Source;   
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
        auto const& It = m_Sources.find(SourceToCheck);
        if(It == m_Sources.end())
        {
            m_AddedSources.push_back(SourceToCheck);
            return(true);
        }
        SourceDependancyInfo const& DepInfo = It->second;
        if(DepInfo.CompileString != CompileString)
        {
            return(false);       
        }
        //dependancy not existing can't be considred an exception, it simply means that the file has to be compiled to determine wheter or not it works
        ReturnValue = p_FileIsUpdated(SourceDirectory,It->second);
        return(ReturnValue);
    }
    MBError DependancyInfo::GetUpdatedFiles(std::filesystem::path const& SourceDirectory,std::string const& CompileString,std::vector<std::string> const& InputFiles,std::vector<std::string>* OutFilesToCompile) 
    {
        MBError ReturnValue = true;
        std::vector<std::string> Result;
        try
        {
            for(std::string const& Source : InputFiles)
            {
                //size_t LastSlashPosition = Source.find_last_of('/');
                //if (LastSlashPosition == Source.npos)
                //{
                //    LastSlashPosition = -1;
                //}
                //auto const& It = m_PathToSourceIndex.find(Source.substr(1));  
                //if(It != m_PathToSourceIndex.end())
                //{
                //    if(m_Sources[It->second].CompileString != CompileString)
                //    {
                //        Result.push_back(Source);
                //        continue;
                //    }
                //    if(p_FileIsUpdated(SourceDirectory,It->second))
                //    {
                //        Result.push_back(Source); 
                //    }
                //}
                //else
                //{
                //    m_AddedSources.push_back(Source);
                //    Result.push_back(Source);
                //}
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
        return(ToolchainSupported);
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

    bool MBBuild_Extension::p_Compile_MSVC(CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::string const& SourceToCompile,std::string const& OutDir,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        //std::string CompileCommand = "vcvars64.bat & cl /c ";
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
        CompileCommand += SourceToCompile+' ';
        //output path
        CompileCommand += "/Fo\"";
        CompileCommand += OutDir;
        CompileCommand += "/"+h_ReplaceExtension(SourceToCompile,"o");
        CompileCommand += "\"";
         
        int Result = std::system(CompileCommand.c_str());    
        ReturnValue = Result == 0;
        return(ReturnValue);
    }
    bool MBBuild_Extension::p_Link_MSVC(CompileConfiguration const& CompileConfig,SourceInfo const& SInfo, std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries)
    {
        bool ReturnValue = true;
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
        return(ReturnValue);   
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
    bool MBBuild_Extension::p_Compile_GCC(std::string const& CompilerName,CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::string const& SourceToCompile,std::string const& OutDir,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        bool ReturnValue = true;
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
        CompileCommand += SourceToCompile+' ';
        CompileCommand += "-o "+OutDir+"/"+h_ReplaceExtension(SourceToCompile,"o");
        int Result = std::system(CompileCommand.c_str());
        ReturnValue = Result == 0;
        return(ReturnValue);
    }
    
    bool MBBuild_Extension::p_Link_GCC(std::string const& CompilerName,CompileConfiguration const& CompileConfig,SourceInfo const& SInfo,std::string const& ConfigName,Target const& TargetToLink,std::vector<std::string> ExtraLibraries)
    {
        bool ReturnValue = true;
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
            LinkCommand += " ";
            for (std::string const& ObjectFile : ObjectFiles)
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
    bool MBBuild_Extension::p_Compile(CompileConfiguration const& CompileConf,SourceInfo const& SInfo,std::string const& SourceToCompile,std::string const& OutDir,std::vector<std::string> const& ExtraIncludeDirectories)
    {
        bool ReturnValue = false;
        if(CompileConf.Toolchain == "gcc")
        {
            std::string CompilerName = "g++";
            if (SInfo.Language == "C")
            {
                CompilerName = "gcc";
            }
            ReturnValue = p_Compile_GCC(CompilerName,CompileConf,SInfo,SourceToCompile,OutDir,ExtraIncludeDirectories);
        }
        else if(CompileConf.Toolchain == "clang")
        {
            std::string CompilerName = "clang++";
            if (SInfo.Language == "C")
            {
                CompilerName = "clang";
            }
            ReturnValue = p_Compile_GCC(CompilerName,CompileConf,SInfo,SourceToCompile,OutDir,ExtraIncludeDirectories);
        }
        else if(CompileConf.Toolchain == "msvc")
        {
            ReturnValue = p_Compile_MSVC(CompileConf,SInfo,SourceToCompile,OutDir,ExtraIncludeDirectories);
        }
        return(ReturnValue);
    }
    bool MBBuild_Extension::p_Link(CompileConfiguration const& CompileConfig,SourceInfo const& SInfo, std::string const& ConfigName, Target const& TargetToLink, std::vector<std::string> ExtraLibraries)
    {
        bool ReturnValue = true;
        if(CompileConfig.Toolchain == "gcc")
        {
            std::string CompilerName = "g++";
            if(SInfo.Language == "C")
            {
                CompilerName = "gcc";   
            }
            ReturnValue = p_Link_GCC(CompilerName,CompileConfig, SInfo,ConfigName,TargetToLink,ExtraLibraries);
        }
        else if(CompileConfig.Toolchain == "clang")
        {
            std::string CompilerName = "clang++";
            if(SInfo.Language == "C")
            {
                CompilerName = "clang";   
            }
            ReturnValue = p_Link_GCC(CompilerName,CompileConfig, SInfo,ConfigName,TargetToLink,ExtraLibraries);
        }
        else if(CompileConfig.Toolchain == "msvc")
        {
            ReturnValue = p_Link_MSVC(CompileConfig, SInfo,ConfigName,TargetToLink,ExtraLibraries);
        }
        return(ReturnValue);
    }

    MBError MBBuild_Extension::p_BuildLanguageConfig(std::filesystem::path const& PacketPath,MBPM_PacketInfo const& PacketInfo,DependancyConfigSpecification const& DependancySpec,CompileConfiguration const& CompileConf,std::string const& CompileConfName, SourceInfo const& InfoToCompile,std::vector<std::string> const& Targets)
    {
        MBError ReturnValue = true;
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
            throw std::runtime_error("Error creating dependancy info when compiling packet for configuration \"" + CompileConfName + "\": " + DependancyInfoResult.ErrorMessage);
        }
        //std::filesystem::path PreviousWD = std::filesystem::current_path();
        //std::filesystem::current_path(BuildFilesDirectory);
        //for(std::string& Source : UpdatedSources)
        //{
        //    Source = "../../"+Source;   
        //}
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
        //TODO use the source info instead of the MBPM_PacketInfo
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
        //TODO fix
        std::string CompileString;
        std::string OutDir = MBUnicode::PathToUTF8(PacketPath/"MBPM_Builds"/CompileConfName)+"/";
        bool CompilationResult = true;
        bool CompilationNeeded = false;
        for (std::string const& Source : TotalSources)
        {
            if(!SourceDependancies.IsSourceOutOfDate(PacketPath,Source,CompileString,DependancyInfoResult))
            {
                continue;    
            }
            CompilationNeeded = true;
            CompilationResult = p_Compile(CompileConf,InfoToCompile,Source,OutDir,ExtraIncludes);
            if(!CompilationResult)
            {
                break;   
            }
            SourceDependancies.UpdateSource(Source,CompileString);
        }
        if(!CompilationResult)
        {
            //Do stuff   
        }
        //Link step
        std::vector<std::string> ExtraLibraries = {};
        std::reverse(DependancyInfos.begin(),DependancyInfos.end());
        std::string LinkString = h_CreateLinkString(CompileConf);
        std::string DependancyString;
        for(MBPM_PacketInfo const& CurrentInfo : DependancyInfos)
        {
            std::string PacketConfig = DependancySpec.GetDependancyConfig(CurrentInfo,CompileConfName);
            //if(CurrentInfo.Attributes.find(MBPM_PacketAttribute::SubOnly) == CurrentInfo.Attributes.end())
            //{
                ExtraLibraries.push_back(InstallDirectory+ CurrentInfo.PacketName+"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(CurrentInfo.PacketName));
            //} 
            for(auto const& SubLib : CurrentInfo.SubLibraries)
            {
                ExtraLibraries.push_back(InstallDirectory+ CurrentInfo.PacketName +"/MBPM_Builds/"+PacketConfig+"/"+h_GetLibraryName(SubLib.LibraryName));
            }
        }
        uint64_t LatestDependancyUpdate = 0;
        for(std::string const& DependantLibrary : ExtraLibraries)
        {
            std::filesystem::directory_entry Entry = std::filesystem::directory_entry(DependantLibrary);     
            if(Entry.exists() == false)
            {
                //Exception   
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
            if(CompilationNeeded || SourceDependancies.IsTargetOutOfDate(TargetName,LatestDependancyUpdate,CurrentTarget.SourceFiles,LinkString))
            {
                bool Result = p_Link(CompileConf, InfoToCompile,CompileConfName,CurrentTarget,ExtraLibraries);
                if(!Result)
                {
                    //Error compilging grej, breaka?   
                }
                else
                {
                    SourceDependancies.UpdateTarget(TargetName,CurrentTarget.SourceFiles, LinkString);
                }
            }
        } 
        //std::filesystem::current_path(PreviousWD);

        MBError UpdateDependanciesResult = SourceDependancies.UpdateUpdatedFilesDependancies(PacketPath);
        if(!UpdateDependanciesResult)
        {
            throw std::runtime_error("Error updating dependancy info after compilation: "+UpdateDependanciesResult.ErrorMessage); 
        }
        std::ofstream DependancyFile = std::ofstream(DependancyInfoPath,std::ios::out|std::ios::binary);
        MBUtility::MBFileOutputStream DependancyOutputStream(&DependancyFile);
        SourceDependancies.WriteDependancyInfo(DependancyOutputStream);
        DependancyOutputStream.Flush();
        return(ReturnValue);
    }
    MBError MBBuild_Extension::BuildPacket(std::filesystem::path const& PacketPath,std::vector<std::string> Configs,std::vector<std::string> Targets)
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
            bool ConfigsAreValid = p_VerifyConfigs(LanguageInfo,Configs);
            if(!ConfigsAreValid)
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
    MBError MBBuild_Extension::ExportPacket(std::filesystem::path const& PacketPath)
    {
        MBError ReturnValue = true;
         
        return(ReturnValue);    
    }
    MBError MBBuild_Extension::RetractPacket(std::filesystem::path const& PacketPath)
    {
        MBError ReturnValue = true;

        return(ReturnValue);
    }
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

        ReturnValue.Commands = {CompileCommand,CreateCommand,ExportCommand,RetractCommand};
        return(ReturnValue);
    }
    void MBBuild_Extension::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError)
    {
        m_ConfigDirectory =  ConfigurationDirectory; 
        //Should maybe read the contents of the config to make sure it's valid, a question of wheter or not
        //failure should be noticed at runtime for specific packets or not
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
        ReturnValue =  BuildPacket(PacketToHandle.PacketURI,Configurations,Targets);
        return(ReturnValue);    
    }
    MBError MBBuild_Extension::p_Handle_Export(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        ReturnValue =  ExportPacket(PacketToHandle.PacketURI);
        return(ReturnValue);    
   }
    MBError MBBuild_Extension::p_Handle_Retract(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true; 
        ReturnValue =  RetractPacket(PacketToHandle.PacketURI);
        return(ReturnValue);
    }
    MBError MBBuild_Extension::p_Handle_Create(CommandInfo const& CommandToHandle,PacketIdentifier const& Packet,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        if(CommandToHandle.Arguments[0] == "sourceinfo")
        {
            MBPM::MBPM_FileInfoExcluder Excluder(Packet.PacketURI+"/MBPM_FileInfoIgnore");
            MBPM_PacketInfo PacketInfo = RetrieverToUse.GetPacketInfo(Packet);
            //ASSUMPTION only comes here on existing packets, file system dataraces are reported as exceptions
            std::filesystem::recursive_directory_iterator PacketDirectoryIterator(Packet.PacketURI);
            MBParsing::JSONObject SourceInfo(MBParsing::JSONObjectType::Aggregate);
            SourceInfo["Language"] = "C++";
            SourceInfo["Standard"] = "C++17";
            SourceInfo["ExtraIncludes"] = PacketInfo.ExtraIncludeDirectories; 
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
                if(Entry.path().extension() == ".cpp")
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
            bool OverrideAll = CommandToHandle.Flags.find("override") == CommandToHandle.Flags.end();
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
            std::vector<PacketIdentifier> Dependancies = RetrieverToUse.GetPacketDependancies(Packet);
            std::reverse(Dependancies.begin(),Dependancies.end());
            std::string TotalCmakeData = "";
            TotalCmakeData += "project("+CurrentPacketInfo.PacketName+")\n";
            TotalCmakeData += "set(TOTAL_MBPM_PACKET_LIBRARIES\n";
            for(PacketIdentifier const& Dependancie : Dependancies)
            {
                //4 spaces
                TotalCmakeData += "    $ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"+Dependancie.PacketName+"/MBPM_Builds/"+DependanciesConfig+"/"+h_GetLibraryName(Dependancie.PacketName)+"\n";
            }
            TotalCmakeData += ")\n";
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
                TotalCmakeData += "target_include_directories("+Target.first+" PRIVATE $ENV{MBPM_PACKETS_INSTALL_DIRECTORY})\n";
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
                ReturnValue.ErrorMessage = "create compile_commands.json for packet requries existinb MBSourceInfo.json"; 
                return(ReturnValue);
            }
            SourceInfo LocalSourceInfo;
            ReturnValue = ParseSourceInfo(SourceInfoPath,LocalSourceInfo);
            if(!ReturnValue) return(ReturnValue);

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
            std::vector<std::string> Sources = h_GetAllSources(LocalSourceInfo);
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
                //TODO fix extra include directories for dependancies
                Arguments.push_back("--include-directory="+MBPM::GetSystemPacketsDirectory());
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
}   
}
