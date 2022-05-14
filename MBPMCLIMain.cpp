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
	
	std::filesystem::current_path(std::filesystem::current_path().parent_path());
	MBPM::MBPP_FileInfoReader RawDirectoryInfo;
	MBPM::MBPP_FileInfoReader VirtualDirectoryInfo;
	MBPM::MBPP_FileInfoReader::CreateFileInfo("./", &RawDirectoryInfo);
	MBUtility::OS_Filesystem FSToUse;
	MBPM::MBPP_FileInfoReader::CreateFileInfo(FSToUse, &VirtualDirectoryInfo);
	std::cout << bool(RawDirectoryInfo == VirtualDirectoryInfo) << std::endl;
	std::string Buffer1;
	std::string Buffer2;
	MBUtility::MBStringOutputStream OutStream1(Buffer1);
	MBUtility::MBStringOutputStream OutStream2(Buffer2);
	RawDirectoryInfo.WriteData(&OutStream1);
	VirtualDirectoryInfo.WriteData(&OutStream2);
	std::cout << bool(Buffer1 == Buffer2) << std::endl;
	std::exit(0);
	//std::exit(0);

	return(MBPM::MBCLI_Main(argc, argv));
}