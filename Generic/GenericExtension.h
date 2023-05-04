#include "../MBPM_CLI.h"

namespace MBPM
{
    enum class RootType
    {
        Null,
        Home,
        Roaming,//only has meaningfull semantics on windows
    };
    enum class OSType
    {
        Default,
        Windows,
        POSIX
    };
    struct ExportConfig
    {
        RootType Root = RootType::Null;
        //for example .mbslippi
        std::string RootSuffix;
        std::string ApplicationName;
        //empty means that the whole packet is symlinked
        std::vector<std::string> ExportedFiles;
    };
    struct OSConfigurations
    {
        std::unordered_map<OSType,ExportConfig> ExportConfigs;
    };
    std::filesystem::path GetRootPath(ExportConfig const& Config);
    class GenericExtension : public CLI_Extension
    {
    private:
        ExportConfig m_ExportConfig;
        std::string m_Name;
        std::vector<std::string> m_PacketTypes;
        MBError p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
        MBError p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal);
    public:
        GenericExtension(std::string const& Name,std::vector<std::string> PacketTypes,OSConfigurations Configs);
        virtual const char* GetName() override;
        virtual MBPM::CustomCommandInfo GetCustomCommands() override;
        virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) override;
        virtual MBError HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) override;
        virtual void HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) override;
    };
}
