#include "MBPM_CLI.h"
#include "MBCLI/MBCLI.h"
#include "MBPacketManager.h"
#include <MBParsing/MBParsing.h>
#include "MB_PacketProtocol.h"
#include <filesystem>
#include <map>
#include <MBUnicode/MBUnicode.h>
#include <exception>
#include <stdexcept>
#include <algorithm>

#include <MBUtility/MBFiles.h>
#include <system_error>
#include <unordered_set>
#include "MBBuild/MBBuild.h"
#include "Vim/MBPM_Vim.h"
#include "Bash/MBPM_Bash.h"
//#include "MBBuild/MBBuild.h"
//#include "MBCLI/"

namespace MBPM
{
    //BEGIN DownloadPrinter
    MBError DownloadPrinter::NotifyFiles(std::vector<std::string> const& FileToNotify)
    {
        //g�r inget egentligen
        return(true);
    }
    MBError DownloadPrinter::Open(std::string const& FileToDownloadName)
    {
        m_AssociatedTerminal->PrintLine(FileToDownloadName);
        m_AssociatedTerminal->PrintLine("---------------------------------------");
        return(true);
    }
    MBError DownloadPrinter::InsertData(const void* Data, size_t DataSize)
    {
        m_AssociatedTerminal->Print(std::string((const char*)Data,DataSize));
        return(true);
    }
    MBError DownloadPrinter::Close()
    {
        m_AssociatedTerminal->PrintLine("\n---------------------------------------");
        return(true);
    }
    //END DownloadPrinter

