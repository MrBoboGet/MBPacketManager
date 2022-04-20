#include "MBPacketManager.h"
#include <assert.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <MBUtility/MBCompileDefinitions.h>
#include <MBUtility/MBFiles.h>

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
					ProjectToPopulate.CommonHeaders.push_back(ProjectName + "/" + RelativeFilePath);
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
				MBPM_PacketInfo DependancyInfo;
				if (std::filesystem::exists(DependancyPath + "MBPM_PacketInfo"))
				{
					DependancyInfo = ParseMBPM_PacketInfo(DependancyPath + "MBPM_PacketInfo");
				}
				else
				{
					GenerationError = false;
					GenerationError.ErrorMessage = "Missing Dependancy \""+Dependancies+"\"";
					return(GenerationError);
				}
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
			throw std::runtime_error("MBPM_PACKETS_INSTALL_DIRECTORY environment variable not set");
			return("");
		}
		else
		{
			return(std::string(Result)+"/");
		}
	}
//	MBError SetSystemPacketsDirectory(std::string const& DirectoryPath)
//	{
//		MBError ReturnValue = true;
//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
//		_putenv_s("MBPM_PACKETS_DIRECTORY", DirectoryPath.c_str());
//#else
//		setenv("MBPM_PACKETS_DIRECTORY", DirectoryPath.c_str(),true);
//#endif // 
//		return(ReturnValue);
//	}


	MBPM_CompileOutputConfiguration h_StringToConfiguration(std::string const& StringToInspect)
	{
		MBPM_CompileOutputConfiguration ReturnValue = MBPM_CompileOutputConfiguration::Null;
		if (StringToInspect == "StaticDebug")
		{
			ReturnValue = MBPM_CompileOutputConfiguration::StaticDebug;
		}
		if (StringToInspect == "StaticRelease")
		{
			ReturnValue = MBPM_CompileOutputConfiguration::StaticRelease;
		}
		if (StringToInspect == "DynamicDebug")
		{
			ReturnValue = MBPM_CompileOutputConfiguration::DynamicDebug;
		}
		if (StringToInspect == "DynamicRelease")
		{
			ReturnValue = MBPM_CompileOutputConfiguration::DynamicRelease;
		}
		return(ReturnValue);
	}

	MBPM_PacketInfo ParseMBPM_PacketInfo(std::string const& PacketPath)
	{
		std::string JsonData = std::string(std::filesystem::file_size(PacketPath), 0);
		std::ifstream PacketFile = std::ifstream(PacketPath, std::ios::in | std::ios::binary);
		PacketFile.read(JsonData.data(), JsonData.size());
		MBError ParseError = true;
		MBParsing::JSONObject ParsedJson = MBParsing::ParseJSONObject(JsonData,0,nullptr,&ParseError);
		MBPM_PacketInfo ReturnValue;
		try
		{
			//if (!ParseError)
//{
//	return(ReturnValue);
//}
			ReturnValue.PacketName = ParsedJson.GetAttribute("PacketName").GetStringData();
			for (auto const& Attribute : ParsedJson.GetAttribute("Attributes").GetArrayData())
			{
				if (Attribute.GetStringData() == "Embeddable")
				{
					ReturnValue.Attributes.insert(MBPM_PacketAttribute::Embedabble);
				}
				if (Attribute.GetStringData() == "NonMBBuild")
				{
					ReturnValue.Attributes.insert(MBPM_PacketAttribute::NonMBBuild);
				}
				if (Attribute.GetStringData() == "IncludeOnly")
				{
					ReturnValue.Attributes.insert(MBPM_PacketAttribute::IncludeOnly);
				}
				if (Attribute.GetStringData() == "TriviallyCompilable")
				{
					ReturnValue.Attributes.insert(MBPM_PacketAttribute::TriviallyCompilable);
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
			//ExportedExecutableTargets
			if (ParsedJson.HasAttribute("ExportedExecutableTargets"))
			{
				MBParsing::JSONObject& ExecutableTargets = ParsedJson.GetAttribute("ExportedExecutableTargets");
				if (ExecutableTargets.GetType() == MBParsing::JSONObjectType::Array)
				{
					std::vector<MBParsing::JSONObject>& Targets = ExecutableTargets.GetArrayData();
					for (size_t i = 0; i < Targets.size(); i++)
					{
						if (Targets[i].GetType() == MBParsing::JSONObjectType::String)
						{
							ReturnValue.ExportedTargets.push_back(Targets[i].GetStringData());
						}
					}
				}
			}
			//F�r include
			if (ParsedJson.HasAttribute("ExtraIncludeDirectories"))
			{
				MBParsing::JSONObject const& IncludeDirectoryObjects = ParsedJson.GetAttribute("ExtraIncludeDirectories");
				if (IncludeDirectoryObjects.GetType() != MBParsing::JSONObjectType::Array)
				{
					//throw std::runtime_error("ErrorIncludeDirectories not of array type!");
					ReturnValue = MBPM_PacketInfo();
					return(ReturnValue);
				}
				std::vector<MBParsing::JSONObject> const& IncludeDirectories = IncludeDirectoryObjects.GetArrayData();
				for (size_t i = 0; i < IncludeDirectories.size(); i++)
				{
					if (IncludeDirectories[i].GetType() != MBParsing::JSONObjectType::String)
					{
						ReturnValue = MBPM_PacketInfo();
						return(ReturnValue);
					}
					ReturnValue.ExtraIncludeDirectories.push_back(IncludeDirectories[i].GetStringData());
				}
			}
			//F�r sub libraries
			if (ParsedJson.HasAttribute("SubLibraries"))
			{
				std::vector<MBParsing::JSONObject> const& SubLibraries = ParsedJson.GetAttribute("SubLibraries").GetArrayData();
				for (size_t i = 0; i < SubLibraries.size(); i++)
				{
					MBPM_SubLibrary NewSubLibrary;
					NewSubLibrary.LibraryName = SubLibraries[i].GetAttribute("LibraryName").GetStringData();
					std::vector<MBParsing::JSONObject> const& LibraryOutputConfiguratiosn = SubLibraries[i].GetAttribute("OutputConfigurations").GetArrayData();
					for (size_t i = 0; i < LibraryOutputConfiguratiosn.size(); i++)
					{
						NewSubLibrary.OutputConfigurations.push_back(h_StringToConfiguration(LibraryOutputConfiguratiosn[i].GetStringData()));
					}
					ReturnValue.SubLibraries.push_back(NewSubLibrary);
				}
			}
			//

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
		}
		catch(std::exception const& e)
		{
			ReturnValue = MBPM_PacketInfo();
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
	std::vector<std::string> h_GetTotalPacketDependancies(MBPM_PacketInfo const& PacketInfo, MBError* OutError)
	{
		std::vector<std::string> ReturnValue = {};
		std::unordered_set<std::string> VisitedPackets = {};
		std::stack<std::string> DependanciesToTraverse = {};
		std::string PacketsDirectory = GetSystemPacketsDirectory();
		for (size_t i = 0; i < PacketInfo.PacketDependancies.size(); i++)
		{
			DependanciesToTraverse.push(PacketInfo.PacketDependancies[i]);
		}
		while (DependanciesToTraverse.size() > 0)
		{
			std::string CurrentPacket = DependanciesToTraverse.top();
			DependanciesToTraverse.pop();
			if (VisitedPackets.find(CurrentPacket) == VisitedPackets.end())
			{
				VisitedPackets.insert(CurrentPacket);
				if (std::filesystem::exists(PacketsDirectory + "/" + CurrentPacket + "/MBPM_PacketInfo"))
				{
					MBPM_PacketInfo DependancyPacket = ParseMBPM_PacketInfo(PacketsDirectory + "/" + CurrentPacket + "/MBPM_PacketInfo");
					if (DependancyPacket.Attributes.find(MBPM_PacketAttribute::IncludeOnly) == DependancyPacket.Attributes.end())
					{
						ReturnValue.push_back(CurrentPacket);
					}
					if (DependancyPacket.PacketName != "")
					{
						for (size_t i = 0; i < DependancyPacket.PacketDependancies.size(); i++)
						{
							if (VisitedPackets.find(DependancyPacket.PacketDependancies[i]) == VisitedPackets.end())
							{
								DependanciesToTraverse.push(DependancyPacket.PacketDependancies[i]);
							}
						}
					}
					else
					{
						*OutError = false;
						OutError->ErrorMessage = "Missing dependancy MBPM_PacketInfo \"" + CurrentPacket + "\"";
						return(ReturnValue);
					}
				}
				else
				{
					*OutError = false;
					OutError->ErrorMessage = "Missing dependancy \"" + CurrentPacket + "\"";
					return(ReturnValue);
				}
			}
		}
		return(ReturnValue);
	}
	std::vector<MBPM_PacketInfo> h_GetDependanciesInfo(std::vector<std::string> const& DependanciesToCheck)
	{
		std::vector<MBPM_PacketInfo> ReturnValue;
		std::string InstallDirectory = GetSystemPacketsDirectory();
		for (size_t i = 0; i < DependanciesToCheck.size(); i++)
		{
			ReturnValue.push_back(ParseMBPM_PacketInfo(InstallDirectory+"/"+DependanciesToCheck[i]+"/MBPM_PacketInfo"));
		}
		return(ReturnValue);
	}
	void h_WriteMBPMCmakeValues(std::vector<std::string> const& Dependancies, std::ofstream& OutputFile,std::string const& StaticMBPMData)
	{
		OutputFile << "##BEGIN MBPM_VARIABLES\n";
		OutputFile << "set(MBPM_DEPENDENCIES \n";

		std::vector<MBPM_PacketInfo> TargetDependancies = h_GetDependanciesInfo(Dependancies);

		for (auto const& Packet : TargetDependancies)
		{
			if (Packet.OutputConfigurationTargetNames.size() > 0)
			{
				OutputFile << "\t" <<"\""<< Packet.PacketName<<"\"" << "\n";
			}
			for (size_t i = 0; i < Packet.SubLibraries.size(); i++)
			{
				OutputFile << "\t"<<"\""<<Packet.PacketName<<"#" << Packet.SubLibraries[i].LibraryName<<"\""<<"\n";
			}
		}
		OutputFile << ")\n";
		
		//System libraries
		OutputFile << "set(MBPM_SystemLibraries\n";
		if constexpr(MBUtility::IsWindows())
		{
			OutputFile << "Ws2_32.lib\n"
				"Secur32.lib\n"
				"Bcrypt.lib\n"
				"Mfplat.lib\n"
				"opengl32\n"
				"Mfuuid.lib\n	"
				"Strmiids.lib\n)\n";
		}
		else//ska egentligen kolla om det �r unix...
		{
			OutputFile <<	"pthread\n"
							"dl\n"
							"stdc++fs"
							"m\n)\n";
		}
		//

		//Include directories
		OutputFile << "set(MBPM_DependanciesIncludeDirectories\n";
		for (size_t i = 0; i < TargetDependancies.size(); i++)
		{
			for (size_t j = 0; j < TargetDependancies[i].ExtraIncludeDirectories.size(); j++)
			{
				//Verifiera pathen...
				OutputFile << "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/" << TargetDependancies[i].PacketName << "/" << TargetDependancies[i].ExtraIncludeDirectories[j]<<"\n";
			}
		}
		OutputFile << ")\n";
		//


		OutputFile << "set(MBPM_TARGET_EXTPACKET_LIBRARIES )\n";
		OutputFile << "set(MBPM_TARGET_COMPILE_OPTIONS )\n";
		OutputFile << "set(MBPM_TARGET_LINK_OPTIONS )\n";
		//Output f�r att overrida CMAKE_CXX_FLAGS
		//OutputFile << "set(MBPM_CXX_FLAGS ${CMAKE_CXX_FLAGS})\n";
		//OutputFile << "set(MBPM_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})\n";
		//OutputFile << "set(MBPM_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})\n";
		//
		if (StaticMBPMData == "")
		{
			OutputFile << "#MBPM_Functions";
			OutputFile << GetMBPMCmakeFunctions() << '\n';
			OutputFile << "##END MBPM_VARIABLES\n";
		}
		else
		{
			OutputFile << "#MBPM_Functions";
			OutputFile << StaticMBPMData<<'\n';
			OutputFile << "##END MBPM_VARIABLES\n";
		}

	}
	MBError WriteCMakeProjectToFile(MBPM_CmakeProject const& ProjectToWrite, std::string const& OutputFilePath)
	{
		MBError ReturnValue = true;
		std::ofstream OutputFile = std::ofstream(OutputFilePath, std::ios::out | std::ios::binary);
		OutputFile << "project("+ProjectToWrite.ProjectName+")\n";
		OutputFile << "set(PROJECT_NAME \"" + ProjectToWrite.ProjectName + "\")\n";
		
		//MBPM variablar
		std::vector<std::string> Dependancies = {};
		for (auto const& Packets : ProjectToWrite.ProjectPacketsDepandancies)
		{
			Dependancies.push_back(Packets);
		}
		h_WriteMBPMCmakeValues(Dependancies, OutputFile,"");
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
	MBError UpdateCmakeMBPMVariables(std::string const& PacketPath,std::string const& StaticMBPMData)
	{
		MBError ReturnValue = true;
		if (!std::filesystem::exists(PacketPath + "/MBPM_PacketInfo") || !std::filesystem::exists(PacketPath+"/CMakeLists.txt"))
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "MBPM_PacketInfo or CMakeLists.txt doesn't exist";
			return(ReturnValue);
		}
		std::ifstream InputFile = std::ifstream(PacketPath + "/CMakeLists.txt",std::ios::in);
		std::string CurrentLine = "";
		std::vector<std::string> FileLines = {};
		while (std::getline(InputFile,CurrentLine))
		{
			if (CurrentLine != "" && CurrentLine.back() == '\r')
			{
				CurrentLine.resize(CurrentLine.size() - 1);
			}
			FileLines.push_back(CurrentLine);
		}
		InputFile.close();
		bool ContainsBegin = false;
		bool ContainsEnd = false;
		for (size_t i = 0; i < FileLines.size(); i++)
		{
			if (FileLines[i] == "##BEGIN MBPM_VARIABLES")
			{
				ContainsBegin = true;
			}
			if (ContainsBegin == true && FileLines[i] == "##END MBPM_VARIABLES")
			{
				ContainsEnd = true;
			}
		}
		if (!(ContainsBegin && ContainsEnd))
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "File doesn't contain valid MBPM_Variables";
			return(ReturnValue);
		}
		MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
		std::vector<std::string> TotalPacketDependancies = h_GetTotalPacketDependancies(PacketInfo,&ReturnValue);
		if (!ReturnValue)
		{
			return(ReturnValue);
		}
		std::ofstream OutputFile = std::ofstream(PacketPath + "/CMakeLists.txt", std::ios::out);
		bool InMBPMVariables = false;
		for (size_t i = 0; i < FileLines.size(); i++)
		{
			if (FileLines[i] == "##BEGIN MBPM_VARIABLES")
			{
				InMBPMVariables = true;
				h_WriteMBPMCmakeValues(TotalPacketDependancies, OutputFile,StaticMBPMData);
			}
			if (!InMBPMVariables)
			{
				OutputFile << FileLines[i] << std::endl;
			}
			if (InMBPMVariables && FileLines[i] == "##END MBPM_VARIABLES")
			{
				InMBPMVariables = false;
			}
		}
		return(ReturnValue);
	}

	MBError GenerateCmakeFile(MBPM_PacketInfo const& PacketToConvert, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& OutputName)
	{
		MBError ReturnValue = true;
		MBPM_CmakeProject GeneratedProject;
		ReturnValue = h_GenerateCmakeFile(GeneratedProject, CompileConfiguration, PacketToConvert, PacketDirectory);
		for(auto const& OutputOption : CompileConfiguration.SupportedLibraryConfigurations)
		{
			GeneratedProject.TargetsData[OutputOption];
		}
		if (ReturnValue)
		{
			WriteCMakeProjectToFile(GeneratedProject, PacketDirectory + "/" + OutputName);
		}
		return(ReturnValue);
	}
	MBError GenerateCmakeFile(std::string const& PacketPath, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& OutputName)
	{
		MBPM_PacketInfo ProjectToConvert = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
		MBError ReturnValue = GenerateCmakeFile(ProjectToConvert, PacketPath, CompileConfiguration, OutputName);
		return(ReturnValue);
	}
	MBError GenerateCmakeFile(std::string const& PacketPath, std::string const& CmakeName)
	{
		MBPM_PacketInfo PacketToConvert = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
		MBPM_MakefileGenerationOptions OptionsToUse;
		OptionsToUse.SupportedLibraryConfigurations = PacketToConvert.SupportedOutputConfigurations;
		return(GenerateCmakeFile(PacketToConvert, PacketPath, OptionsToUse, CmakeName));
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
	struct CompileTargetInfo_MSBuild
	{
		std::string Solution = "";
		std::string Target = "";
		std::string Configuration = "";
		std::string Platform = "";
		std::string OutputPath = "";
		std::string NewName = "";
	};
	MBError h_ParseCompileTargetInfo_MSBuild(MBParsing::JSONObject const& ObjectToParse,CompileTargetInfo_MSBuild* OutResult)
	{
		MBError ReturnValue = true;
		CompileTargetInfo_MSBuild NewTarget;
		NewTarget.Target = ObjectToParse.GetAttribute("Target").GetStringData();
		NewTarget.Solution = ObjectToParse.GetAttribute("Solution").GetStringData();
		NewTarget.Configuration = ObjectToParse.GetAttribute("Configuration").GetStringData();
		NewTarget.Platform = ObjectToParse.GetAttribute("Platform").GetStringData();
		NewTarget.OutputPath = ObjectToParse.GetAttribute("OutputPath").GetStringData();
		NewTarget.NewName = ObjectToParse.GetAttribute("NewName").GetStringData();
		if (ReturnValue)
		{
			*OutResult = std::move(NewTarget);
		}
		return(ReturnValue);
	}
	MBError h_CompileTarget_MSBuild(std::string const& PacketDirectory,MBParsing::JSONObject const& ObjectToParse)
	{
		MBError ReturnValue = true;
		//if (ObjectToParse.GetAttribute("MakeType").GetStringData() != "MSBuild")
		//{
		//	ReturnValue = false;
		//	ReturnValue.ErrorMessage = "Unsuported MakeType for windows platform: "+ ObjectToParse.GetAttribute("MakeType").GetStringData();
		//	return(ReturnValue);
		//}
		CompileTargetInfo_MSBuild TargetInfo;
		ReturnValue = h_ParseCompileTargetInfo_MSBuild(ObjectToParse, &TargetInfo);
		if(!ReturnValue)
		{
			return(ReturnValue);
		}
		std::string CommandToExecute = "cd " + PacketDirectory + "& msbuild " + TargetInfo.Solution + " /t:" + TargetInfo.Target + " /p:Configuration=" + TargetInfo.Configuration;
		CommandToExecute += " /p:Platform=" + TargetInfo.Platform;
		int CommandResult = std::system(CommandToExecute.c_str());
		if (CommandResult != 0)
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Failed compiling target";
		}
		else
		{
			if (!std::filesystem::exists(PacketDirectory+"/MBPM_Builds/"))
			{
				std::filesystem::create_directories(PacketDirectory + "/MBPM_Builds/");
			}
			//nu ska vi ta och flytta den till dess korrekta st�lle
			std::filesystem::rename(PacketDirectory + "/" + TargetInfo.OutputPath, PacketDirectory + "/MBPM_Builds/" + TargetInfo.NewName);
		}
		return(ReturnValue);
	}
	MBError h_CompileTarget(std::string const& PacketDirectory, MBParsing::JSONObject const& ObjectToParse);
	//struct CompileTargetInfo_Cmake
	//{
	//	//std::vector<std::string> CMakeOptions = {};
	//	MBParsing::JSONObject CMakeTarget;
	//};
	struct CompileInfo_CMake
	{
		std::string BuildConfiguration = "";
		std::vector<std::string> BuildOptions = {};
		std::vector<std::pair<std::string, std::string>> FilesToMove = {};
	};
	MBError h_ParseCompileInfo_CMake(MBParsing::JSONObject const& ObjectToParse,CompileInfo_CMake* OutInfo)
	{
		MBError ReturnValue = true;
		CompileInfo_CMake ParsedInfo;
		try
		{
			if (ObjectToParse.HasAttribute("BuildConfiguration"))
			{
				ParsedInfo.BuildConfiguration = ObjectToParse.GetAttribute("BuildConfiguration").GetStringData();
			}
			std::vector<MBParsing::JSONObject> const& BuildOptions = ObjectToParse.GetAttribute("BuildOptions").GetArrayData();
			for (size_t i = 0; i < BuildOptions.size(); i++)
			{
				ParsedInfo.BuildOptions.push_back(BuildOptions[i].GetStringData());
			}
			std::vector<MBParsing::JSONObject> const& FilesToMove = ObjectToParse.GetAttribute("FilesToMove").GetArrayData();
			for (size_t i = 0; i < FilesToMove.size(); i++)
			{
				ParsedInfo.FilesToMove.push_back(std::pair<std::string, std::string>(FilesToMove[i].GetArrayData()[0].GetStringData(), FilesToMove[i].GetArrayData()[1].GetStringData()));
			}
			*OutInfo = std::move(ParsedInfo);
		}
		catch (const std::exception& e)
		{
			*OutInfo = CompileInfo_CMake();
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Failed parsing CMake compile info: ";
			ReturnValue.ErrorMessage += e.what();
		}
		return(ReturnValue);
	}
	MBError h_Compile_CMakeBuild(std::string const& PacketDirectory, MBParsing::JSONObject const& ObjectToParse)
	{
		MBError ReturnValue = true;
		CompileInfo_CMake CompileInfo;
		ReturnValue = h_ParseCompileInfo_CMake(ObjectToParse, &CompileInfo);
		if (!ReturnValue)
		{
			return(ReturnValue);
		}
		std::string CompileCommand = "cmake --build " + PacketDirectory + "/MBPM_BuildFiles/ ";
		if (CompileInfo.BuildConfiguration != "")
		{
			CompileCommand += "--config " + CompileInfo.BuildConfiguration;
		}
		for (size_t i = 0; i < CompileInfo.BuildOptions.size(); i++)
		{
			CompileCommand += " -D" + CompileInfo.BuildOptions[i];
		}
		int BuildResult = std::system(CompileCommand.c_str());
		//DEBUG
		//std::cout << "Build Result: " << BuildResult << std::endl;
		//DEBUG
		if (BuildResult == 0)
		{
			try
			{
				for (size_t i = 0; i < CompileInfo.FilesToMove.size(); i++)
				{
					if (!std::filesystem::exists(std::filesystem::path(PacketDirectory + CompileInfo.FilesToMove[i].second).parent_path()))
					{
						std::filesystem::create_directories(std::filesystem::path(PacketDirectory + CompileInfo.FilesToMove[i].second).parent_path());
					}
					std::filesystem::rename(PacketDirectory+"/"+CompileInfo.FilesToMove[i].first, PacketDirectory+"/"+ CompileInfo.FilesToMove[i].second);
					//DEBUG
					//std::cout << "Moving file: " << CompileInfo.FilesToMove[i].first << " to: " << CompileInfo.FilesToMove[i].second << std::endl;
					//DEBUG
				}
			}
			catch (const std::exception& e)
			{
				ReturnValue = false;
				ReturnValue.ErrorMessage = "Failed moving files: ";
				ReturnValue.ErrorMessage += e.what();
			}
		}
		else
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Failed to compile with CMakeBuild";
		}
		return(ReturnValue);
	}
	MBError h_CompileTarget_CMake(std::string const& PacketDirectory, MBParsing::JSONObject const& ObjectToParse)
	{
		MBError ReturnValue = true;
		//CompileTargetInfo_Cmake TargetInfo;
		std::vector<MBParsing::JSONObject> const& Options = ObjectToParse.GetAttribute("CMakeOptions").GetArrayData();
		std::vector<std::string> CMakeOptions = {};
		for (size_t i = 0; i < Options.size(); i++)
		{
			CMakeOptions.push_back(Options[i].GetStringData());
		}
		//TargetInfo.CMakeTarget = ObjectToParse.GetAttribute("CMakeTarget");
		std::string CMakeCommand = "cmake -S " + PacketDirectory + " -B " + PacketDirectory + "/MBPM_BuildFiles/ ";
		for (size_t i = 0; i < CMakeOptions.size(); i++)
		{
			CMakeCommand += "-D" + CMakeOptions[i]+" ";
		}
		int CommandResult = std::system(CMakeCommand.c_str());
		if (CommandResult == 0)
		{
			//h_CompileTarget(PacketDirectory, TargetInfo.CMakeTarget);
			if (ObjectToParse.HasAttribute("CMakeTarget"))
			{
				ReturnValue = h_CompileTarget(PacketDirectory, ObjectToParse.GetAttribute("CMakeTarget"));
			}
			else if (ObjectToParse.HasAttribute("CMakeBuild"))
			{
				ReturnValue = h_Compile_CMakeBuild(PacketDirectory, ObjectToParse.GetAttribute("CMakeBuild"));
			}
			else
			{
				ReturnValue = false;
				ReturnValue.ErrorMessage = "Error compiling packet: No valid build info";
			}
		}
		else
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Error compiling packet";
		}
		return(ReturnValue);
	}
	struct CompileTargetInfo_Make
	{
		std::string Target = "";
		std::string Directory = "";
		std::string OutputPath = "";
		std::string NewName = "";
	};
	MBError h_CompileTarget_Make(std::string const& PacketDirectory, MBParsing::JSONObject const& ObjectToParse)
	{
		MBError ReturnValue = true;
		CompileTargetInfo_Make TargetInfo;
		TargetInfo.Target = ObjectToParse.GetAttribute("Target").GetStringData();
		TargetInfo.Directory = ObjectToParse.GetAttribute("Directory").GetStringData();
		TargetInfo.OutputPath = ObjectToParse.GetAttribute("OutputPath").GetStringData();
		TargetInfo.NewName = ObjectToParse.GetAttribute("NewName").GetStringData();

		std::string Command = "make -C " + PacketDirectory + TargetInfo.Directory + " " + TargetInfo.Target;

		int CommandResult = std::system(Command.c_str());
		if (CommandResult == 0)
		{
			if (!std::filesystem::exists(PacketDirectory + "/MBPM_Builds/"))
			{
				std::filesystem::create_directories(PacketDirectory + "/MBPM_Builds/");
			}
			std::filesystem::rename(PacketDirectory + "/" + TargetInfo.OutputPath, PacketDirectory + "/MBPM_Builds/" + TargetInfo.NewName);
		}
		else
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Failed executing build command";
		}

		return(ReturnValue);
	}
	MBError h_CompileTarget(std::string const& PacketDirectory, MBParsing::JSONObject const& ObjectToParse)
	{
		MBError ReturnValue = true;
		if (ObjectToParse.GetAttribute("MakeType").GetStringData() == "MSBuild")
		{
			ReturnValue = h_CompileTarget_MSBuild(PacketDirectory, ObjectToParse);
		}
		else if(ObjectToParse.GetAttribute("MakeType").GetStringData() == "CMake")
		{
			ReturnValue = h_CompileTarget_CMake(PacketDirectory, ObjectToParse);
		}
		else if (ObjectToParse.GetAttribute("MakeType").GetStringData() == "Make")
		{
			ReturnValue = h_CompileTarget_Make(PacketDirectory, ObjectToParse);
		}
		else
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "Unkown build type: " + ObjectToParse.GetAttribute("MakeType").GetStringData();
		}
		return(ReturnValue);
	}
	MBError h_CompilePacket_CompileInfo(std::string const& PacketDirectory)
	{
		MBError ReturnValue = true;
		if (!std::filesystem::exists(PacketDirectory + "/MBPM_CompileInfo.json"))
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "No compile MBPM_CompileInfo.json in directory";
			return(ReturnValue);
		}
		try
		{
			std::string CompileInfoData = MBUtility::ReadWholeFile(PacketDirectory + "/MBPM_CompileInfo.json");
			MBParsing::JSONObject CompileInfoObject = MBParsing::ParseJSONObject(CompileInfoData, 0, nullptr, &ReturnValue);
			if (!ReturnValue)
			{
				return(ReturnValue);
			}
			if (MBUtility::IsWindows())
			{
				std::map<std::string,MBParsing::JSONObject> const& WindowsTargets = CompileInfoObject.GetAttribute("Windows").GetMapData();
				for (auto const& Targets : WindowsTargets)
				{
					ReturnValue = h_CompileTarget(PacketDirectory,Targets.second);
					if (!ReturnValue)
					{
						break;
					}
				}
			}
			//Antar nu att det �r linux eller unix
			else
			{
				std::map<std::string, MBParsing::JSONObject> CurrentTargets = {};
				if (CompileInfoObject.HasAttribute("Linux"))
				{
					CurrentTargets = CompileInfoObject.GetAttribute("Linux").GetMapData();
				}
				else if (CompileInfoObject.HasAttribute("Unix"))
				{
					CurrentTargets = CompileInfoObject.GetAttribute("Unix").GetMapData();
				}
				else
				{
					ReturnValue = false;
					ReturnValue.ErrorMessage = "No targets for current operating system";
					return(ReturnValue);
				}
				for (auto const& Targets : CurrentTargets)
				{
					ReturnValue = h_CompileTarget(PacketDirectory, Targets.second);
					if (!ReturnValue)
					{
						break;
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = e.what();
		}
		return(ReturnValue);
	}
	MBError h_CompilePacket_MBBuild(std::string const& PacketDirectory)
	{
		MBError ReturnValue = false;
		int CommandReturnValue = 0;
		CommandReturnValue = std::system(("cmake -S " + PacketDirectory + " -B " + PacketDirectory + "/MBPM_BuildFiles/").c_str());
		CommandReturnValue = std::system(("cmake --build " + PacketDirectory + "/MBPM_BuildFiles/").c_str());
		if (CommandReturnValue == 0)
		{
#ifdef _WIN32
			std::string BuildDirectory = PacketDirectory + "/MBPM_Builds/";
			std::filesystem::copy_options Overwrite = std::filesystem::copy_options::overwrite_existing;
			if (std::filesystem::exists(BuildDirectory + "Debug"))
			{
				std::filesystem::copy(BuildDirectory + "Debug", BuildDirectory, Overwrite);
				std::filesystem::remove_all(BuildDirectory + "Debug");
			}
#endif // 
			ReturnValue = true;
		}
		else
		{
			ReturnValue = false;
		}
		return(ReturnValue);
	}

	MBError CompilePacket(std::string const& PacketDirectory)
	{
		MBError ReturnValue = true;
		MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectory + "/MBPM_PacketInfo");
		if (PacketInfo.Attributes.find(MBPM_PacketAttribute::Embedabble) != PacketInfo.Attributes.end() || PacketInfo.Attributes.find(MBPM_PacketAttribute::TriviallyCompilable) != PacketInfo.Attributes.end())
		{
			ReturnValue = h_CompilePacket_MBBuild(PacketDirectory);
		}
		else if(std::filesystem::exists(PacketDirectory+"/MBPM_CompileInfo.json"))
		{
			ReturnValue = h_CompilePacket_CompileInfo(PacketDirectory);
		}
		else
		{
			ReturnValue = false;
		}
		return(ReturnValue);
	}
	MBError InstallCompiledPacket(std::string const& PacketDirectory)
	{
		MBError ReturnValue = true;
		MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectory + "/MBPM_PacketInfo");
		if (PacketInfo.PacketName != "")
		{
			std::string PacketInstallDirectory = GetSystemPacketsDirectory();
			for (auto const& Configurations : PacketInfo.OutputConfigurationTargetNames)
			{
				std::string NewLibraryName = "";
				if (!std::filesystem::exists(PacketInstallDirectory + "/MBPM_CompiledLibraries"))
				{
					std::filesystem::create_directories(PacketInstallDirectory + "/MBPM_CompiledLibraries");
				}
				if (Configurations.first == MBPM_CompileOutputConfiguration::StaticDebug || Configurations.first == MBPM_CompileOutputConfiguration::StaticRelease)
				{
					if (MBUtility::IsWindows())
					{
						NewLibraryName = Configurations.second + ".lib";
					}
					else
					{
						NewLibraryName = "lib"+Configurations.second + ".a";
					}
				}
				else if (Configurations.first == MBPM_CompileOutputConfiguration::DynamicDebug || Configurations.first == MBPM_CompileOutputConfiguration::DynamicRelease)
				{
					if (MBUtility::IsWindows())
					{
						NewLibraryName = Configurations.second + ".dll";
					}
					else
					{
						NewLibraryName = "lib" + Configurations.second + ".so";
					}
				}
				try
				{
					std::string SymlinkName = PacketInstallDirectory + "/MBPM_CompiledLibraries/" + NewLibraryName;
					std::string TargetName = PacketDirectory + "/MBPM_Builds/";
					if (std::filesystem::is_symlink(SymlinkName))
					{
						std::filesystem::remove(SymlinkName);
					}
					std::filesystem::create_symlink(TargetName, SymlinkName);
				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;
				}
			}
			for (auto const& ExportedExecutables : PacketInfo.ExportedTargets)
			{
				std::string NewExecutableName = ExportedExecutables;
				if (MBUtility::IsWindows())
				{
					NewExecutableName += ".exe";
				}
				if (!std::filesystem::exists(PacketInstallDirectory + "/MBPM_ExportedExecutables"))
				{
					std::filesystem::create_directories(PacketInstallDirectory + "/MBPM_ExportedExecutables");
				}
				try
				{
					std::string SymlinkName = PacketInstallDirectory + "/MBPM_ExportedExecutables/" + NewExecutableName;
					std::string TargetName = PacketDirectory + "/MBPM_Builds/" + NewExecutableName;
					if (std::filesystem::is_symlink(SymlinkName))
					{
						std::filesystem::remove(SymlinkName);
					}
					std::filesystem::create_symlink(TargetName, SymlinkName);
				}
				catch (const std::exception& e)
				{
					std::cout << e.what() << std::endl;
				}

			}
		}
		else
		{
			ReturnValue = false;
			ReturnValue.ErrorMessage = "No packet in packet directory";
		}
		return(ReturnValue);
	}
	MBError CompileAndInstallPacket(std::string const& PacketToCompileDirectory)
	{
		MBError ReturnValue = CompilePacket(PacketToCompileDirectory);
		if (ReturnValue)
		{
			ReturnValue = InstallCompiledPacket(PacketToCompileDirectory);
		}
		return(ReturnValue);
	}
	std::string h_ConfigurationPostfix(MBPM_CompileOutputConfiguration Configuration)
	{
		std::string ReturnValue = "";
		if (Configuration == MBPM_CompileOutputConfiguration::StaticDebug || Configuration == MBPM_CompileOutputConfiguration::StaticRelease)
		{
			ReturnValue += "_S";
		}
		else
		{
			ReturnValue += "_D";
		}
		if(Configuration == MBPM_CompileOutputConfiguration::DynamicDebug || Configuration == MBPM_CompileOutputConfiguration::StaticDebug)
		{
			ReturnValue += "D";
		}
		else
		{
			ReturnValue += "R";
		}
		return(ReturnValue);
	}
	bool PacketIsPrecompiled(std::string const& PacketDirectoryToCheck, MBError* OutError)
	{
		bool ReturnValue = true;
		try
		{
			MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectoryToCheck + "/MBPM_PacketInfo");
			if (PacketInfo.PacketName == "")
			{
				*OutError = false;
				OutError->ErrorMessage = "No packet in packet directory";
				return(false);
			}
			for (size_t i = 0; i < PacketInfo.ExportedTargets.size(); i++)
			{
				std::string TargetPath = PacketDirectoryToCheck+"/MBPM_Builds/" + PacketInfo.ExportedTargets[i];
				if constexpr (MBUtility::IsWindows())
				{
					TargetPath += ".exe";
				}
				if (!std::filesystem::exists(TargetPath))
				{
					ReturnValue = false;
					return(ReturnValue);
				}
				if (!std::filesystem::is_regular_file(TargetPath))
				{
					ReturnValue = false;
					return(ReturnValue);
				}
			}
			for (auto const& Configuration : PacketInfo.OutputConfigurationTargetNames)
			{
				std::string LibraryName = Configuration.second;
				if constexpr (MBUtility::IsWindows())
				{
					LibraryName = LibraryName + ".lib";
				}
				else
				{
					LibraryName = "lib" + LibraryName + ".a";
				}
				if (!std::filesystem::exists(PacketDirectoryToCheck + "/MBPM_Builds/" + LibraryName))
				{
					ReturnValue = false;
					return(ReturnValue);
				}
				if (!std::filesystem::is_regular_file(PacketDirectoryToCheck + "/MBPM_Builds/" + LibraryName))
				{
					ReturnValue = false;
					return(ReturnValue);
				}
			}
			for (size_t i = 0; i < PacketInfo.SubLibraries.size(); i++)
			{
				for (size_t j = 0; j < PacketInfo.SubLibraries[j].OutputConfigurations.size(); j++)
				{
					std::string LibraryName = PacketInfo.SubLibraries[i].LibraryName;
					LibraryName += h_ConfigurationPostfix(PacketInfo.SubLibraries[i].OutputConfigurations[j]);
					if constexpr (MBUtility::IsWindows())
					{
						LibraryName = LibraryName + ".lib";
					}
					else
					{
						LibraryName = "lib" + LibraryName + ".a";
					}
					if (!std::filesystem::exists(PacketDirectoryToCheck + "/MBPM_Builds/" + LibraryName))
					{
						ReturnValue = false;
						return(ReturnValue);
					}
					if (!std::filesystem::is_regular_file(PacketDirectoryToCheck + "/MBPM_Builds/" + LibraryName))
					{
						ReturnValue = false;
						return(ReturnValue);
					}
				}
			}
		}
		catch (std::exception const& e)
		{
			if (OutError != nullptr)
			{
				*OutError = false;
				OutError->ErrorMessage = e.what();
			}
			ReturnValue = false;
		}
		return(ReturnValue);
	}
};