#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
#include <MBPM_CLI.h>

int main(int argc,char** argv)
{
	//DEBUG GREJER
	//mbpm upload --allinstalled -d /MBPM_Builds/ --computerdiff
	//std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPM_ExternalPackets/cryptopp/");
	////return(MBPM::MBCLI_Main(argc, argv));
	//
	//const int argc2 = 4;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "upload";
	//argv2[2] = "MBPacketManager";
	//argv2[3] = "--user";
	//argc = argc2;
	//argv = argv2;
	
	return(MBPM::MBCLI_Main(argc, argv));
}