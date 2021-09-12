#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
int main()
{
	MBSockets::Init();
	MBSockets::HTTPConnectSocket TestSocket("www.google.com","443");
	TestSocket.Connect();
	TestSocket.EstablishTLSConnection();
	std::cout << TestSocket.GetDataFromRequest("GET","/")<<std::endl;
	return(0);
}