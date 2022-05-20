
#include "MBPacketManager.h"

#include <iostream>
#include <filesystem>
int main(int argc,const char** argv)
{
	//debug
	std::filesystem::current_path(std::filesystem::current_path().parent_path());

	std::cout << "Update index test" << std::endl;
	MBPM::MBPP_FileInfoReader OSUpdateReader("MBPM_Tests/UpdateTests/MBPM_FileInfo");
	MBPM::MBPP_FileInfoReader::UpdateFileInfo("MBPM_Tests/UpdateTests",OSUpdateReader);
	MBPM::MBPP_FileInfoReader VirtualUpdateReader("MBPM_Tests/UpdateTests/MBPM_FileInfo");
	MBUtility::OS_Filesystem Filesystem;
	MBPM::MBPP_FileInfoReader::UpdateFileInfo(Filesystem,"MBPM_Tests/UpdateTests", VirtualUpdateReader);
	if (!(OSUpdateReader == VirtualUpdateReader))
	{
		std::cout << "Update index test failed: Virtual reader and os reader produces different results" << std::endl;
		std::exit(1);
	}
	std::cout << "Update index passed" << std::endl;
}