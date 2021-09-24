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
	argc = 4;
	char* TestData[4];
	TestData[0] = "MBPM";
	TestData[1] = "get";
	TestData[2] = "TestPacket";
	TestData[3] = "skruvhornsget-93x_1646.jpg";
	argv = TestData;
	return(MBPM::MBCLI_Main(argc, argv));
}