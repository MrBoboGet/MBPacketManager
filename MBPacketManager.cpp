#include "MBPacketManager.h"
#include "MBParsing/MBParsing.h"
#include "MBUnicode/MBUnicode.h"
#include "MBUtility/MBErrorHandling.h"
#include <assert.h>
#include <exception>
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
	//MBError h_AddDirectoryFiles(MBPM_CmakeProject& ProjectToPopulate, std::string const& ProjectDirectory,std::string const& ProjectName)
	//{
	//	MBError ReturnValue = true;
	//	std::filesystem::recursive_directory_iterator ProjectIterator(ProjectDirectory);
	//	MBPM_FileInfoExcluder FileExcluder;
	//	if (std::filesystem::exists(ProjectDirectory + "/MBPM_FileInfoIgnore") && std::filesystem::directory_entry(ProjectDirectory + "/MBPM_FileInfoIgnore").is_regular_file())
	//	{
	//		FileExcluder = MBPM_FileInfoExcluder(ProjectDirectory + "/MBPM_FileInfoIgnore");
	//	}
	//	for (auto const& Entries : ProjectIterator)
	//	{
	//		if (Entries.is_regular_file())
	//		{
	//			//Kommer ge fel p� windows med paths som har u8, f�r l�gga till den dependancyn
	//			std::string RelativeFilePath = std::filesystem::relative(Entries.path(),ProjectDirectory).generic_u8string();
	//			std::string FilePacketPath = +"/" + RelativeFilePath;
	//			std::string Filename = Entries.path().filename().generic_u8string();	
	//			std::string FileEnding = "";
	//			size_t LastDot = Filename.find_last_of('.');
	//			if (FileExcluder.Excludes(FilePacketPath) && !FileExcluder.Includes(FilePacketPath))
	//			{
	//				continue;
	//			}
	//			if (LastDot != Filename.npos && LastDot != Filename.size()-1)
	//			{
	//				FileEnding = Filename.substr(LastDot + 1);
	//			}
	//			if (FileEnding == "h" || FileEnding == "hpp")
	//			{
	//				ProjectToPopulate.CommonHeaders.push_back(ProjectName + "/" + RelativeFilePath);
	//			}
	//			if (FileEnding == "c" || FileEnding == "cpp")
	//			{
	//				ProjectToPopulate.CommonSources.push_back(ProjectName + "/" + RelativeFilePath);
	//			}
	//		}
	//	}
	//	return(ReturnValue);
	//}
	//MBError h_EmbeddPacket_CmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration, MBPM_PacketInfo const& PacketInfo, std::string const& PacketDirectory)
	//{
	//	//Just nu l�gger vi bara till om det g�r, annars tar bort, i framtiden kanske vi vill l�gga till
	//	MBError ReturnValue = "";
	//	if (PacketInfo.Attributes.find(MBPM_PacketAttribute::TriviallyEmbedabble) != PacketInfo.Attributes.end())
	//	{
	//		ReturnValue = false;
	//		ReturnValue.ErrorMessage = "Error embedding packet\"" + PacketInfo.PacketName + "\": Embedding not supported";
	//		return(ReturnValue);
	//		//if (PacketInfo.SupportedOutputConfigurations.find(MBPM_CompileOutputConfiguration::StaticDebug) == PacketInfo.SupportedOutputConfigurations.end())
	//		//{
	//		//
	//		//}
	//	}
	//	else
	//	{
	//		ReturnValue = h_AddDirectoryFiles(ProjectToPopulate,PacketDirectory,"${MBPM_PACKETS_DIRECTORY}/"+PacketInfo.PacketName);
	//	}
	//	return(ReturnValue);
	//}
	//MBError h_GenerateCmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration,MBPM_PacketInfo const& PacketInfo,std::string const& PacketDirectory)
	//{
	//	MBError GenerationError = true;
	//	bool IsTopPacket = false;
	//	if (ProjectToPopulate.ProjectName == "")
	//	{
	//		IsTopPacket = true;
	//	}
	//	if (IsTopPacket)
	//	{
	//		ProjectToPopulate.ProjectName = PacketInfo.PacketName;
	//	}
	//	std::stack<std::string> TotalDependancies = {};
	//	for (size_t i = 0; i < PacketInfo.PacketDependancies.size(); i++)
	//	{
	//		TotalDependancies.push(PacketInfo.PacketDependancies[i]);
	//		//ProjectToPopulate.ProjectPacketsDepandancies.insert(PacketInfo.PacketDependancies[i]);
	//	}
	//	while (TotalDependancies.size() > 0)
	//	{
	//		std::string Dependancies = TotalDependancies.top();
	//		TotalDependancies.pop();
	//		if (ProjectToPopulate.ProjectPacketsDepandancies.find(Dependancies) == ProjectToPopulate.ProjectPacketsDepandancies.end())
	//		{
	//			std::string DependancyPath = GetSystemPacketsDirectory() + Dependancies+"/";
	//			MBPM_PacketInfo DependancyInfo;
	//			if (std::filesystem::exists(DependancyPath + "MBPM_PacketInfo"))
	//			{
	//				DependancyInfo = ParseMBPM_PacketInfo(DependancyPath + "MBPM_PacketInfo");
	//			}
	//			else
	//			{
	//				GenerationError = false;
	//				GenerationError.ErrorMessage = "Missing Dependancy \""+Dependancies+"\"";
	//				return(GenerationError);
	//			}
	//			for (auto const& NewDependancies : DependancyInfo.PacketDependancies)
	//			{
	//				if (ProjectToPopulate.ProjectPacketsDepandancies.find(NewDependancies) == ProjectToPopulate.ProjectPacketsDepandancies.end())
	//				{
	//					TotalDependancies.push(NewDependancies);
	//				}
	//			}
	//			ProjectToPopulate.ProjectPacketsDepandancies.insert(Dependancies);
	//		}
	//	}
	//	if (IsTopPacket)
	//	{
	//		h_AddDirectoryFiles(ProjectToPopulate, PacketDirectory,"");
	//	}
	//	return(GenerationError);
	//}
	//MBError h_GenerateCmakeFile(MBPM_CmakeProject& ProjectToPopulate, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& PacketDirectory)
	//{
	//	MBError GenerationError = true;
	//	MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectory + "MBPM_PacketInfo");

	//	GenerationError = h_GenerateCmakeFile(ProjectToPopulate, CompileConfiguration, PacketInfo, PacketDirectory);

	//	return(GenerationError);
	//}
	//END PrivateFunctions

	std::string GetSystemPacketsDirectory()
	{
		char* Result = std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY");
		if (Result == nullptr)
		{
			throw std::runtime_error("MBPM_PACKETS_INSTALL_DIRECTORY environment variable not set");
            assert(false && "MBPM_PACKTETS_INSTALL_DIRECTORY not set");
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
                ReturnValue.Attributes.insert(Attribute.GetStringData());
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
			if (ParsedJson.HasAttribute("PacketType"))
			{
                ReturnValue.Type = ParsedJson["PacketType"].GetStringData();
			}
			else
			{
                ReturnValue.Type = "C++";
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
					ReturnValue.SubLibraries.push_back(NewSubLibrary);
				}
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
                    //TODO add to C++ PacketInfo
					//if (DependancyPacket.Attributes.find(MBPM_PacketAttribute::IncludeOnly) == DependancyPacket.Attributes.end())
					//{
					//	ReturnValue.push_back(CurrentPacket);
					//}
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
			ReturnValue.push_back(ParseMBPM_PacketInfo(InstallDirectory + "/" + DependanciesToCheck[i] + "/MBPM_PacketInfo"));
		}
		return(ReturnValue);
	}
	void h_WriteMBPMCmakeValues(std::vector<std::string> const& Dependancies, std::ofstream& OutputFile, std::string const& StaticMBPMData)
	{
		OutputFile << "##BEGIN MBPM_VARIABLES\n";
		OutputFile << "set(MBPM_DEPENDENCIES \n";

		std::vector<MBPM_PacketInfo> TargetDependancies = h_GetDependanciesInfo(Dependancies);

		for (auto const& Packet : TargetDependancies)
		{
			///if(Packet.Attributes.find(MBPM_PacketAttribute::SubOnly) == Packet.Attributes.end())
			///{
			///	OutputFile << "\t" <<"\""<< Packet.PacketName<<"\"" << "\n";
			///}
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
			OutputFile <<	"Ws2_32.lib\n"
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
							"stdc++fs\n"
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
	//MBError WriteCMakeProjectToFile(MBPM_CmakeProject const& ProjectToWrite, std::string const& OutputFilePath)
	//{
	//	MBError ReturnValue = true;
	//	std::ofstream OutputFile = std::ofstream(OutputFilePath, std::ios::out | std::ios::binary);
	//	OutputFile << "project("+ProjectToWrite.ProjectName+")\n";
	//	OutputFile << "set(PROJECT_NAME \"" + ProjectToWrite.ProjectName + "\")\n";
	//	
	//	//MBPM variablar
	//	std::vector<std::string> Dependancies = {};
	//	for (auto const& Packets : ProjectToWrite.ProjectPacketsDepandancies)
	//	{
	//		Dependancies.push_back(Packets);
	//	}
	//	h_WriteMBPMCmakeValues(Dependancies, OutputFile,"");
	//	//write common sources

	//	OutputFile << "set(PROJECT_SOURCES \n\n";
	//	for (std::string const& SourceFiles : ProjectToWrite.CommonSources)
	//	{
	//		OutputFile <<"\t\"${CMAKE_CURRENT_SOURCE_DIR}/"<< SourceFiles<<"\"" << std::endl;
	//	}
	//	OutputFile << ")\n";
	//	//write common headers
	//	OutputFile << "set(PROJECT_HEADERS \n";
	//	for (std::string const& Headers : ProjectToWrite.CommonHeaders)
	//	{
	//		OutputFile << "\t\"${CMAKE_CURRENT_SOURCE_DIR}/"<< Headers <<"\""<< std::endl;
	//	}
	//	OutputFile << ")\n";
	//	OutputFile << "set(COMMON_FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS})\n";

	//	//librarys
	//	OutputFile << "set(COMMON_DYNAMIC_LIBRARIES \n";
	//	for (auto const& CommonDynamicLibraries : ProjectToWrite.CommonDynamicLibrarysNeeded)
	//	{
	//		OutputFile << '\t' << CommonDynamicLibraries << '\n';
	//	}
	//	OutputFile << ")\n";
	//	OutputFile << "set(COMMON_STATIC_LIBRARIES \n";
	//	for(auto const& CommonStaticLibraries : ProjectToWrite.CommonStaticLibrarysNeeded)
	//	{
	//		OutputFile << '\t' << CommonStaticLibraries << '\n';
	//	}
	//	OutputFile << ")\n";

	//	OutputFile << "set(COMMON_LIBRARIES ${MBPM_SystemLibraries} ${COMMON_STATIC_LIBRARIES} ${COMMON_DYNAMIC_LIBRARIES})\n";



	//	return(ReturnValue);
	//}
	MBError UpdateCmakeMBPMVariables(std::string const& PacketPath, std::vector<std::string> const& TotalPacketDependancies, std::string const& StaticMBPMData)
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
		//MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
		//std::vector<std::string> TotalPacketDependancies = h_GetTotalPacketDependancies(PacketInfo,&ReturnValue);
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

	//MBError GenerateCmakeFile(MBPM_PacketInfo const& PacketToConvert, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& OutputName)
	//{
	//	MBError ReturnValue = true;
	//	MBPM_CmakeProject GeneratedProject;
	//	ReturnValue = h_GenerateCmakeFile(GeneratedProject, CompileConfiguration, PacketToConvert, PacketDirectory);
	//	if (ReturnValue)
	//	{
	//		WriteCMakeProjectToFile(GeneratedProject, PacketDirectory + "/" + OutputName);
	//	}
	//	return(ReturnValue);
	//}
	//MBError GenerateCmakeFile(std::string const& PacketPath, MBPM_MakefileGenerationOptions const& CompileConfiguration,std::string const& OutputName)
	//{
	//	MBPM_PacketInfo ProjectToConvert = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
	//	MBError ReturnValue = GenerateCmakeFile(ProjectToConvert, PacketPath, CompileConfiguration, OutputName);
	//	return(ReturnValue);
	//}
	//MBError GenerateCmakeFile(std::string const& PacketPath, std::string const& CmakeName)
	//{
	//	MBPM_PacketInfo PacketToConvert = ParseMBPM_PacketInfo(PacketPath + "/MBPM_PacketInfo");
	//	MBPM_MakefileGenerationOptions OptionsToUse;
	//	return(GenerateCmakeFile(PacketToConvert, PacketPath, OptionsToUse, CmakeName));
	//}

	//MBError EmbeddDependancies(std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath)
	//{
	//	MBError ReturnValue = true;
	//	MBPM_PacketInfo CurrentProject = ParseMBPM_PacketInfo(PacketDirectory + "MBPM_PacketInfo");
	//	ReturnValue = EmbeddDependancies(PacketDirectory, CompileConfiguration, TargetFilepath);
	//	return(ReturnValue);
	//}
	//MBError EmbeddDependancies(MBPM_PacketInfo const& PacketInfo, std::string const& PacketDirectory, MBPM_MakefileGenerationOptions const& CompileConfiguration, std::string const& TargetFilepath)
	//{
	//	MBError ReturnValue = true;
	//	MBPM_CmakeProject EmbeddProject;
	//	//ReturnValue = h_GenerateCmakeFile(EmbeddProject, CompileConfiguration, PacketInfo,PacketDirectory);



	//	return(ReturnValue);
	//}
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
			ReturnValue = true;
		}
		else
		{
			ReturnValue = false;
		}
		return(ReturnValue);
	}

	[[deprecated]]
	//MBError CompilePacket(std::string const& PacketDirectory)
	//{
	//	MBError ReturnValue = true;
	//	MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectory + "/MBPM_PacketInfo");
	//	if (PacketInfo.Attributes.find(MBPM_PacketAttribute::Embedabble) != PacketInfo.Attributes.end() || PacketInfo.Attributes.find(MBPM_PacketAttribute::TriviallyCompilable) != PacketInfo.Attributes.end())
	//	{
	//		ReturnValue = h_CompilePacket_MBBuild(PacketDirectory);
	//	}
	//	else if(std::filesystem::exists(PacketDirectory+"/MBPM_CompileInfo.json"))
	//	{
	//		ReturnValue = h_CompilePacket_CompileInfo(PacketDirectory);
	//	}
	//	else
	//	{
	//		ReturnValue = false;
	//	}
	//	return(ReturnValue);
	//}
	MBError InstallCompiledPacket(std::string const& PacketDirectory)
	{
		MBError ReturnValue = true;
		MBPM_PacketInfo PacketInfo = ParseMBPM_PacketInfo(PacketDirectory + "/MBPM_PacketInfo");
		if (PacketInfo.PacketName != "")
		{
			std::string PacketInstallDirectory = GetSystemPacketsDirectory();
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
	MBError CompileMBCmake(std::string const& PacketDirectory, std::string const& Configuration, std::vector<std::string> const& Targets)
	{
		MBError ReturnValue = false;
		int CommandReturnValue = 0;
		CommandReturnValue = std::system(("cmake -S " + PacketDirectory + " -B " + PacketDirectory + "/MBPM_BuildFiles/"+Configuration+" -DCMAKE_BUILD_TYPE="+Configuration).c_str());
		if (CommandReturnValue != 0)
		{
			ReturnValue = false; 
			return(ReturnValue);
		}
		if (Targets.size() == 0)
		{
			CommandReturnValue = std::system(("cmake --build " + PacketDirectory + "/MBPM_BuildFiles/"+Configuration).c_str());
		}
		else
		{
			for (std::string const& Target : Targets)
			{
				CommandReturnValue = std::system(("cmake --build " + PacketDirectory + "/MBPM_BuildFiles/" + Configuration + " --target "+Target + " --config "+Configuration).c_str());
				if (CommandReturnValue != 0)
				{
					break;
				}
			}
		}
		if (CommandReturnValue == 0)
		{
			ReturnValue = true;
		}
		else
		{
			ReturnValue = false;
		}
		return(ReturnValue);
	}
	MBError CompileAndInstallPacket(std::string const& PacketToCompileDirectory)
	{
		//MBError ReturnValue = CompilePacket(PacketToCompileDirectory);
		//if (ReturnValue)
		//{
		//	ReturnValue = InstallCompiledPacket(PacketToCompileDirectory);
		//}
		//return(ReturnValue);
        return(false);
	}
	bool PacketIsPrecompiled(std::string const& PacketDirectoryToCheck, MBError* OutError)
	{
		bool ReturnValue = false;
		return(ReturnValue);
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
			for (size_t i = 0; i < PacketInfo.SubLibraries.size(); i++)
			{
				
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

    //BEGIN PacketRetriever
    PacketIdentifier PacketRetriever::p_GetInstalledPacket(std::string const& PacketName)
    {
        PacketIdentifier ReturnValue;
        ReturnValue.PacketName = PacketName;
        ReturnValue.PacketLocation = PacketLocationType::Installed;
        ReturnValue.PacketURI = GetSystemPacketsDirectory();
        if (ReturnValue.PacketURI.back() != '/')
        {
            ReturnValue.PacketURI += "/";
        }
        if (std::filesystem::exists(ReturnValue.PacketURI + PacketName) && std::filesystem::exists(ReturnValue.PacketURI + PacketName + "/MBPM_PacketInfo"))
        {
            ReturnValue.PacketURI = ReturnValue.PacketURI + PacketName + "/";
        }
        else
        {
            ReturnValue = PacketIdentifier();
        }
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::p_GetUserPackets()
    {
        std::vector<PacketIdentifier> ReturnValue = {};
        std::set<std::string> SearchDirectories = {};
        std::string PacketInstallDirectory = GetSystemPacketsDirectory();
        //l�ser in config filen
        if (std::filesystem::exists(PacketInstallDirectory + "/LocalUploadPackets.txt"))
        {
            std::ifstream UploadPacketsFile = std::ifstream(PacketInstallDirectory + "/LocalUploadPackets.txt",std::ios::in);
            std::string CurrentLine = "";
            while (std::getline(UploadPacketsFile,CurrentLine))
            {
                SearchDirectories.insert(CurrentLine);
            }
        }
        for (std::string const& Directories : SearchDirectories)
        {
            std::filesystem::directory_iterator CurrentDirectoryIterator = std::filesystem::directory_iterator(Directories);
            for (auto const& Entries : CurrentDirectoryIterator)
            {
                if (Entries.is_directory())
                {
                    std::string EntryPath = MBUnicode::PathToUTF8(Entries.path())+"/";
                    if (std::filesystem::exists(EntryPath+"/MBPM_PacketInfo"))
                    {
                        MBPM::MBPM_PacketInfo CurrentPacket = MBPM::ParseMBPM_PacketInfo(EntryPath + "/MBPM_PacketInfo");
                        if (CurrentPacket.PacketName != "")
                        {
                            PacketIdentifier NewPacket;
                            NewPacket.PacketName = CurrentPacket.PacketName;
                            NewPacket.PacketURI = EntryPath;
                            NewPacket.PacketLocation = PacketLocationType::User;
                            ReturnValue.push_back(NewPacket);
                        }
                    }
                }
            }
        }

        return(ReturnValue);
    }
    MBPM::MBPM_PacketInfo PacketRetriever::p_GetPacketInfo(PacketIdentifier const& PacketToInspect, MBError* OutError)
	{
		MBPM::MBPM_PacketInfo ReturnValue;
		MBError Error = true;
		if (PacketToInspect.PacketLocation == PacketLocationType::Installed)
		{
			std::string CurrentURI = PacketToInspect.PacketURI;
			if (CurrentURI == "")
			{
				CurrentURI = GetSystemPacketsDirectory() + "/" + PacketToInspect.PacketName+"/";
			}
			if (std::filesystem::exists(CurrentURI + "/MBPM_PacketInfo") && std::filesystem::is_regular_file(CurrentURI + "/MBPM_PacketInfo"))
			{
				ReturnValue = MBPM::ParseMBPM_PacketInfo(CurrentURI + "/MBPM_PacketInfo");
			}
			else
			{
				Error = false;
				Error.ErrorMessage = "Can't find user packet";
			}
		}
		else if (PacketToInspect.PacketLocation == PacketLocationType::Remote)
		{
			Error = false;
			//TODO att implementera
			Error.ErrorMessage = "Packet info of remote packet not implemented yet xd";
		}
		else if (PacketToInspect.PacketLocation == PacketLocationType::Local)
		{
			if (std::filesystem::exists(PacketToInspect.PacketURI + "/MBPM_PacketInfo") && std::filesystem::is_regular_file(PacketToInspect.PacketURI + "/MBPM_PacketInfo"))
			{
				ReturnValue = MBPM::ParseMBPM_PacketInfo(PacketToInspect.PacketURI + "/MBPM_PacketInfo");
			}
			else
			{
				Error = false;
				Error.ErrorMessage = "Can't find user packet";
			}
		}
		else if (PacketToInspect.PacketLocation == PacketLocationType::User)
		{
			if (std::filesystem::exists(PacketToInspect.PacketURI + "/MBPM_PacketInfo") && std::filesystem::is_regular_file(PacketToInspect.PacketURI + "/MBPM_PacketInfo"))
			{
				ReturnValue = MBPM::ParseMBPM_PacketInfo(PacketToInspect.PacketURI + "/MBPM_PacketInfo");
			}
			else
			{
				Error = false;
				Error.ErrorMessage = "Can't find user packet";
			}
		}
		if (!Error && OutError != nullptr)
		{
			*OutError = std::move(Error);
		}
		return(ReturnValue);
	}
    uint32_t h_GetPacketDepth2(std::map<std::string, MBPM_PacketDependancyRankInfo>& PacketInfoToUpdate, std::string const& PacketName, std::vector<std::string>* OutDependancies)
    {
        if (PacketInfoToUpdate.find(PacketName) == PacketInfoToUpdate.end())
        {
            OutDependancies->push_back(PacketName);
            return(0);
        }
        MBPM_PacketDependancyRankInfo& PacketToEvaluate = PacketInfoToUpdate[PacketName];
        if (PacketToEvaluate.DependancyDepth != -1)
        {
            return(PacketToEvaluate.DependancyDepth);
        }
        else
        {
            std::vector<std::string>& PacketDependancies = PacketToEvaluate.PacketDependancies;
            if (PacketDependancies.size() == 0)
            {
                PacketToEvaluate.DependancyDepth = 0;
                return(0);
            }
            else
            {
                uint32_t DeepestDepth = 0;
                for (size_t i = 0; i < PacketDependancies.size(); i++)
                {
                    uint32_t NewDepth = h_GetPacketDepth2(PacketInfoToUpdate, PacketDependancies[i], OutDependancies) + 1;
                    if (NewDepth > DeepestDepth)
                    {
                        DeepestDepth = NewDepth;
                    }
                }
                PacketToEvaluate.DependancyDepth = DeepestDepth;
                return(DeepestDepth);
            }
        }
    }
    std::map<std::string, MBPM_PacketDependancyRankInfo> PacketRetriever::p_GetPacketDependancieInfo(
            std::vector<PacketIdentifier> const& InPacketsToCheck,
            MBError& OutError,
            std::vector<std::string>* OutMissing)
    {
        std::map<std::string, MBPM_PacketDependancyRankInfo> ReturnValue;
        std::vector<PacketIdentifier> PacketsToCheck = InPacketsToCheck;
        std::set<std::string> TotalPacketNames;
        while (PacketsToCheck.size() > 0)
        {
            PacketIdentifier NewPacket = PacketsToCheck.back();
            PacketsToCheck.pop_back();
            if (ReturnValue.find(NewPacket.PacketName) != ReturnValue.end())
            {
                continue;
            }
            MBPM_PacketInfo PacketInfo = p_GetPacketInfo(NewPacket, &OutError);
            assert(PacketInfo.PacketName != "");//contract by p_GetPacketInfo
            if (!OutError)
            {
                return(ReturnValue);
            }
            //TotalPacketNames.push_back(PacketInfo.PacketName);
            TotalPacketNames.insert(PacketInfo.PacketName);
            MBPM_PacketDependancyRankInfo NewDependancyInfo;
            NewDependancyInfo.PacketName = PacketInfo.PacketName;
            NewDependancyInfo.PacketDependancies = PacketInfo.PacketDependancies;
            for (auto const& Dependancy : NewDependancyInfo.PacketDependancies)
            {
                if (TotalPacketNames.find(Dependancy) == TotalPacketNames.end())
                {
                    PacketIdentifier NewPacketToCheck;
                    NewPacketToCheck = p_GetInstalledPacket(Dependancy);
                    if (NewPacketToCheck.PacketName == "")
                    {
                        OutMissing->push_back(Dependancy);
                    }
                    else
                    {
                        TotalPacketNames.insert(Dependancy);
                        PacketsToCheck.push_back(std::move(NewPacketToCheck));
                    }
                }
            }
            ReturnValue[NewDependancyInfo.PacketName] = std::move(NewDependancyInfo);
        }
        for (auto const& Name : TotalPacketNames)
        {
			h_GetPacketDepth2(ReturnValue, Name, OutMissing);
        }
        if (OutMissing->size() > 0)
        {
            OutError = false;
            OutError.ErrorMessage = "Missing dependancies";
        }
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::p_GetPacketDependancies_DependancyOrder(std::vector<PacketIdentifier> const& InPacketsToCheck,MBError& OutError, std::vector<std::string>* MissingPackets)
    {
        std::vector<PacketIdentifier> ReturnValue;

        std::map<std::string, MBPM_PacketDependancyRankInfo> PacketDependancyInfo = p_GetPacketDependancieInfo(InPacketsToCheck, OutError, MissingPackets);
        std::set<MBPM_PacketDependancyRankInfo> OrderedPackets;
        for (auto const& Packet : PacketDependancyInfo)
        {
            OrderedPackets.insert(Packet.second);
        }
        for (auto const& Packet : OrderedPackets)
        {
            bool Continue = false;
            PacketIdentifier NewIdentifier = p_GetInstalledPacket(Packet.PacketName);
            for (size_t i = 0; i < InPacketsToCheck.size(); i++)
            {
                if (Packet.PacketName == InPacketsToCheck[i].PacketName)
                {
                    Continue = true;
                    break;
                }
            }
            if (Continue)
            {
                continue;
            }
            ReturnValue.push_back(NewIdentifier);
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::p_GetUserPacket(std::string const& PacketName)
    {
        PacketIdentifier ReturnValue;
        std::vector<PacketIdentifier> AllUserPackets = p_GetUserPackets();
        for (size_t i = 0; i < AllUserPackets.size(); i++)
        {
            if (AllUserPackets[i].PacketName == PacketName)
            {
                ReturnValue = AllUserPackets[i];
            }
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::p_GetLocalPacket(std::string const& PacketPath)
    {
        PacketIdentifier ReturnValue;
        if (std::filesystem::exists(PacketPath + "/MBPM_PacketInfo"))
        {
            MBPM::MBPM_PacketInfo PacketInfo = MBPM::ParseMBPM_PacketInfo(PacketPath+"/MBPM_PacketInfo");
            if (PacketInfo.PacketName != "")
            {
                ReturnValue.PacketName = PacketInfo.PacketName;
                ReturnValue.PacketURI = PacketPath;
                ReturnValue.PacketLocation = PacketLocationType::Local;
            }
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::p_GetRemotePacketIdentifier(std::string const& PacketName)
    {
        PacketIdentifier ReturnValue;
        ReturnValue.PacketLocation = PacketLocationType::Remote;
        ReturnValue.PacketName = PacketName;
        ReturnValue.PacketURI = "default";
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::GetInstalledPacket(std::string const& PacketName)
    {
        return(p_GetInstalledPacket(PacketName));
    }
    PacketIdentifier PacketRetriever::GetUserPacket(std::string const& PacketName)
    {
        return(p_GetUserPacket(PacketName)); 
    }
    std::vector<PacketIdentifier> PacketRetriever::GetPacketDependancies(PacketIdentifier const& Packet)
    {
        MBError Result = true;
        std::vector<std::string> MissingPackets;
        
        std::vector<PacketIdentifier> ReturnValue = p_GetPacketDependancies_DependancyOrder({Packet},Result,&MissingPackets);
        return(ReturnValue);
    }
    MBPM_PacketInfo PacketRetriever::GetPacketInfo(PacketIdentifier const& PacketToRetrieve)
    {
        MBError Result = true;
        return(p_GetPacketInfo(PacketToRetrieve,&Result));
    }

    //END PacketRetriever
};