    MBError h_TransferLocalFile(std::string const& FilePath,std::string const& ObjectName, MBPP_FileListDownloadHandler* DownloadHandler)
    {
        MBError ReturnValue = true;
        //std::string FilePath = PacketDirectory + FilesystemObjectsToDownload[i];
        if (!std::filesystem::exists(FilePath))
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Failed to find filesystem object: \"" + ObjectName + "\"";
            return(ReturnValue);
        }
        std::ifstream InputStream = std::ifstream(FilePath, std::ios::in | std::ios::binary);
        std::string DataBuffer = std::string(4096, 0);
        DownloadHandler->Open(ObjectName);
        size_t DataToRead = 4096;
        size_t ReadBytes = 0;
        do
        {
            ReadBytes = InputStream.read((char*)DataBuffer.data(), DataToRead).gcount();
            DownloadHandler->InsertData(DataBuffer.data(), ReadBytes);
        } while (ReadBytes == DataToRead);
        DownloadHandler->Close();
        return(ReturnValue);
    }
    MBError DownloadDirectory(std::string const& PacketDirectory, std::vector<std::string> const& FilesystemObjectsToDownload, MBPP_FileListDownloadHandler* DownloadHandler)
    {
        //implicit uppdatrerar directoryn
        MBError ReturnValue = true;
        MBPM::UpdateFileInfo(PacketDirectory);
        MBPM::MBPP_FileInfoReader PacketFileInfo = MBPM::MBPP_FileInfoReader(PacketDirectory+"/MBPM_FileInfo");
        for (size_t i = 0; i < FilesystemObjectsToDownload.size(); i++)
        {
            if (PacketFileInfo.ObjectExists(FilesystemObjectsToDownload[i]))
            {
                if (PacketFileInfo.GetFileInfo(FilesystemObjectsToDownload[i]) != nullptr)
                {
                    std::string FilePath = PacketDirectory + FilesystemObjectsToDownload[i];
                     ReturnValue = h_TransferLocalFile(FilePath, FilesystemObjectsToDownload[i], DownloadHandler);
                }
                else if(PacketFileInfo.GetDirectoryInfo(FilesystemObjectsToDownload[i]) != nullptr)
                {
                    const MBPM::MBPP_DirectoryInfoNode* NodeToIterator = PacketFileInfo.GetDirectoryInfo(FilesystemObjectsToDownload[i]);
                    MBPM::MBPP_DirectoryInfoNode_ConstIterator Iterator = NodeToIterator->begin();
                    while (Iterator != NodeToIterator->end())
                    {
                        std::string FilePath = PacketDirectory+ Iterator.GetCurrentDirectory() + Iterator->FileName;
                        ReturnValue = h_TransferLocalFile(FilePath,FilesystemObjectsToDownload[i]+Iterator.GetCurrentDirectory()+Iterator->FileName,DownloadHandler);
                        if (!ReturnValue)
                        {
                            break;
                        }
                        Iterator++;
                    }
                }
                if (!ReturnValue)
                {
                    break;
                }
            }
            else
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Failed to find filesystem object: \"" + FilesystemObjectsToDownload[i] + "\"";
                return(ReturnValue);
            }
        }
        return(ReturnValue);
    }


    //BEGIN MBPM_CLI
    std::string MBPM_ClI::p_GetPacketInstallDirectory()
    {
        const char* Data = std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY");
        if (Data == nullptr)
        {
            throw std::runtime_error("MBPM_PACKETS_INSTALL_DIRECTORY environment variable is not set");
        }
        std::string ReturnValue = Data;
        if(ReturnValue == "")
        {
            throw std::runtime_error("invalid value for MBPM_PACKETS_INSTALL_DIRECTORY, empty string");
        }
        if (ReturnValue.back() != '/')
        {
            ReturnValue += "/";
        }
        return(ReturnValue);
    }

    MBPP_PacketHost MBPM_ClI::p_GetDefaultPacketHost()
    {
        //MBPP_PacketHost ReturnValue = { "mrboboget.se/MBPM/",MBPP_TransferProtocol::HTTPS,443 };
        MBPP_PacketHost ReturnValue = { "mrboboget.se/MBPM/",MBPP_TransferProtocol::HTTPS,443 };
        return(ReturnValue);
    }
    void MBPM_ClI::p_FilterTypes(MBCLI::ProcessedCLInput const& Command,std::vector<PacketIdentifier>& PacketsToFilter)
    {
        std::vector<PacketIdentifier> NewResult;
        auto PositiveFilters = Command.GetSingleArgumentOptionList("type");
        auto NegativeFilters = Command.GetSingleArgumentOptionList("ntype");
        if(PositiveFilters.size() == 0 && NegativeFilters.size() == 0)
        {
            return;
        }
        for(auto& Packet : PacketsToFilter)
        {
            bool IsIncluded = PositiveFilters.size() == 0;
            MBPM_PacketInfo CurrentPacketInfo = m_PacketRetriever.GetPacketInfo(Packet);
            for(auto const& ValidType : PositiveFilters)
            {
                if(ValidType.first == CurrentPacketInfo.Type)
                {
                    IsIncluded = true;   
                    break;
                }
            } 
            for(auto const& InvalidType : NegativeFilters)
            {
                if(InvalidType.first == CurrentPacketInfo.Type)
                {
                    IsIncluded = false;   
                    break;
                }
            } 
            if(IsIncluded)
            {
                NewResult.push_back(std::move(Packet));   
            }
        }
        std::swap(PacketsToFilter,NewResult);
    }
    MBError MBPM_ClI::p_GetCommandPackets(MBCLI::ProcessedCLInput const& CLIInput, PacketLocationType DefaultType,std::vector<PacketIdentifier>& OutPackets,size_t ArgumentOffset)
    {
        MBError ReturnValue = true;
        //individual packet specifiers
        int DefaultSpecifications = 0;
        if (CLIInput.CommandOptions.find("user") != CLIInput.CommandOptions.end())
        {
            DefaultType = PacketLocationType::User;
            DefaultSpecifications += 1;
        }
        if (CLIInput.CommandOptions.find("installed") != CLIInput.CommandOptions.end())
        {
            DefaultType = PacketLocationType::Installed;
            DefaultSpecifications += 1;
        }
        if (CLIInput.CommandOptions.find("remote") != CLIInput.CommandOptions.end())
        {
            DefaultType = PacketLocationType::Remote;
            DefaultSpecifications += 1;
        }
        if (CLIInput.CommandOptions.find("localpacket") != CLIInput.CommandOptions.end())
        {
            DefaultType = PacketLocationType::Local;
            DefaultSpecifications += 1;
        }
        if (DefaultSpecifications > 1)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "More than 1 default packet type specification";
            return(ReturnValue);
        }
        for (size_t i = ArgumentOffset; i < CLIInput.TopCommandArguments.size(); i++)
        {
            PacketIdentifier NewPacket = p_GetPacketIdentifier(CLIInput.TopCommandArguments[i], DefaultType, ReturnValue);
            if (!ReturnValue)
            {
                return(ReturnValue);
            }
            OutPackets.push_back(std::move(NewPacket));
        }
        auto InstalledSpecifications = CLIInput.GetSingleArgumentOptionList("i");
        for (auto const& PacketName : InstalledSpecifications)
        {
            PacketIdentifier NewIdentifier = p_GetPacketIdentifier(PacketName.first, PacketLocationType::Installed, ReturnValue);
            if (!ReturnValue)
            {
                return(ReturnValue);
            }
            OutPackets.push_back(std::move(NewIdentifier));
        }
        auto LocalSpecifications = CLIInput.GetSingleArgumentOptionList("l");
        for (auto const& PacketName : LocalSpecifications)
        {
            PacketIdentifier NewIdentifier = p_GetPacketIdentifier(PacketName.first, PacketLocationType::Local, ReturnValue);
            if (!ReturnValue)
            {
                return(ReturnValue);
            }
            OutPackets.push_back(std::move(NewIdentifier));
        }
        auto RemoteSpecifications = CLIInput.GetSingleArgumentOptionList("r");
        for (auto const& PacketName : RemoteSpecifications)
        {
            PacketIdentifier NewIdentifier = p_GetPacketIdentifier(PacketName.first, PacketLocationType::Remote, ReturnValue);
            if (!ReturnValue)
            {
                return(ReturnValue);
            }
            OutPackets.push_back(std::move(NewIdentifier));
        }
        auto UserSpecifications = CLIInput.GetSingleArgumentOptionList("u");
        for (auto const& PacketName : UserSpecifications)
        {
            PacketIdentifier NewIdentifier = p_GetPacketIdentifier(PacketName.first, PacketLocationType::User, ReturnValue);
            if (!ReturnValue)
            {
                return(ReturnValue);
            }
            OutPackets.push_back(std::move(NewIdentifier));
        }
        //total packet specifiers
        if (CLIInput.CommandOptions.find("allinstalled") != CLIInput.CommandOptions.end())
        {
            std::vector<PacketIdentifier> InstalledPackets = m_PacketRetriever.GetInstalledPackets(ReturnValue);
            if(!ReturnValue)
            {
                return(ReturnValue);
            }
            for (size_t i = 0; i < InstalledPackets.size(); i++)
            {
                OutPackets.push_back(std::move(InstalledPackets[i]));
            }
        }
        if (CLIInput.CommandOptions.find("alluser") != CLIInput.CommandOptions.end())
        {
            std::vector<PacketIdentifier> UserPackets = m_PacketRetriever.GetUserPackets(ReturnValue);
            if(!ReturnValue)
            {
                return ReturnValue;
            }
            for (size_t i = 0; i < UserPackets.size(); i++)
            {
                OutPackets.push_back(std::move(UserPackets[i]));
            }
        }
        //specified packet modifiers
        int PacketModifiers = 0;
        if (CLIInput.CommandOptions.find("dependants") != CLIInput.CommandOptions.end())
        {
            PacketModifiers += 1;
            std::vector<std::string> MissingPackets;
            OutPackets = m_PacketRetriever.GetPacketsDependees(OutPackets,ReturnValue);
            if(!ReturnValue)
            {
                return(ReturnValue);   
            }
        }
        if (CLIInput.CommandOptions.find("dependantsi") != CLIInput.CommandOptions.end())
        {
            PacketModifiers += 1;
            std::vector<std::string> MissingPackets;
            OutPackets = m_PacketRetriever.GetPacketsDependees(OutPackets,ReturnValue,true);
            if(!ReturnValue)
            {
                return(ReturnValue);   
            }
        }
        if (CLIInput.CommandOptions.find("dependanciesi") != CLIInput.CommandOptions.end())
        {
            PacketModifiers += 1;
            std::vector<std::string> MissingPackets;
            OutPackets = m_PacketRetriever.GetPacketsDependancies(OutPackets,ReturnValue,true);
            if(!ReturnValue)
            {
                return(ReturnValue);   
            }
        }
        if (CLIInput.CommandOptions.find("dependancies") != CLIInput.CommandOptions.end())
        {
            PacketModifiers += 1;
            std::vector<std::string> MissingPackets;
            OutPackets = m_PacketRetriever.GetPacketsDependancies(OutPackets,ReturnValue);
            if(!ReturnValue)
            {
                return(ReturnValue);   
            }
        }
        if (PacketModifiers > 1)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Can only specify at 1 packet modifier";
            return(ReturnValue);
        }
        //Sub Packets
        auto SubsIt = CLIInput.CommandOptions.find("subs");
        auto SubsiIt = CLIInput.CommandOptions.find("subsi");
        if(SubsiIt != CLIInput.CommandOptions.end() || SubsIt != CLIInput.CommandOptions.end())
        {
            if(SubsiIt != CLIInput.CommandOptions.end() && SubsIt != CLIInput.CommandOptions.end())
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Can only specify either subs or subsi"; 
                return(ReturnValue);
            } 
            bool Inclusive = SubsiIt != CLIInput.CommandOptions.end();
            std::vector<PacketIdentifier> NewPackets;
            for(auto& Packet : OutPackets)
            {
                auto SubPackets = m_PacketRetriever.GetSubPackets(Packet,ReturnValue);
                if(!ReturnValue) return(ReturnValue);
                for(auto& SubPacket : SubPackets)
                {
                    NewPackets.push_back(std::move(SubPacket)); 
                }
                if(Inclusive)
                {
                    NewPackets.push_back(std::move(Packet));   
                }
            }
            OutPackets = std::move(NewPackets);
        }
        //Packet filters
        auto PositiveAttributeStrings = CLIInput.GetSingleArgumentOptionList("a");
        auto NegativeAttributeStrings = CLIInput.GetSingleArgumentOptionList("na");
        if(PositiveAttributeStrings.size() > 0 || NegativeAttributeStrings.size() > 0)
        {
            std::unordered_set<std::string> PositiveAttributeFilters;
            std::unordered_set<std::string> NegativeAttributeFilters; 
            for(auto const& String : PositiveAttributeStrings)
            {
                std::string NewAttribute = String.first; 
                PositiveAttributeFilters.insert(NewAttribute);
            }
            for(auto const& String : NegativeAttributeStrings)
            {
                std::string NewAttribute = String.first; 
                NegativeAttributeFilters.insert(NewAttribute);
            }
            std::vector<PacketIdentifier> NewReturnValue;
            for(PacketIdentifier const& Packet : OutPackets)
            {
                bool PacketHasPositiveAttribute = PositiveAttributeFilters.size() > 0 ? false : true; 
                bool PacketHasNegativeAttribute = false;
                MBPM_PacketInfo CurrentPacketInfo = m_PacketRetriever.GetPacketInfo(Packet);
                if(!ReturnValue)
                {
                    return(ReturnValue);
                }
                for(std::string const& Attribute : CurrentPacketInfo.Attributes)
                {
                    if(PositiveAttributeFilters.find(Attribute) != PositiveAttributeFilters.end())
                    {
                        PacketHasPositiveAttribute = true;
                    }
                    if(NegativeAttributeFilters.find(Attribute) != NegativeAttributeFilters.end())
                    {
                        PacketHasNegativeAttribute = true;
                    }
                }
                if(PacketHasPositiveAttribute && !PacketHasNegativeAttribute)
                {
                    NewReturnValue.push_back(Packet);   
                }
            }
            OutPackets = std::move(NewReturnValue);
        }
        p_FilterTypes(CLIInput,OutPackets);
        // 

        //Make x grejer
        int MakeCommandsCount = CLIInput.GetSingleArgumentOptionList("m").size(); 
        if(MakeCommandsCount > 1)
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = "Only one make command can be specified";   
            return(ReturnValue);
        }
        if(MakeCommandsCount == 1)
        {
            std::string MakeString = CLIInput.GetSingleArgumentOptionList("m")[0].first;
            PacketLocationType PacketType = PacketLocationType::Null;
            if(MakeString == "installed")
            {
                PacketType = PacketLocationType::Installed; 
            }
            else if(MakeString == "user")
            {
                PacketType = PacketLocationType::User;
            }
            else if(MakeCommandsCount)
            {
                ReturnValue = false;
                ReturnValue.ErrorMessage = "Invalid make type: "+MakeString +". Only \"installed\" and user \"allowed\""; 
                return(ReturnValue);
            }
            if(PacketType != PacketLocationType::Null)
            {
                std::vector<PacketIdentifier> NewReturnValue;
                for(PacketIdentifier const& Identifier : OutPackets)
                {
                    if(Identifier.PacketLocation == PacketType)
                    {
                        NewReturnValue.push_back(Identifier);
                        continue;   
                    }
                    PacketIdentifier NewPacket = p_GetPacketIdentifier(Identifier.PacketName,PacketType,ReturnValue); 
                    if(!ReturnValue)
                    {
                        return(ReturnValue);  
                    } 
                    NewReturnValue.push_back(std::move(NewPacket));
                } 
                OutPackets = NewReturnValue;
            }
        }
        // 

        return(ReturnValue);
    }
    PacketIdentifier MBPM_ClI::p_GetPacketIdentifier(std::string const& PacketName, PacketLocationType Type, MBError& OutError)
    {
        PacketIdentifier ReturnValue;


        //Implicitly interpreting invalid packet names as a local packet
        bool IsLocalPath = false;
        if (PacketName == "."|| (PacketName.size() >= 2 && (PacketName.substr(0,2) == "./" || PacketName.substr(0,2) == ".\\" || PacketName.substr(0,2) == ".." || PacketName[0] == '/' || PacketName[1] == ':')))
        {
            IsLocalPath = true;
        }
        if (Type == PacketLocationType::Local || IsLocalPath)
        {
            ReturnValue = m_PacketRetriever.GetLocalpacket(PacketName,OutError);
        }
        else if (Type == PacketLocationType::User)
        {
            ReturnValue = m_PacketRetriever.GetUserPacket(PacketName,OutError);
        }
        else if (Type == PacketLocationType::Installed)
        {
            ReturnValue = m_PacketRetriever.GetInstalledPacket(PacketName,OutError);
        }
        else if (Type == PacketLocationType::Remote)
        {
            ReturnValue.PacketLocation = PacketLocationType::Remote;
            ReturnValue.PacketName = PacketName;
            ReturnValue.PacketURI = "default";
        }
        else if (Type == PacketLocationType::Local)
        {
            ReturnValue = m_PacketRetriever.GetLocalpacket(PacketName,OutError);
        }
        else
        {
            throw std::runtime_error("Invalid packet type in p_GetPacketIdentifier");
        }
        return(ReturnValue);
    }

    MBPM::MBPP_FileInfoReader MBPM_ClI::i_GetRemoteFileInfo(std::string const& RemoteFile, MBError* OutError)
    {
        m_ClientToUse.Connect(p_GetDefaultPacketHost());
        MBPM::MBPP_FileInfoReader ReturnValue = m_ClientToUse.GetPacketFileInfo(RemoteFile, OutError);
        return(ReturnValue);
    }
    MBPM::MBPP_FileInfoReader MBPM_ClI::i_GetInstalledFileInfo(std::string const& InstalledPacket,MBError* OutError)
    {
        MBPM::MBPP_FileInfoReader ReturnValue;
        std::string PacketDirectory = m_PacketRetriever.GetInstalledPacket(InstalledPacket,*OutError).PacketURI;
        if(!*OutError)
        {
            return(ReturnValue);
        }
        if (PacketDirectory == "" || !std::filesystem::exists(PacketDirectory + "/MBPM_FileInfo"))
        {
            *OutError = false;
            OutError->ErrorMessage = "Packet not installed or packet FileInfo missing";
            return(ReturnValue);
        }
        ReturnValue = MBPM::MBPP_FileInfoReader(PacketDirectory + "/MBPM_FileInfo");
        return(ReturnValue);
    }
    MBPM::MBPP_FileInfoReader MBPM_ClI::i_GetPathFileInfo(std::string const& PacketPath, MBError* OutError)
    {
        MBPM::MBPP_FileInfoReader ReturnValue;
        if (std::filesystem::exists(PacketPath) && std::filesystem::directory_entry(PacketPath).is_regular_file())
        {
            ReturnValue = MBPM::MBPP_FileInfoReader(PacketPath);
        }
        else if (std::filesystem::exists(PacketPath + "/MBPM_FileInfo") && std::filesystem::directory_entry(PacketPath + "/MBPM_FileInfo").is_regular_file())
        {
            ReturnValue = MBPM::MBPP_FileInfoReader(PacketPath+ "/MBPM_FileInfo");
        }
        else
        {
            *OutError = false;
            OutError->ErrorMessage = "Couldnt find MBPM_FileInfo at \"" + PacketPath + "\" or \"" + PacketPath + "/MBPM_FileInfo\"";
        }
        return(ReturnValue);
    }
    MBPM::MBPP_FileInfoReader MBPM_ClI::i_GetUserFileInfo(std::string const& PacketName, MBError* OutError)
    {
        //Kan optimeras
        MBPM::MBPP_FileInfoReader ReturnValue;
        std::vector<PacketIdentifier> AllUserPackets = m_PacketRetriever.GetUserPackets(*OutError);
        if(!OutError)
        {
            return(ReturnValue);   
        }
        bool FoundPacket = false;
        for (size_t i = 0; i < AllUserPackets.size(); i++)
        {
            if (AllUserPackets[i].PacketName == PacketName)
            {
                ReturnValue = i_GetPathFileInfo(AllUserPackets[i].PacketURI,OutError);
                FoundPacket = true;
                break;
            }
        }
        if (!FoundPacket)
        {
            *OutError = false;
            OutError->ErrorMessage = "Couldn't find user packet";
        }
        return(ReturnValue);
    }
    MBPM::MBPP_FileInfoReader MBPM_ClI::p_GetPacketFileInfo(PacketIdentifier const& PacketToInspect, MBError* OutError)
    {
        MBPM::MBPP_FileInfoReader ReturnValue;
        MBError Error = true;
        if (PacketToInspect.PacketLocation == PacketLocationType::Installed)
        {
            ReturnValue = i_GetInstalledFileInfo(PacketToInspect.PacketName, &Error);
        }
        else if (PacketToInspect.PacketLocation == PacketLocationType::Remote)
        {
            ReturnValue = i_GetRemoteFileInfo(PacketToInspect.PacketName, &Error);
        }
        else if (PacketToInspect.PacketLocation == PacketLocationType::Local)
        {
            ReturnValue = i_GetPathFileInfo(PacketToInspect.PacketURI, &Error);
        }
        else if (PacketToInspect.PacketLocation == PacketLocationType::User)
        {
            ReturnValue = i_GetUserFileInfo(PacketToInspect.PacketName, &Error);
        }
        if (!Error && OutError != nullptr) 
        {
            *OutError = std::move(Error);
        }
        return(ReturnValue);
    }
    std::string h_FileSizeToString(uint64_t FileSize)
    {
        std::string ReturnValue = "";
        uint8_t FileSizeLog = std::log10(FileSize);
        if (FileSizeLog < 3)
        {
            ReturnValue = std::to_string(FileSize) + " B";
        }
        else if (FileSizeLog >= 3 && FileSizeLog < 6)
        {
            ReturnValue = std::to_string(FileSize / 1000) + " KB";
        }
        else if (FileSizeLog >= 6 && FileSizeLog < 9)
        {
            ReturnValue = std::to_string(FileSize / 1000000) + " MB";
        }
        else
        {
            ReturnValue = std::to_string(FileSize / 1000000000) + " GB";
        }
        return(ReturnValue);
    }
    void PrintFileInfo(MBPM::MBPP_FileInfoReader const& InfoToPrint, std::vector<std::string> const& FilesystemObjectsToPrint, MBCLI::MBTerminal* AssociatedTerminal)
    {
        std::vector<std::string> ObjectsToprint = FilesystemObjectsToPrint;
        if (ObjectsToprint.size() == 0)
        {
            ObjectsToprint = { "/" };
        }
        std::vector<std::string> FilePaths = {};
        size_t LargestPath = 0;
        std::vector<std::string> FileSizes = {};
        size_t LargestSize = 0;
        std::vector<std::string> Hashes = {};
        size_t LargestHash = 0;
        std::vector<std::string> Errors = {};
        for (size_t i = 0; i < ObjectsToprint.size(); i++)
        {
            if (!InfoToPrint.ObjectExists(ObjectsToprint[i]))
            {
                Errors.push_back(ObjectsToprint[i]); 
            }
            if (InfoToPrint.GetDirectoryInfo(ObjectsToprint[i]) != nullptr)
            {
                const MBPM::MBPP_DirectoryInfoNode* InfoTopNode = InfoToPrint.GetDirectoryInfo(ObjectsToprint[i]);
                MBPM::MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = InfoTopNode->begin();
                MBPM::MBPP_DirectoryInfoNode_ConstIterator End = InfoTopNode->end();
                while (DirectoryIterator != End)
                {
                    std::string Filename        = DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName;
                    std::string FileHash        = MBUtility::ReplaceAll(MBUtility::HexEncodeString((*DirectoryIterator).FileHash), " ", "");
                    std::string FileSizeString    = h_FileSizeToString((*DirectoryIterator).FileSize);
                    //f�ruts�tter s�klart ascii B)
                    if (Filename.size() > LargestPath)
                    {
                        LargestPath = Filename.size();
                    }
                    if (FileSizeString.size() > LargestSize)
                    {
                        LargestSize = FileSizeString.size();
                    }
                    //AssociatedTerminal->PrintLine(Filename+"\t\t\t\t"+FileSizeString+"\t"+ FileHash);
                    FilePaths.push_back(Filename);
                    FileSizes.push_back(FileSizeString);
                    Hashes.push_back(FileHash);
                    DirectoryIterator++;
                }
            }
            else if (InfoToPrint.GetFileInfo(ObjectsToprint[i]) != nullptr)
            {
                auto FileInfo = InfoToPrint.GetFileInfo(ObjectsToprint[i]);
                std::string Filename = ObjectsToprint[i];
                std::string FileHash = MBUtility::ReplaceAll(MBUtility::HexEncodeString(FileInfo->FileHash)," ","");
                std::string FileSizeString = h_FileSizeToString(FileInfo->FileSize);
                if (Filename.size() > LargestPath)
                {
                    LargestPath = Filename.size();
                }
                if (FileSizeString.size() > LargestSize)
                {
                    LargestSize = FileSizeString.size();
                }
                //AssociatedTerminal->PrintLine(Filename+"\t\t\t\t"+FileSizeString+"\t"+ FileHash);
                FilePaths.push_back(Filename);
                FileSizes.push_back(FileSizeString);
                Hashes.push_back(FileHash);
            }
        }
        for (size_t i = 0; i < FilePaths.size(); i++)
        {
            std::string StringToPrint = FilePaths[i] + std::string(LargestPath - FilePaths[i].size(), ' ') + std::string(4, ' ') +
                FileSizes[i] + std::string(LargestSize - FileSizes[i].size(), ' ') + std::string(4, ' ') +
                Hashes[i];
            AssociatedTerminal->PrintLine(StringToPrint);
        }
        if (Errors.size() > 0)
        {
            AssociatedTerminal->PrintLine("Couldn't find following filesystem objects:");
            for (size_t i = 0; i < Errors.size(); i++)
            {
                AssociatedTerminal->PrintLine(Errors[i]);
            }
        }
    }
    void PrintFileInfoDiff(MBPM::MBPP_FileInfoDiff const& InfoToPrint,MBCLI::MBTerminal* AssociatedTerminal)
    {
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
        AssociatedTerminal->PrintLine("New directories:");
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Green);
        for (auto const& NewDirectories : InfoToPrint.AddedDirectories)
        {
            AssociatedTerminal->PrintLine(NewDirectories);
        }
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
        AssociatedTerminal->PrintLine("New files:");
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Green);
        for (auto const& NewFiles : InfoToPrint.AddedFiles)
        {
            AssociatedTerminal->PrintLine(NewFiles);
        }
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
        AssociatedTerminal->PrintLine("Updated files:");
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Cyan);
        for (auto const& UpdatedFiles : InfoToPrint.UpdatedFiles)
        {
            AssociatedTerminal->PrintLine(UpdatedFiles);
        }
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
        AssociatedTerminal->PrintLine("Removed directories:");
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
        for (auto const& RemovedDirectories : InfoToPrint.DeletedDirectories)
        {
            AssociatedTerminal->PrintLine(RemovedDirectories);
        }
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Yellow);
        AssociatedTerminal->PrintLine("Removed files:");
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
        for (auto const& RemovedFiles : InfoToPrint.RemovedFiles)
        {
            AssociatedTerminal->PrintLine(RemovedFiles);
        }
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
    }
    int MBPM_ClI::p_HandleUpdate(std::vector<PacketIdentifier> PacketsToUpdate,MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        bool AllowUser = CommandInput.CommandOptions.find("allowuser") != CommandInput.CommandOptions.end();
        for (size_t i = 0; i < PacketsToUpdate.size(); i++)
        {
            if((PacketsToUpdate[i].Flags & PacketIdentifierFlag::SubPacket) != 0)
            {
                AssociatedTerminal->PrintLine("Can't update subpackets individually"); 
                return(1);
            }
            if (PacketsToUpdate[i].PacketLocation != PacketLocationType::Installed)
            {
                if(PacketsToUpdate[i].PacketLocation == PacketLocationType::User && AllowUser)
                {
                    continue;    
                }
                AssociatedTerminal->PrintLine("Can only update installed packets");
                return 1;
            }
        }
        std::string InstallDirectory = p_GetPacketInstallDirectory();
        std::vector<std::string> FailedPackets = {};
        m_ClientToUse.Connect(p_GetDefaultPacketHost());
        for (size_t i = 0; i < PacketsToUpdate.size(); i++)
        {
            //ANTAGANDE det h�r g�r bara f�r packets som man inte �ndrar p�, annars kommer MBPM_FileInfo beh�va uppdateras f�rst, f�r se hur l�ng tid det tar f�r grejer
            AssociatedTerminal->PrintLine("Updating packet " + PacketsToUpdate[i].PacketName);
            MBError UpdateResult = m_ClientToUse.UpdatePacket(PacketsToUpdate[i].PacketURI, PacketsToUpdate[i].PacketName);
            if (!UpdateResult)
            {
                FailedPackets.push_back(PacketsToUpdate[i].PacketName);
            }
            else
            {
                AssociatedTerminal->PrintLine("Successfully updated packet " + PacketsToUpdate[i].PacketName);
            }
        }
        if (FailedPackets.size() == 0)
        {
            AssociatedTerminal->PrintLine("Successfully updated all packets!");
        }
        else
        {
            AssociatedTerminal->PrintLine("Failed updating following packets:");
            ReturnValue = 1;
            for (size_t i = 0; i < FailedPackets.size(); i++)
            {
                AssociatedTerminal->PrintLine(FailedPackets[i]);
            }
        }
        return(ReturnValue);
    }
    std::vector<MBCLI::ProcessedCLInput> h_SplitCommand(MBCLI::ProcessedCLInput const& CommandToSplit,size_t MaxCount = -1)
    {
        std::vector<MBCLI::ProcessedCLInput> ReturnValue; 
        size_t PreviousDelimPos = 0;
        size_t ParseOffset = 0;
        std::vector<const char*> CommandTokens;
        for(auto const& String : CommandToSplit.TotalCommandTokens)
        {
            CommandTokens.push_back(String.data());   
        }
        while(ParseOffset < CommandToSplit.TotalCommandTokens.size())
        {
            if(ReturnValue.size() >= MaxCount)
            {
                ParseOffset = CommandToSplit.TotalCommandTokens.size();
                break;
            }
            if(CommandToSplit.TotalCommandTokens[ParseOffset] == "--")
            {
                ReturnValue.push_back(MBCLI::ProcessedCLInput(ParseOffset-PreviousDelimPos,CommandTokens.data()+PreviousDelimPos));
                PreviousDelimPos = ParseOffset;
            }    
            ParseOffset++;
        }
        ReturnValue.push_back(MBCLI::ProcessedCLInput(ParseOffset-PreviousDelimPos,CommandTokens.data()+PreviousDelimPos));
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandlePackSpec(MBCLI::ProcessedCLInput const& CommandInput,MBCLI::ProcessedCLInput& OutInput,std::vector<PacketIdentifier>& OutPackets,MBCLI::MBTerminal& AssociatedTerminal)
    {
        int ReturnValue = 0; 
        std::vector<MBCLI::ProcessedCLInput> Commands = h_SplitCommand(CommandInput,1);
        if(Commands.size() < 2)
        {
            AssociatedTerminal.PrintLine("pathspec requires delimiting -- to determine the command to execute");
            return(1);
        }
        if(Commands[0].TopCommandArguments.size() == 0)
        {
            AssociatedTerminal.PrintLine("pathspec requires subcommand to evaluate");    
            return(1);
        }
        //Evaluate sub command
        OutInput = std::move(Commands[1]);  
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandleExec(std::vector<PacketIdentifier> PacketList,MBCLI::ProcessedCLInput const& CommandInput,MBCLI::MBTerminal& AssociatedTerminal)
    {
        int ReturnValue = 0;
        MBError PacketListError = true;
        std::string CommandToExecute = "";
        std::vector<MBCLI::ProcessedCLInput> Commands = h_SplitCommand(CommandInput,1);
        if(Commands.size() < 2)
        {
            AssociatedTerminal.PrintLine("exec command needs -- to delimi arguments to exec and command to execute");   
            return 1;
        }
        for(size_t i = 1; i < Commands[1].TotalCommandTokens.size();i++)
        {
            CommandToExecute += Commands[1].TotalCommandTokens[i] + " ";
        }
        bool QuitOnError = CommandInput.CommandOptions.find("continue-error") == CommandInput.CommandOptions.end();
        std::filesystem::path OriginalDir = std::filesystem::current_path();
        std::vector<PacketIdentifier> FailedPackets;
        for(auto const& Packet : PacketList)
        {
            std::error_code Error;
            std::filesystem::current_path(Packet.PacketURI,Error);
            if(Error)
            {
                AssociatedTerminal.PrintLine("Error changing directory to "+MBUnicode::PathToUTF8(Packet.PacketURI));
                return 1;
            }
            int Result = std::system(CommandToExecute.c_str());
            if(Result != 0)
            {
               if(QuitOnError)
               {
                   AssociatedTerminal.PrintLine("Error executing command");
                   return 1;
               }    
               FailedPackets.push_back(Packet);    
            }
            std::filesystem::current_path(OriginalDir);
        }
        if(FailedPackets.size() > 0)
        {
            ReturnValue = 1;   
            AssociatedTerminal.PrintLine("Command failed for following packets:");
            for(auto const& FailedPacket : FailedPackets)
            {
                AssociatedTerminal.PrintLine(FailedPacket.PacketName);
            }
        }
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandleInstall(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        std::string InstallDirectory = p_GetPacketInstallDirectory();
        std::stack<std::string> PacketsToInstall = {};
        for (size_t i = 0; i < CommandInput.TopCommandArguments.size(); i++)
        {
            PacketsToInstall.push(CommandInput.TopCommandArguments[i]);
        }
        MBPM::MBPP_ComputerInfo PreviousInfo = m_ClientToUse.GetComputerInfo();
        if (CommandInput.CommandOptions.find("nocomputerdiff") != CommandInput.CommandOptions.end())
        {
            m_ClientToUse.SetComputerInfo(MBPM::MBPP_ComputerInfo());
        }
        else
        {
            m_ClientToUse.SetComputerInfo(MBPM::MBPP_Client::GetSystemComputerInfo());
        }
        std::unordered_set<std::string> AlreadyInstalledPackets = {};
        std::stack<std::string> NewPacketsToCompile = {};
        while(PacketsToInstall.size() > 0)
        {
            std::string CurrentPacket = PacketsToInstall.top();
            PacketsToInstall.pop();
            if (std::filesystem::exists(InstallDirectory + CurrentPacket) || AlreadyInstalledPackets.find(CurrentPacket) != AlreadyInstalledPackets.end())
            {
                AlreadyInstalledPackets.insert(CurrentPacket);
                continue;
            }
            MBError DownloadStatus = m_ClientToUse.Connect(p_GetDefaultPacketHost());
            if (DownloadStatus)
            {
                DownloadStatus = m_ClientToUse.DownloadPacket(InstallDirectory + CurrentPacket + "/", CurrentPacket);
            }
            if (DownloadStatus)
            {
                MBPM::CreatePacketFilesData(InstallDirectory + CurrentPacket + "/");
                if (std::filesystem::exists(InstallDirectory + CurrentPacket + "/MBPM_PacketInfo"))
                {
                    MBPM::MBPM_PacketInfo InstalledPacketInfo = MBPM::ParseMBPM_PacketInfo(InstallDirectory + CurrentPacket + "/MBPM_PacketInfo");
                    if (InstalledPacketInfo.PacketName != "")
                    {
                        for (size_t i = 0; i < InstalledPacketInfo.PacketDependancies.size(); i++)
                        {
                            if (AlreadyInstalledPackets.find(InstalledPacketInfo.PacketDependancies[i]) == AlreadyInstalledPackets.end())
                            {
                                PacketsToInstall.push(InstalledPacketInfo.PacketDependancies[i]);
                            }
                        }
                    }
                }
            }
            AlreadyInstalledPackets.insert(CurrentPacket);
        }
        return ReturnValue;
    }
    int MBPM_ClI::p_HandleGet(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        if (CommandInput.TopCommandArguments.size() < 2)
        {
            AssociatedTerminal->PrintLine("Command \"get\" needs atleast a package and a filesystem object");
            return 1;
        }
        std::string PacketName = CommandInput.TopCommandArguments[0];

        PacketIdentifier PacketToGet;
        bool RemotePacket = CommandInput.CommandOptions.find("remote") != CommandInput.CommandOptions.end();
        bool LocalPacket = CommandInput.CommandOptions.find("local") != CommandInput.CommandOptions.end();
        bool UserPacket = CommandInput.CommandOptions.find("user") != CommandInput.CommandOptions.end();
        bool InstalledPacket = CommandInput.CommandOptions.find("installed") != CommandInput.CommandOptions.end();
        if (RemotePacket + LocalPacket + UserPacket + InstalledPacket > 1)
        {
            AssociatedTerminal->PrintLine("Can only specify one packet type");
            return 1;
        }
        if (RemotePacket + LocalPacket + UserPacket + InstalledPacket == 0)
        {
            //implicit att det �r remote packet
            RemotePacket = true;
        }
        bool PrintToStdout = false;
        bool DownloadLocally = true;
        std::string OutputDirectory = "./";
        if (CommandInput.CommandOptions.find("stdout") != CommandInput.CommandOptions.end())
        {
            PrintToStdout = true;
            DownloadLocally = false;
        }
        std::vector<std::pair<std::string,int>> SpecifiedOutputDirectories = CommandInput.GetSingleArgumentOptionList("o");
        if (SpecifiedOutputDirectories.size() > 1)
        {
            AssociatedTerminal->PrintLine("Can only specify one output directory");
            return 1;
        }
        else if (SpecifiedOutputDirectories.size() == 1)
        {
            OutputDirectory = SpecifiedOutputDirectories[0].first;
        }
        std::vector<std::string> ObjectsToGet = {};
        for (size_t i = 1; i < CommandInput.TopCommandArguments.size(); i++)
        {
            if (CommandInput.TopCommandArguments[i] == "" || CommandInput.TopCommandArguments[i][0] != '/')
            {
                AssociatedTerminal->PrintLine("Filesystem objects to get has to be absolute path starting with /");
                return 1;
            }
            ObjectsToGet.push_back(CommandInput.TopCommandArguments[i]);
        }
        if (ObjectsToGet.size() == 0)
        {
            AssociatedTerminal->PrintLine("Didnt specify a fileystem object to get");
            return 1;
        }
        std::vector<std::string> GetObjectErrors = {};
        std::unique_ptr<MBPM::MBPP_FileListDownloadHandler> DownloadHandler = nullptr;
        if (DownloadLocally)
        {
            DownloadHandler = std::unique_ptr<MBPP_FileListDownloadHandler>(new MBPM::MBPP_FileListDownloader(OutputDirectory));
        }
        else
        {
            m_ClientToUse.DisableLogging();
            DownloadHandler = std::unique_ptr<MBPP_FileListDownloadHandler>(new DownloadPrinter(AssociatedTerminal));
        }
        if (RemotePacket)
        {
            if (CommandInput.CommandOptions.find("path") != CommandInput.CommandOptions.end())
            {
                AssociatedTerminal->PrintLine("cannot get path of remote packet");
                return 1;
            }
            MBError NetworkError = m_ClientToUse.Connect(p_GetDefaultPacketHost()); 
            if (!NetworkError)
            {
                AssociatedTerminal->PrintLine("Error connecting to host: " + NetworkError.ErrorMessage);
                return 1;
            }
            MBPP_FileInfoReader RemoteFileInfo = m_ClientToUse.GetPacketFileInfo(PacketName, &NetworkError);
            if (!NetworkError)
            {
                AssociatedTerminal->PrintLine("Error getting packet file info: " + NetworkError.ErrorMessage);
                return 1;
            }
            for (size_t i = 0; i < ObjectsToGet.size(); i++)
            {
                if (RemoteFileInfo.ObjectExists(ObjectsToGet[i]))
                {
                    if (RemoteFileInfo.GetFileInfo(ObjectsToGet[i]) != nullptr)
                    {
                        m_ClientToUse.DownloadPacketFiles(PacketName, { ObjectsToGet[i] }, DownloadHandler.get());
                    }
                    else if (RemoteFileInfo.GetDirectoryInfo(ObjectsToGet[i]) != nullptr)
                    {
                        m_ClientToUse.DownloadPacketDirectories(PacketName, { ObjectsToGet[i] }, DownloadHandler.get());
                    }
                }
                else
                {
                    GetObjectErrors.push_back("Error getting object: \""+ObjectsToGet[i]+"\" doesn't exist");
                }
            }
        }
        else
        {
            std::string PacketDirectory = "";
            MBError Result = true;
            if (LocalPacket)
            {
                PacketDirectory = CommandInput.TopCommandArguments[0];
            }
            else if (UserPacket)
            {
                PacketIdentifier CurrentPacket = m_PacketRetriever.GetUserPacket(CommandInput.TopCommandArguments[0],Result);
                PacketDirectory = CurrentPacket.PacketURI;
            }
            else if(InstalledPacket)
            {
                PacketIdentifier CurrentPacket = m_PacketRetriever.GetUserPacket(CommandInput.TopCommandArguments[0],Result);
                PacketDirectory = CurrentPacket.PacketURI;
            }
            if(!Result)
            {
                AssociatedTerminal->PrintLine("Failed retrieving packet to download from: "+Result.ErrorMessage);   
                return(1);
            }
            if (CommandInput.CommandOptions.find("path") != CommandInput.CommandOptions.end())
            {
                //a absolute packet path always begins with /
                for (std::string const& PacketPath : ObjectsToGet)
                {
                    std::filesystem::path PathToPrint = std::filesystem::path(PacketDirectory + PacketPath.substr(1)).lexically_normal();
                    AssociatedTerminal->PrintLine(MBUnicode::PathToUTF8(PathToPrint));
                }
            }
            else 
            {
                Result = DownloadDirectory(PacketDirectory, ObjectsToGet, DownloadHandler.get());
            }
            if(!Result)
            {
                AssociatedTerminal->PrintLine("Error downloading files: " + Result.ErrorMessage);
            }
        }
        return(ReturnValue);
    }
    std::vector<std::string> h_GetDirectoryFiles(std::string const& DirectoryPath, std::string const& DirectoryPrefix)
    {
        std::vector<std::string> ReturnValue = {};
        std::filesystem::recursive_directory_iterator DirectoryIterator = std::filesystem::recursive_directory_iterator(DirectoryPath);
        for (auto const& Entry : DirectoryIterator)
        {
            if (Entry.is_regular_file())
            {
                std::filesystem::path RelativePath = std::filesystem::relative(Entry.path(), DirectoryPath);
                ReturnValue.push_back(DirectoryPrefix + MBUnicode::PathToUTF8(RelativePath));
            }
        }
        return(ReturnValue);
    }
    std::set<std::string> h_GetPacketsToSend(MBCLI::ProcessedCLInput const& CommandInput, std::string const& PacketPath, std::string const& PacketDirectory,MBCLI::MBTerminal* AssociatedTerminal)
    {
        std::set<std::string> ReturnValue = {};
        if (!(std::filesystem::exists(PacketPath + PacketDirectory) && std::filesystem::directory_entry(PacketPath + PacketDirectory).is_regular_file()))
        {
            return(ReturnValue);
        }
        std::vector<size_t> const& CommandPosition = CommandInput.CommandPositionalOptions.at("d");
        for (size_t i = 0; i < CommandPosition.size(); i++)
        {
            if (CommandPosition[i] + 1 >= CommandInput.TotalCommandTokens.size())
            {
                AssociatedTerminal->PrintLine("Positional argument -d given without additional input");
                return (ReturnValue);
            }
            std::string DirectoryToSend = CommandInput.TotalCommandTokens[CommandPosition[i] + 1];
            //ANTAGANDE denna funktion anv�nds f�r att specificera directorys som annars kanske inte tas upp af FileInfoIgnoren, och g�r d�rf�r igenom hela directoryn
            std::vector<std::string> NewFiles = h_GetDirectoryFiles(PacketPath + DirectoryToSend, DirectoryToSend);
            for(std::string const& Paths : NewFiles)
            {
                ReturnValue.insert(Paths);
            }
        }
        return(ReturnValue);
    }
    void h_GetUpdateToSend(MBCLI::ProcessedCLInput const& CommandInput, MBPP_FileInfoReader const& ServerFileInfo, std::string const& PacketDirectory, std::vector<std::string>& OutFilesToSend, std::vector<std::string>& OutFilesToDelete, MBCLI::MBTerminal* AssociatedTerminal)
    {
        //aningen innefektivt iomed all kopiering av data men �r viktigare med korrekthet just nu
        std::set<std::string> FilesToSend = {};
        std::set<std::string> ObjectsToDelete = {};
        
        std::vector<std::pair<std::string,int>> FileSpecifications = CommandInput.GetSingleArgumentOptionList("f");
        for (size_t i = 0; i < FileSpecifications.size(); i++)
        {
            if (FileSpecifications[i].first.size() == 0 || FileSpecifications[i].first[0] != '/')
            {
                AssociatedTerminal->PrintLine("Invalid file specification: needs to be an absolute path starting with / relative to packet directory");
                return;
            }
            if (std::filesystem::exists(PacketDirectory + "/" + FileSpecifications[i].first) && std::filesystem::is_regular_file(PacketDirectory + "/" + FileSpecifications[i].first))
            {
                FilesToSend.insert(FileSpecifications[i].first);
            }
            else
            {
                AssociatedTerminal->PrintLine("Cant find file \"" + FileSpecifications[i].first + "\" in packet, omitting to send update");
                return;
            }
        }
        std::vector<std::pair<std::string, int>> DirectorySpecifications = CommandInput.GetSingleArgumentOptionList("d");
        for (size_t i = 0; i < DirectorySpecifications.size(); i++)
        {
            std::string const& CurrentDirectory = DirectorySpecifications[i].first;
            if (CurrentDirectory.size() == 0 || CurrentDirectory.front() != '/')
            {
                AssociatedTerminal->PrintLine("Invalid directory specification: needs to be an absolute path starting with / relative to packet directory");
                return;
            }
            if (!std::filesystem::exists(PacketDirectory + CurrentDirectory) || !std::filesystem::is_directory(PacketDirectory + CurrentDirectory))
            {
                AssociatedTerminal->PrintLine("Cant find directory \"" + DirectorySpecifications[i].first + "\" in packet, omitting to send update");
                return;
            }
            MBPP_FileInfoReader DirectoryInfo = CreateFileInfo(PacketDirectory + CurrentDirectory);
            if (ServerFileInfo.ObjectExists(CurrentDirectory))
            {
                MBPP_FileInfoDiff DirectoriesDiff = MBPP_FileInfoReader::GetFileInfoDifference(*ServerFileInfo.GetDirectoryInfo(CurrentDirectory), *DirectoryInfo.GetDirectoryInfo("/"));
                for (auto const& NewFiles : DirectoriesDiff.AddedFiles)
                {
                    FilesToSend.insert(CurrentDirectory+NewFiles);
                }
                for (auto const& UpdateFiles : DirectoriesDiff.UpdatedFiles)
                {
                    FilesToSend.insert(CurrentDirectory + UpdateFiles);
                }
                for (auto const& RemovedFiles : DirectoriesDiff.RemovedFiles)
                {
                    ObjectsToDelete.insert(CurrentDirectory + RemovedFiles);
                }
                for (auto const& DeletedDirectories : DirectoriesDiff.DeletedDirectories)
                {
                    ObjectsToDelete.insert(CurrentDirectory+DeletedDirectories);
                }
                for (auto const& NewDirectories : DirectoriesDiff.AddedDirectories)
                {
                    const MBPP_DirectoryInfoNode* NewDirectory = DirectoryInfo.GetDirectoryInfo(NewDirectories);
                    MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = NewDirectory->begin();
                    while (DirectoryIterator != NewDirectory->end())
                    {
                        FilesToSend.insert(CurrentDirectory + DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName);
                        DirectoryIterator++;
                    }
                }
            }
            else
            {
                //tar bara datan vi har
                const MBPP_DirectoryInfoNode* NewDirectory = DirectoryInfo.GetDirectoryInfo("/");
                MBPP_DirectoryInfoNode_ConstIterator DirectoryIterator = NewDirectory->begin();
                while (DirectoryIterator != NewDirectory->end())
                {
                    FilesToSend.insert(CurrentDirectory + DirectoryIterator.GetCurrentDirectory() + (*DirectoryIterator).FileName);
                    DirectoryIterator++;
                }
                        
            }
        }
        OutFilesToSend = {};
        OutFilesToDelete = {};
        for (auto const& Files : FilesToSend)
        {
            OutFilesToSend.push_back(Files);
        }
        for (auto const& Files : ObjectsToDelete)
        {
            OutFilesToDelete.push_back(Files);
        }
    }
    int MBPM_ClI::p_UploadPacketsLocal(std::vector<PacketIdentifier> const& PacketsToUpload,MBCLI::ProcessedCLInput const& Command,MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        std::string InstalledPacketsDirectory = GetSystemPacketsDirectory();
        std::vector<std::pair<std::string, std::string>> FailedPackets = {};
        bool UseDirectPath = false;
        for (size_t i = 0; i < PacketsToUpload.size(); i++)
        {
            std::string PacketInstallPath = InstalledPacketsDirectory + PacketsToUpload[i].PacketName + "/";
            if (std::filesystem::exists(PacketInstallPath) == false)
            {
                std::filesystem::create_directories(PacketInstallPath);
                MBPM::CreatePacketFilesData(PacketInstallPath);
            }
            //OBS antar h�r att packets inte �r up to date, vi kan om vi vill optimera g�ra det men blir relevant n�r det blir relevant
            MBPM::UpdateFileInfo(PacketInstallPath); // redundant med det ovan men men
            MBPM::UpdateFileInfo(PacketsToUpload[i].PacketURI);
            MBPM::MBPP_FileInfoReader InstalledInfo = MBPM::MBPP_FileInfoReader(PacketInstallPath+"/MBPM_FileInfo");
            MBPM::MBPP_FileInfoReader LocalPacketInfo = MBPM::MBPP_FileInfoReader(PacketsToUpload[i].PacketURI +"/MBPM_FileInfo");
            std::string LocalPacketPath = PacketsToUpload[i].PacketURI;

            MBPM::MBPP_FileInfoDiff FileInfoDiff;
            std::vector<std::string> ObjectsToCopy = {};
            std::vector<std::string> ObjectsToRemove = {};
            for (auto const& File : Command.GetSingleArgumentOptionList("f"))
            {
                UseDirectPath = true;
                if (File.first.size() == 0 || File.first[0] != '/') 
                {
                    FailedPackets.push_back(std::pair<std::string, std::string>(PacketsToUpload[i].PacketName, "Can't upload file \"" + File.first + "\": Invalid PacketPath"));
                }
                if (LocalPacketInfo.GetFileInfo(File.first) == nullptr)
                {
                    if (std::filesystem::exists(LocalPacketPath + File.first) && std::filesystem::is_regular_file(LocalPacketPath + File.first))
                    {
                        FileInfoDiff.AddedFiles.push_back(File.first);
                        continue;
                    }
                    FailedPackets.push_back(std::pair<std::string, std::string>(PacketsToUpload[i].PacketName, "Can't upload file \"" + File.first + "\": File not found"));
                    continue;
                }
                FileInfoDiff.AddedFiles.push_back(File.first);
            }
            for (auto const& Directory : Command.GetSingleArgumentOptionList("d"))
            {
                UseDirectPath = true;
                if (Directory.first.size() == 0 || Directory.first[0] != '/')
                {
                    FailedPackets.push_back(std::pair<std::string, std::string>(PacketsToUpload[i].PacketName, "Can't upload directory \"" + Directory.first + "\": Invalid PacketPath"));
                }
                if (LocalPacketInfo.GetDirectoryInfo(Directory.first) == nullptr)
                {
                    if (std::filesystem::exists(LocalPacketPath + Directory.first) && std::filesystem::is_directory(LocalPacketPath + Directory.first))
                    {
                        FileInfoDiff.AddedDirectories.push_back(Directory.first);
                        continue;
                    }
                    FailedPackets.push_back(std::pair<std::string, std::string>(PacketsToUpload[i].PacketName, "Can't upload directory \"" + Directory.first + "\": Directory not found"));
                    continue;
                }
                //FileInfoDiff.AddedFiles.push_back(File.first);
                //MBPP_FileInfoReader::GetFileInfoDifference(*InstalledInfo.GetDirectoryInfo(Directory.first),LocalPacketInfo.GetDirectoryInfo(Directory.first));
                if (InstalledInfo.GetDirectoryInfo(Directory.first) == nullptr)
                {
                    ObjectsToCopy.push_back(Directory.first);
                }
                else 
                {
                    MBPM::MBPP_FileInfoDiff DirectoryDifference = MBPM::MBPP_FileInfoReader::GetFileInfoDifference(*InstalledInfo.GetDirectoryInfo(Directory.first),*LocalPacketInfo.GetDirectoryInfo(Directory.first));
                    for (auto const& NewFile : DirectoryDifference.AddedFiles)
                        FileInfoDiff.AddedFiles.push_back(NewFile);
                    for (auto const& NewDirectory : DirectoryDifference.AddedDirectories)
                        FileInfoDiff.AddedDirectories.push_back(NewDirectory);
                    for (auto const& UpdatedFile : DirectoryDifference.UpdatedFiles)
                        FileInfoDiff.UpdatedFiles.push_back(UpdatedFile);
                    for (auto const& DeletedDirectory : DirectoryDifference.DeletedDirectories)
                        FileInfoDiff.DeletedDirectories.push_back(DeletedDirectory);
                    for (auto const& RemovedFiles : DirectoryDifference.RemovedFiles)
                        FileInfoDiff.RemovedFiles.push_back(RemovedFiles);
                }
            }
            if (!UseDirectPath)
            {
                FileInfoDiff = MBPM::GetFileInfoDifference(InstalledInfo, LocalPacketInfo);
            }
            AssociatedTerminal->PrintLine("Files to copy:");
            AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Green);
            for (size_t i = 0; i < FileInfoDiff.AddedFiles.size(); i++)
            {
                AssociatedTerminal->PrintLine(FileInfoDiff.AddedFiles[i]);
                ObjectsToCopy.push_back(FileInfoDiff.AddedFiles[i]);
            }
            for (size_t i = 0; i < FileInfoDiff.UpdatedFiles.size(); i++)
            {
                AssociatedTerminal->PrintLine(FileInfoDiff.UpdatedFiles[i]);
                ObjectsToCopy.push_back(FileInfoDiff.UpdatedFiles[i]);
            }
            AssociatedTerminal->PrintLine("Directories to copy:");
            for (size_t i = 0; i < FileInfoDiff.AddedDirectories.size(); i++)
            {
                AssociatedTerminal->PrintLine(FileInfoDiff.AddedDirectories[i]);
                //ObjectsToCopy.push_back(FileInfoDiff.AddedDirectories[i]);
                std::filesystem::path NewPath = PacketInstallPath+FileInfoDiff.AddedDirectories[i];
                std::filesystem::create_directories(NewPath);
                //actual files to copy
                if (!UseDirectPath)
                {
                    auto* LocalDirectory = LocalPacketInfo.GetDirectoryInfo(FileInfoDiff.AddedDirectories[i]);
                    auto Begin = LocalDirectory->begin();
                    auto End = LocalDirectory->end();
                    while (Begin != End)
                    {
                        ObjectsToCopy.push_back(FileInfoDiff.AddedDirectories[i] + Begin.GetCurrentDirectory() + Begin->FileName);
                        ++Begin;
                    }
                }
                else 
                {
                    ObjectsToCopy.push_back(FileInfoDiff.AddedDirectories[i]);
                }
            }
            AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
            AssociatedTerminal->PrintLine("Files to remove:");
            AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
            for (size_t i = 0; i < FileInfoDiff.RemovedFiles.size(); i++)
            {
                AssociatedTerminal->PrintLine(FileInfoDiff.RemovedFiles[i]);
                ObjectsToRemove.push_back(FileInfoDiff.RemovedFiles[i]);
            }
            AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
            AssociatedTerminal->PrintLine("Directories to remove:");
            AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::Red);
            for (size_t i = 0; i < FileInfoDiff.DeletedDirectories.size(); i++)
            {
                AssociatedTerminal->PrintLine(FileInfoDiff.DeletedDirectories[i]);
                ObjectsToRemove.push_back(FileInfoDiff.DeletedDirectories[i]);
            }
            AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
            AssociatedTerminal->PrintLine("Copying files:");
            std::filesystem::copy_options CopyOptions = std::filesystem::copy_options::overwrite_existing|std::filesystem::copy_options::recursive;
            for (size_t i = 0; i < ObjectsToCopy.size(); i++)
            {
                std::filesystem::path NewPath = PacketInstallPath+ObjectsToCopy[i];
                if(!std::filesystem::exists(NewPath.parent_path()))
                {
                    std::filesystem::create_directories(NewPath.parent_path());   
                }
                std::filesystem::copy(LocalPacketPath + ObjectsToCopy[i], PacketInstallPath + ObjectsToCopy[i], CopyOptions);
                AssociatedTerminal->PrintLine("Copied " + ObjectsToCopy[i]);
            }
            AssociatedTerminal->PrintLine("Removing objects:");
            for (size_t i = 0; i < ObjectsToRemove.size(); i++)
            {
                std::filesystem::remove_all(PacketInstallPath + ObjectsToRemove[i]);
                AssociatedTerminal->PrintLine("Removed " + ObjectsToRemove[i]);
            }
            MBPM::UpdateFileInfo(PacketInstallPath);
            AssociatedTerminal->PrintLine("Successfully uploaded packet " + PacketsToUpload[i].PacketURI + "!");
        }
        if (FailedPackets.size() > 0)
        {
            AssociatedTerminal->PrintLine("Failed uploading following packets:");
            ReturnValue = 1;
        }
        for (size_t i = 0; i < FailedPackets.size(); i++)
        {
            AssociatedTerminal->PrintLine(FailedPackets[i].first + ": " + FailedPackets[i].second);
        }
        AssociatedTerminal->SetTextColor(MBCLI::ANSITerminalColor::White);
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandleUpload(std::vector<PacketIdentifier> PacketsToUpload,MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        //f�rst s� beh�ver vi skicka data f�r att se login typen
        MBPM::MBPP_ComputerInfo PreviousComputerInfo = m_ClientToUse.GetComputerInfo();
        MBError ParseError = true;
        //kollar att alla packets har unika namn
        std::unordered_set<std::string> PacketNames = {};
        bool NameClashes = false;
        for (size_t i = 0; i < PacketsToUpload.size(); i++)
        {
            if((PacketsToUpload[i].Flags & PacketIdentifierFlag::SubPacket) != 0)
            {
                AssociatedTerminal->PrintLine("Error uploading packets: Can't upload subpackets individually");   
                return(1);
            }
            if (PacketNames.find(PacketsToUpload[i].PacketName) != PacketNames.end())
            {
                NameClashes = true;
                break;
            }
            PacketNames.insert(PacketsToUpload[i].PacketName);
        }
        if (NameClashes)
        {
            AssociatedTerminal->PrintLine("Error: you are trying to upload multiple packets with the same name");
            return 1;
        }
        if (CommandInput.CommandOptions.find("local") != CommandInput.CommandOptions.end())
        {
            return(p_UploadPacketsLocal(PacketsToUpload,CommandInput,AssociatedTerminal));
        }

        MBPP_UploadRequest_Response RequestResponse;
        if (CommandInput.CommandOptions.find("computerdiff") == CommandInput.CommandOptions.end())
        {
            m_ClientToUse.SetComputerInfo(MBPM::MBPP_ComputerInfo());
        }
        m_ClientToUse.Connect(p_GetDefaultPacketHost());
        std::string VerificationData = "";
        //std::set<std::string> FilesToSend = {};
        bool UseDirectPaths = false;
        if (CommandInput.GetSingleArgumentOptionList("d").size() != 0 || CommandInput.GetSingleArgumentOptionList("f").size() != 0)
        {
            UseDirectPaths = true;
        }
        MBError UploadError = true;
        MBPP_UserCredentialsType CurrentCredentialType = MBPP_UserCredentialsType::Plain;
        for (size_t i = 0; i < PacketsToUpload.size(); i++)
        {
            std::vector<std::string> FilesToUpload = {};
            std::vector<std::string> FilesToDelete = {};
            std::string PacketName = PacketsToUpload[i].PacketName;
            std::string PacketToUploadDirectory = PacketsToUpload[i].PacketURI;
            AssociatedTerminal->PrintLine("Uploading packet " + PacketsToUpload[i].PacketName);
            MBError VerificationResult = p_ExecuteEmptyCommand(PacketsToUpload[i],"verify",*AssociatedTerminal);
            if(!VerificationResult)
            {
                AssociatedTerminal->PrintLine("Error verifying packet for upload: "+VerificationResult.ErrorMessage);
                ReturnValue = 1;
                continue;
            }
            if (!UseDirectPaths)
            {
                MBPM::UpdateFileInfo(PacketToUploadDirectory);
            }
            else
            {
                MBPP_FileInfoReader ServerInfo = m_ClientToUse.GetPacketFileInfo(PacketName,&UploadError);
                if (!UploadError)
                {
                    AssociatedTerminal->PrintLine("Error downloading server file info for packet "+PacketName+" : " + UploadError.ErrorMessage);
                    return(1);
                }
                h_GetUpdateToSend(CommandInput, ServerInfo, PacketToUploadDirectory, FilesToUpload, FilesToDelete, AssociatedTerminal);
            }
            MBError UploadError = true;
            if (VerificationData == "")
            {
                if (!UseDirectPaths)
                {
                    UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Request, "", &RequestResponse);
                }
                else
                {
                    UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Request, "", FilesToUpload, FilesToDelete, &RequestResponse);
                }
            }
            else
            {
                if (!UseDirectPaths)
                {
                    UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, CurrentCredentialType, VerificationData, &RequestResponse);
                }
                else
                {
                    UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, CurrentCredentialType, VerificationData, FilesToUpload, FilesToDelete, &RequestResponse);
                }
                if (UploadError)
                {
                    AssociatedTerminal->PrintLine("Sucessfully uploaded " + PacketName);
                }
            }
            while (RequestResponse.Result != MBPP_ErrorCode::Ok || !UploadError)
            {
                AssociatedTerminal->PrintLine("Need verification for " + RequestResponse.DomainToLogin + ":");
                //Borde egentligen kolla vilka typer den st�djer
                AssociatedTerminal->Print("Username: ");
                std::string Username;
                AssociatedTerminal->GetLine(Username);
                std::string Password;
                AssociatedTerminal->Print("Password: ");
                AssociatedTerminal->SetPasswordInput(true);
                AssociatedTerminal->GetLine(Password);
                AssociatedTerminal->SetPasswordInput(false);
                VerificationData = MBPP_EncodeString(Username) + MBPP_EncodeString(Password);
                //m_ClientToUse.Connect(p_GetDefaultPacketHost());


                if (!UseDirectPaths)
                {
                    UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Plain, VerificationData, &RequestResponse);
                }
                else
                {
                    UploadError = m_ClientToUse.UploadPacket(PacketToUploadDirectory, PacketName, MBPP_UserCredentialsType::Plain, VerificationData, FilesToUpload, FilesToDelete, &RequestResponse);
                }
                
                if (!UploadError)
                {
                    AssociatedTerminal->PrintLine("Error uploading "+PacketName+": " + UploadError.ErrorMessage);
                    VerificationData = "";
                }
                else
                {
                    AssociatedTerminal->PrintLine("Sucessfully uploaded "+PacketName);
                }

            }
        }
        m_ClientToUse.SetComputerInfo(PreviousComputerInfo);
        return(ReturnValue);
        //else
        //{
        //    AssociatedTerminal->PrintLine("Upload packet sucessful");
        //}
    }
    MBError MBPM_ClI::p_ExecuteEmptyCommand(PacketIdentifier const& PacketToHandle,std::string const& CommandName,MBCLI::MBTerminal& TerminalToUse)
    {
        MBError ReturnValue = true;
        MBPM_PacketInfo CurrentPacketInfo = m_PacketRetriever.GetPacketInfo(PacketToHandle);
        const char* Argv[] = {"mbpm",CommandName.c_str()};
        MBCLI::ProcessedCLInput EmptyCommandInfo(2,Argv);
        std::pair<CLI_Extension*,CommandInfo> HandlingExtension = p_GetHandlingExtension(EmptyCommandInfo,CurrentPacketInfo);
        if(HandlingExtension.first != nullptr)
        {
            ReturnValue = HandlingExtension.first->HandleCommand(HandlingExtension.second,PacketToHandle,m_PacketRetriever,TerminalToUse);
        }
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandleCustomCommand(std::vector<PacketIdentifier> PacketList ,MBCLI::ProcessedCLInput const& CommandInput,MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        MBError PacketListError = true;
        size_t PacketOffset = 0;
        auto TypeIt = m_CustomCommandTypes.find(CommandInput.TopCommand);
        if(TypeIt == m_CustomCommandTypes.end())
        {
            AssociatedTerminal->PrintLine("Invalid command \""+CommandInput.TopCommand+"\"");   
            return(1);
        }
        if(TypeIt->second == CommandType::SubCommand)
        {
            PacketOffset = 1;   
        }
        bool QuitOnFirstError = CommandInput.CommandOptions.find("continue-error") == CommandInput.CommandOptions.end();
        bool QuitOnNoExtension = CommandInput.CommandOptions.find("quit-no-extension") != CommandInput.CommandOptions.end();
        bool CommandExecuted = false;
        std::vector<std::pair<PacketIdentifier,std::string>> FailedPackets;
        for(auto const& Packet : PacketList)
        {
            MBPM_PacketInfo CurrentPacketInfo = m_PacketRetriever.GetPacketInfo(Packet);
            std::pair<CLI_Extension*,CommandInfo> HandlingExtension = p_GetHandlingExtension(CommandInput,CurrentPacketInfo);
            if(HandlingExtension.first == nullptr)
            {
                if(QuitOnNoExtension)
                {
                    AssociatedTerminal->PrintLine("Error executing command on packet at location: "+Packet.PacketURI);
                    AssociatedTerminal->PrintLine("No extension found that handles packet of type \""+CurrentPacketInfo.Type+"\"");
                    return(1);
                }
                continue; 
            }
            CLI_Extension& ExtensionToUse = *(HandlingExtension.first);
            MBError Result = ExtensionToUse.HandleCommand(HandlingExtension.second,Packet,m_PacketRetriever,*AssociatedTerminal);
            if (!Result)
            {
                if(QuitOnFirstError)
                {
                    AssociatedTerminal->PrintLine("Error executing command for packet \""+Packet.PacketName+"\": "+Result.ErrorMessage);
                    return(1);
                }
                FailedPackets.push_back({Packet,Result.ErrorMessage});
            }
        }        
        for(auto const& FailedPacket : FailedPackets)
        {
            AssociatedTerminal->PrintLine("Error executing command for packet \""+FailedPacket.first.PacketName+"\": "+
                    FailedPacket.second);
        }
        if(FailedPackets.size() > 0)
        {
            return(1);   
        }
        return ReturnValue;
    }
    std::unordered_set<std::string> i_ReservedFlags = {"local","user","installed","remote","allinstalled"};
    std::unordered_set<std::string> i_ReservedOptions = {"l","i","r"};
    CommandInfo h_ConvertCLIInput(MBCLI::ProcessedCLInput const& InputToConvert,CustomCommand const& TargetCommand)
    {
        CommandInfo ReturnValue;    
        ReturnValue.CommandName = InputToConvert.TopCommand; 
        for(auto const& Flag : InputToConvert.CommandOptions)
        {
            if(i_ReservedFlags.find(Flag.first) == i_ReservedFlags.end())
            {
                ReturnValue.Flags.insert(Flag.first);
            }
        }
        for(auto const& Option : InputToConvert.CommandPositionalOptions)
        {
            std::string OptionData = Option.first;
            std::string OptionName = OptionData.substr(0, OptionData.find(':'));
            if(i_ReservedOptions.find(OptionName) != i_ReservedOptions.end())
            {
                continue;   
            }
            for(auto const& OptionValue : InputToConvert.GetSingleArgumentOptionList(OptionName))
            {
                ReturnValue.SingleValueOptions[OptionName].push_back(OptionValue.first);
            }
        }
        if(TargetCommand.Type == CommandType::SubCommand)
        {
            if(InputToConvert.TopCommandArguments.size() == 0)
            {
                throw std::runtime_error("subcommand not specified for command \""+InputToConvert.TopCommand+"\"");   
            }
            ReturnValue.Arguments.push_back(InputToConvert.TopCommandArguments[0]);
        }
        return(ReturnValue);
    }
    std::pair<CLI_Extension*,CommandInfo> MBPM_ClI::p_GetHandlingExtension(MBCLI::ProcessedCLInput const& CLIInput,MBPM_PacketInfo const& Info)
    {
        std::pair<CLI_Extension*,CommandInfo> ReturnValue;
        ReturnValue.first = nullptr;
        auto const& HookIt = m_TopCommandHooks.find(CLIInput.TopCommand);
        if(HookIt != m_TopCommandHooks.end())
        {
            //Verify condition   
            auto& RegisteredHooks = HookIt->second;
            for(auto const& Hook : RegisteredHooks)
            {
                for(auto const& SupportedType : Hook.first.SupportedTypes)
                {
                    if(SupportedType == "Any" || SupportedType == Info.Type)
                    {
                        ReturnValue.first = m_RegisteredExtensions[Hook.second].get();
                        ReturnValue.second =  h_ConvertCLIInput(CLIInput,Hook.first);
                        break;
                    }  
                } 
                if(ReturnValue.first != nullptr)
                {
                    break;   
                }
            }
        }
        return(ReturnValue);
    }
    void MBPM_ClI::p_RegisterExtension(std::unique_ptr<CLI_Extension,void (*)(void*)> ExtensionToRegister)
    {
        //std::string ExtensionName = ExtensionToRegister->GetName();
        CustomCommandInfo NewCommandInfo = ExtensionToRegister->GetCustomCommands(); 
        std::string ExtensionName = ExtensionToRegister->GetName();
        if (ExtensionName.find('/') != ExtensionName.npos || ExtensionName.find('\\') != ExtensionName.npos)
        {
            throw std::runtime_error("Error registering extension: name contains invalid characters");
        }
        for(auto const& Command : NewCommandInfo.Commands)
        {
            if(m_CustomCommandTypes.find(Command.Name) != m_CustomCommandTypes.end() && m_CustomCommandTypes[Command.Name] 
                    != Command.Type)
            {
                throw std::runtime_error("Extensions trying to register the same custom command with different type: "+Command.Name);   
            }
            m_CustomCommandTypes[Command.Name] = Command.Type;
            m_TopCommandHooks[Command.Name].push_back({Command,m_RegisteredExtensions.size()});
        }
        std::string ExtensionDataPath = MBPM::GetSystemPacketsDirectory() + "/Extensions/" + ExtensionName + "/";
        const char* Error = nullptr;
        ExtensionToRegister->SetConfigurationDirectory(ExtensionDataPath.c_str(),&Error);
        m_RegisteredExtensions.push_back(std::move(ExtensionToRegister));
    }

    int MBPM_ClI::p_HandlePackets(std::vector<PacketIdentifier> SpecifiedPackets,MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        bool PrintName = CommandInput.CommandOptions.find("name") != CommandInput.CommandOptions.end();
        bool PrintLocation = CommandInput.CommandOptions.find("location") != CommandInput.CommandOptions.end(); 
        bool PrintPath = CommandInput.CommandOptions.find("path") != CommandInput.CommandOptions.end();
        if(!PrintName && !PrintLocation && !PrintPath)
        {
            PrintName = true;   
            PrintLocation = true;   
            PrintPath = true;   
        }
        for (auto const& Packet : SpecifiedPackets)
        {
            if(PrintName)
            {
                AssociatedTerminal->Print(Packet.PacketName+ " ");   
            }
            if(PrintLocation)
            {
                if (Packet.PacketLocation == PacketLocationType::Remote)
                {
                    AssociatedTerminal->Print("Remote ");
                }
                else if (Packet.PacketLocation == PacketLocationType::Installed)
                {
                    AssociatedTerminal->Print("Installed ");
                }
                else if (Packet.PacketLocation == PacketLocationType::User)
                {
                    AssociatedTerminal->Print("User ");
                }
                else if (Packet.PacketLocation == PacketLocationType::Local)
                {
                    AssociatedTerminal->Print("Local ");
                }
            }
            if (PrintPath)
            {
                AssociatedTerminal->Print(Packet.PacketURI+" ");
            }
            AssociatedTerminal->PrintLine("");
        }
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandleIndex(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        if (CommandInput.TopCommandArguments.size() < 1)
        {
            AssociatedTerminal->PrintLine("Command \"info\" requires atleast 1 arguments");
            return 1;
        }
        bool QuitOnError = CommandInput.CommandOptions.find("continue-error") == CommandInput.CommandOptions.end();
        if (CommandInput.TopCommandArguments[0] == "create")
        {
            if (CommandInput.TopCommandArguments.size() < 2 && CommandInput.CommandOptions.find("alluser") == CommandInput.CommandOptions.end() && CommandInput.CommandOptions.find("allinstalled") == CommandInput.CommandOptions.end())
            {
                AssociatedTerminal->PrintLine("no directory path provided");
                return 1;
            }
            std::vector<std::string> DirectoriesToProcess = {};
            if (CommandInput.TopCommandArguments.size() >= 2)
            {
                DirectoriesToProcess.push_back(CommandInput.TopCommandArguments[1]+"/");
            }
            if (CommandInput.CommandOptions.find("alluser") != CommandInput.CommandOptions.end())
            {
                MBError Result = true;
                std::vector<PacketIdentifier> UserPackets = m_PacketRetriever.GetUserPackets(Result);
                if(!Result)
                {
                    AssociatedTerminal->PrintLine("Error retrieving user packets: "+Result.ErrorMessage);   
                    return(1);
                }
                for (size_t i = 0; i < UserPackets.size(); i++)
                {
                    DirectoriesToProcess.push_back(UserPackets[i].PacketURI);
                }
            }
            if (CommandInput.CommandOptions.find("allinstalled") != CommandInput.CommandOptions.end())
            {
                MBError Result = true;
                std::vector<PacketIdentifier> InstalledPackets = m_PacketRetriever.GetInstalledPackets(Result);
                if(!Result)
                {
                    AssociatedTerminal->PrintLine("Error retrieving installed packets: "+Result.ErrorMessage);    
                    return(1);
                }
                std::string PacketInstallDirectory = p_GetPacketInstallDirectory();
                for (size_t i = 0; i < InstalledPackets.size(); i++)
                {
                    DirectoriesToProcess.push_back(PacketInstallDirectory + "/" + InstalledPackets[i].PacketName);
                }
            }
            std::string FileInfoName = "MBPM_FileInfo";
            if (CommandInput.TopCommandArguments.size() >= 3)
            {
                FileInfoName = CommandInput.TopCommandArguments[2];
            }
            for (size_t i = 0; i < DirectoriesToProcess.size(); i++)
            {
                CreatePacketFilesData(DirectoriesToProcess[i], FileInfoName);
            }
        }
        else if (CommandInput.TopCommandArguments[0] == "update")
        {
            MBError GetPacketsError = true;
            std::vector<PacketIdentifier> Packets;
            GetPacketsError = p_GetCommandPackets(CommandInput, PacketLocationType::Local, Packets, 1);
            if (!GetPacketsError)
            {
                AssociatedTerminal->PrintLine("Error retrieving command packets: " + GetPacketsError.ErrorMessage);
                return 1;
            }
            std::vector<PacketIdentifier> FailedPackets;
            for (PacketIdentifier const& Packet : Packets)
            {
                if (Packet.PacketLocation == PacketLocationType::Remote)
                {
                    AssociatedTerminal->PrintLine("Can't update packets for remote index, skipping packet " + Packet.PacketName);
                    ReturnValue = 1;
                    continue;
                }
                UpdateFileInfo(Packet.PacketURI);
            }
        }
        else if (CommandInput.TopCommandArguments[0] == "list")
        {
            MBPM::MBPP_FileInfoReader InfoToPrint;
            if (CommandInput.TopCommandArguments.size() < 2)
            {
                AssociatedTerminal->PrintLine("No packet directory given");
                return 1;
            }
            if (CommandInput.CommandOptions.find("remote") != CommandInput.CommandOptions.end())
            {
                m_ClientToUse.Connect(p_GetDefaultPacketHost());
                MBError GetFileInfoResult = true;
                MBPM::MBPP_FileInfoReader ServerInfo = m_ClientToUse.GetPacketFileInfo(CommandInput.TopCommandArguments[1], &GetFileInfoResult);
                if (GetFileInfoResult)
                {
                    InfoToPrint = std::move(ServerInfo);
                }
                else
                {
                    AssociatedTerminal->PrintLine("Error connectin to remote: "+GetFileInfoResult.ErrorMessage);
                }
            }
            else if (CommandInput.CommandOptions.find("installed") != CommandInput.CommandOptions.end())
            {
                //std::string PacketDirectory = p_GetInstalledPacket(CommandInput.TopCommandArguments[1]).PacketURI;
                MBError Result = true;
                PacketIdentifier InstalledPacket = m_PacketRetriever.GetInstalledPacket(CommandInput.TopCommandArguments[1],Result);
                if(!Result)
                {
                    AssociatedTerminal->PrintLine("Error reading installed packet: "+Result.ErrorMessage);
                    return(1);
                }
                else
                {
                    std::string FilePath = InstalledPacket.PacketURI+"/" + "MBPM_FileInfo";
                    if (std::filesystem::exists(FilePath) && std::filesystem::directory_entry(FilePath).is_regular_file())
                    {
                        InfoToPrint = MBPM::MBPP_FileInfoReader(FilePath);
                    }
                    else
                    {
                        AssociatedTerminal->PrintLine("Couldn't find installed packet MBPM_FileInfo");//borde den implicit skapa den kanske?
                        return 1;
                    }
                }
            }
            else if (CommandInput.CommandOptions.find("user") != CommandInput.CommandOptions.end())
            {
                MBError Result = true;
                //PacketIdentifier UserPacket = p_GetUserPacket(CommandInput.TopCommandArguments[1]);
                PacketIdentifier UserPacket = m_PacketRetriever.GetUserPacket(CommandInput.TopCommandArguments[1],Result);
                if(!Result)
                {
                    AssociatedTerminal->PrintLine("Error reading user packet: "+Result.ErrorMessage);
                    return(1);
                }
                if (UserPacket.PacketName != "")
                {
                    InfoToPrint = p_GetPacketFileInfo(UserPacket,nullptr);
                }
                else
                {
                    AssociatedTerminal->PrintLine("Couldn't find User packet");
                    return 1;
                }
            }
            else
            {
                //vanlig, ta directoryn
                if (std::filesystem::exists(CommandInput.TopCommandArguments[1]) && std::filesystem::directory_entry(CommandInput.TopCommandArguments[1]).is_regular_file())
                {
                    InfoToPrint = MBPM::MBPP_FileInfoReader((CommandInput.TopCommandArguments[1]));
                }
                else
                {
                    std::string FolderPath = CommandInput.TopCommandArguments[1];
                    if (FolderPath.back() != '/')
                    {
                        FolderPath += '/';
                    }
                    std::string FilePath = FolderPath + "MBPM_FileInfo";
                    if (std::filesystem::exists(FilePath) && std::filesystem::directory_entry(FilePath).is_regular_file())
                    {
                        InfoToPrint = MBPM::MBPP_FileInfoReader(FilePath);
                    }
                    else
                    {
                        AssociatedTerminal->PrintLine("Couldn't find local MBPM_FileInfo file");
                        return 1;
                    }
                }
            }
            std::vector<std::string> ObjectsToPrint = {};
            for (size_t i = 2; i < CommandInput.TopCommandArguments.size(); i++)
            {
                ObjectsToPrint.push_back(CommandInput.TopCommandArguments[i]);
            }
            PrintFileInfo(InfoToPrint,ObjectsToPrint,AssociatedTerminal);
        }
        else if (CommandInput.TopCommandArguments[0] == "diff")
        {
            MBError GetFileInfoError = true;
            std::vector<PacketIdentifier> PacketsToDiff;
            GetFileInfoError = p_GetCommandPackets(CommandInput,PacketLocationType::Installed,PacketsToDiff,1);
            if(PacketsToDiff.size() > 0)
            {
                AssociatedTerminal->PrintLine("To many packets in packet specification: only need client and server packet");    
            }
            MBPM::MBPP_FileInfoReader ClientFileInfo = p_GetPacketFileInfo(PacketsToDiff[0],&GetFileInfoError);
            if (!GetFileInfoError)
            {
                AssociatedTerminal->PrintLine("Error retrieving FileInfo: "+GetFileInfoError.ErrorMessage);
                return 1;
            }
            MBPM::MBPP_FileInfoReader ServerFileInfo = p_GetPacketFileInfo(PacketsToDiff[1],&GetFileInfoError);
            if (!GetFileInfoError)
            {
                AssociatedTerminal->PrintLine("Error retrieving FileInfo: " + GetFileInfoError.ErrorMessage);
                return 1;
            }
            MBPM::MBPP_FileInfoDiff InfoDiff = MBPM::GetFileInfoDifference(ClientFileInfo, ServerFileInfo);
            PrintFileInfoDiff(InfoDiff,AssociatedTerminal);
        }
        return(ReturnValue);
    }
    int MBPM_ClI::p_HandleCommands(MBCLI::ProcessedCLInput const& CommandInput,MBCLI::MBTerminal& AssociatedTerminal)
    {
        int ReturnValue = 0; 
        //lists all builtin commands, aswell as all user defined commands
        std::vector<std::string> StringToPrint = {"update","install","index","upload","get","packets"};
        for(auto const& Hook : m_CustomCommandTypes)
        {
            StringToPrint.push_back(Hook.first);
        }
        for(std::string const& StringToPrint : StringToPrint)
        {
            if(StringToPrint == "")
            {
                continue; 
            }
            AssociatedTerminal.PrintLine(StringToPrint);
        }
        return(ReturnValue);
    }
    std::string h_ReadMBPMStaticData(std::string const& Filepath)
    {
        std::string ReturnValue = "";
        size_t FileSize = std::filesystem::file_size(Filepath);
        std::ifstream FileToRead = std::ifstream(Filepath, std::ios::in|std::ios::binary);
        std::string Filedata = std::string(FileSize, 0);
        FileToRead.read(Filedata.data(), FileSize);
        Filedata = MBUtility::ReplaceAll(Filedata, "\r\n", "\n");
        std::string BeginDelimiter = "R\"1337xd(";
        size_t BeginOffset = Filedata.find(BeginDelimiter);
        if (BeginOffset != Filedata.npos)
        {
            BeginOffset += BeginDelimiter.size();
            size_t EndOffset = Filedata.find(")1337xd\"");
            if (EndOffset == Filedata.npos)
            {
                EndOffset = FileSize;
            }
            ReturnValue = Filedata.substr(BeginOffset, EndOffset - BeginOffset)+"\n";
        }
        else
        {
            ReturnValue = std::move(Filedata);
        }
        return(ReturnValue);
    }
    template <typename T>
    void __Delete(void* PointerToDelete)
    {
        delete (T*)PointerToDelete;
    }
    int MBPM_ClI::HandleCommand(MBCLI::ProcessedCLInput const& CommandInput, MBCLI::MBTerminal* AssociatedTerminal)
    {
        int ReturnValue = 0;
        //denna �r en global inst�llning
        //Extensions
        std::unique_ptr<CLI_Extension, void (*)(void*)> BuildExtension = std::unique_ptr<CLI_Extension, void (*)(void*)>(new MBBuild::MBBuild_Extension(),
            __Delete<MBBuild::MBBuild_Extension>);
        std::unique_ptr<CLI_Extension, void (*)(void*)> VimExtension = std::unique_ptr<MBPM_Vim, void (*)(void*)>(new MBPM_Vim(),
            __Delete<MBPM_Vim>);
        std::unique_ptr<CLI_Extension, void (*)(void*)> BashExtension = std::unique_ptr<MBPM_Bash, void (*)(void*)>(new MBPM_Bash(),
            __Delete<MBPM_Bash>);
        //
        p_RegisterExtension(std::move(BuildExtension));
        p_RegisterExtension(std::move(VimExtension));
        p_RegisterExtension(std::move(BashExtension));

        if (CommandInput.CommandOptions.find("computerdiff") != CommandInput.CommandOptions.end())
        {
            m_ClientToUse.SetComputerInfo(MBPM::MBPP_Client::GetSystemComputerInfo());
        }
        if (CommandInput.CommandOptions.find("nocomputerdiff") != CommandInput.CommandOptions.end())
        {
            m_ClientToUse.SetComputerInfo(MBPM::MBPP_ComputerInfo());
        }
        m_ClientToUse.SetLogTerminal(AssociatedTerminal);
        //Builtins
        if (CommandInput.CommandOptions.find("help") != CommandInput.CommandOptions.end())
        {
            //ReturnValue = p_HandleHelp(CommandInput,AssociatedTerminal);
        }
        else if (CommandInput.TopCommand == "install")
        {
            return(p_HandleInstall(CommandInput,AssociatedTerminal));
        }
        else if(CommandInput.TopCommand == "get")
        {
            return( p_HandleGet(CommandInput, AssociatedTerminal));
        }
        else if(CommandInput.TopCommand == "commands")
        {
            return(p_HandleCommands(CommandInput, *AssociatedTerminal));
        }
        else if (CommandInput.TopCommand == "index")
        {
            return(p_HandleIndex(CommandInput, AssociatedTerminal));
        }
        
        MBCLI::ProcessedCLInput VectorizedCommandInput = CommandInput; 
        std::vector<PacketIdentifier> VectorizedPackets;
        
        if(CommandInput.TopCommand == "packspec")
        {
            ReturnValue = p_HandlePackSpec(CommandInput, VectorizedCommandInput,VectorizedPackets,*AssociatedTerminal);
            if(ReturnValue != 0)
            {
                return(ReturnValue);   
            }
        }
        MBError RetrievePacketResult = true;
        RetrievePacketResult = p_GetCommandPackets(VectorizedCommandInput,PacketLocationType::Installed,VectorizedPackets);
        if(!RetrievePacketResult)
        {
            AssociatedTerminal->PrintLine("Error evaluating packet specification: "+RetrievePacketResult.ErrorMessage);   
            return(1);
        }
        if (VectorizedCommandInput.TopCommand == "update")
        {
            ReturnValue = p_HandleUpdate(VectorizedPackets,VectorizedCommandInput, AssociatedTerminal);
        }
        else if(VectorizedCommandInput.TopCommand == "exec")
        {
            ReturnValue = p_HandleExec(VectorizedPackets,VectorizedCommandInput,*AssociatedTerminal);   
        }
        else if (VectorizedCommandInput.TopCommand == "upload")
        {
            ReturnValue = p_HandleUpload(VectorizedPackets,VectorizedCommandInput, AssociatedTerminal);
        }
        else if (VectorizedCommandInput.TopCommand == "packets")
        {
            ReturnValue = p_HandlePackets(VectorizedPackets,VectorizedCommandInput, AssociatedTerminal);
        }
        else
        {
            ReturnValue = p_HandleCustomCommand(VectorizedPackets,VectorizedCommandInput,AssociatedTerminal);
        }
        return(ReturnValue);
    }
    //END MBPM_CLI
    int MBCLI_Main(int argc,const char** argv)
    {
        try
        {
            MBSockets::Init();
            MBCLI::ProcessedCLInput CommandInput(argc, argv);
            MBCLI::MBTerminal ProgramTerminal;
            MBPM_ClI CLIHandler;
            return(CLIHandler.HandleCommand(CommandInput, &ProgramTerminal));
        }
        catch(std::exception const& e)
        {
            std::cout << "Uncaught error in executing command: " << e.what() << std::endl;
        }
        return(0);
    }
}
