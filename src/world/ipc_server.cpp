/*
===========================================================================

  Copyright (c) 2025 LandSandBoat Dev Teams

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

===========================================================================
*/

#include "ipc_server.h"

#include "besieged_system.h"
#include "campaign_system.h"
#include "colonization_system.h"
#include "conquest_system.h"
#include "world_server.h"

#include <concurrentqueue.h>
#include <memory>
#include <queue>
#include <set>

#include "common/database.h"
#include "common/logging.h"
#include "common/regional_event.h"

namespace
{
    auto getZMQEndpointString() -> std::string
    {
        return fmt::format("tcp://{}:{}", settings::get<std::string>("network.ZMQ_IP"), settings::get<uint16>("network.ZMQ_PORT"));
    }
} // namespace

IPCServer::IPCServer(WorldServer& worldServer)
: worldServer_(worldServer)
, zmqRouterWrapper_(getZMQEndpointString())
{
    TracyZoneScoped;
}

//
// IPP Lookup
//

auto IPCServer::getIPPForCharId(uint32 charId) -> std::optional<IPP>
{
    TracyZoneScoped;

    // TODO: We know when chars move, we could be caching this information

    const auto rset = db::preparedStmt("SELECT server_addr, server_port FROM accounts_sessions WHERE charid = ? LIMIT 1", charId);
    if (rset && rset->rowsCount() && rset->next())
    {
        const auto ip   = rset->get<uint64>("server_addr");
        const auto port = rset->get<uint64>("server_port");
        return IPP(ip, port);
    }

    return std::nullopt;
}

auto IPCServer::getIPPForCharName(const std::string& charName) -> std::optional<IPP>
{
    TracyZoneScoped;

    // TODO: We know when chars move, we could be caching this information

    const auto rset = db::preparedStmt("SELECT server_addr, server_port FROM accounts_sessions LEFT JOIN chars ON "
                                       "accounts_sessions.charid = chars.charid WHERE charname = ? LIMIT 1",
                                       charName);
    if (rset && rset->rowsCount() && rset->next())
    {
        const auto ip   = rset->get<uint64>("server_addr");
        const auto port = rset->get<uint64>("server_port");
        return IPP(ip, port);
    }

    return std::nullopt;
}

auto IPCServer::getIPPForZoneId(uint16 zoneId) -> std::optional<IPP>
{
    TracyZoneScoped;

    if (const auto it = zoneSettings_.zoneSettingsMap_.find(zoneId); it != zoneSettings_.zoneSettingsMap_.end())
    {
        return it->second.ipp;
    }

    return std::nullopt;
}

auto IPCServer::getIPPsForParty(uint32 partyId) -> std::vector<IPP>
{
    TracyZoneScoped;

    // TODO: We know when chars move, we could be caching this infor

    // TODO: Simplify query now that there's alliance versions?
    const auto query = "SELECT server_addr, server_port, MIN(charid) FROM accounts_sessions JOIN accounts_parties USING (charid) "
                       "WHERE IF (allianceid <> 0, allianceid = (SELECT MAX(allianceid) FROM accounts_parties WHERE partyid = ?), "
                       "partyid = ?) GROUP BY server_addr, server_port";

    const auto rset = db::preparedStmt(query, partyId, partyId);
    if (rset && rset->rowsCount())
    {
        std::vector<IPP> ippList;
        while (rset->next())
        {
            const auto ip   = rset->get<uint64>("server_addr");
            const auto port = rset->get<uint64>("server_port");
            ippList.emplace_back(ip, port);
        }

        return ippList;
    }

    return {};
}

auto IPCServer::getIPPsForAlliance(uint32 allianceId) -> std::vector<IPP>
{
    TracyZoneScoped;

    // TODO: We know when chars move, we could be caching this infor

    const auto query = "SELECT server_addr, server_port, MIN(charid) FROM accounts_sessions JOIN accounts_parties USING (charid) "
                       "WHERE allianceid = ? "
                       "GROUP BY server_addr, server_port";

    const auto rset = db::preparedStmt(query, allianceId);
    if (rset && rset->rowsCount())
    {
        std::vector<IPP> ippList;
        while (rset->next())
        {
            const auto ip   = rset->get<uint64>("server_addr");
            const auto port = rset->get<uint64>("server_port");
            ippList.emplace_back(ip, port);
        }

        return ippList;
    }

    return {};
}

