#include <filesystem>
#include <time.h>
#include <iostream>

#include "MBBuild.h"
int main(int argc,const char** argv)
{
    std::filesystem::path PathToCheck = std::filesystem::current_path();
    if(argc > 1)
    {
        PathToCheck /= argv[1]; 
    } 
    std::filesystem::recursive_directory_iterator DirectoryIterator = std::filesystem::recursive_directory_iterator(PathToCheck);
    clock_t Time = clock(); 
    int TotalFiles = 0;
    for(std::filesystem::directory_entry const& Entry : DirectoryIterator)
    {
        if(Entry.is_regular_file())
        {
            TotalFiles++;  
        } 
    }
    std::cout<<"Files processed: "<<TotalFiles<<std::endl;
    std::cout<<"Total time: "<<double(Time)/CLOCKS_PER_SEC<<std::endl;
    //
    MBPM::MBBuild::SourceInfo Source;
    MBError Result =  MBPM::MBBuild::ParseSourceInfo("../../MBSourceInfo.json", Source);
    if (!Result)
    {
        std::cout << "Error reading source info: " << Result << "\n";
        std::exit(1);
    }
    MBPM::MBBuild::DependancyInfo Dependancies;
    MBError CreateResult = MBPM::MBBuild::DependancyInfo::CreateDependancyInfo(Source, "../../", &Dependancies);
    if (!Result)
    {
        std::cout << "Error creating dependancies: " << Result << "\n";
        std::exit(1);
    }
    std::string Buffer;
    MBUtility::MBStringOutputStream OutStream(Buffer);
    Dependancies.WriteDependancyInfo(OutStream);
    MBPM::MBBuild::DependancyInfo ParsedDependancies;
    Result =  MBPM::MBBuild::DependancyInfo::ParseDependancyInfo(MBUtility::MBBufferInputStream(Buffer.data(),Buffer.size()), &ParsedDependancies);
    if (!Result)
    {
        std::cout << "Error parsing serialized dependancies: " << Result << "\n";
        std::exit(1);
    }
    assert(ParsedDependancies == Dependancies);
}
