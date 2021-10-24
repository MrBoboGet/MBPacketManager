#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
#include <MBPM_CLI.h>

int main(int argc,char** argv)
{
	//DEBUG GREJER
	std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPacketManager");
	//return(MBPM::MBCLI_Main(argc, argv));

	//const int argc2 = 7;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "upload";
	//argv2[2] = "MBPacketManager";
	//argv2[3] = "./";
	//argv2[4] = "-d";
	//argv2[5] = "/MBPM_Builds/";
	//argv2[6] = "--computerdiff";
	//argc = argc2;
	//argv = argv2;

	const int argc2 = 6;
	char* argv2[argc2];
	argv2[0] = "mbpm";
	argv2[1] = "index";
	argv2[2] = "list";
	argv2[3] = "--remote";
	argv2[4] = "MBPacketManager";
	argv2[5] = "--computerdiff";
	argc = argc2;
	argv = argv2;

	return(MBPM::MBCLI_Main(argc, argv));
}