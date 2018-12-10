/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PacketLog.h"
#include "Config.h"
#include "IpAddress.h"
#include "WorldPacket.h"
#include "Timer.h"
#include "Log.h"
#include "Opcodes.h"
#include <iomanip>

#pragma pack(push, 1)

// Packet logging structures in PKT 3.1 format
struct LogHeader
{
    char Signature[3];
    uint16 FormatVersion;
    uint8 SnifferId;
    uint32 Build;
    char Locale[4];
    uint8 SessionKey[40];
    uint32 SniffStartUnixtime;
    uint32 SniffStartTicks;
    uint32 OptionalDataSize;
};

struct PacketHeader
{
    // used to uniquely identify a connection
    struct OptionalData
    {
        uint8 SocketIPBytes[16];
        uint32 SocketPort;
    };

    char Direction[4];
    uint32 ConnectionId;
    uint32 ArrivalTicks;
    uint32 OptionalDataSize;
    uint32 Length;
    OptionalData OptionalData;
    uint32 Opcode;
};

#pragma pack(pop)

PacketLog::PacketLog() : _file(NULL)
{
    std::call_once(_initializeFlag, &PacketLog::Initialize, this);
}

PacketLog::~PacketLog()
{
    if (_file)
        fclose(_file);

    _file = NULL;
}

void PacketLog::Initialize()
{
    std::string logsDir = sConfigMgr->GetStringDefault("LogsDir", "");

    if (!logsDir.empty())
        if ((logsDir.at(logsDir.length() - 1) != '/') && (logsDir.at(logsDir.length() - 1) != '\\'))
            logsDir.push_back('/');

    std::string logname = sConfigMgr->GetStringDefault("PacketLogFile", "");
    if (!logname.empty())
    {
        _file = fopen((logsDir + logname).c_str(), "wb");

        LogHeader header;
        header.Signature[0] = 'P'; header.Signature[1] = 'K'; header.Signature[2] = 'T';
        header.FormatVersion = 0x0301;
        header.SnifferId = 'T';
        header.Build = 12340;
        header.Locale[0] = 'e'; header.Locale[1] = 'n'; header.Locale[2] = 'U'; header.Locale[3] = 'S';
        std::memset(header.SessionKey, 0, sizeof(header.SessionKey));
        header.SniffStartUnixtime = time(NULL);
        header.SniffStartTicks = GetMSTime();
        header.OptionalDataSize = 0;

        fwrite(&header, sizeof(header), 1, _file);
    }
}

void PacketLog::LogPacket(WorldPacket const& packet, Direction direction, boost::asio::ip::address addr, uint16 port)
{
    std::lock_guard<std::mutex> lock(_logPacketLock);

    PacketHeader header;
    *reinterpret_cast<uint32*>(header.Direction) = direction == CLIENT_TO_SERVER ? 0x47534d43 : 0x47534d53;
    header.ConnectionId = 0;
    header.ArrivalTicks = GetMSTime();

    header.OptionalDataSize = sizeof(header.OptionalData);
    memset(header.OptionalData.SocketIPBytes, 0, sizeof(header.OptionalData.SocketIPBytes));
    if (addr.is_v4())
    {
        auto bytes = addr.to_v4().to_bytes();
        memcpy(header.OptionalData.SocketIPBytes, bytes.data(), bytes.size());
    }
    else if (addr.is_v6())
    {
        auto bytes = addr.to_v6().to_bytes();
        memcpy(header.OptionalData.SocketIPBytes, bytes.data(), bytes.size());
    }

    header.OptionalData.SocketPort = port;
    header.Length = packet.size() + sizeof(header.Opcode);
    header.Opcode = packet.GetOpcode();

    fwrite(&header, sizeof(header), 1, _file);
    if (!packet.empty())
        fwrite(packet.contents(), 1, packet.size(), _file);

    fflush(_file);
}

void PacketLog::DumpPacket(LogLevel const level, Direction const dir, WorldPacket const& packet, std::string const& comment)
{
    TC_LOG_MESSAGE_BODY("network.opcode",level,"%s", dir == SERVER_TO_CLIENT ? "SERVER -> CLIENT" : "CLIENT -> SERVER");
    if(!comment.empty())
        TC_LOG_MESSAGE_BODY("network.opcode", level,"%s", comment.c_str());
    TC_LOG_MESSAGE_BODY("network.opcode", level, "Opcode: %s", GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet.GetOpcode())).c_str());
    uint32 p = 0;

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    while (p < packet.size ())
    {
        for (uint32 j = 0; j < 16 && p < packet.size (); j++)
            ss << std::setw(2) << static_cast<unsigned>(packet[p++]) << " ";
        
        TC_LOG_MESSAGE_BODY("network.opcode",level,"%s",ss.str().c_str());
        ss.str(""); //clear ss
        ss.clear();
    }

    TC_LOG_MESSAGE_BODY("network.opcode",level,"\n");
}