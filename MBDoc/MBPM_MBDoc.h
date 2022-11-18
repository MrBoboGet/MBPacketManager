#pragma once
#include "../MBPM_CLI.h"

namespace MBPM
{
    struct MBDoc_PacketInfo
    {
        std::string Name;
    };
    MBDoc_PacketInfo ParseMBDocPacketInfo(MBPM::MBPM_PacketInfo const& InfoToConvert);
    class MBPM_MBDoc : public MBPM::CLI_Extension
    {
    private:
        MBParsing::JSONObject m_TopBuildObject;
        
        std::string p_GetEmptyBuildInfo();
        std::ofstream p_GetTopBuild(); 
        std::filesystem::path m_ExtensionPath;
        MBError p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
    public:   
        virtual const char* GetName() override;
        virtual MBPM::CustomCommandInfo GetCustomCommands() override;
        virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) override;
        virtual MBError HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
        virtual void HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) override;
        virtual int HandleTotalCommand(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
    };
};
