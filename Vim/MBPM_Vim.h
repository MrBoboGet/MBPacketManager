#include "../MBPM_CLI.h"
#include "MBUtility/MBErrorHandling.h"

namespace MBPM
{
       
    struct VimPacketInfo
    {
        std::vector<std::string> PluginFiles;
        std::vector<std::string> FiletypeFiles;
    };
    MBError ParseVimPacketInfo(std::filesystem::path const& FilePath,VimPacketInfo& OutInfo);
    class MBPM_Vim : public MBPM::CLI_Extension
    {
        private:
            MBError p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        public:
            virtual const char* GetName() override;
            virtual MBPM::CustomCommandInfo GetCustomCommands() override;
            virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) override;
            virtual MBError HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
            virtual void HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) override;
    };
};
