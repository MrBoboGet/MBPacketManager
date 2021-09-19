#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
int main()
{
	//MBSockets::Init();
	//MBSockets::HTTPConnectSocket TestSocket("www.google.com","443");
	//TestSocket.Connect();
	//TestSocket.EstablishTLSConnection();
	//std::cout << TestSocket.GetDataFromRequest("GET","/")<<std::endl;
	
	
	std::filesystem::current_path("C:\\Users\\emanu\\Desktop\\Program\\C++\\MBPacketManager");
	//MBPM::MBPM_MakefileGenerationOptions Options;
	//MBPM::GenerateCmakeFile("./",Options,"TestCmake.txt");

	MBPM::CreatePacketFilesData("./TestDirectory/","ClientVersion");
	MBPM::MBPP_FileInfoReader ClientVersion("./TestDirectory/ClientVersion");
	MBPM::MBPP_FileInfoReader ServerVersion("./TestDirectory/ServerVersion");
	MBPM::MBPP_FileInfoDiff VersionDifferance = MBPM::GetFileInfoDifference(ClientVersion, ServerVersion);

	std::cout << "New Files:" << std::endl;
	for (auto const& NewFile : VersionDifferance.AddedFiles)
	{
		std::cout << NewFile << std::endl;
	}
	std::cout << "RemovedFiles:" << std::endl;
	for (auto const& RemovedFiles : VersionDifferance.RemovedFiles)
	{
		std::cout << RemovedFiles << std::endl;
	}
	std::cout << "UpdatedFiles:" << std::endl;
	for (auto const& UpdatedFiles : VersionDifferance.UpdatedFiles)
	{
		std::cout << UpdatedFiles << std::endl;
	}
	std::cout << "AddedDirectories:" << std::endl;
	for (auto const& AddedDirectories : VersionDifferance.AddedDirectories)
	{
		std::cout << AddedDirectories << std::endl;
	}
	std::cout << "RemovedDirectories:" << std::endl;
	for (auto const& RemovedDirectories : VersionDifferance.DeletedDirectories)
	{
		std::cout << RemovedDirectories << std::endl;
	}
	//MBPM::MBPP_FileInfoReader TestReader("./TestDirectory/ClientVersion");
	//std::cout << TestReader.ObjectExists("/TestCmake.txt")<<std::endl;
	//std::cout << TestReader.ObjectExists("/.git/description")<<std::endl;
	//std::cout << MBUtility::HexEncodeString(TestReader.GeFileInfo("/TestCmake.txt")->FileHash) << std::endl;
	
	
	//std::string TestData = std::string(std::filesystem::file_size("MBPM_PacketInfo"), 0);
	//std::ifstream PacketFile = std::ifstream("MBPM_PacketInfo", std::ios::in | std::ios::binary);
	//PacketFile.read(TestData.data(), TestData.size());
	//MBError OutError = true;
	//MBParsing::JSONObject TestObject = MBParsing::ParseJSONObject(TestData, 0, nullptr, &OutError);
	//std::cout << OutError.ErrorMessage << std::endl;
	//std::cout << TestObject.GetAttribute("PacketName").GetStringData() << std::endl;
	return(0);
}