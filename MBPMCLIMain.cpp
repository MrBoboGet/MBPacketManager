#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
#include <MBPM_CLI.h>

void TestServerFunc()
{
	MBPM::MBPP_Server TestServer("./TestPacket/");
	MBSockets::ServerSocket ServerSocket("42069");
	ServerSocket.Bind();
	ServerSocket.Listen();
	ServerSocket.Accept();
	while (true)
	{
		while (!TestServer.ClientRequestFinished())
		{
			TestServer.InsertClientData(ServerSocket.RecieveData());
		}
		MBPM::MBPP_ServerResponseIterator* ResponseIterator = TestServer.GetResponseIterator();
		while (ResponseIterator->IsFinished() == false)
		{
			ServerSocket.SendData(*(*ResponseIterator));
			ResponseIterator->Increment();
		}
		TestServer.FreeResponseIterator(ResponseIterator);
	}
	std::cout << "Server done" << std::endl;
}
int main(int argc,char** argv)
{
	//DEBUG GREJER
	std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPacketManager");
	//MBPM::CreatePacketFilesData("./TestUploadPacket/");
	//MBPM::MBPP_FileInfoReader InfoToIterate("./MBPM_FileInfo");
	//const MBPM::MBPP_DirectoryInfoNode* TopNode = InfoToIterate.GetDirectoryInfo("/");
	//MBPM::MBPP_DirectoryInfoNode_ConstIterator TestIterator = TopNode->begin();
	//MBPM::MBPP_DirectoryInfoNode_ConstIterator EndIterator = TopNode->end();
	//while(!(TestIterator == EndIterator))
	//{
	//	std::cout << TestIterator.GetCurrentDirectory() + (*TestIterator).FileName << std::endl;
	//	TestIterator++;
	//}

	//return(MBPM::MBCLI_Main(argc, argv));

	//const int argr = 4;
	//char* TestData[argr];
	//TestData[0] = "MBPM";
	//TestData[1] = "upload";
	//TestData[2] = "TestPacket";
	//TestData[3] = "./TestUploadPacket/";
	//argc = argr;
	//argv = TestData;

	//const int argr = 5;
	//char* TestData[argr];
	//TestData[0] = "MBPM";
	//TestData[1] = "compile";
	//TestData[2] = "--allinstalled";
	//TestData[3] = "--cmake";
	//TestData[4] = "--create";
	//argc = argr;
	//argv = TestData;

	const int argr = 4;
	char* TestData[argr];
	TestData[0] = "MBPM";
	TestData[1] = "compile";
	TestData[2] = "MrBoboSockets";
	TestData[3] = "MBPacketManager";
	argc = argr;
	argv = TestData;

	//const int argr = 5;
	//char* TestData[argr];
	//TestData[0] = "MBPM";
	//TestData[1] = "compile";
	//TestData[2] = "MrBoboSockets";
	//TestData[3] = "--cmake";
	//TestData[4] = "--create";
	//argc = argr;
	//argv = TestData;

	//MBPM::MBPM_MakefileGenerationOptions TestConfig;
	//TestConfig.SupportedLibraryConfigurations = {MBPM::MBPM_CompileOutputConfiguration::StaticDebug,MBPM::MBPM_CompileOutputConfiguration::StaticRelease};
	//MBPM::GenerateCmakeFile("./", TestConfig, "TestCmake2.txt");

	return(MBPM::MBCLI_Main(argc, argv));
}