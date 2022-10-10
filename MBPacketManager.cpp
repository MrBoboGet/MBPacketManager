#include "MBPacketManager.h"
#include <MBParsing/MBParsing.h>
#include <MBUnicode/MBUnicode.h>
#include <MBUtility/MBErrorHandling.h>
#include <assert.h>
#include <exception>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <MBUtility/MBCompileDefinitions.h>
#include <MBUtility/MBFiles.h>

//magiska namn macros
#define MBPM_PACKETINFO_FILENAME "MBPM_PacketInfo";

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else
#include <stdlib.h>
#endif // 
namespace MBPM
{
	std::string GetSystemPacketsDirectory()
	{
		char* Result = std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY");
		if (Result == nullptr)
		{
			throw std::runtime_error("MBPM_PACKETS_INSTALL_DIRECTORY environment variable not set");
            assert(false && "MBPM_PACKTETS_INSTALL_DIRECTORY not set");
			return("");
		}
		else
		{
			return(std::string(Result)+"/");
		}
	}
	MBPM_PacketInfo ParseMBPM_PacketInfo(std::string const& PacketPath)
	{
		std::string JsonData = std::string(std::filesystem::file_size(PacketPath), 0);
		std::ifstream PacketFile = std::ifstream(PacketPath, std::ios::in | std::ios::binary);
		PacketFile.read(JsonData.data(), JsonData.size());
		MBError ParseError = true;
		MBParsing::JSONObject ParsedJson = MBParsing::ParseJSONObject(JsonData,0,nullptr,&ParseError);
		MBPM_PacketInfo ReturnValue = MBPM_PacketInfo();
		try
		{
			if (!ParseError)
            {
                return(ReturnValue);
            }
			ReturnValue.PacketName = ParsedJson.GetAttribute("PacketName").GetStringData();
			for (auto const& Attribute : ParsedJson.GetAttribute("Attributes").GetArrayData())
			{
                ReturnValue.Attributes.insert(Attribute.GetStringData());
			}
			//ExportedExecutableTargets
			if (ParsedJson.HasAttribute("PacketType"))
			{
                ReturnValue.Type = ParsedJson["PacketType"].GetStringData();
			}
			else
			{
                ReturnValue.Type = "C++";
			}
			//ReturnValue.PacketDependancies = ParsedAttributes["Dependancies"];
			for (auto const& Dependancie : ParsedJson.GetAttribute("Dependancies").GetArrayData())
			{
				ReturnValue.PacketDependancies.push_back(Dependancie.GetStringData());
			}
            if(ParsedJson.HasAttribute("TypeInfo"))
            {
                ReturnValue.TypeInfo = std::move(ParsedJson["TypeInfo"]);
                //std::cout<<"Type info !"<<std::endl<<ReturnValue.TypeInfo.ToPrettyString()<<std::endl;
            }
            else
            {
                ReturnValue.TypeInfo = MBParsing::JSONObject(MBParsing::JSONObjectType::Aggregate);   
            }
		}
		catch(std::exception const& e)
		{
			ReturnValue = MBPM_PacketInfo();
		}
		return(ReturnValue);
	}
    
