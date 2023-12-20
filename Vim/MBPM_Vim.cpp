#include "MBPM_Vim.h"
#include <MBUtility/MBCompileDefinitions.h>
#include <MBUtility/MBFiles.h>
#include <MBSystem/MBSystem.h>
#include <filesystem>
namespace MBPM
{
    std::string i_GetVimUserDir()
    {
        std::string ReturnValue;
        if constexpr(MBUtility::IsWindows())
        {
            return("vimfiles");        
        }
        else
        {
            return(".vim"); 
        }
        return(ReturnValue);
    }
    const char* MBPM_Vim::GetName()
    {
        return("MBPM_Vim");    
    }
    MBPM::CustomCommandInfo MBPM_Vim::GetCustomCommands() 
    {
        MBPM::CustomCommandInfo ReturnValue; 
        CustomCommand ExportCommand;
        ExportCommand.Name = "export";
        ExportCommand.SupportedTypes = {"Vim"};
        ExportCommand.Type = CommandType::TopCommand;
        CustomCommand RetractCommand;
        RetractCommand.Name = "retract";
        RetractCommand.SupportedTypes = {"Vim"};
        RetractCommand.Type = CommandType::TopCommand;
        ReturnValue.Commands = {std::move(RetractCommand),std::move(ExportCommand)};
        return(ReturnValue);
    }
    void MBPM_Vim::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) 
    {
        //no configs needed 
    }
    MBError MBPM_Vim::HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) 
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
    MBError ParseVimPacketInfo(std::filesystem::path const& FilePath,VimPacketInfo& OutInfo)
    {
        MBError ReturnValue = true;
        //VimPacketInfo Result;
        //std::string TotalFileData = MBUtility::ReadWholeFile(MBUnicode::PathToUTF8(FilePath));
        //MBParsing::JSONObject JsonData = MBParsing::ParseJSONObject(TotalFileData,0,nullptr,&ReturnValue);
        //if(!ReturnValue) return(ReturnValue);
        //try
        //{
        //    if(JsonData.HasAttribute("PluginFiles"))
        //    {
        //        for(auto const& PluginFile : JsonData["PluginFiles"].GetArrayData())
        //        {
        //            Result.PluginFiles.push_back(PluginFile.GetStringData());
        //        }
        //    }
        //    if(JsonData.HasAttribute("FiletypeFiles"))
        //    {
        //        for(auto const& FiletypeFile : JsonData["FiletypeFiles"].GetArrayData())
        //        {
        //            Result.FiletypeFiles.push_back(FiletypeFile.GetStringData());
        //        }
        //    }
        //}
        //catch(std::exception const& e)
        //{
        //    ReturnValue = false;
        //    ReturnValue.ErrorMessage = e.what();
        //}
        //OutInfo = std::move(Result);
        return(ReturnValue);    
    }
    MBError MBPM_Vim::p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        //Creates a symlink to the specified directory, doesn't copy any files
        std::filesystem::path UserVimDirectory =  MBSystem::GetUserHomeDirectory()/i_GetVimUserDir();
        std::filesystem::path VimPluginDirectory = UserVimDirectory/"mbpm";
        if(!std::filesystem::exists(VimPluginDirectory))
        {
            std::filesystem::create_directories(VimPluginDirectory);       
        }
        std::filesystem::path MBPMScriptPath = UserVimDirectory/"autoload"/"mbpm.vim";
        if(!std::filesystem::exists(MBPMScriptPath))
        {
            std::filesystem::create_directories(MBPMScriptPath.parent_path());
            std::ofstream OutFile = std::ofstream(MBPMScriptPath);
            OutFile << 
#include "mbpmscript.inc"
                ;
        }
        if(std::filesystem::exists(VimPluginDirectory/PacketToHandle.PacketName))
        {
            std::filesystem::remove(VimPluginDirectory/PacketToHandle.PacketName);
        }
        std::filesystem::create_directory_symlink(std::filesystem::absolute(PacketToHandle.PacketURI),VimPluginDirectory/PacketToHandle.PacketName);
        return(ReturnValue);
    }
    MBError MBPM_Vim::p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true;
        std::filesystem::path UserVimDirectory =  MBSystem::GetUserHomeDirectory()/i_GetVimUserDir();
        std::filesystem::path DirectoryToRemove = UserVimDirectory/"mbpm"/PacketToHandle.PacketName;
        if(std::filesystem::exists(DirectoryToRemove))
        {
            std::filesystem::remove(DirectoryToRemove);
        }
        return(ReturnValue);
    }
    void MBPM_Vim::HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal)
    {
         
    }
};