auto IPCServer::getIPPsForLinkshell(uint32 linkshellId) -> std::vector<IPP>
{
    TracyZoneScoped;

    // TODO: We know when chars move, we could be caching this infor

    const auto query = "SELECT server_addr, server_port FROM accounts_sessions "
                       "WHERE linkshellid1 = ? OR linkshellid2 = ? GROUP BY server_addr, server_port";

    const auto rset = db::preparedStmt(query, linkshellId, linkshellId);
    if (rset && rset->rowsCount())
    {
        std::vector<IPP> ippList;
        while (rset->next())
        {
            const auto ip   = rset->get<uint64>("server_addr");
            const auto port = rset->get<uint64>("server_port");
            ippList.emplace_back(ip, port);
        }

        return ippList;
    }

    return {};
}

auto IPCServer::getIPPsForUnity(uint32 unityId) -> std::vector<IPP>
{
    TracyZoneScoped;

    // TODO: We know when chars move, we could be caching this infor

    const auto query = "SELECT server_addr, server_port FROM accounts_sessions "
                       "WHERE unitychat = ? GROUP BY server_addr, server_port";

    const auto rset = db::preparedStmt(query, unityId);
    if (rset && rset->rowsCount())
    {
        std::vector<IPP> ippList;
        while (rset->next())
        {
            const auto ip   = rset->get<uint64>("server_addr");
            const auto port = rset->get<uint64>("server_port");
            ippList.emplace_back(ip, port);
        }

        return ippList;
    }

    return {};
}

auto IPCServer::getIPPsForYellZones() -> std::vector<IPP>
{
    TracyZoneScoped;

    return zoneSettings_.yellMapEndpoints_;
}

auto IPCServer::getIPPsForAllZones() -> std::vector<IPP>
{
    TracyZoneScoped;

    return zoneSettings_.mapEndpoints_;
}

//
// Message routing
//

void IPCServer::rerouteMessageToCharId(uint32 charId, const auto& message)
{
    TracyZoneScoped;

    if (const auto maybeCharIPP = getIPPForCharId(charId))
    {
        const auto charIPP = *maybeCharIPP;
        DebugIPCFmt("Message: -> rerouting to char<{}> on {}", charId, charIPP.toString());
        sendMessage(charIPP, std::move(message));
    }
}

void IPCServer::rerouteMessageToCharName(const std::string& charName, const auto& message)
{
    TracyZoneScoped;

    if (const auto maybeCharIPP = getIPPForCharName(charName))
    {
        const auto charIPP = *maybeCharIPP;
        DebugIPCFmt("Message: -> rerouting to char<{}> on {}", charName, charIPP.toString());
        sendMessage(charIPP, std::move(message));
    }
}

void IPCServer::rerouteMessageToZoneId(uint16 zoneId, const auto& message)
{
    TracyZoneScoped;

    if (const auto maybeZoneIPP = getIPPForZoneId(zoneId))
    {
        const auto zoneIPP = *maybeZoneIPP;
        DebugIPCFmt("Message: -> rerouting to zone<{}> on {}", zoneId, zoneIPP.toString());
        sendMessage(zoneIPP, std::move(message));
    }
}

void IPCServer::rerouteMessageToPartyMembers(uint32 partyId, const auto& message)
{
    TracyZoneScoped;

    for (const auto& ipp : getIPPsForParty(partyId))
    {
        DebugIPCFmt("Message: -> rerouting to party<{}> on {}", partyId, ipp.toString());
        sendMessage(ipp, message);
    }
}

void IPCServer::rerouteMessageToAllianceMembers(uint32 allianceId, const auto& message)
{
    TracyZoneScoped;

    for (const auto& ipp : getIPPsForAlliance(allianceId))
    {
        DebugIPCFmt("Message: -> rerouting to alliance<{}> on {}", allianceId, ipp.toString());
        sendMessage(ipp, message);
    }
}

void IPCServer::rerouteMessageToLinkshellMembers(uint32 linkshellId, const auto& message)
{
    TracyZoneScoped;

    for (const auto& ipp : getIPPsForLinkshell(linkshellId))
    {
        DebugIPCFmt("Message: -> rerouting to linkshell<{}> on {}", linkshellId, ipp.toString());
        sendMessage(ipp, message);
    }
}

void IPCServer::rerouteMessageToUnityMembers(uint32 unityId, const auto& message)
{
    TracyZoneScoped;

    for (const auto& ipp : getIPPsForUnity(unityId))
    {
        DebugIPCFmt("Message: -> rerouting to unity<{}> on {}", unityId, ipp.toString());
        sendMessage(ipp, message);
    }
}