	std::vector<std::string> h_GetTotalPacketDependancies(MBPM_PacketInfo const& PacketInfo, MBError* OutError)
	{
		std::vector<std::string> ReturnValue = {};
		std::unordered_set<std::string> VisitedPackets = {};
		std::stack<std::string> DependanciesToTraverse = {};
		std::string PacketsDirectory = GetSystemPacketsDirectory();
		for (size_t i = 0; i < PacketInfo.PacketDependancies.size(); i++)
		{
			DependanciesToTraverse.push(PacketInfo.PacketDependancies[i]);
		}
		while (DependanciesToTraverse.size() > 0)
		{
			std::string CurrentPacket = DependanciesToTraverse.top();
			DependanciesToTraverse.pop();
			if (VisitedPackets.find(CurrentPacket) == VisitedPackets.end())
			{
				VisitedPackets.insert(CurrentPacket);
				if (std::filesystem::exists(PacketsDirectory + "/" + CurrentPacket + "/MBPM_PacketInfo"))
				{
					MBPM_PacketInfo DependancyPacket = ParseMBPM_PacketInfo(PacketsDirectory + "/" + CurrentPacket + "/MBPM_PacketInfo");
					if (DependancyPacket.PacketName != "")
					{
						for (size_t i = 0; i < DependancyPacket.PacketDependancies.size(); i++)
						{
							if (VisitedPackets.find(DependancyPacket.PacketDependancies[i]) == VisitedPackets.end())
							{
								DependanciesToTraverse.push(DependancyPacket.PacketDependancies[i]);
							}
						}
					}
					else
					{
						*OutError = false;
						OutError->ErrorMessage = "Missing dependancy MBPM_PacketInfo \"" + CurrentPacket + "\"";
						return(ReturnValue);
					}
				}
				else
				{
					*OutError = false;
					OutError->ErrorMessage = "Missing dependancy \"" + CurrentPacket + "\"";
					return(ReturnValue);
				}
			}
		}
		return(ReturnValue);
	}
	std::vector<MBPM_PacketInfo> h_GetDependanciesInfo(std::vector<std::string> const& DependanciesToCheck)
	{
		std::vector<MBPM_PacketInfo> ReturnValue;
		std::string InstallDirectory = GetSystemPacketsDirectory();
		for (size_t i = 0; i < DependanciesToCheck.size(); i++)
		{
			ReturnValue.push_back(ParseMBPM_PacketInfo(InstallDirectory + "/" + DependanciesToCheck[i] + "/MBPM_PacketInfo"));
		}
		return(ReturnValue);
	}
    //BEGIN PacketRetriever
    PacketIdentifier PacketRetriever::p_GetInstalledPacket(std::string const& PacketName)
    {
        PacketIdentifier ReturnValue;
        ReturnValue.PacketName = PacketName;
        ReturnValue.PacketLocation = PacketLocationType::Installed;
        ReturnValue.PacketURI = GetSystemPacketsDirectory();
        if (ReturnValue.PacketURI.back() != '/')
        {
            ReturnValue.PacketURI += "/";
        }
        if (std::filesystem::exists(ReturnValue.PacketURI + PacketName) && std::filesystem::exists(ReturnValue.PacketURI + PacketName + "/MBPM_PacketInfo"))
        {
            ReturnValue.PacketURI = ReturnValue.PacketURI + PacketName + "/";
        }
        else
        {
            ReturnValue = PacketIdentifier();
        }
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::p_GetUserPackets()
    {
        std::vector<PacketIdentifier> ReturnValue = {};
        std::set<std::string> SearchDirectories = {};
        std::string PacketInstallDirectory = GetSystemPacketsDirectory();
        //l�ser in config filen
        if (std::filesystem::exists(PacketInstallDirectory + "/LocalUploadPackets.txt"))
        {
            std::ifstream UploadPacketsFile = std::ifstream(PacketInstallDirectory + "/LocalUploadPackets.txt",std::ios::in);
            std::string CurrentLine = "";
            while (std::getline(UploadPacketsFile,CurrentLine))
            {
                SearchDirectories.insert(CurrentLine);
            }
        }
        for (std::string const& Directories : SearchDirectories)
        {
            std::filesystem::directory_iterator CurrentDirectoryIterator = std::filesystem::directory_iterator(Directories);
            for (auto const& Entries : CurrentDirectoryIterator)
            {
                if (Entries.is_directory())
                {
                    std::string EntryPath = MBUnicode::PathToUTF8(Entries.path())+"/";
                    if (std::filesystem::exists(EntryPath+"/MBPM_PacketInfo"))
                    {
                        MBPM::MBPM_PacketInfo CurrentPacket = MBPM::ParseMBPM_PacketInfo(EntryPath + "/MBPM_PacketInfo");
                        if (CurrentPacket.PacketName != "")
                        {
                            PacketIdentifier NewPacket;
                            NewPacket.PacketName = CurrentPacket.PacketName;
                            NewPacket.PacketURI = EntryPath;
                            NewPacket.PacketLocation = PacketLocationType::User;
                            ReturnValue.push_back(NewPacket);
                        }
                    }
                }
            }
        }

        return(ReturnValue);
    }
    MBPM::MBPM_PacketInfo PacketRetriever::p_GetPacketInfo(PacketIdentifier const& PacketToInspect, MBError* OutError)
	{
		MBPM::MBPM_PacketInfo ReturnValue;
		MBError Error = true;
		if (PacketToInspect.PacketLocation == PacketLocationType::Installed)
		{
			std::string CurrentURI = PacketToInspect.PacketURI;
			if (CurrentURI == "")
			{
				CurrentURI = GetSystemPacketsDirectory() + "/" + PacketToInspect.PacketName+"/";
			}
			if (std::filesystem::exists(CurrentURI + "/MBPM_PacketInfo") && std::filesystem::is_regular_file(CurrentURI + "/MBPM_PacketInfo"))
			{
				ReturnValue = MBPM::ParseMBPM_PacketInfo(CurrentURI + "/MBPM_PacketInfo");
			}
			else
			{
				Error = false;
				Error.ErrorMessage = "Can't find user packet";
			}
		}
		else if (PacketToInspect.PacketLocation == PacketLocationType::Remote)
		{
			Error = false;
			//TODO att implementera
			Error.ErrorMessage = "Packet info of remote packet not implemented yet xd";
		}
		else if (PacketToInspect.PacketLocation == PacketLocationType::Local)
		{
			if (std::filesystem::exists(PacketToInspect.PacketURI + "/MBPM_PacketInfo") && std::filesystem::is_regular_file(PacketToInspect.PacketURI + "/MBPM_PacketInfo"))
			{
				ReturnValue = MBPM::ParseMBPM_PacketInfo(PacketToInspect.PacketURI + "/MBPM_PacketInfo");
			}
			else
			{
				Error = false;
				Error.ErrorMessage = "Can't find user packet";
			}
		}
		else if (PacketToInspect.PacketLocation == PacketLocationType::User)
		{
			if (std::filesystem::exists(PacketToInspect.PacketURI + "/MBPM_PacketInfo") && std::filesystem::is_regular_file(PacketToInspect.PacketURI + "/MBPM_PacketInfo"))
			{
				ReturnValue = MBPM::ParseMBPM_PacketInfo(PacketToInspect.PacketURI + "/MBPM_PacketInfo");
			}
			else
			{
				Error = false;
				Error.ErrorMessage = "Can't find user packet";
			}
		}
        if(ReturnValue.PacketName == "")
        {
            Error = false;
            Error.ErrorMessage = "Failed parsing packet "+PacketToInspect.PacketName;
        }
        if (!Error && OutError != nullptr)
		{
			*OutError = std::move(Error);
		}
        if(!Error)
        {
            ReturnValue = MBPM::MBPM_PacketInfo();   
        }
		return(ReturnValue);
	}
    uint32_t h_GetPacketDepth2(std::map<std::string, MBPM_PacketDependancyRankInfo>& PacketInfoToUpdate, std::string const& PacketName, std::vector<std::string>* OutDependancies)
    {
        if (PacketInfoToUpdate.find(PacketName) == PacketInfoToUpdate.end())
        {
            OutDependancies->push_back(PacketName);
            return(0);
        }
        MBPM_PacketDependancyRankInfo& PacketToEvaluate = PacketInfoToUpdate[PacketName];
        if (PacketToEvaluate.DependancyDepth != -1)
        {
            return(PacketToEvaluate.DependancyDepth);
        }
        else
        {
            std::vector<std::string>& PacketDependancies = PacketToEvaluate.PacketDependancies;
            if (PacketDependancies.size() == 0)
            {
                PacketToEvaluate.DependancyDepth = 0;
                return(0);
            }
            else
            {
                uint32_t DeepestDepth = 0;
                for (size_t i = 0; i < PacketDependancies.size(); i++)
                {
                    uint32_t NewDepth = h_GetPacketDepth2(PacketInfoToUpdate, PacketDependancies[i], OutDependancies) + 1;
                    if (NewDepth > DeepestDepth)
                    {
                        DeepestDepth = NewDepth;
                    }
                }
                PacketToEvaluate.DependancyDepth = DeepestDepth;
                return(DeepestDepth);
            }
        }
    }
    std::map<std::string, MBPM_PacketDependancyRankInfo> PacketRetriever::p_GetPacketDependancieInfo(
            std::vector<PacketIdentifier> const& InPacketsToCheck,
            MBError& OutError,
            std::vector<std::string>* OutMissing)
    {
        std::map<std::string, MBPM_PacketDependancyRankInfo> ReturnValue;
        std::vector<PacketIdentifier> PacketsToCheck = InPacketsToCheck;
        std::set<std::string> TotalPacketNames;
        while (PacketsToCheck.size() > 0)
        {
            PacketIdentifier NewPacket = PacketsToCheck.back();
            PacketsToCheck.pop_back();
            if (ReturnValue.find(NewPacket.PacketName) != ReturnValue.end())
            {
                continue;
            }
            MBPM_PacketInfo PacketInfo = p_GetPacketInfo(NewPacket, &OutError);
            //assert(PacketInfo.PacketName != "");//contract by p_GetPacketInfo
            if (!OutError)
            {
                return(ReturnValue);
            }
            //TotalPacketNames.push_back(PacketInfo.PacketName);
            TotalPacketNames.insert(PacketInfo.PacketName);
            MBPM_PacketDependancyRankInfo NewDependancyInfo;
            NewDependancyInfo.PacketName = PacketInfo.PacketName;
            NewDependancyInfo.PacketDependancies = PacketInfo.PacketDependancies;
            for (auto const& Dependancy : NewDependancyInfo.PacketDependancies)
            {
                if (TotalPacketNames.find(Dependancy) == TotalPacketNames.end())
                {
                    PacketIdentifier NewPacketToCheck;
                    NewPacketToCheck = p_GetInstalledPacket(Dependancy);
                    if (NewPacketToCheck.PacketName == "")
                    {
                        OutMissing->push_back(Dependancy);
                    }
                    else
                    {
                        TotalPacketNames.insert(Dependancy);
                        PacketsToCheck.push_back(std::move(NewPacketToCheck));
                    }
                }
            }
            ReturnValue[NewDependancyInfo.PacketName] = std::move(NewDependancyInfo);
        }
        for (auto const& Name : TotalPacketNames)
        {
			h_GetPacketDepth2(ReturnValue, Name, OutMissing);
        }
        if (OutMissing->size() > 0)
        {
            OutError = false;
            OutError.ErrorMessage = "Missing dependancies: ";
			for (auto const& String : *OutMissing)
			{
				OutError.ErrorMessage += String + " ";
			}
        }
        return(ReturnValue);
    }

    std::vector<PacketIdentifier> PacketRetriever::p_GetPacketDependancies_DependancyOrder(std::vector<PacketIdentifier> const& InPacketsToCheck,MBError& OutError, std::vector<std::string>* MissingPackets,bool IncludeInitial)
    {
        std::vector<PacketIdentifier> ReturnValue;

        std::map<std::string, MBPM_PacketDependancyRankInfo> PacketDependancyInfo = p_GetPacketDependancieInfo(InPacketsToCheck, OutError, MissingPackets);
        if(!OutError)
        {
            return(ReturnValue);
        }
        std::set<MBPM_PacketDependancyRankInfo> OrderedPackets;
        for (auto const& Packet : PacketDependancyInfo)
        {
            OrderedPackets.insert(Packet.second);
        }
        for (auto const& Packet : OrderedPackets)
        {
            bool Continue = false;
            PacketIdentifier NewIdentifier = p_GetInstalledPacket(Packet.PacketName);
            if(!IncludeInitial)
            {
                for (size_t i = 0; i < InPacketsToCheck.size(); i++)
                {
                    if (Packet.PacketName == InPacketsToCheck[i].PacketName)
                    {
                        Continue = true;
                        break;
                    }
                }
                if (Continue)
                {
                    continue;
                }
            }
            ReturnValue.push_back(NewIdentifier);
        }
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::p_GetPacketDependants_DependancyOrder(std::vector<PacketIdentifier> const& InPacketsToCheck,MBError& OutError,bool IncludeInitial)
    {
        std::vector<PacketIdentifier> ReturnValue;
        std::vector<PacketIdentifier> TotalDependants = {};
        std::vector<PacketIdentifier> InstalledPackets = GetInstalledPackets(OutError);
        if(!OutError)
        {
            return(ReturnValue);
        }
        std::set<std::string> Dependants;
        for (size_t i = 0; i < InPacketsToCheck.size(); i++)
        {
            Dependants.insert(InPacketsToCheck[i].PacketName);
        }
        for (auto const& Packet : InstalledPackets)
        {
            MBPM_PacketInfo CurrentPacketInfo = p_GetPacketInfo(Packet, &OutError);
            if (!OutError)
            {
                return(ReturnValue);
            }
            for (auto const& Dependancy : CurrentPacketInfo.PacketDependancies)
            {
                if (Dependants.find(Dependancy) != Dependants.end())
                {
                    TotalDependants.push_back(Packet);
                    Dependants.insert(Packet.PacketName);
                    break;
                }
            }
        }
        std::vector<std::string> Missing;
        std::map<std::string, MBPM_PacketDependancyRankInfo> PacketDependancyInfo = p_GetPacketDependancieInfo(TotalDependants, OutError,&Missing);
        if (!OutError)
        {
            return(ReturnValue);
        }
        //TODO ineffiecient
        std::set<MBPM_PacketDependancyRankInfo> DependantsOrder;
        for (auto const& Packet : TotalDependants)
        {
            DependantsOrder.insert(PacketDependancyInfo[Packet.PacketName]);
        }
        if(IncludeInitial)
        {
            for(auto const& Packet : InPacketsToCheck)
            {
                DependantsOrder.insert(PacketDependancyInfo[Packet.PacketName]);
            }   
        }
        for (auto const& Packet : DependantsOrder)
        {
            ReturnValue.push_back(p_GetInstalledPacket(Packet.PacketName));
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::p_GetUserPacket(std::string const& PacketName)
    {
        PacketIdentifier ReturnValue;
        std::vector<PacketIdentifier> AllUserPackets = p_GetUserPackets();
        for (size_t i = 0; i < AllUserPackets.size(); i++)
        {
            if (AllUserPackets[i].PacketName == PacketName)
            {
                ReturnValue = AllUserPackets[i];
            }
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::p_GetLocalPacket(std::string const& PacketPath)
    {
        PacketIdentifier ReturnValue;
        if (std::filesystem::exists(PacketPath + "/MBPM_PacketInfo"))
        {
            MBPM::MBPM_PacketInfo PacketInfo = MBPM::ParseMBPM_PacketInfo(PacketPath+"/MBPM_PacketInfo");
            if (PacketInfo.PacketName != "")
            {
                ReturnValue.PacketName = PacketInfo.PacketName;
                ReturnValue.PacketURI = PacketPath;
                ReturnValue.PacketLocation = PacketLocationType::Local;
            }
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::p_GetRemotePacketIdentifier(std::string const& PacketName)
    {
        PacketIdentifier ReturnValue;
        ReturnValue.PacketLocation = PacketLocationType::Remote;
        ReturnValue.PacketName = PacketName;
        ReturnValue.PacketURI = "default";
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::GetInstalledPacket(std::string const& PacketName,MBError& OutError)
    {
        PacketIdentifier ReturnValue; 
        ReturnValue = p_GetInstalledPacket(PacketName);
        if(ReturnValue.PacketName == "")
        {
            OutError = false;
            OutError.ErrorMessage = "Can't find installed packet \""+PacketName+"\"";   
        }
        return(ReturnValue);
    }
    PacketIdentifier PacketRetriever::GetUserPacket(std::string const& PacketName,MBError&  OutError)
    {
        PacketIdentifier ReturnValue; 
        ReturnValue = p_GetUserPacket(PacketName);
        if(ReturnValue.PacketName == "")
        {
            OutError = false;
            OutError.ErrorMessage = "Can't find user packet \""+PacketName+"\"";   
        }
        return(ReturnValue);
        return(p_GetUserPacket(PacketName)); 
    }
    PacketIdentifier PacketRetriever::GetLocalpacket(std::string const& path,MBError& OutError)
    {
        PacketIdentifier ReturnValue; 
        ReturnValue = p_GetLocalPacket(path);
        if(ReturnValue.PacketName == "")
        {
            OutError = false;
            OutError.ErrorMessage = "invalid local packet";
        }
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::GetInstalledPackets(MBError& OutError)
    {
       	std::map<std::string, MBPM_PacketDependancyRankInfo> Packets = {};
        std::vector<std::string> InstalledPackets = {};
        std::filesystem::directory_iterator PacketDirectoryIterator = std::filesystem::directory_iterator(GetSystemPacketsDirectory());
        for (auto const& Entries : PacketDirectoryIterator)
        {
            if (Entries.is_directory())
            {
                std::string InfoPath = MBUnicode::PathToUTF8(Entries.path()) + "/MBPM_PacketInfo";
                if (std::filesystem::exists(InfoPath))
                {
                    MBPM::MBPM_PacketInfo PacketInfo = MBPM::ParseMBPM_PacketInfo(InfoPath);
                    if (PacketInfo.PacketName == "") //kanske ocks� borde asserta att packetens folder r samma som dess namn
                    {
                        continue;
                    }
                    MBPM_PacketDependancyRankInfo InfoToAdd;
                    InfoToAdd.PacketName = PacketInfo.PacketName;
                    InfoToAdd.PacketDependancies = PacketInfo.PacketDependancies;
                    InstalledPackets.push_back(InfoToAdd.PacketName);
                    Packets[PacketInfo.PacketName] = InfoToAdd;
                }
            }
        }
        std::set<MBPM_PacketDependancyRankInfo> OrderedPackets = {};
		std::vector<std::string> MissingPackets;
        for (auto const& Packet : InstalledPackets)
        {
            Packets[Packet].DependancyDepth = h_GetPacketDepth2(Packets, Packet, &MissingPackets);
            OrderedPackets.insert(Packets[Packet]);
        }
        std::vector<PacketIdentifier> ReturnValue = {};
        for (auto const& Packet : OrderedPackets)
        {
            PacketIdentifier NewPacket;
            NewPacket.PacketName = Packet.PacketName;
            NewPacket.PacketURI = GetSystemPacketsDirectory() + "/" + Packet.PacketName + "/";
            NewPacket.PacketLocation = PacketLocationType::Installed;
            ReturnValue.push_back(std::move(NewPacket));
        }
		if(MissingPackets.size() > 0)
        {
            OutError = false;
            OutError.ErrorMessage = "Missing dependancies: ";   
            for(std::string const& Dependancy : MissingPackets)
            {
                OutError.ErrorMessage += Dependancy+" ";   
            }
        }
        return(ReturnValue);    
    }
    std::vector<PacketIdentifier> PacketRetriever::GetPacketDependancies(PacketIdentifier const& Packet,MBError& OutError,bool Inclusive)
    {
        MBError Result = true;
        std::vector<std::string> MissingPackets;
        
        std::vector<PacketIdentifier> ReturnValue = p_GetPacketDependancies_DependancyOrder({Packet},OutError,&MissingPackets,Inclusive);
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::GetTotalDependancies(std::vector<std::string> const& Dependancies,MBError& OutError,bool Inclusive)
    {
        std::vector<PacketIdentifier> ReturnValue;
        for(auto const& PacketName : Dependancies)
        {
            ReturnValue.push_back(p_GetInstalledPacket(PacketName)); 
            if(ReturnValue.back().PacketName == "")
            {
                OutError = false;
                OutError.ErrorMessage = "Failed finding installed packet: "+PacketName;   
                return(ReturnValue);
            }
        }
        std::vector<std::string> MissingPacketes;
        ReturnValue = p_GetPacketDependancies_DependancyOrder(ReturnValue,OutError,&MissingPacketes,Inclusive);
        return(ReturnValue);        
    }
    std::vector<PacketIdentifier> PacketRetriever::GetPacketsDependancies(std::vector<PacketIdentifier> const& PacketsToInspect,
            MBError& OutError,bool Inclusive)
    {
        std::vector<std::string> MissingPackets;
        std::vector<PacketIdentifier> ReturnValue; 
        ReturnValue = p_GetPacketDependancies_DependancyOrder(PacketsToInspect,OutError,&MissingPackets,Inclusive);
        return(ReturnValue);
    }

    std::vector<PacketIdentifier> PacketRetriever::GetPacketsDependees(std::vector<PacketIdentifier> const& PacketsToInspect,MBError& OutError,bool Inclusive)
    {
        std::vector<PacketIdentifier> ReturnValue; 
        ReturnValue = p_GetPacketDependants_DependancyOrder(PacketsToInspect,OutError,Inclusive);
        return(ReturnValue);
    }
    std::vector<PacketIdentifier> PacketRetriever::GetPacketDependees(std::string const& PacketName,MBError& OutError,bool Inclusive)
    {
        std::vector<PacketIdentifier> ReturnValue; 
        PacketIdentifier InstalledPacket = GetInstalledPacket(PacketName,OutError);
        if(!OutError)
        {
            return(ReturnValue);   
        }
        ReturnValue = p_GetPacketDependants_DependancyOrder({InstalledPacket},OutError,Inclusive);
        return(ReturnValue);
    }


    MBPM_PacketInfo PacketRetriever::GetPacketInfo(PacketIdentifier const& PacketToRetrieve)
    {
        MBError Result = true;
        return(p_GetPacketInfo(PacketToRetrieve,&Result));
    }
    //END PacketRetriever
};
