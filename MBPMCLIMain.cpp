#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
int main()
{
	//MBSockets::Init();
	//MBSockets::HTTPConnectSocket TestSocket("www.google.com","443");
	//TestSocket.Connect();
	//TestSocket.EstablishTLSConnection();
	//std::cout << TestSocket.GetDataFromRequest("GET","/")<<std::endl;
	std::filesystem::current_path("C:\\Users\\emanu\\Desktop\\Program\\C++\\MBPacketManager");
	MBPM::MBPM_MakefileGenerationOptions Options;
	MBPM::GenerateCmakeFile("./",Options,"TestCmake.txt");


	//std::string TestData = std::string(std::filesystem::file_size("MBPM_PacketInfo"), 0);
	//std::ifstream PacketFile = std::ifstream("MBPM_PacketInfo", std::ios::in | std::ios::binary);
	//PacketFile.read(TestData.data(), TestData.size());
	//MBError OutError = true;
	//MBParsing::JSONObject TestObject = MBParsing::ParseJSONObject(TestData, 0, nullptr, &OutError);
	//std::cout << OutError.ErrorMessage << std::endl;
	//std::cout << TestObject.GetAttribute("PacketName").GetStringData() << std::endl;
	return(0);
}