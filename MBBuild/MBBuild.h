#include "MBUtility/MBInterfaces.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <MBUtility/MBErrorHandling.h>

namespace MBBuild
{
    typedef uint32_t FileID;
    
    class DependancyInfo
    {
    private:
        struct DependancyStruct
        {
            bool External = false;
            std::vector<FileID> Dependancies; 
        };
        std::unordered_map<FileID,std::string> m_IDToStringMap;
        
        //A FileID as an Index to the depenancyies
        std::vector<DependancyInfo> m_Dependancies;


    public:
        //uint32_t 
        std::vector<FileID> GetUpdatedFiles();
        MBError ParseDependancyInfo(MBUtility::MBOctetInputStream& InputStream,DependancyInfo* OutInfo);
        MBError GetUpdatedFiles(std::filesystem::path const& SourceDirectory,std::vector<std::string>* OutFilesToMake);
    };
    
}
