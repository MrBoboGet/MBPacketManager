#include "MBPacketManager.h"
#include <assert.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>
//magiska namn macros
#define MBPM_PACKETINFO_FILENAME "MBPM_PacketInfo";

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else
#include <stdlib.h>
#endif // 
namespace MBPM
{
	//BEGIN Private functions
	MBError h_AddDirectoryFiles(MBPM_CmakeProject& ProjectToPopulate, std::string const& ProjectDirectory,std::string const& ProjectName)
	{
		MBError ReturnValue = true;
		std::filesystem::recursive_directory_iterator ProjectIterator(ProjectDirectory);
		for (auto const& Entries : ProjectIterator)
		{
			if (Entries.is_regular_file())
			{
				//Kommer ge fel på windows med paths som har u8, får lägga till den dependancyn
				std::string TotalFilePath = Entries.path().generic_u8string();
				std::string RelativePath = TotalFilePath.substr(GetSystemPacketsDirectory().size());
				std::string Filename = Entries.path().filename().generic_u8string();	
				std::string FileEnding = "";
				size_t LastDot = Filename.find_last_of('.');
				if (LastDot != Filename.npos && LastDot != Filename.size()-1)
				{
					FileEnding = Filename.substr(LastDot + 1);
				}
				if (FileEnding[0] == 'h' || FileEnding == "hpp")
				{
					ProjectToPopulate.CommonHeaders.push_back(ProjectName + "/" + Filename);
				}
				if (FileEnding[0] == 'c' || FileEnding == "cpp")
				{
					ProjectToPopulate.CommonSources.push_back(ProjectName + "/" + RelativePath);
				}
			}
		}
		return(ReturnValue);
	}
	MBError h_EmbeddPacket_CmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration, MBPM_PacketInfo const& PacketInfo, std::string const& PacketDirectory)
	{
		//Just nu lägger vi bara till om det går, annars tar bort, i framtiden kanske vi vill lägga till
		MBError ReturnValue = "";
		if (PacketInfo.Attributes.find(MBPM_PacketAttribute::TriviallyEmbedabble) != PacketInfo.Attributes.end())
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Error embedding packet\"" + PacketInfo.PacketName + "\": Embedding not supported";
			return(ReturnValue);
			//if (PacketInfo.SupportedOutputConfigurations.find(MBPM_CompileOutputConfiguration::StaticDebug) == PacketInfo.SupportedOutputConfigurations.end())
			//{
			//
			//}
		}
		else
		{
			ReturnValue = h_AddDirectoryFiles(ProjectToPopulate,PacketDirectory,"${MBPM_PACKETS_DIRECTORY}/"+PacketInfo.PacketName);
		}
		return(ReturnValue);
	}
	MBError h_GenerateCmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration,MBPM_PacketInfo const& PacketInfo,std::string const& PacketDirectory)
	{
		MBError GenerationError = true;
		bool IsTopPacket = false;
		if (ProjectToPopulate.ProjectName == "")
		{
			IsTopPacket = true;
		}
		if (IsTopPacket)
		{
			ProjectToPopulate.ProjectName = PacketInfo.PacketName;
		}
		for (auto const& Dependancies : PacketInfo.PacketDependancies)
		{
			if (ProjectToPopulate.ProjectPacketsDepandancies.find(Dependancies) == ProjectToPopulate.ProjectPacketsDepandancies.end())
			{
				std::string DependancyPath = GetSystemPacketsDirectory() + Dependancies+"/";
				MBPM_PacketInfo DependancyInfo = ParseMBPM_PacketInfo(DependancyPath +"MBPM_PacketInfo");
				ProjectToPopulate.ProjectPacketsDepandancies.insert(Dependancies);
				if (CompileConfiguration.SupportedLibraryConfigurations.find(MBPM_CompileOutputConfiguration::Embedded) != CompileConfiguration.SupportedLibraryConfigurations.end())
				{
					GenerationError = h_EmbeddPacket_CmakeFile(ProjectToPopulate, CompileConfiguration, DependancyInfo, DependancyPath);
					if (!GenerationError)
					{
						return(GenerationError);
					}
				}
				if (CompileConfiguration.SupportedLibraryConfigurations.find(MBPM_CompileOutputConfiguration::StaticRelease) != CompileConfiguration.SupportedLibraryConfigurations.end())
				{
					ProjectToPopulate.TargetsData[MBPM_CompileOutputConfiguration::StaticRelease].StaticLibrarysNeeded.push_back(Dependancies);
				}
				if (CompileConfiguration.SupportedLibraryConfigurations.find(MBPM_CompileOutputConfiguration::StaticDebug) != CompileConfiguration.SupportedLibraryConfigurations.end())
				{
					ProjectToPopulate.TargetsData[MBPM_CompileOutputConfiguration::StaticDebug].StaticLibrarysNeeded.push_back(Dependancies);
				}
				if (CompileConfiguration.SupportedLibraryConfigurations.find(MBPM_CompileOutputConfiguration::DynamicRelease) != CompileConfiguration.SupportedLibraryConfigurations.end())
				{
					ProjectToPopulate.TargetsData[MBPM_CompileOutputConfiguration::DynamicRelease].DynamicLibrarysNeeded.push_back(Dependancies);
				}
				if (CompileConfiguration.SupportedLibraryConfigurations.find(MBPM_CompileOutputConfiguration::DynamicDebug) != CompileConfiguration.SupportedLibraryConfigurations.end())
				{
					ProjectToPopulate.TargetsData[MBPM_CompileOutputConfiguration::DynamicDebug].DynamicLibrarysNeeded.push_back(Dependancies);
				}
			}
		}
		if (IsTopPacket)
		{
			h_AddDirectoryFiles(ProjectToPopulate, PacketDirectory,"");
		}
		return(GenerationError);
	}
	MBError h_GenerateCmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& PacketDirectory)
	{
		MBError GenerationError = true;
		MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectory + "MBPM_PacketInfo");

		GenerationError = h_GenerateCmakeFile(ProjectToPopulate, CompileConfiguration, PacketInfo, PacketDirectory);

		return(GenerationError);
	}
	//END PrivateFunctions

	std::string GetSystemPacketsDirectory()
	{
		return("");
	}
	MBError SetSystemPacketsDirectory(std::string const& DirectoryPath)
	{
		MBError ReturnValue = true;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		_putenv_s("MBPM_PACKETS_DIRECTORY", DirectoryPath.c_str());
#else
		setenv("MBPM_PACKETS_DIRECTORY", DirectoryPath.c_str());
#endif // 
		return(ReturnValue);
	}


	MBPM_PacketInfo ParseMBPM_PacketInfo(std::string const& PacketPath)
	{
		std::map<std::string, std::vector<std::string>> ParsedAttributes = {};
		std::ifstream FileToRead = std::ifstream(PacketPath,std::ios::in);
		std::string CurrentLine = "";
		std::string CurrentAttribute = "";
		while (std::getline(FileToRead,CurrentLine))
		{
			if (CurrentAttribute == "")
			{
				CurrentAttribute = CurrentLine;
			}
			else if (CurrentLine == "END")
			{
				CurrentAttribute = "";
			}
			else
			{
				ParsedAttributes[CurrentAttribute].push_back(CurrentLine);
			}
		}
		MBPM_PacketInfo ReturnValue;
		ReturnValue.PacketName = ParsedAttributes["PacketName"].front();
		for (auto const& Attribute : ParsedAttributes["Attributes"])
		{
			if (Attribute == "Embeddable")
			{
				ReturnValue.Attributes.insert(MBPM_PacketAttribute::Embedabble);
			}
		}
		for (auto const& OutputConfigurations : ParsedAttributes["OutputConfigurations"])
		{
			if (OutputConfigurations == "StaticDebug")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::StaticRelease);
			}
			if (OutputConfigurations == "StaticRelease")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::DynamicDebug);
			}
			if (OutputConfigurations == "DynamicDebug")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::DynamicRelease);
			}
			if (OutputConfigurations == "DynamicRelease")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::DynamicRelease);
			}
		}
		for (auto const& TargetNames : ParsedAttributes["OutputTargetNames"])
		{
			size_t FirstSpace = TargetNames.find(' ');
			std::string TargetConfiguration = TargetNames.substr(FirstSpace);
			std::string TargetName = TargetNames.substr(0,FirstSpace);
			if (TargetConfiguration == "StaticDebug")
			{
				ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::StaticDebug] = TargetName;
			}
			if (TargetConfiguration == "StaticRelease")
			{
				ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::StaticRelease] = TargetName;
			}
			if (TargetConfiguration == "DynamicDebug")
			{
				ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::DynamicDebug] = TargetName;
			}
			if (TargetConfiguration == "DynamicRelease")
			{
				ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::DynamicRelease] = TargetName;
			}
		}
		ReturnValue.PacketDependancies = ParsedAttributes["Dependancies"];
		return(ReturnValue);
	}
	MBError WriteMBPM_PacketInfo(MBPM_PacketInfo const& PacketToWrite)
	{
		return(MBError());
	}

	MBError WriteCMakeProjectToFile(MBPM_CmakeProject const& ProjectToWrite, std::string const& OutputFilePath)
	{
		MBError ReturnValue = true;
		std::ofstream OutputFile = std::ofstream(OutputFilePath, std::ios::out | std::ios::binary);
		OutputFile << "project("+ProjectToWrite.ProjectName+")\n";
		OutputFile << "set(PROJECT_NAME \"" + ProjectToWrite.ProjectName + "\")\n";
		
		//write common sources
		OutputFile << "set(PROJECT_SOURCES \n";
		for (std::string const& SourceFiles : ProjectToWrite.CommonSources)
		{
			OutputFile <<'\t'<< SourceFiles << std::endl;
		}
		OutputFile << ")\n";
		//write common headers
		OutputFile << "set(PROJECT_HEADERS \n";
		for (std::string const& Headers : ProjectToWrite.CommonHeaders)
		{
			OutputFile << '\t' <<"    " << Headers << std::endl;
		}
		OutputFile << ")\n";
		OutputFile << "set(COMMON_FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS})\n";

		//librarys
		OutputFile << "set(COMMON_DYNAMIC_LIBRARIES \n";
		for (auto const& CommonDynamicLibraries : ProjectToWrite.CommonDynamicLibrarysNeeded)
		{
			OutputFile << '\t' << CommonDynamicLibraries << '\n';
		}
		OutputFile << ")\n";
		OutputFile << "set(COMMON_STATIC_LIBRARIES \n";
		for(auto const& CommonStaticLibraries : ProjectToWrite.CommonStaticLibrarysNeeded)
		{
			OutputFile << '\t' << CommonStaticLibraries << '\n';
		}
		OutputFile << ")\n";

		OutputFile << "set(COMMON_LIBRARIES ${COMMON_STATIC_LIBRARIES} ${COMMON_DYNAMIC_LIBRARIES})\n";


		for (auto const& TargetData : ProjectToWrite.TargetsData)
		{
			OutputFile << "\n";
			std::string CmakeTargetName = ProjectToWrite.ProjectName+'_';
			bool IsDebug = false;
			bool IsStatic = true;
			if (TargetData.first == MBPM::MBPM_CompileOutputConfiguration::DynamicDebug || TargetData.first == MBPM_CompileOutputConfiguration::DynamicRelease)
			{
				CmakeTargetName += "Dynamic";
				IsStatic = false;
			}
			else
			{
				CmakeTargetName += "Static";
			}
			if (TargetData.first == MBPM_CompileOutputConfiguration::StaticDebug || TargetData.first == MBPM_CompileOutputConfiguration::DynamicDebug)
			{
				CmakeTargetName += "Debug";
				IsDebug = true;
			}
			else
			{
				CmakeTargetName += "Release";
			}

			std::string ExtraSourceFiles = "";
			//skapar target Sources
			if (TargetData.second.ExplicitSourceFiles.size() > 0)
			{
				OutputFile << "set(" + CmakeTargetName + "_Sources \n";
				for (auto const& Source : TargetData.second.ExplicitSourceFiles)
				{
					OutputFile << '\t' << Source + "\n";
				}
				OutputFile << ")\n";
				ExtraSourceFiles += " {" + CmakeTargetName + "_Sources}";
			}
			//target headers
			if(TargetData.second.ExplicitHeaders.size() > 0)
			{
				OutputFile << "set(" + CmakeTargetName + "_Headers \n";
				for (auto const& Header : TargetData.second.ExplicitHeaders)
				{
					OutputFile << '\t' << Header << '\n';
				}
				ExtraSourceFiles += " {" + CmakeTargetName + "_Headers}";
			}

			//skapar targeten
			OutputFile << "add_library(" << CmakeTargetName << " ";
			if (IsStatic)
			{
				OutputFile << "ARCHIVE ";
			}
			else
			{
				OutputFile << "SHARED ";
			}
			OutputFile << "${COMMON_FILES}"+ExtraSourceFiles+")\n";
			//

			//librarys to link
			std::string ExtraLibraries = "";
			if (TargetData.second.StaticLibrarysNeeded.size() > 0)
			{
				OutputFile << "set(" << CmakeTargetName << "_NeededStaticLibraries\n";
				for (auto const& NeededStaticLibrarys : TargetData.second.StaticLibrarysNeeded)
				{
					OutputFile << '\t' << NeededStaticLibrarys << '\n';
				}
				OutputFile << ")\n";
				ExtraLibraries += " ${" + CmakeTargetName + "_NeededStaticLibraries}";
			}
			if (TargetData.second.DynamicLibrarysNeeded.size() > 0)
			{
				OutputFile << "set(" << CmakeTargetName << "_NeededDynamicLibraries\n";
				for (auto const& NeededDynamicLibrarys : TargetData.second.DynamicLibrarysNeeded)
				{
					OutputFile << '\t' << NeededDynamicLibrarys << '\n';
				}
				OutputFile << ")\n";
				ExtraLibraries += " ${" + CmakeTargetName + "_NeededDynamicLibraries}";
			}
			OutputFile<<"set("<<CmakeTargetName<<"_NeededLibraries "<<ExtraLibraries<<")\n";
			OutputFile << "target_link_libraries(" << CmakeTargetName << " ${" << CmakeTargetName << "_NeedLibraries} ${COMMON_LIBRARIES})\n";
			
		}
		return(ReturnValue);
	}

	MBError GenerateCmakeFile(MBPM_PacketInfo const& PacketToConvert, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& OutputName)
	{
		MBError ReturnValue = true;
		MBPM_CmakeProject GeneratedProject;
		ReturnValue = h_GenerateCmakeFile(GeneratedProject, CompileConfiguration, PacketToConvert, PacketDirectory);
		WriteCMakeProjectToFile(GeneratedProject, OutputName);
		return(ReturnValue);
	}
	MBError GenerateCmakeFile(std::string const& PacketPath, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& OutputName)
	{
		MBPM_PacketInfo ProjectToConvert = ParseMBPM_PacketInfo(PacketPath + "MBPM_PacketInfo");
		MBError ReturnValue = GenerateCmakeFile(ProjectToConvert, PacketPath, CompileConfiguration, OutputName);
		return(ReturnValue);
	}


	MBError EmbeddDependancies(std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath)
	{
		MBError ReturnValue = true;
		MBPM_PacketInfo CurrentProject = ParseMBPM_PacketInfo(PacketDirectory + "MBPM_PacketInfo");
		ReturnValue = EmbeddDependancies(PacketDirectory, CompileConfiguration, TargetFilepath);
		return(ReturnValue);
	}
	MBError EmbeddDependancies(MBPM_PacketInfo const& PacketInfo, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath)
	{
		MBError ReturnValue = true;
		MBPM_CmakeProject EmbeddProject;
		//ReturnValue = h_GenerateCmakeFile(EmbeddProject, CompileConfiguration, PacketInfo,PacketDirectory);



		return(ReturnValue);
	}


	MBError CompileAndInstallPacket(std::string const& PacketToCompileDirectory)
	{
		return(MBError());
	}

	MBError DownloadPacket(std::string const& DestinationDirectory, std::string const& URI)
	{
		return(MBError());
	}
};