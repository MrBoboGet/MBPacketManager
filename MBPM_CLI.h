#pragma once
#include <MBUtility/MBErrorHandling.h>
#include <unordered_set>
//#pragma p_HandleRetract
#include "MBPacketManager.h"
#include <MBCLI/MBCLI.h>
#include <MBUnicode/MBUnicode.h>
#include <unordered_map>
namespace MBPM
{
    class DownloadPrinter : public MBPP_FileListDownloadHandler
    {
    private:
        MBCLI::MBTerminal* m_AssociatedTerminal = nullptr;
    public:
        DownloadPrinter(MBCLI::MBTerminal* TerminalToUse) { m_AssociatedTerminal = TerminalToUse; }
        virtual MBError NotifyFiles(std::vector<std::string> const& FileToNotify) override;
        virtual MBError Open(std::string const& FileToDownloadName) override;
        virtual MBError InsertData(const void* Data, size_t DataSize) override;
        virtual MBError Close() override;
    };
    MBError DownloadDirectory(std::string const& PacketDirectory, std::vector<std::string> const& FilesystemObjectsToDownload, MBPP_FileListDownloadHandler* DownloadHandler);
    
    void PrintFileInfo(MBPM::MBPP_FileInfoReader const& InfoToPrint, std::vector<std::string> const& FilesystemObjectsToPrint, MBCLI::MBTerminal* AssociatedTerminal);
    void PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint, MBCLI::MBTerminal* AssociatedTerminal);


    struct CommandInfo
    {
        std::string CommandName = "";
        std::vector<std::string> Arguments;
        std::unordered_set<std::string> Flags;
        std::unordered_map<std::string, std::vector<std::string>> SingleValueOptions;

        CommandInfo GetSubCommand() const
        {
            CommandInfo ReturnValue;
            if(Arguments.size() == 0)
            {
                throw std::runtime_error("Error creating subcommand: requires atleast 1 argument");   
            }
            ReturnValue.CommandName = Arguments[0];
            ReturnValue.Arguments.insert(ReturnValue.Arguments.end(),Arguments.begin()+1,Arguments.end());
            ReturnValue.Flags = Flags;
            ReturnValue.SingleValueOptions = SingleValueOptions;
            return(ReturnValue);
        }
    };
    enum class CommandType
    {
        TopCommand,
        SubCommand,
        TotalCommand,
        Null
    };
    struct CustomCommand
    {
        CommandType Type = CommandType::Null;
        std::string Name;
        std::vector<std::string> SupportedTypes;   
    };
    struct CustomCommandInfo
    {
        std::vector<CustomCommand> Commands;
    };
    class CLI_Extension
    {
    private:
    public:      
        virtual const char* GetName() = 0; 
        virtual CustomCommandInfo GetCustomCommands() = 0;
        virtual void SetConfigurationDirectory(const char* ConfigurationDirectory,const char** OutError) = 0;
        virtual MBError HandleCommand(CommandInfo const& CommandToHandle,PacketIdentifier const& PacketToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) = 0;
        virtual int HandleTotalCommand(CommandInfo const& CommandToHandle,PacketRetriever& RetrieverToUse,MBCLI::MBTerminal& AssociatedTerminal) {return(0);};
        virtual void HandleHelp(CommandInfo const& CommandToHandle,MBCLI::MBTerminal& AssociatedTerminal) = 0;
    };
    //extern "C"
    //{
    //    CLI_Extension* CreateExtension();   
    //    void DeleteExtension(CLI_Extension* ExtensionToDelete);
    //}
      
    class MBPM_ClI
    {
    private:
        std::string m_PacketInstallDirectory = "";//kan bara finnas en
        MBPM::MBPP_Client m_ClientToUse;
        PacketRetriever m_PacketRetriever;
        
        std::vector<std::unique_ptr<CLI_Extension,void (*)(void*)>> m_RegisteredExtensions;
        std::unordered_map<std::string, CommandType> m_CustomCommandTypes;
        std::unordered_map<std::string,std::vector<std::pair<CustomCommand,size_t>>> m_TopCommandHooks;


        std::pair<CLI_Extension*,CommandInfo> p_GetHandlingExtension(MBCLI::ProcessedCLInput const& CLIInput,MBPM_PacketInfo const& PacketToHandle);
        void p_RegisterExtension(std::unique_ptr<CLI_Extension,void (*)(void*)> ExtensionToRegister);
        

        //Not vectorized commands
        int p_HandleInstall(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
        int p_HandleGet(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
        int p_HandleIndex(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
        int p_HandleCommands(MBCLI::ProcessedCLInput const& CommandInput,MBCLI::MBTerminal& AssociatedTerminal);


        //Vectorized commands
        int p_HandleUpload(std::vector<PacketIdentifier> PacketsToUpload,MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
        int p_HandleUpdate(std::vector<PacketIdentifier> PacketsToUpdate,MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
        int p_HandleExec(std::vector<PacketIdentifier> PacketsToExecute,MBCLI::ProcessedCLInput const& CommandInput,MBCLI::MBTerminal& AssociatedTerminal);
        int p_HandlePackets(std::vector<PacketIdentifier> PacketsToDisplay,MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
        int p_HandleCustomCommand(std::vector<PacketIdentifier> PackestToHandle,MBCLI::ProcessedCLInput const& CommandInput,MBCLI::MBTerminal* AssociatedTerminal);
        MBError p_ExecuteEmptyCommand(PacketIdentifier const& PacketToHandle,std::string const& CommandName,MBCLI::MBTerminal& TerminalToUse);

        int p_HandlePackSpec(MBCLI::ProcessedCLInput const& CommandInput,MBCLI::ProcessedCLInput& OutInput,std::vector<PacketIdentifier>& OutPackets,MBCLI::MBTerminal& AssociatedTerminal);

        int p_UploadPacketsLocal(std::vector<PacketIdentifier> const& PacketsToUpload,MBCLI::ProcessedCLInput const& Command,MBCLI::MBTerminal* AssociatedTerminal);
        
        std::string p_GetPacketInstallDirectory();
        MBPP_PacketHost p_GetDefaultPacketHost();


        MBPM::MBPP_FileInfoReader i_GetRemoteFileInfo(std::string const& RemoteFile,MBError* OutError);
        MBPM::MBPP_FileInfoReader i_GetInstalledFileInfo(std::string const& InstalledPacket, MBError* OutError);
        MBPM::MBPP_FileInfoReader i_GetPathFileInfo(std::string const& InstalledPacket, MBError* OutError);
        MBPM::MBPP_FileInfoReader i_GetUserFileInfo(std::string const& PacketName, MBError* OutError);
        
        //packet retrievers
        MBPM::MBPP_FileInfoReader p_GetPacketFileInfo(PacketIdentifier const& PacketToInspect,MBError* OutError);
        void p_FilterTypes(MBCLI::ProcessedCLInput const& Command,std::vector<PacketIdentifier>& PacketsToFilter);

        //std::string p_GetInstalledPacketDirectory();
        MBError p_GetCommandPackets(MBCLI::ProcessedCLInput const& CLIInput,PacketLocationType DefaultType,std::vector<PacketIdentifier>& OutPackets, size_t ArgumentOffset = 0);
        PacketIdentifier p_GetPacketIdentifier(std::string const& PacketName, PacketLocationType Type, MBError& OutError);

    public:
        int HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal);
    };
    int MBCLI_Main(int argv,const char** args);
}
