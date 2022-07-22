#include <filesystem>
#include <time.h>
#include <iostream>
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
}
