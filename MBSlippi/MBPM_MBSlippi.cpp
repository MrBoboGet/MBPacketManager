#include "MBPM_MBSlippi.h"
#include <MBSystem/MBSystem.h>
namespace MBPM
{
    MBError MBPM_MBSlippi::p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        std::filesystem::path UserReplaysDirectory = MBSystem::GetUserHomeDirectory()/".mbslippi/Replays";
        if(!std::filesystem::exists(UserReplaysDirectory))
        {
            std::filesystem::create_directories(UserReplaysDirectory);
        }
        if(std::filesystem::exists(UserReplaysDirectory/PacketToHandle.PacketName))
        {
            std::filesystem::remove(UserReplaysDirectory/PacketToHandle.PacketName);
        }
        std::filesystem::create_directory_symlink(PacketToHandle.PacketURI,UserReplaysDirectory/PacketToHandle.PacketName);
        return(ReturnValue);
    }
    MBError MBPM_MBSlippi::p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        std::filesystem::path DirectoryToRemove = MBSystem::GetUserHomeDirectory()/".mbslippi/Replays"/PacketToHandle.PacketName;
        if(std::filesystem::exists(DirectoryToRemove))
        {
            std::filesystem::remove(DirectoryToRemove);
        }
        return(ReturnValue);
    }
    const char* MBPM_MBSlippi::GetName() 
    {
        return("MBSlippi");
    }
    MBPM::CustomCommandInfo MBPM_MBSlippi::GetCustomCommands() 
    {
        MBPM::CustomCommandInfo ReturnValue; 
        CustomCommand ExportCommand;
        ExportCommand.Name = "export";
        ExportCommand.SupportedTypes = {"SLPReplays"};
        ExportCommand.Type = CommandType::TopCommand;
        CustomCommand RetractCommand;
        RetractCommand.Name = "retract";
        RetractCommand.SupportedTypes = {"SLPReplays"};
        RetractCommand.Type = CommandType::TopCommand;
        ReturnValue.Commands = {std::move(RetractCommand),std::move(ExportCommand)};
        return(ReturnValue);
    }
    void MBPM_MBSlippi::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) 
    {
           
    }
    MBError MBPM_MBSlippi::HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) 
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
        else
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Invalid command for MBSlippi extension: \""+CommandToHandle.CommandName+"\"";
        }
        return(ReturnValue);
    }
    void MBPM_MBSlippi::HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) 
    {
           
    }


}
