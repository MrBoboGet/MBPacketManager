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
		MBPM_FileInfoExcluder FileExcluder;
		if (std::filesystem::exists(ProjectDirectory + "/MBPM_FileInfoIgnore") && std::filesystem::directory_entry(ProjectDirectory + "/MBPM_FileInfoIgnore").is_regular_file())
		{
			FileExcluder = MBPM_FileInfoExcluder(ProjectDirectory + "/MBPM_FileInfoIgnore");
		}
		for (auto const& Entries : ProjectIterator)
		{
			if (Entries.is_regular_file())
			{
				//Kommer ge fel p� windows med paths som har u8, f�r l�gga till den dependancyn
				std::string RelativeFilePath = std::filesystem::relative(Entries.path(),ProjectDirectory).generic_u8string();
				std::string FilePacketPath = +"/" + RelativeFilePath;
				std::string Filename = Entries.path().filename().generic_u8string();	
				std::string FileEnding = "";
				size_t LastDot = Filename.find_last_of('.');
				if (FileExcluder.Excludes(FilePacketPath) && !FileExcluder.Includes(FilePacketPath))
				{
					continue;
				}
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
					ProjectToPopulate.CommonSources.push_back(ProjectName + "/" + RelativeFilePath);
				}
			}
		}
		return(ReturnValue);
	}
	MBError h_EmbeddPacket_CmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration, MBPM_PacketInfo const& PacketInfo, std::string const& PacketDirectory)
	{
		//Just nu l�gger vi bara till om det g�r, annars tar bort, i framtiden kanske vi vill l�gga till
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
		std::stack<std::string> TotalDependancies = {};
		for (size_t i = 0; i < PacketInfo.PacketDependancies.size(); i++)
		{
			TotalDependancies.push(PacketInfo.PacketDependancies[i]);
			//ProjectToPopulate.ProjectPacketsDepandancies.insert(PacketInfo.PacketDependancies[i]);
		}
		while (TotalDependancies.size() > 0)
		{
			std::string Dependancies = TotalDependancies.top();
			TotalDependancies.pop();
			if (ProjectToPopulate.ProjectPacketsDepandancies.find(Dependancies) == ProjectToPopulate.ProjectPacketsDepandancies.end())
			{
				std::string DependancyPath = GetSystemPacketsDirectory() + Dependancies+"/";
				MBPM_PacketInfo DependancyInfo = ParseMBPM_PacketInfo(DependancyPath +"MBPM_PacketInfo");
				for (auto const& NewDependancies : DependancyInfo.PacketDependancies)
				{
					if (ProjectToPopulate.ProjectPacketsDepandancies.find(NewDependancies) == ProjectToPopulate.ProjectPacketsDepandancies.end())
					{
						TotalDependancies.push(NewDependancies);
					}
				}
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
		char* Result = std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY");
		if (Result == nullptr)
		{
			return("");
		}
		else
		{
			return(Result);
		}
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
		std::string JsonData = std::string(std::filesystem::file_size(PacketPath), 0);
		std::ifstream PacketFile = std::ifstream(PacketPath, std::ios::in | std::ios::binary);
		PacketFile.read(JsonData.data(), JsonData.size());
		MBParsing::JSONObject ParsedJson = MBParsing::ParseJSONObject(JsonData,0,nullptr,nullptr);


		MBPM_PacketInfo ReturnValue;
		ReturnValue.PacketName = ParsedJson.GetAttribute("PacketName").GetStringData();
		for (auto const& Attribute : ParsedJson.GetAttribute("Attributes").GetArrayData())
		{
			if (Attribute.GetStringData() == "Embeddable")
			{
				ReturnValue.Attributes.insert(MBPM_PacketAttribute::Embedabble);
			}
		}
		for (auto const& OutputConfigurations : ParsedJson.GetAttribute("OutputConfigurations").GetArrayData())
		{
			if (OutputConfigurations.GetStringData() == "StaticDebug")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::StaticDebug);
			}
			if (OutputConfigurations.GetStringData() == "StaticRelease")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::StaticRelease);
			}
			if (OutputConfigurations.GetStringData() == "DynamicDebug")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::DynamicDebug);
			}
			if (OutputConfigurations.GetStringData() == "DynamicRelease")
			{
				ReturnValue.SupportedOutputConfigurations.insert(MBPM_CompileOutputConfiguration::DynamicRelease);
			}
		}
		//f�r outout configuratuions
		MBParsing::JSONObject OutputConfiguration = ParsedJson.GetAttribute("OutputTargetNames");
		if (OutputConfiguration.HasAttribute("StaticDebug"))
		{
			ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::StaticDebug] = OutputConfiguration.GetAttribute("StaticDebug").GetStringData();
		}
		if (OutputConfiguration.HasAttribute("StaticRelease"))
		{
			ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::StaticRelease] = OutputConfiguration.GetAttribute("StaticRelease").GetStringData();
		}
		if (OutputConfiguration.HasAttribute("DynamicDebug"))
		{
			ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::DynamicDebug] = OutputConfiguration.GetAttribute("DynamicDebug").GetStringData();
		}
		if (OutputConfiguration.HasAttribute("DynamicRelease"))
		{
			ReturnValue.OutputConfigurationTargetNames[MBPM_CompileOutputConfiguration::DynamicRelease] = OutputConfiguration.GetAttribute("DynamicRelease").GetStringData();
		}
		//ReturnValue.PacketDependancies = ParsedAttributes["Dependancies"];
		for (auto const& Dependancie : ParsedJson.GetAttribute("Dependancies").GetArrayData())
		{
			ReturnValue.PacketDependancies.push_back(Dependancie.GetStringData());
		}
		return(ReturnValue);
	}
	MBError WriteMBPM_PacketInfo(MBPM_PacketInfo const& PacketToWrite)
	{
		return(MBError());
	}
	std::string GetMBPMCmakeFunctions()
	{
		std::string ReturnValue =
#include "MBPM_CMakeFunctions.txt"
			;
		return(ReturnValue);
	}
	MBError WriteCMakeProjectToFile(MBPM_CmakeProject const& ProjectToWrite, std::string const& OutputFilePath)
	{
		MBError ReturnValue = true;
		std::ofstream OutputFile = std::ofstream(OutputFilePath, std::ios::out | std::ios::binary);
		OutputFile << "project("+ProjectToWrite.ProjectName+")\n";
		OutputFile << "set(PROJECT_NAME \"" + ProjectToWrite.ProjectName + "\")\n";
		
		//MBPM variablar
		OutputFile << "##BEGIN MBPM_VARIABLES\n";
		OutputFile << "set(MBPM_DEPENDENCIES \n";
		for (auto const& Packet : ProjectToWrite.ProjectPacketsDepandancies)
		{
			OutputFile << "\t" << Packet<<std::endl;
		}
		OutputFile << ")\n";
		OutputFile << "set(MBPM_TARGET_EXTPACKET_LIBRARIES )\n";

		OutputFile << "set(MBPM_TARGET_COMPILE_OPTIONS )\n";
		OutputFile << "set(MBPM_TARGET_LINK_OPTIONS )\n";
		OutputFile << "##END MBPM_VARIABLES\n";
		OutputFile << "##BEGIN MBPM_FUNCTIONS\n";
		OutputFile << GetMBPMCmakeFunctions();
		OutputFile << "##END MBPM_FUNCTIONS\n";
		//write common sources
		OutputFile << "set(PROJECT_SOURCES \n\n";
		for (std::string const& SourceFiles : ProjectToWrite.CommonSources)
		{
			OutputFile <<"\t\"${CMAKE_CURRENT_SOURCE_DIR}/"<< SourceFiles<<"\"" << std::endl;
		}
		OutputFile << ")\n";
		//write common headers
		OutputFile << "set(PROJECT_HEADERS \n";
		for (std::string const& Headers : ProjectToWrite.CommonHeaders)
		{
			OutputFile << "\t\"${CMAKE_CURRENT_SOURCE_DIR}/"<< Headers <<"\""<< std::endl;
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
				CmakeTargetName += "D";
				IsStatic = false;
			}
			else
			{
				CmakeTargetName += "S";
			}
			if (TargetData.first == MBPM_CompileOutputConfiguration::StaticDebug || TargetData.first == MBPM_CompileOutputConfiguration::DynamicDebug)
			{
				CmakeTargetName += "D";
				IsDebug = true;
			}
			else
			{
				CmakeTargetName += "R";
			}

			//skapar targeten
			OutputFile << "add_library(" << CmakeTargetName << " ";
			if (IsStatic)
			{
				OutputFile << "STATIC ";
			}
			else
			{
				OutputFile << "SHARED ";
			}
			OutputFile << "${COMMON_FILES})\n";
			//

			//librarys to link
			//OutputFile << "MBPM_UpdateTargetLibraries("; //bara n�dv�ndig om vi ska skapa en executable
			//OutputFile << IsStatic ? "\"Static\"" : "\"Dynamic\"";
			//OutputFile << IsDebug ? " \"Debug\")\n" : " \"Release\")\n";
			//OutputFile << "target_link_libraries(" << CmakeTargetName << " ${MBPM_EXTPACKET_LIBRARIES} ${COMMON_LIBRARIES})\n";
			OutputFile << "MBPM_ApplyTargetConfiguration(\""+CmakeTargetName+"\" ";
			OutputFile << (IsStatic ? "\"STATIC\"" : "\"DYNAMIC\"");
			OutputFile << (IsDebug ? " \"DEBUG\")\n" : " \"RELEASE\")\n");
			
			//OutputFile << "set_target_properties(\"" + CmakeTargetName + "\" PROPERTIES PREFIX \"\")\n";
			//OutputFile << "set_target_properties(\"" + CmakeTargetName + "\" PROPERTIES SUFFIX \"\")\n";
			//OutputFile << "set_target_properties(\"" + CmakeTargetName + "\" PROPERTIES OUTPUT_NAME \""+CmakeTargetName+"\")\n";
			OutputFile << "target_compile_features(\"" << CmakeTargetName << "\" PRIVATE cxx_std_17)\n";
			OutputFile << "target_link_libraries(\""<< CmakeTargetName << "\" ${COMMON_LIBRARIES})\n";
			
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
		MBPM_PacketInfo ProjectToConvert = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
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