void IPCServer::rerouteMessageToYellZones(const auto& message)
{
    TracyZoneScoped;

    for (const auto& ipp : getIPPsForYellZones())
    {
        DebugIPCFmt("Message: -> rerouting to yell zone on {}", ipp.toString());
        sendMessage(ipp, message);
    }
}

void IPCServer::rerouteMessageToAllZones(const auto& message)
{
    TracyZoneScoped;

    for (const auto& ipp : getIPPsForAllZones())
    {
        DebugIPCFmt("Message: -> rerouting to all zones on {}", ipp.toString());
        sendMessage(ipp, message);
    }
}

void IPCServer::handleIncomingMessages()
{
    TracyZoneScoped;

    // TODO: Can we stop more messages appearing on the queue while we're processing?
    HandleableMessage out;
    while (zmqRouterWrapper_.incomingQueue_.try_dequeue(out))
    {
        const auto firstByte = out.payload[0];
        const auto msgType   = ipc::toString(static_cast<ipc::MessageType>(firstByte));

        DebugIPCFmt("Incoming {} message from {}", msgType, out.ipp.toString());

        ipc::IIPCMessageHandler::handleMessage(out.ipp, { out.payload.data(), out.payload.size() });
    }
}

void IPCServer::handleMessage_EmptyStruct(const IPP& ipp, const ipc::EmptyStruct& message)
{
    TracyZoneScoped;

    ShowWarningFmt("Received EmptyStruct message from {} - this is probably a bug", ipp.toString());
}

void IPCServer::handleMessage_CharLogin(const IPP& ipp, const ipc::CharLogin& message)
{
    TracyZoneScoped;

    DebugIPCFmt("Received CharLogin message from {} for account {} char {}", ipp.toString(), message.accountId, message.charId);

    // NOTE: Originally a NO-OP
}

void IPCServer::handleMessage_CharVarUpdate(const IPP& ipp, const ipc::CharVarUpdate& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.charId, message);
}

void IPCServer::handleMessage_ChatMessageTell(const IPP& ipp, const ipc::ChatMessageTell& message)
{
    TracyZoneScoped;

    rerouteMessageToCharName(message.recipientName, message);
}

void IPCServer::handleMessage_ChatMessageParty(const IPP& ipp, const ipc::ChatMessageParty& message)
{
    TracyZoneScoped;

    rerouteMessageToPartyMembers(message.partyId, message);
}

void IPCServer::handleMessage_ChatMessageAlliance(const IPP& ipp, const ipc::ChatMessageAlliance& message)
{
    TracyZoneScoped;

    rerouteMessageToAllianceMembers(message.allianceId, message);
}

void IPCServer::handleMessage_ChatMessageLinkshell(const IPP& ipp, const ipc::ChatMessageLinkshell& message)
{
    TracyZoneScoped;

    rerouteMessageToLinkshellMembers(message.linkshellId, message);
}

void IPCServer::handleMessage_ChatMessageUnity(const IPP& ipp, const ipc::ChatMessageUnity& message)
{
    TracyZoneScoped;

    rerouteMessageToUnityMembers(message.unityLeaderId, message);
}

void IPCServer::handleMessage_ChatMessageYell(const IPP& ipp, const ipc::ChatMessageYell& message)
{
    TracyZoneScoped;

    rerouteMessageToYellZones(message);
}

void IPCServer::handleMessage_ChatMessageServerMessage(const IPP& ipp, const ipc::ChatMessageServerMessage& message)
{
    TracyZoneScoped;

    rerouteMessageToAllZones(message);
}

void IPCServer::handleMessage_ChatMessageCustom(const IPP& ipp, const ipc::ChatMessageCustom& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.recipientId, message);
}

void IPCServer::handleMessage_PartyInvite(const IPP& ipp, const ipc::PartyInvite& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.inviteeId, message);
}

void IPCServer::handleMessage_PartyInviteResponse(const IPP& ipp, const ipc::PartyInviteResponse& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.inviterId, message);
}

void IPCServer::handleMessage_PartyReload(const IPP& ipp, const ipc::PartyReload& message)
{
    TracyZoneScoped;

    rerouteMessageToPartyMembers(message.partyId, message);
}

void IPCServer::handleMessage_PartyDisband(const IPP& ipp, const ipc::PartyDisband& message)
{
    TracyZoneScoped;

    rerouteMessageToPartyMembers(message.partyId, message);
}

void IPCServer::handleMessage_AllianceReload(const IPP& ipp, const ipc::AllianceReload& message)
{
    TracyZoneScoped;

    rerouteMessageToAllianceMembers(message.allianceId, message);
}

