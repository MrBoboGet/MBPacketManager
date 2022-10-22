#include "MBPM_Bash.h"
#include "MBParsing/MBParsing.h"
#include "MBUnicode/MBUnicode.h"
#include <MBSystem/MBSystem.h>
#include <filesystem>
namespace MBPM
{
    std::filesystem::path i_GetBashHome()
    {
        return(MBSystem::GetUserHomeDirectory()/".mbbash");
    }
    MBError ParseBashPacketInfo(std::filesystem::path const& FileToRead,BashPacketInfo& OutInfo)
    {
        MBError ReturnValue = true;
        BashPacketInfo Result;
        MBParsing::JSONObject JsonToParse = MBParsing::ParseJSONObject(FileToRead,&ReturnValue);
        if(!ReturnValue) return(ReturnValue);
        try
        {
            for(auto const& Source : JsonToParse["Sources"].GetArrayData())
            {
                Result.Sources.push_back(Source.GetStringData());
            }
        }
        catch(std::exception const& e)
        {
            ReturnValue = false; 
            ReturnValue.ErrorMessage = e.what(); 
        }
        if(ReturnValue)
        {
            OutInfo = std::move(Result);
        }
        return(ReturnValue);
    }
    MBError MBPM_Bash::p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        BashPacketInfo BashInfo;
        //ReturnValue = ParseBashPacketInfo(PacketToHandle.PacketURI+"/MBPMBashInfo.json",BashInfo);
        //if(!ReturnValue) return(ReturnValue);

        std::filesystem::path MBBashHome = i_GetBashHome();
        std::filesystem::path PacketMountDir = MBBashHome/PacketToHandle.PacketName;
        if(!std::filesystem::exists(MBBashHome))
        {
            std::filesystem::create_directories(MBBashHome);
        }
        if(!std::filesystem::exists(MBBashHome/"mbbash.sh"))
        {
            std::ofstream SetupScript = std::ofstream(MBBashHome/"mbbash.sh");
            SetupScript<<
#include "mbbash.inc"
                ;
        }
        if(std::filesystem::exists(PacketMountDir))
        {
            if(!std::filesystem::remove(PacketMountDir))
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Failed removing directory: "+MBUnicode::PathToUTF8(PacketMountDir);
                return(ReturnValue);
            }
        }
        std::filesystem::create_directory_symlink(std::filesystem::canonical(PacketToHandle.PacketURI),PacketMountDir);
        return(ReturnValue);
    }
    MBError MBPM_Bash::p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        BashPacketInfo BashInfo;
        std::filesystem::path MBBashHome = i_GetBashHome();
        std::filesystem::path PacketMountDir = MBBashHome/PacketToHandle.PacketName;
        if(std::filesystem::exists(PacketMountDir))
        {
            std::filesystem::remove(PacketMountDir);
        }
        return(ReturnValue);
    }
    const char* MBPM_Bash::GetName() 
    {
        return("MBBash");       
    }
    MBPM::CustomCommandInfo MBPM_Bash::GetCustomCommands() 
    {
        MBPM::CustomCommandInfo ReturnValue;
        CustomCommand ExportCommand;
        ExportCommand.Name = "export";
        ExportCommand.Type = CommandType::TopCommand;
        ExportCommand.SupportedTypes = {"Bash"};
        CustomCommand RetractCommand;
        RetractCommand.Name = "retract";
        RetractCommand.Type = CommandType::TopCommand;
        RetractCommand.SupportedTypes = {"Bash"};
        
        ReturnValue.Commands.push_back(std::move(ExportCommand));
        ReturnValue.Commands.push_back(std::move(RetractCommand));
        return(ReturnValue);
    }
    void MBPM_Bash::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) 
    {
        //nothing   
    }
    MBError MBPM_Bash::HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) 
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
    void MBPM_Bash::HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) 
    {
            
    }
}
