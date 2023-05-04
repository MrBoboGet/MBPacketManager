#include "GenericExtension.h"
#include <MBSystem/MBSystem.h>

namespace MBPM
{
    std::filesystem::path GetRootPath(ExportConfig const& Config)
    {
        std::filesystem::path ReturnValue;
        if(Config.Root == RootType::Home)
        {
            return(MBSystem::GetUserHomeDirectory());
        }
        else if(Config.Root == RootType::Roaming)
        {
            return(MBSystem::GetUserHomeDirectory()/"AppData/Roaming"/Config.ApplicationName);
        }
        else
        {
            assert(false && "Invalid os path in export config");   
        }
        return(ReturnValue);
    }
    GenericExtension::GenericExtension(std::string const& Name,std::vector<std::string> PacketTypes,OSConfigurations Configs)
    {
        m_Name = Name;
        m_PacketTypes = PacketTypes;

        if constexpr(MBUtility::IsWindows())
        {
            if(auto const& WindowsIt = Configs.ExportConfigs.find(OSType::Windows); WindowsIt != Configs.ExportConfigs.end())
            {
                m_ExportConfig = std::move(WindowsIt->second);
            }
        }
        else if constexpr(MBUtility::IsPOSIX())
        {
            if(auto const& PosixIt = Configs.ExportConfigs.find(OSType::POSIX); PosixIt != Configs.ExportConfigs.end())
            {
                m_ExportConfig = std::move(PosixIt->second);
            }
        }
        //no operative system specific config matched
        if(m_ExportConfig.Root == RootType::Null)
        {
            if(auto const& GenericIt = Configs.ExportConfigs.find(OSType::Default); GenericIt != Configs.ExportConfigs.end())
            {
                m_ExportConfig = std::move(GenericIt->second);
            }
            else
            {
                throw std::runtime_error("Error initializing generic extension: no export config matched. Missing default?");   
            }
        }
    }
    const char* GenericExtension::GetName()
    {
        return(m_Name.c_str());
    }
    MBPM::CustomCommandInfo GenericExtension::GetCustomCommands()
    {
        MBPM::CustomCommandInfo ReturnValue; 
        CustomCommand ExportCommand;
        ExportCommand.Name = "export";
        ExportCommand.SupportedTypes = m_PacketTypes;
        ExportCommand.Type = CommandType::TopCommand;
        CustomCommand RetractCommand;
        RetractCommand.Name = "retract";
        RetractCommand.SupportedTypes = m_PacketTypes;
        RetractCommand.Type = CommandType::TopCommand;
        ReturnValue.Commands = {std::move(RetractCommand),std::move(ExportCommand)};
        return(ReturnValue);
    }
    void GenericExtension::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError)
    {
        //nothing to config, lmao
    }
    MBError GenericExtension::p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError Result = true;
        std::filesystem::path Root = GetRootPath(m_ExportConfig)/m_ExportConfig.RootSuffix;
        if(m_ExportConfig.ExportedFiles.size() == 0)
        {
            if(std::filesystem::exists(Root/PacketToHandle.PacketName))
            {
                std::filesystem::remove(Root/PacketToHandle.PacketName);
            }
        }
        //TODO implement xd
        else
        {

        }
        std::filesystem::create_directory_symlink(PacketToHandle.PacketURI,Root/PacketToHandle.PacketName);
        return(Result);
    }
    MBError GenericExtension::p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError Result = true;
        std::filesystem::path Root = GetRootPath(m_ExportConfig)/m_ExportConfig.RootSuffix;
        if(m_ExportConfig.ExportedFiles.size() == 0)
        {
            if(std::filesystem::exists(Root/PacketToHandle.PacketName))
            {
                std::filesystem::remove(Root/PacketToHandle.PacketName);
            }
        }
        //TODO implement xd
        else
        {

        }
        return(Result);
    }
    MBError GenericExtension::HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        if(CommandToHandle.CommandName == "export")
        {
            ReturnValue = p_HandleExport(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal);
        }
        else if(CommandToHandle.CommandName == "retract")
        {
            ReturnValue = p_HandleRetract(CommandToHandle,PacketToHandle,RetrieverToUse,AssociatedTerminal);
        }
        return(ReturnValue);
    }
    void GenericExtension::HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal)
    {
           
    }
}