void IPCServer::handleMessage_AllianceDissolve(const IPP& ipp, const ipc::AllianceDissolve& message)
{
    TracyZoneScoped;

    rerouteMessageToAllianceMembers(message.allianceId, message);
}

void IPCServer::handleMessage_PlayerKick(const IPP& ipp, const ipc::PlayerKick& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.charId, message);
}

void IPCServer::handleMessage_MessageStandard(const IPP& ipp, const ipc::MessageStandard& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.charId, message);
}

void IPCServer::handleMessage_MessageSystem(const IPP& ipp, const ipc::MessageSystem& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.charId, message);
}

void IPCServer::handleMessage_LinkshellRankChange(const IPP& ipp, const ipc::LinkshellRankChange& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.charId, message);
}

void IPCServer::handleMessage_LinkshellRemove(const IPP& ipp, const ipc::LinkshellRemove& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.charId, message);
}

void IPCServer::handleMessage_LinkshellSetMessage(const IPP& ipp, const ipc::LinkshellSetMessage& message)
{
    TracyZoneScoped;

    rerouteMessageToLinkshellMembers(message.linkshellId, message);
}

void IPCServer::handleMessage_LuaFunction(const IPP& ipp, const ipc::LuaFunction& message)
{
    TracyZoneScoped;

    rerouteMessageToZoneId(message.zoneId, message);
}

void IPCServer::handleMessage_KillSession(const IPP& ipp, const ipc::KillSession& message)
{
    TracyZoneScoped;

    const auto rset = db::preparedStmt("SELECT pos_prevzone, pos_zone from chars where charid = ? LIMIT 1", message.charId);

    // Get zone ID from query and try to send to _just_ the previous zone
    if (rset && rset->rowsCount() && rset->next())
    {
        const auto prevZoneID = rset->get<uint32>("pos_prevzone");
        const auto nextZoneID = rset->get<uint32>("pos_zone");

        if (prevZoneID != nextZoneID)
        {
            const auto zoneSettings = zoneSettings_.zoneSettingsMap_.at(prevZoneID);

            DebugIPCFmt(fmt::format("Message: -> rerouting to {}", zoneSettings.ipp.toString()));

            sendMessage(zoneSettings.ipp, message);
        }
    }
    else // Otherwise, send to all zones
    {
        // TODO: Is this insane, do we need to send this to _every_ zone?
        for (const auto& ipp : zoneSettings_.mapEndpoints_)
        {
            DebugIPCFmt(fmt::format("Message: -> rerouting to {}", ipp.toString()));

            sendMessage(ipp, message);
        }
    }
}

void IPCServer::handleMessage_RegionalEvent(const IPP& ipp, const ipc::RegionalEvent& message)
{
    TracyZoneScoped;

    // Dispatch to relevant system
    switch (message.type)
    {
        case RegionalEventType::Conquest:
            worldServer_.conquestSystem_->handleMessage(message.subType, { ipp, message.payload });
            break;
        case RegionalEventType::Besieged:
            worldServer_.besiegedSystem_->handleMessage(message.subType, { ipp, message.payload });
            break;
        case RegionalEventType::Campaign:
            worldServer_.campaignSystem_->handleMessage(message.subType, { ipp, message.payload });
            break;
        case RegionalEventType::Colonization:
            worldServer_.colonizationSystem_->handleMessage(message.subType, { ipp, message.payload });
            break;
        default:
            ShowWarning("Received unknown RegionalEvent type {}", static_cast<uint8>(message.type));
            break;
    }
}

void IPCServer::handleMessage_GMSendToZone(const IPP& ipp, const ipc::GMSendToZone& message)
{
    TracyZoneScoped;

    rerouteMessageToCharId(message.targetId, message);
}

void IPCServer::handleMessage_GMSendToEntity(const IPP& ipp, const ipc::GMSendToEntity& message)
{
    TracyZoneScoped;

    rerouteMessageToZoneId(message.zoneId, message);
}

void IPCServer::handleMessage_RPCSend(const IPP& ipp, const ipc::RPCSend& message)
{
    TracyZoneScoped;

    rerouteMessageToZoneId(message.zoneId, message);
}

void IPCServer::handleMessage_RPCRecv(const IPP& ipp, const ipc::RPCRecv& message)
{
    TracyZoneScoped;

    rerouteMessageToZoneId(message.zoneId, message);
}

void IPCServer::handleUnknownMessage(const IPP& ipp, const std::span<uint8_t> message)
{
    TracyZoneScoped;

    ShowWarningFmt("Received unknown message from {} with code {} and size {}", ipp.toString(), message[0], message.size());
}
