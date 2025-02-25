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

#pragma once

#include "common/ipc.h"
#include "common/ipp.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/zmq_dealer_wrapper.h"

#include "character_cache.h"
#include "world_server.h"
#include "zone_settings.h"

#include <nonstd/jthread.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>

class IPCServer final : public ipc::IIPCMessageHandler
{
public:
    IPCServer(WorldServer& worldServer);
    ~IPCServer() override = default;

    void handleIncomingMessages();

    template <typename T>
    void sendMessage(const IPP& ipp, const T& message);

    //
    // IPP Lookup
    //

    auto getIPPForCharId(uint32 charId) -> std::optional<IPP>;
    auto getIPPForCharName(const std::string& charName) -> std::optional<IPP>;
    auto getIPPForZoneId(uint16 zoneId) -> std::optional<IPP>;
    auto getIPPsForParty(uint32 partyId) -> std::vector<IPP>;
    auto getIPPsForAlliance(uint32 allianceId) -> std::vector<IPP>;
    auto getIPPsForLinkshell(uint32 linkshellId) -> std::vector<IPP>;
    auto getIPPsForUnity(uint32 unityId) -> std::vector<IPP>;
    auto getIPPsForYellZones() -> std::vector<IPP>;
    auto getIPPsForAllZones() -> std::vector<IPP>;

    //
    // Message routing
    //

    void rerouteMessageToCharId(uint32 charId, const auto& message);
    void rerouteMessageToCharName(const std::string& charName, const auto& message);
    void rerouteMessageToZoneId(uint16 zoneId, const auto& message);
    void rerouteMessageToPartyMembers(uint32 partyId, const auto& message);
    void rerouteMessageToAllianceMembers(uint32 allianceId, const auto& message);
    void rerouteMessageToLinkshellMembers(uint32 linkshellId, const auto& message);
    void rerouteMessageToUnityMembers(uint32 unityId, const auto& message);
    void rerouteMessageToYellZones(const auto& message);
    void rerouteMessageToAllZones(const auto& message);

    //
    // ipc::IIPCMessageHandler
    //

    void handleMessage_EmptyStruct(const IPP& ipp, const ipc::EmptyStruct& message) override;
    void handleMessage_CharLogin(const IPP& ipp, const ipc::CharLogin& message) override;
    void handleMessage_CharVarUpdate(const IPP& ipp, const ipc::CharVarUpdate& message) override;
    void handleMessage_ChatMessageTell(const IPP& ipp, const ipc::ChatMessageTell& message) override;
    void handleMessage_ChatMessageParty(const IPP& ipp, const ipc::ChatMessageParty& message) override;
    void handleMessage_ChatMessageAlliance(const IPP& ipp, const ipc::ChatMessageAlliance& message) override;
    void handleMessage_ChatMessageLinkshell(const IPP& ipp, const ipc::ChatMessageLinkshell& message) override;
    void handleMessage_ChatMessageUnity(const IPP& ipp, const ipc::ChatMessageUnity& message) override;
    void handleMessage_ChatMessageYell(const IPP& ipp, const ipc::ChatMessageYell& message) override;
    void handleMessage_ChatMessageServerMessage(const IPP& ipp, const ipc::ChatMessageServerMessage& message) override;
    void handleMessage_ChatMessageCustom(const IPP& ipp, const ipc::ChatMessageCustom& message) override;
    void handleMessage_PartyInvite(const IPP& ipp, const ipc::PartyInvite& message) override;
    void handleMessage_PartyInviteResponse(const IPP& ipp, const ipc::PartyInviteResponse& message) override;
    void handleMessage_PartyReload(const IPP& ipp, const ipc::PartyReload& message) override;
    void handleMessage_PartyDisband(const IPP& ipp, const ipc::PartyDisband& message) override;
    void handleMessage_AllianceReload(const IPP& ipp, const ipc::AllianceReload& message) override;
    void handleMessage_AllianceDissolve(const IPP& ipp, const ipc::AllianceDissolve& message) override;
    void handleMessage_PlayerKick(const IPP& ipp, const ipc::PlayerKick& message) override;
    void handleMessage_MessageStandard(const IPP& ipp, const ipc::MessageStandard& message) override;
    void handleMessage_MessageSystem(const IPP& ipp, const ipc::MessageSystem& message) override;
    void handleMessage_LinkshellRankChange(const IPP& ipp, const ipc::LinkshellRankChange& message) override;
    void handleMessage_LinkshellRemove(const IPP& ipp, const ipc::LinkshellRemove& message) override;
    void handleMessage_LinkshellSetMessage(const IPP& ipp, const ipc::LinkshellSetMessage& message) override;
    void handleMessage_LuaFunction(const IPP& ipp, const ipc::LuaFunction& message) override;
    void handleMessage_KillSession(const IPP& ipp, const ipc::KillSession& message) override;
    void handleMessage_RegionalEvent(const IPP& ipp, const ipc::RegionalEvent& message) override;
    void handleMessage_GMSendToZone(const IPP& ipp, const ipc::GMSendToZone& message) override;
    void handleMessage_GMSendToEntity(const IPP& ipp, const ipc::GMSendToEntity& message) override;
    void handleMessage_RPCSend(const IPP& ipp, const ipc::RPCSend& message) override;
    void handleMessage_RPCRecv(const IPP& ipp, const ipc::RPCRecv& message) override;

    void handleUnknownMessage(const IPP& ipp, const std::span<uint8_t> message) override;

private:
    WorldServer& worldServer_;

    CharacterCache   characterCache_;
    ZoneSettings     zoneSettings_;
    ZMQRouterWrapper zmqRouterWrapper_;
};

//
// Inline implementations
//

template <typename T>
void IPCServer::sendMessage(const IPP& ipp, const T& message)
{
    TracyZoneScoped;

    DebugIPCFmt("Sending {} message to {}", ipc::toString(ipc::getEnumType<T>()), ipp.toString());

    const auto bytes = ipc::toBytesWithHeader<T>(message);
    const auto out   = HandleableMessage{ ipp, std::vector<uint8>{ bytes.begin(), bytes.end() } };

    zmqRouterWrapper_.outgoingQueue_.enqueue(std::move(out));
}
