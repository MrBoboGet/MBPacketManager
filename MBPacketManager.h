#pragma once
#include <filesystem>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <MBUtility/MBErrorHandling.h>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_set>
#include <MBParsing/MBParsing.h>
#include <unordered_map>

#include "MB_PacketProtocol.h"
namespace MBPM
{
	struct MBPM_PacketInfo
	{
		std::string PacketName = "";
		std::unordered_set<std::string> Attributes = {};
		std::vector<std::string> PacketDependancies = {};
        std::string Type = "C++";
        std::vector<std::string> SubPackets;
        MBParsing::JSONObject TypeInfo;
	};
	MBPM_PacketInfo ParseMBPM_PacketInfo(std::string const& PacketPath);

    enum class PacketLocationType
    {
        Null,
        User,
        Installed,
        Local,
        Remote
    };
    struct MBPM_PacketDependancyRankInfo
	{
		uint32_t DependancyDepth = -1;
		std::string PacketName = "";
		std::vector<std::string> PacketDependancies = {};
		bool operator<(MBPM_PacketDependancyRankInfo const& PacketToCompare) const
		{
			bool ReturnValue = DependancyDepth < PacketToCompare.DependancyDepth;
			if (DependancyDepth == PacketToCompare.DependancyDepth)
			{
				ReturnValue = PacketName < PacketToCompare.PacketName;
			}
			return(ReturnValue);
		}
	};
    enum class PacketIdentifierFlag
    {
        Null = 0,
        SubPacket = 1
    };
    inline PacketIdentifierFlag operator&(PacketIdentifierFlag Lhs,PacketIdentifierFlag Rhs)
    {
        return(PacketIdentifierFlag(uint64_t(Lhs)&uint64_t(Rhs)));
    };
    inline PacketIdentifierFlag operator|(PacketIdentifierFlag Lhs,PacketIdentifierFlag Rhs)
    {
        return(PacketIdentifierFlag(uint64_t(Lhs)|uint64_t(Rhs)));
    };
    inline bool operator==(PacketIdentifierFlag Lhs,uint64_t Rhs)
    {
        return(uint64_t(Lhs) == Rhs);
    }
    inline bool operator!=(PacketIdentifierFlag Lhs,uint64_t Rhs)
    {
        return(!(Lhs == Rhs));
    }
    struct PacketIdentifier
    {
        std::string PacketName = "";
        std::string PacketURI = "";//Default/"" implicerar att man anv�nder default remoten
        PacketLocationType PacketLocation = PacketLocationType::Null;
        PacketIdentifierFlag Flags = PacketIdentifierFlag::Null;
        bool operator==(PacketIdentifier const& rhs) const
        {
            bool ReturnValue = true;
            if (PacketName != rhs.PacketName)
            {
                ReturnValue = false;
            }
            if (PacketURI != rhs.PacketURI)
            {
                ReturnValue = false;
            }
            if (PacketLocation != rhs.PacketLocation)
            {
                ReturnValue = false;
            }
            return(ReturnValue);
        }
    };
    class PacketRetriever
    {
    private:
        //std::unordered_map<std::filesystem::path, std::pair<PacketIdentifier,MBPM_PacketInfo>> m_CachedPackets;

        std::vector<PacketIdentifier> p_GetUserPackets();
        
        MBPM::MBPM_PacketInfo p_GetPacketInfo(PacketIdentifier const& PacketToInspect, MBError* OutError);
		std::map<std::string, MBPM_PacketDependancyRankInfo> p_GetPacketDependancieInfo(
			std::vector<PacketIdentifier> const& InPacketsToCheck,
			MBError& OutError,
			std::vector<std::string>* OutMissing);
        std::vector<PacketIdentifier> p_GetPacketDependancies_DependancyOrder(std::vector<PacketIdentifier> const& InPacketsToCheck,MBError& OutError, std::vector<std::string>* MissingPackets,bool IncludeInitial = false);
        std::vector<PacketIdentifier> p_GetPacketDependants_DependancyOrder(std::vector<PacketIdentifier> const& InPacketsToCheck,MBError& OutError,bool IncludeInitial = false);



        PacketIdentifier p_GetUserPacket(std::string const& PacketName);
        PacketIdentifier p_GetInstalledPacket(std::string const& PacketName);
        PacketIdentifier p_GetLocalPacket(std::string const& PacketName);
        PacketIdentifier p_GetRemotePacketIdentifier(std::string const& PacketName);
    public:
        PacketIdentifier GetInstalledPacket(std::string const& PacketName,MBError& OutError);
        PacketIdentifier GetUserPacket(std::string const& PacketName,MBError& OutError);
        PacketIdentifier GetLocalpacket(std::string const& path,MBError& OutError);

        std::vector<PacketIdentifier> GetSubPackets(PacketIdentifier const& PacketToInspect,MBError& OutError);

        std::vector<PacketIdentifier> GetInstalledPackets(MBError& OutError);
        std::vector<PacketIdentifier> GetUserPackets(MBError& OutError);

        std::vector<PacketIdentifier> GetPacketDependancies(PacketIdentifier const& PacketToInspect,MBError& OutError,bool Inclusive=false);
        std::vector<PacketIdentifier> GetPacketsDependancies(std::vector<PacketIdentifier> const& PacketsToInspect,MBError& OutError,bool Inclusive=false);
        std::vector<PacketIdentifier> GetTotalDependancies(std::vector<std::string> const& DependancyNames,MBError& OutError,bool Inclusive=true);

        std::vector<PacketIdentifier> GetPacketsDependees(std::vector<PacketIdentifier> const& PacketsToInspect,MBError& OutError,bool Inclusive=false);
        std::vector<PacketIdentifier> GetPacketDependees(std::string const& PacketName,MBError& OutError,bool Inclusive=false);
        MBPM_PacketInfo GetPacketInfo(PacketIdentifier const& PacketToRetrieve);
    };
	std::string GetSystemPacketsDirectory();
};
