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


	return(MBPM::MBCLI_Main(argc, argv));
}