#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
#include <MBPM_CLI.h>

int main(int argc,char** argv)
{
	//DEBUG GREJER
	//std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MrBoboGraphicsEngine");
	////return(MBPM::MBCLI_Main(argc, argv));
	//
	//const int argc2 = 5;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "compile";
	//argv2[2] = "--create";
	//argv2[3] = "--cmake";
	//argv2[4] = "./";
	//argc = argc2;
	//argv = argv2;

	
	return(MBPM::MBCLI_Main(argc, argv));
}