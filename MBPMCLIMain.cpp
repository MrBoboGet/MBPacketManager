#include "MBPacketManager.h"
#include <MrBoboSockets/MrBoboSockets.h>
#include <iostream>
#include <MBParsing/MBParsing.h>
#include <MB_PacketProtocol.h>
#include <MBPM_CLI.h>

int main(int argc,const char** argv)
{
	//DEBUG GREJER
	//mbpm upload --allinstalled -d /MBPM_Builds/ --computerdiff
	//std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPM_ExternalPackets/cryptopp/");
	////return(MBPM::MBCLI_Main(argc, argv));
	//
	//mbpm upload MBSearchEngine --installed -d /MBPM_Builds/ --computerdiff
	//const int argc2 = 3;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "compile";
	//argv2[2] = "MBRadio";
	//argc = argc2;
	//argv = argv2;
	
	//Test grejer
	//std::filesystem::current_path("C:/Users/emanu/Desktop/Program/C++/MBPM_ExternalPackets/cryptopp/");
	//MBPM::MBPP_FileInfoReader OldInfo = MBPM::CreateFileInfo("./");
	//MBPM::MBPP_FileInfoReader NewInfo; 
	//MBPM::MBPP_FileInfoReader::CreateFileInfo("./", &NewInfo);
	//std::cout << bool(OldInfo == NewInfo) << std::endl;
	//std::filesystem::current_path("C:\\Users\\emanu\\Desktop\\BackupReplayDolphin\\Slippi");

	//const char* NewArgv[] = { "mbpm","index","list","./" };
	//argc = sizeof(NewArgv) / sizeof(const char*);
	//argv = NewArgv;
	//MBPM::MBPP_FileInfoReader::CreateFileInfo("./", "./TestIndexNew");
	
	
	//MBPM::MBPP_FileInfoReader OldInfo = MBPM::MBPP_FileInfoReader("./TestIndexNew");
	//MBPM::MBPP_FileInfoReader UpdatedInfo = OldInfo;
	//MBPM::MBPP_FileInfoReader::UpdateFileInfo("./",UpdatedInfo);
	//MBPM::MBPP_FileInfoReader NewIndex;
	//MBPM::MBPP_FileInfoReader::CreateFileInfo("./", &NewIndex);
	//std::cout << bool(UpdatedInfo == NewIndex) << std::endl;
	

	//std::exit(0);

	return(MBPM::MBCLI_Main(argc, argv));
}