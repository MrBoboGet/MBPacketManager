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
	const int argr = 4;
	char* TestData[argr];
	std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPacketManager");
	TestData[0] = "MBPM";
	TestData[1] = "upload";
	TestData[2] = "TestPacket";
	TestData[3] = "./TestPacket";
	argc = argr;
	argv = TestData;
	return(MBPM::MBCLI_Main(argc, argv));
}