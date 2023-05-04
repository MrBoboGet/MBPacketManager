#pragma once
#include "../MBPM_CLI.h"
namespace MBPM
{
    class MBPM_MBSlippi : public MBPM::CLI_Extension
    {
        private:
            MBError p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
            MBError p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        public:
            virtual const char* GetName() override;
            virtual MBPM::CustomCommandInfo GetCustomCommands() override;
            virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) override;
            virtual MBError HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
            virtual void HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) override;
    };
}
