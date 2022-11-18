#include "MBPM_MBDoc.h"
#include <filesystem>

namespace MBPM
{
    MBDoc_PacketInfo ParseMBDocPacketInfo(MBPM::MBPM_PacketInfo const& InfoToConvert)
    {
        MBDoc_PacketInfo ReturnValue;    
        if(!InfoToConvert.TypeInfo.HasAttribute("Name"))
        {
            throw std::runtime_error("MBDoc packet requires \"Name\" in the packet TypeInfo");   
        }
        if(InfoToConvert.TypeInfo["Name"].GetType() != MBParsing::JSONObjectType::String)
        {
            throw std::runtime_error("\"Name\" attribute has to be a string");
        }
        ReturnValue.Name = InfoToConvert.TypeInfo["Name"].GetStringData(); 
        return(ReturnValue);
    }
    const char* MBPM_MBDoc::GetName()
    {
        return("MBDoc");    
    }
    MBPM::CustomCommandInfo MBPM_MBDoc::GetCustomCommands()
    {
        MBPM::CustomCommandInfo ReturnValue;
        CustomCommand ExportCommand;
        ExportCommand.Name = "export";
        ExportCommand.SupportedTypes = {"MBDoc"};
        ExportCommand.Type = CommandType::TopCommand;
        CustomCommand RetractCommand;
        RetractCommand.Name = "retract";
        RetractCommand.SupportedTypes = {"MBDoc"};
        RetractCommand.Type = CommandType::TopCommand;
        CustomCommand DocPathCommand;
        DocPathCommand.Name = "docpath";
        DocPathCommand.Type = CommandType::TotalCommand;

        ReturnValue.Commands.push_back(std::move(ExportCommand));
        ReturnValue.Commands.push_back(std::move(RetractCommand));
        ReturnValue.Commands.push_back(std::move(DocPathCommand));
        return(ReturnValue);
    }
    std::string MBPM_MBDoc::p_GetEmptyBuildInfo()
    {
        std::string ReturnValue = "{\"TopDirectory\":{\"Files\": [\"index.mbd\"],\"SubDirectories\":{}}}";
        return(ReturnValue);
    }
    std::ofstream MBPM_MBDoc::p_GetTopBuild()
    {
        return(std::ofstream(m_ExtensionPath/"MBDocBuild.json"));    
    }
    void MBPM_MBDoc::SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError)
    {
        m_ExtensionPath = std::filesystem::canonical(ConfigurationDirectory);
        if(!std::filesystem::exists(m_ExtensionPath/"MBDocBuild.json"))
        {
            std::ofstream TopBuild = std::ofstream(m_ExtensionPath/"MBDocBuild.json");
            TopBuild<<p_GetEmptyBuildInfo();
        }
        if(!std::filesystem::exists(m_ExtensionPath/"index.mbd"))
        {
            std::ofstream TopIndex = std::ofstream(m_ExtensionPath/"index.mbd");
            TopIndex<<"";
        }
        MBError ParseResult = true;
        m_TopBuildObject = MBParsing::ParseJSONObject(m_ExtensionPath/"MBDocBuild.json",&ParseResult);
        if(!ParseResult)
        {
            throw std::runtime_error("Error reading MBDocBuild.json for MBDoc extension: "+ParseResult.ErrorMessage);
        }
    }
    MBError MBPM_MBDoc::p_HandleExport(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true; 
        try
        {
            std::filesystem::path SubBuildPath = std::filesystem::canonical(PacketToHandle.PacketURI+"/MBDocBuild.json");
            if(!std::filesystem::exists(SubBuildPath))
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "exporting MBDoc packet requires a MBDocBuild.json at the packet root";
                return(ReturnValue);
            }
            MBDoc_PacketInfo MBDocInfo = ParseMBDocPacketInfo(RetrieverToUse.GetPacketInfo(PacketToHandle));
            m_TopBuildObject["TopDirectory"]["SubDirectories"][MBDocInfo.Name] = MBUnicode::PathToUTF8(SubBuildPath);
            p_GetTopBuild()<<m_TopBuildObject.ToPrettyString();
        } 
        catch(std::exception const& e)
        {
            ReturnValue.ErrorMessage = "Error exporting packet: "+std::string(e.what());
            ReturnValue = false;
        }
        return(ReturnValue);
    }
    MBError MBPM_MBDoc::p_HandleRetract(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        MBError ReturnValue = true; 
        try
        {
            MBDoc_PacketInfo MBDocInfo = ParseMBDocPacketInfo(RetrieverToUse.GetPacketInfo(PacketToHandle));
            m_TopBuildObject["TopDirectory"]["SubDirectories"].GetMapData().erase(MBDocInfo.Name);
            p_GetTopBuild()<<m_TopBuildObject.ToPrettyString();
        } 
        catch(std::exception const& e)
        {
            ReturnValue.ErrorMessage = "Error retracting packet: "+std::string(e.what());
            ReturnValue = false;
        }
        return(ReturnValue);
    }

    MBError MBPM_MBDoc::HandleCommand(MBPM::CommandInfo const& CommandToHandle,MBPM::PacketIdentifier const& PacketToHandle,MBPM::PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
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
    int MBPM_MBDoc::HandleTotalCommand(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal)
    {
        int ReturnValue = 0; 
        if(CommandToHandle.CommandName == "docpath")
        {
            AssociatedTerminal.PrintLine(MBUnicode::PathToUTF8(std::filesystem::canonical(m_ExtensionPath/"MBDocBuild.json")));
        }
        return(ReturnValue);
    }
    void MBPM_MBDoc::HandleHelp(MBPM::CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal)
    {
        
    }
};
