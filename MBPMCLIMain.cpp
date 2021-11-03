#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
#include <MBPM_CLI.h>

int main(int argc,char** argv)
{
	//DEBUG GREJER
	//std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPacketManager");
	//return(MBPM::MBCLI_Main(argc, argv));

	//const int argc2 = 6;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "compile";
	//argv2[2] = "--allinstalled";
	//argv2[3] = "--cmake";
	//argv2[4] = "-sdata";
	//argv2[5] = "MBPM_CMakeFunctions.txt";
	//argv2[3] = "";
	//argc = argc2;
	//argv = argv2;

	
	return(MBPM::MBCLI_Main(argc, argv));
}