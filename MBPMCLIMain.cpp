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
	//MBParsing::ParseJSONObject(;
	
	MBPM::CreatePacketFilesData("./","MBPM_FileInfo2");
	MBPM::MBPP_FileInfoReader InfoToIterate("./MBPM_FileInfo2");
	std::string TestOutputString = "";
	MBUtility::MBStringOutputStream TestOutputStream(TestOutputString);
	InfoToIterate.WriteData(&TestOutputStream);
	MBPM::MBPP_FileInfoReader StringReader(TestOutputString.data(), TestOutputString.size());

	std::cout << bool(StringReader == InfoToIterate) << std::endl;

	const MBPM::MBPP_DirectoryInfoNode* TopNode = InfoToIterate.GetDirectoryInfo("/");
	MBPM::MBPP_DirectoryInfoNode_ConstIterator TestIterator = TopNode->begin();
	MBPM::MBPP_DirectoryInfoNode_ConstIterator EndIterator = TopNode->end();
	while(!(TestIterator == EndIterator))
	{
		std::cout << TestIterator.GetCurrentDirectory() + (*TestIterator).FileName << std::endl;
		TestIterator++;
	}
	std::cout << "Nu med mergad" << std::endl;
	//MBPM::CreatePacketFilesData("./.mbpm/Windows_x86-64/Data/", "MBPM_UpdatedFileInfo");
	MBPM::MBPP_FileInfoReader InfoToMerge = MBPM::MBPP_FileInfoReader("./.mbpm/Windows_x86-64/MBPM_UpdatedFileInfo");
	StringReader.UpdateInfo(InfoToMerge);
	const MBPM::MBPP_DirectoryInfoNode* MergedTopNode = StringReader.GetDirectoryInfo("/");
	MBPM::MBPP_DirectoryInfoNode_ConstIterator MergedIterator = MergedTopNode->begin();
	MBPM::MBPP_DirectoryInfoNode_ConstIterator MergedEnd = MergedTopNode->end();
	while (!(MergedIterator == MergedEnd))
	{
		std::cout << MergedIterator.GetCurrentDirectory() + (*MergedIterator).FileName << std::endl;
		MergedIterator++;
	}
	//return(MBPM::MBCLI_Main(argc, argv));

	//const int argc2 = 3;
	//char* argv2[argc2];
	//argv2[0] = "mbpm";
	//argv2[1] = "compile";
	//argv2[2] = "MrBoboDatabase";
	//argc = argc2;
	//argv = argv2;

	return(MBPM::MBCLI_Main(argc, argv));
}