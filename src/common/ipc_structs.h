/*
===========================================================================

  Copyright (c) 2024 LandSandBoat Dev Teams

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

#include "cbasetypes.h"

#include "common/regional_event.h"

#include "map/packets/chat_message.h"
#include "map/packets/message_standard.h"
#include "map/packets/party_invite.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ipc
{
    struct EmptyStruct
    {
    };

    struct CharLogin
    {
        uint32 accountId{};
        uint32 charId{};
    };

    struct CharZone
    {
        uint32 charId{};
        uint16 destinationZoneId{};
    };

    struct CharVarUpdate
    {
        uint32      charId{};
        int32       value{};
        uint32      expiry{};
        std::string varName{};
    };

    struct ChatMessageTell
    {
        uint32      senderId{};
        std::string senderName{};
        std::string recipientName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageParty
    {
        uint32      partyId{};
        uint32      senderId{};
        std::string senderName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageAlliance
    {
        uint32      allianceId{};
        uint32      senderId{};
        std::string senderName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageLinkshell
    {
        uint32      linkshellId{};
        uint32      senderId{};
        std::string senderName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageUnity
    {
        uint32      unityLeaderId{};
        uint32      senderId{};
        std::string senderName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageYell
    {
        uint32      senderId{};
        std::string senderName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageServerMessage
    {
        uint32      senderId{};
        std::string senderName{};
        std::string message{};
        uint16      zoneId{};
        uint8       gmLevel{};
    };

    struct ChatMessageCustom
    {
        uint32            recipientId{};
        std::string       senderName{};
        std::string       message{};
        CHAT_MESSAGE_TYPE messageType{};
    };

    struct PartyInvite
    {
        uint32      inviteeId{};
        uint16      inviteeTargId{};
        uint32      inviterId{};
        uint16      inviterTargId{};
        std::string inviterName{};
        INVITETYPE  inviteType{};
    };

    struct PartyInviteResponse
    {
        uint32 inviteeId{};
        uint16 inviteeTargId{};
        uint32 inviterId{};
        uint16 inviterTargId{};
        uint8  inviteAnswer{};
    };

    struct PartyReload
    {
        uint32 partyId{};
    };

    struct PartyDisband
    {
        uint32 partyId{};
    };

    struct AllianceReload
    {
        uint32 allianceId{};
    };

    struct AllianceDissolve
    {
        uint32 allianceId{};
    };

    struct PlayerKick
    {
        uint32 victimId{};
    };

    struct MessageStandard
    {
        uint32 recipientId{};
        MsgStd message{};
        uint32 param0{};
        uint32 param1{};
    };

    struct MessageSystem
    {
        uint32 recipientId{};
        MsgStd message{};
        uint32 param0{};
        uint32 param1{};
    };

    struct LinkshellRankChange
    {
        uint32      changerId{};
        std::string memberName{};
        uint32      linkshellId{};
        uint8       permission{};
    };

    struct LinkshellRemove
    {
        uint32      changerId{};
        std::string victimName{};
        uint32      linkshellId{};
        uint8       linkshellType{};
    };

    struct LinkshellSetMessage
    {
        uint32      linkshellId{};
        std::string linkshellName{};
        std::string poster{};
        std::string message{};
        uint32      postTime{};
    };

    struct LuaFunction
    {
        uint16      requesterZoneId{};
        uint16      executorZoneId{};
        std::string funcString{};
    };

    struct KillSession
    {
        uint32 victimId{};
    };

    struct ConquestEvent
    {
        ConquestMessage    type{};
        std::vector<uint8> payload{};
    };

    struct BesiegedEvent
    {
        BesiegedMessage    type{};
        std::vector<uint8> payload{};
    };

    struct CampaignEvent
    {
        CampaignMessage    type{};
        std::vector<uint8> payload{};
    };

    struct ColonizationEvent
    {
        ColonizationMessage type{};
        std::vector<uint8>  payload{};
    };

    struct EntityInformationRequest
    {
        uint32 requesterId{};
        uint32 targetId{};
        uint8  entityType{};
        bool   warp{};
        bool   spawnedOnly{};
    };

    struct EntityInformationResponse
    {
        uint32 requesterId{};
        uint32 targetId{};
        uint8  entityType{};
        bool   warp{};

        uint16 zoneId{};
        float  x{};
        float  y{};
        float  z{};
        uint8  rot{};
        uint32 moghouseId{};
    };

    struct SendPlayerToLocation
    {
        uint32 targetId{};
        uint16 zoneId{};
        float  x{};
        float  y{};
        float  z{};
        uint8  rot{};
        uint32 moghouseId{};
    };
} // namespace ipc
