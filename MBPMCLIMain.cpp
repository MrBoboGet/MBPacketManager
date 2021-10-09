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
	//MBPM::CreatePacketFilesData("./");
	//MBPM::MBPP_FileInfoReader InfoToIterate("./MBPM_FileInfo");
	//const MBPM::MBPP_DirectoryInfoNode* TopNode = InfoToIterate.GetDirectoryInfo("/");
	//MBPM::MBPP_DirectoryInfoNode_ConstIterator TestIterator = TopNode->begin();
	//MBPM::MBPP_DirectoryInfoNode_ConstIterator EndIterator = TopNode->end();
	//while(!(TestIterator == EndIterator))
	//{
	//	std::cout << TestIterator.GetCurrentDirectory() + (*TestIterator).FileName << std::endl;
	//	TestIterator++;
	//}

	//return(MBPM::MBCLI_Main(argc, argv));

	//const int argc2 = 3;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "compile";
	//argv2[2] = "MBUtility";
	//argc = argc2;
	//argv = argv2;

	return(MBPM::MBCLI_Main(argc, argv));
}