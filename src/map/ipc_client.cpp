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

#include "ipc_client.h"

#include "common/ipp.h"

#include <concurrentqueue.h>
#include <queue>

#include "alliance.h"
#include "conquest_system.h"
#include "linkshell.h"
#include "party.h"
#include "status_effect_container.h"
#include "unitychat.h"

#include "entities/charentity.h"

#include "lua/luautils.h"

#include "packets/chat_message.h"
#include "packets/message_standard.h"
#include "packets/message_system.h"
#include "packets/party_invite.h"
#include "packets/server_ip.h"

#include "items/item_linkshell.h"

#include "utils/charutils.h"
#include "utils/jailutils.h"
#include "utils/serverutils.h"
#include "utils/zoneutils.h"

namespace
{
    auto getZMQEndpointString() -> std::string
    {
        return fmt::format("tcp://{}:{}", settings::get<std::string>("network.ZMQ_IP"), settings::get<uint16>("network.ZMQ_PORT"));
    }

    auto getZMQRoutingId() -> uint64
    {
        // Original IPP/RoutingId logic

        uint64 ipp  = map_ip.s_addr;
        uint64 port = map_port;

        // if no ip/port were supplied, set to 1 (0 is not valid for an identity)
        if (map_ip.s_addr == 0 && map_port == 0)
        {
            const auto rset = db::preparedStmt("SELECT zoneip, zoneport FROM zone_settings GROUP BY zoneip, zoneport ORDER BY COUNT(*) DESC");
            if (rset && rset->rowsCount() && rset->next())
            {
                const auto zoneip   = rset->get<std::string>("zoneip");
                const auto zoneport = rset->get<uint16>("zoneport");

                inet_pton(AF_INET, zoneip.c_str(), &ipp);
                port = zoneport;
            }
        }

        ipp |= (port << 32);

        return ipp;
    }
} // namespace

// TODO: Don't do this
std::unique_ptr<IPCClient> ipcClient_;

void message::init()
{
    ipcClient_ = std::make_unique<IPCClient>();
}

void message::handle_incoming()
{
    ipcClient_->handleIncomingMessages();
}

IPCClient::IPCClient()
: zmqDealerWrapper_(getZMQEndpointString(), getZMQRoutingId())
{
    TracyZoneScoped;
}

void IPCClient::handleIncomingMessages()
{
    TracyZoneScoped;

    // TODO: Can we stop more messages appearing on the queue while we're processing?
    zmq::message_t out;
    while (zmqDealerWrapper_.incomingQueue_.try_dequeue(out))
    {
        const auto firstByte = out.data<uint8>()[0];
        const auto msgType   = ipc::toString(static_cast<ipc::MessageType>(firstByte));

        // TODO: Make an IPP for the world server, so we can use it here
        DebugIPCFmt("Incoming {} message", msgType);

        ipc::IIPCMessageHandler::handleMessage(IPP(), { static_cast<uint8*>(out.data()), out.size() });
    }
}

void IPCClient::handleMessage_EmptyStruct(const IPP& ipp, const ipc::EmptyStruct& message)
{
    TracyZoneScoped;

    ShowWarningFmt("Received EmptyStruct message from {} - this is probably a bug", ipp.toString());
}

void IPCClient::handleMessage_CharLogin(const IPP& ipp, const ipc::CharLogin& message)
{
    TracyZoneScoped;

    CCharEntity* PChar = zoneutils::GetChar(message.charId);
    if (!PChar)
    {
        db::preparedStmt("DELETE FROM accounts_sessions WHERE charid = ?", message.charId);
    }
    else
    {
        // TODO: disconnect the client, but leave the character in the disconnecting state
        // PChar->status = STATUS_SHUTDOWN;
        // won't save their position but not a huge deal
        // PChar->pushPacket<CServerIPPacket>(PChar, 1, 0);
    }
}

void IPCClient::handleMessage_CharVarUpdate(const IPP& ipp, const ipc::CharVarUpdate& message)
{
    TracyZoneScoped;

    if (auto* PChar = zoneutils::GetChar(message.charId))
    {
        DebugIPCFmt("Received CharVarUpdate message from {}, char {}: {} = {} (expiry {})", ipp.toString(), message.charId, message.varName, message.value, message.expiry);
        PChar->updateCharVarCache(message.varName, message.value, message.expiry);
    }
}

void IPCClient::handleMessage_ChatMessageTell(const IPP& ipp, const ipc::ChatMessageTell& message)
{
    TracyZoneScoped;

    DebugIPCFmt("Received MessageTell message from {}, char {} to char {}: {}", ipp.toString(), message.senderName, message.recipientName, message.message);

    CCharEntity* PChar = zoneutils::GetCharByName(message.recipientName);
    if (PChar && PChar->status != STATUS_TYPE::DISAPPEAR && !jailutils::InPrison(PChar))
    {
        const auto gmSent = message.gmLevel > 0;

        if (settings::get<bool>("map.BLOCK_TELL_TO_HIDDEN_GM") && PChar->m_isGMHidden && !gmSent)
        {
            message::send(ipc::MessageStandard{
                .charId  = PChar->id,
                .message = MsgStd::TellNotReceivedOffline,
            });
        }
        else if (PChar->isAway() && !gmSent)
        {
            message::send(ipc::MessageStandard{
                .charId  = PChar->id,
                .message = MsgStd::TellNotReceivedAway,
            });
        }
        else
        {
            PChar->pushPacket(std::make_unique<CChatMessagePacket>(PChar, MESSAGE_TELL, message.message, message.senderName));
        }
    }
    else
    {
        message::send(ipc::MessageStandard{
            .charId  = PChar->id,
            .message = MsgStd::TellNotReceivedOffline,
        });
    }
}

void IPCClient::handleMessage_ChatMessageParty(const IPP& ipp, const ipc::ChatMessageParty& message)
{
    TracyZoneScoped;

    CParty* PParty = nullptr;

    const auto partyid = message.partyId;

    // TODO: When Party/Alliance gets a rewrite, make a zoneutils::ForEachParty or some other accessor to reduce the amount of iterations significantly.

    // clang-format off
    zoneutils::ForEachZone([partyid, &PParty](CZone* PZone)
    {
        PZone->ForEachChar([partyid, &PParty](CCharEntity* PChar)
        {
            if (PChar->PParty && PChar->PParty->GetPartyID() == partyid)
            {
                PParty = PChar->PParty;
                return;
            }
        });
        if (PParty)
        {
            return;
        }
    });
    if (PParty)
    {
        PParty->PushPacket(message.senderId, 0, std::make_unique<CChatMessagePacket>(message.senderName, message.zoneId, MESSAGE_PARTY, message.message, message.gmLevel));
    }
    // clang-format on
}

void IPCClient::handleMessage_ChatMessageAlliance(const IPP& ipp, const ipc::ChatMessageAlliance& message)
{
    TracyZoneScoped;

    CAlliance* PAlliance = nullptr;

    const auto allianceid = message.allianceId;

    // TODO: When Party/Alliance gets a rewrite, make a zoneutils::ForEachParty or some other accessor to reduce the amount of iterations significantly.

    // clang-format off
    zoneutils::ForEachZone([allianceid, &PAlliance](CZone* PZone)
    {
        PZone->ForEachChar([allianceid, &PAlliance](CCharEntity* PChar)
        {
            if (PChar->PParty && PChar->PParty && PChar->PParty->m_PAlliance && PChar->PParty->m_PAlliance->m_AllianceID == allianceid)
            {
                PAlliance = PChar->PParty->m_PAlliance;
                return;
            }
        });
        if (PAlliance)
        {
            return;
        }
    });
    if (PAlliance)
    {
        for (const auto& currentParty : PAlliance->partyList)
        {
            currentParty->PushPacket(message.senderId, 0, std::make_unique<CChatMessagePacket>(message.senderName, message.zoneId, MESSAGE_PARTY, message.message, message.gmLevel));
        }
    }
    // clang-format on
}

void IPCClient::handleMessage_ChatMessageLinkshell(const IPP& ipp, const ipc::ChatMessageLinkshell& message)
{
    TracyZoneScoped;

    if (CLinkshell* PLinkshell = linkshell::GetLinkshell(message.linkshellId))
    {
        // TODO: Linkshell 1 vs 2?
        PLinkshell->PushPacket(message.senderId, std::make_unique<CChatMessagePacket>(message.senderName, message.zoneId, MESSAGE_LINKSHELL, message.message, message.gmLevel));
    }
}

void IPCClient::handleMessage_ChatMessageUnity(const IPP& ipp, const ipc::ChatMessageUnity& message)
{
    TracyZoneScoped;

    if (CUnityChat* PUnityChat = unitychat::GetUnityChat(message.unityLeaderId))
    {
        PUnityChat->PushPacket(message.senderId, std::make_unique<CChatMessagePacket>(message.senderName, message.zoneId, MESSAGE_UNITY, message.message, message.gmLevel));
    }
}

void IPCClient::handleMessage_ChatMessageYell(const IPP& ipp, const ipc::ChatMessageYell& message)
{
    TracyZoneScoped;

    // clang-format off
    zoneutils::ForEachZone([&](CZone* PZone)
    {
        if (PZone->CanUseMisc(MISC_YELL))
        {
            PZone->ForEachChar([&](CCharEntity* PChar)
            {
                // Don't push to sender
                if (PChar->id != message.senderId)
                {
                    PChar->pushPacket(std::make_unique<CChatMessagePacket>(message.senderName, message.zoneId, MESSAGE_YELL, message.message, message.gmLevel));
                }
            });
        }
    });
    // clang-format on
}

void IPCClient::handleMessage_ChatMessageServerMessage(const IPP& ipp, const ipc::ChatMessageServerMessage& message)
{
    TracyZoneScoped;

    // clang-format off
    zoneutils::ForEachZone([&](CZone* PZone)
    {
        PZone->ForEachChar([&](CCharEntity* PChar)
        {
            PChar->pushPacket(std::make_unique<CChatMessagePacket>(message.senderName, message.zoneId, MESSAGE_SYSTEM_1, message.message, message.gmLevel));
        });
    });
    // clang-format on
}

void IPCClient::handleMessage_ChatMessageCustom(const IPP& ipp, const ipc::ChatMessageCustom& message)
{
    DebugIPCFmt("Received MessageTell message from {}, char {} to char {}: {}", ipp.toString(), message.senderName, message.recipientId, message.message);

    CCharEntity* PChar = zoneutils::GetChar(message.recipientId);
    if (PChar && PChar->status != STATUS_TYPE::DISAPPEAR && !jailutils::InPrison(PChar))
    {
        PChar->pushPacket(std::make_unique<CChatMessagePacket>(PChar, message.messageType, message.message, message.senderName));
    }
}

void IPCClient::handleMessage_PartyInvite(const IPP& ipp, const ipc::PartyInvite& message)
{
    TracyZoneScoped;

    if (CCharEntity* PInvitee = zoneutils::GetChar(message.inviteeId))
    {
        // make sure invitee isn't dead or in jail, they aren't a party member and don't already have an invite pending, and your party is not full
        if (PInvitee->isDead() ||
            jailutils::InPrison(PInvitee) ||
            PInvitee->InvitePending.id != 0 ||
            (PInvitee->PParty && message.inviteType == INVITE_PARTY) ||
            (message.inviteType == INVITE_ALLIANCE && (!PInvitee->PParty || PInvitee->PParty->GetLeader() != PInvitee || (PInvitee->PParty && PInvitee->PParty->m_PAlliance))))
        {
            message::send(ipc::MessageStandard{
                .charId  = message.inviteeId,
                .message = MsgStd::CannotInvite,
            });

            return;
        }

        if (PInvitee->getBlockingAid())
        {
            // Target is blocking assistance
            message::send(ipc::MessageSystem{
                .charId  = message.inviterId,
                .message = MsgStd::TargetIsCurrentlyBlocking,
            });

            // Interaction was blocked
            PInvitee->pushPacket<CMessageSystemPacket>(0, 0, MsgStd::BlockedByBlockaid);

            // You cannot invite that person at this time.
            message::send(ipc::MessageStandard{
                .charId  = message.inviteeId,
                .message = MsgStd::CannotInvite,
            });

            return;
        }

        if (PInvitee->StatusEffectContainer->HasStatusEffect(EFFECT_LEVEL_SYNC))
        {
            message::send(ipc::MessageStandard{
                .charId  = message.inviteeId,
                .message = MsgStd::CannotInviteLevelSync,
            });

            return;
        }

        PInvitee->InvitePending.id     = message.inviterId;
        PInvitee->InvitePending.targid = message.inviterTargId;

        PInvitee->pushPacket(std::make_unique<CPartyInvitePacket>(message.inviterId, message.inviterTargId, message.inviterName, INVITE_PARTY));
    }
}

void IPCClient::handleMessage_PartyInviteResponse(const IPP& ipp, const ipc::PartyInviteResponse& message)
{
    TracyZoneScoped;

    CCharEntity* PInviter = zoneutils::GetChar(message.inviterId);
    if (PInviter)
    {
        if (message.inviteAnswer == 0)
        {
            PInviter->pushPacket<CMessageStandardPacket>(PInviter, 0, 0, MsgStd::InvitationDeclined);
        }
        else
        {
            // both party leaders?
            const auto rset = db::preparedStmt("SELECT * FROM accounts_parties WHERE partyid <> 0 AND "
                                               "((charid = ? OR charid = ?) AND partyflag & ?)",
                                               message.inviterId, message.inviteeId, PARTY_LEADER);
            if (rset && rset->rowsCount() == 2)
            {
                if (PInviter->PParty)
                {
                    if (PInviter->PParty->m_PAlliance)
                    {
                        const auto rset2 = db::preparedStmt("SELECT * FROM accounts_parties WHERE allianceid <> 0 AND "
                                                            "allianceid = (SELECT allianceid FROM accounts_parties where "
                                                            "charid = ?) GROUP BY partyid",
                                                            message.inviterId);
                        if (rset2 && rset2->rowsCount() > 0 && rset2->rowsCount() < 3)
                        {
                            PInviter->PParty->m_PAlliance->addParty(message.inviteeId);
                        }
                        else
                        {
                            message::send(ipc::MessageStandard{
                                .charId  = message.inviterId,
                                .message = MsgStd::CannotBeProcessed,
                            });
                        }
                    }
                    else if (PInviter->PParty)
                    {
                        // make new alliance
                        CAlliance* PAlliance = new CAlliance(PInviter);
                        PAlliance->addParty(message.inviteeId);
                    }
                }
                else // Somehow, the inviter didn't have a party despite the database thinking they did.
                {
                    message::send(ipc::MessageStandard{
                        .charId  = message.inviterId,
                        .message = MsgStd::CannotBeProcessed,
                    });
                }
            }
            else
            {
                if (PInviter->PParty == nullptr)
                {
                    PInviter->PParty = new CParty(PInviter);
                }

                if (PInviter->PParty && PInviter->PParty->GetLeader() == PInviter)
                {
                    const auto rset2 = db::preparedStmt("SELECT * FROM accounts_parties WHERE partyid <> 0 AND charid = ?", message.inviteeId);
                    if (rset2 && rset2->rowsCount() == 0)
                    {
                        PInviter->PParty->AddMember(message.inviteeId);
                    }
                }
            }
        }
    }
}

void IPCClient::handleMessage_PartyReload(const IPP& ipp, const ipc::PartyReload& message)
{
    TracyZoneScoped;

    const auto rset = db::preparedStmt("SELECT charid FROM accounts_parties WHERE partyid = ?", message.partyId);
    if (rset && rset->rowsCount())
    {
        while (rset->next())
        {
            const auto charid = rset->get<uint32>("charid");
            if (CCharEntity* PChar = zoneutils::GetChar(charid))
            {
                PChar->ReloadPartyInc();
            }
        }
    }
}

void IPCClient::handleMessage_PartyDisband(const IPP& ipp, const ipc::PartyDisband& message)
{
    TracyZoneScoped;

    CParty* PParty = nullptr;

    const auto partyid = message.partyId;

    // TODO: When Party/Alliance gets a rewrite, make a zoneutils::ForEachParty or some other accessor to reduce the amount of iterations significantly.

    // clang-format off
    zoneutils::ForEachZone([partyid, &PParty](CZone* PZone)
    {
        PZone->ForEachChar([partyid, &PParty](CCharEntity* PChar)
        {
            if (PChar->PParty && PChar->PParty->GetPartyID() == partyid)
            {
                PParty = PChar->PParty;
                return;
            }
        });
        if (PParty)
        {
            return;
        }
    });
    if (PParty)
    {
        PParty->DisbandParty(false);
    }
    // clang-format on
}

void IPCClient::handleMessage_AllianceReload(const IPP& ipp, const ipc::AllianceReload& message)
{
    TracyZoneScoped;

    const auto rset = db::preparedStmt("SELECT charid FROM accounts_parties WHERE allianceid = ?", message.allianceId);
    if (rset && rset->rowsCount())
    {
        while (rset->next())
        {
            const auto charid = rset->get<uint32>("charid");
            if (CCharEntity* PChar = zoneutils::GetChar(charid))
            {
                PChar->ReloadPartyInc();
            }
        }
    }
}

void IPCClient::handleMessage_AllianceDissolve(const IPP& ipp, const ipc::AllianceDissolve& message)
{
    TracyZoneScoped;

    CAlliance* PAlliance = nullptr;

    const auto allianceid = message.allianceId;

    // TODO: When Party/Alliance gets a rewrite, make a zoneutils::ForEachAlliance or some other accessor to reduce the amount of iterations significantly.

    // clang-format off
    zoneutils::ForEachZone([allianceid, &PAlliance](CZone* PZone)
    {
        PZone->ForEachChar([allianceid, &PAlliance](CCharEntity* PChar)
        {
            if (PChar->PParty && PChar->PParty->m_PAlliance && PChar->PParty->m_PAlliance->m_AllianceID == allianceid)
            {
                PAlliance = PChar->PParty->m_PAlliance;
                return;
            }
        });
        if (PAlliance)
        {
            return;
        }
    });
    if (PAlliance)
    {
        PAlliance->dissolveAlliance(false);
    }
    // clang-format on
}

void IPCClient::handleMessage_PlayerKick(const IPP& ipp, const ipc::PlayerKick& message)
{
    TracyZoneScoped;

    // player was kicked and is no longer in alliance/party db -- they need a direct update.
    if (CCharEntity* PChar = zoneutils::GetChar(message.charId))
    {
        PChar->ReloadPartyInc();
    }
}

void IPCClient::handleMessage_MessageStandard(const IPP& ipp, const ipc::MessageStandard& message)
{
    TracyZoneScoped;

    if (CCharEntity* PChar = zoneutils::GetChar(message.charId))
    {
        PChar->pushPacket(std::make_unique<CMessageStandardPacket>(PChar, 0, 0, message.message));
    }
}

void IPCClient::handleMessage_MessageSystem(const IPP& ipp, const ipc::MessageSystem& message)
{
    TracyZoneScoped;

    if (CCharEntity* PChar = zoneutils::GetChar(message.charId))
    {
        PChar->pushPacket(std::make_unique<CMessageStandardPacket>(PChar, 0, 0, message.message));
    }
}

void IPCClient::handleMessage_LinkshellRankChange(const IPP& ipp, const ipc::LinkshellRankChange& message)
{
    TracyZoneScoped;

    if (CLinkshell* PLinkshell = linkshell::GetLinkshell(message.linkshellId))
    {
        PLinkshell->ChangeMemberRank(message.linkshellName, message.permission);
    }
}

void IPCClient::handleMessage_LinkshellRemove(const IPP& ipp, const ipc::LinkshellRemove& message)
{
    TracyZoneScoped;

    /*
    CCharEntity* PChar = zoneutils::GetCharByName(message.linkshellName); // TODO: Should this be memberName?

    if (PChar && PChar->PLinkshell1 && PChar->PLinkshell1->getID() == message.linkshellId)
    {
        uint8           kickerRank = ref<uint8>((uint8*)extra.data(), 28);
        CItemLinkshell* targetLS   = (CItemLinkshell*)PChar->getEquip(SLOT_LINK1);
        if (targetLS && (kickerRank == LSTYPE_LINKSHELL || (kickerRank == LSTYPE_PEARLSACK && targetLS->GetLSType() == LSTYPE_LINKPEARL)))
        {
            PChar->PLinkshell1->RemoveMemberByName(memberName,
                                                   (targetLS->GetLSType() == (uint8)LSTYPE_LINKSHELL ? (uint8)LSTYPE_PEARLSACK : kickerRank));
        }
    }
    else if (PChar && PChar->PLinkshell2 && PChar->PLinkshell2->getID() == ref<uint32>((uint8*)extra.data(), 24))
    {
        uint8           kickerRank = ref<uint8>((uint8*)extra.data(), 28);
        CItemLinkshell* targetLS   = (CItemLinkshell*)PChar->getEquip(SLOT_LINK2);
        if (targetLS && (kickerRank == LSTYPE_LINKSHELL || (kickerRank == LSTYPE_PEARLSACK && targetLS->GetLSType() == LSTYPE_LINKPEARL)))
        {
            PChar->PLinkshell2->RemoveMemberByName(memberName, kickerRank);
        }
    }
    */
}

void IPCClient::handleMessage_LinkshellSetMessage(const IPP& ipp, const ipc::LinkshellSetMessage& message)
{
    TracyZoneScoped;

    // TODO: This was originally piggybacking on the Linkshell chat messages, so we need to reimplement this
    //     : in here now

    ShowWarning("Not implemented");
}

void IPCClient::handleMessage_LuaFunction(const IPP& ipp, const ipc::LuaFunction& message)
{
    TracyZoneScoped;

    auto result = lua.safe_script(message.funcString);
    if (!result.valid())
    {
        sol::error err = result;
        ShowError("MSG_LUA_FUNCTION: error: %s: %s", err.what(), message.funcString.c_str());
    }
}

void IPCClient::handleMessage_KillSession(const IPP& ipp, const ipc::KillSession& message)
{
    TracyZoneScoped;

    map_session_data_t* sessionToDelete = nullptr;

    for (const auto [_, session] : map_session_list)
    {
        if (session->charID == message.charId)
        {
            sessionToDelete = session;
            break;
        }
    }

    if (sessionToDelete && sessionToDelete->blowfish.status == BLOWFISH_PENDING_ZONE)
    {
        DebugSockets(fmt::format("Closing session of charid {} on request of other process", message.charId));
        map_close_session(server_clock::now(), sessionToDelete);
    }
}

void IPCClient::handleMessage_RegionalEvent(const IPP& ipp, const ipc::RegionalEvent& message)
{
    TracyZoneScoped;

    switch (message.type)
    {
        case RegionalEventType::Conquest:
            conquest::HandleZMQMessage(message.subType, { message.payload.data(), message.payload.size() });
            break;
        default:
            ShowWarningFmt("Received unknown regional event type {} from {}", message.type, ipp.toString());
            break;
    }
}

void IPCClient::handleMessage_GMSendToZone(const IPP& ipp, const ipc::GMSendToZone& message)
{
    TracyZoneScoped;

    // TODO:
    /*
    CCharEntity* PChar = zoneutils::GetChar(message.targetId);
    if (PChar && PChar->loc.zone)
    {
        uint32 requester = ref<uint32>(message.requesterId);
        if (requester != 0)
        {
            char buf[30];
            std::memset(&buf[0], 0, sizeof(buf));

            ref<uint32>(&buf, 0)  = requester;
            ref<uint16>(&buf, 8)  = PChar->getZone();
            ref<float>(&buf, 10)  = PChar->loc.p.x;
            ref<float>(&buf, 14)  = PChar->loc.p.y;
            ref<float>(&buf, 18)  = PChar->loc.p.z;
            ref<uint8>(&buf, 22)  = PChar->loc.p.rotation;
            ref<uint32>(&buf, 23) = PChar->m_moghouseID;

            message::send(MSG_SEND_TO_ZONE, &buf, sizeof(buf), nullptr);
            break;
        }

        uint16 zoneId     = ref<uint16>((uint8*)extra.data(), 8);
        float  x          = ref<float>((uint8*)extra.data(), 10);
        float  y          = ref<float>((uint8*)extra.data(), 14);
        float  z          = ref<float>((uint8*)extra.data(), 18);
        uint8  rot        = ref<uint8>((uint8*)extra.data(), 22);
        uint32 moghouseID = ref<uint32>((uint8*)extra.data(), 23);

        PChar->updatemask = 0;

        PChar->m_moghouseID = 0;

        PChar->loc.p.x         = x;
        PChar->loc.p.y         = y;
        PChar->loc.p.z         = z;
        PChar->loc.p.rotation  = rot;
        PChar->loc.destination = zoneId;
        PChar->m_moghouseID    = moghouseID;
        PChar->loc.boundary    = 0;
        PChar->status          = STATUS_TYPE::DISAPPEAR;
        PChar->animation       = ANIMATION_NONE;
        PChar->clearPacketList();

        charutils::SendToZone(PChar, 2, zoneutils::GetZoneIPP(zoneId));
    }
    */
}

void IPCClient::handleMessage_GMSendToEntity(const IPP& ipp, const ipc::GMSendToEntity& message)
{
    TracyZoneScoped;

    // TODO:
    /*
    // Need to check which server we're on so we don't get null pointers
    bool toTargetServer = ref<bool>((uint8*)extra.data(), 0);
    bool spawnedOnly    = ref<bool>((uint8*)extra.data(), 1);

    if (toTargetServer) // This is going to the target's game server
    {
        CBaseEntity* Entity = zoneutils::GetEntity(ref<uint32>((uint8*)extra.data(), 6));

        if (Entity && Entity->loc.zone)
        {
            char buf[22];
            std::memset(&buf[0], 0, sizeof(buf));

            uint16 targetZone = ref<uint16>((uint8*)extra.data(), 2);
            uint16 playerZone = ref<uint16>((uint8*)extra.data(), 4);
            uint16 playerID   = ref<uint16>((uint8*)extra.data(), 10);

            float X = Entity->GetXPos();
            float Y = Entity->GetYPos();
            float Z = Entity->GetZPos();
            uint8 R = Entity->GetRotPos();

            ref<bool>(&buf, 1) = true; // Found, so initiate warp back on the requesting server

            if (Entity->status == STATUS_TYPE::DISAPPEAR)
            {
                if (spawnedOnly)
                {
                    ref<bool>(&buf, 1) = false; // Spawned only, so do not initiate warp
                }
                else
                {
                    // If entity not spawned, go to default location as listed in database
                    const char* query = "SELECT pos_x, pos_y, pos_z FROM mob_spawn_points WHERE mobid = %u";
                    auto        fetch = _sql->Query(query, Entity->id);

                    if (fetch != SQL_ERROR && _sql->NumRows() != 0)
                    {
                        while (_sql->NextRow() == SQL_SUCCESS)
                        {
                            X = _sql->GetFloatData(0);
                            Y = _sql->GetFloatData(1);
                            Z = _sql->GetFloatData(2);
                        }
                    }
                }
            }

            ref<bool>(&buf, 0)    = false;
            ref<uint16>(&buf, 2)  = playerZone;
            ref<uint16>(&buf, 4)  = playerID;
            ref<float>(&buf, 6)   = X;
            ref<float>(&buf, 10)  = Y;
            ref<float>(&buf, 14)  = Z;
            ref<uint8>(&buf, 18)  = R;
            ref<uint16>(&buf, 20) = targetZone;

            message::send(MSG_SEND_TO_ENTITY, &buf, sizeof(buf), nullptr);
            break;
        }
    }
    else // This is going to the player's game server
    {
        CCharEntity* PChar = zoneutils::GetChar(ref<uint16>((uint8*)extra.data(), 4));

        if (PChar && PChar->loc.zone)
        {
            if (ref<bool>((uint8*)extra.data(), 1))
            {
                PChar->loc.p.x         = ref<float>((uint8*)extra.data(), 6);
                PChar->loc.p.y         = ref<float>((uint8*)extra.data(), 10);
                PChar->loc.p.z         = ref<float>((uint8*)extra.data(), 14);
                PChar->loc.p.rotation  = ref<uint8>((uint8*)extra.data(), 18);
                PChar->loc.destination = ref<uint16>((uint8*)extra.data(), 20);

                PChar->m_moghouseID = 0;
                PChar->loc.boundary = 0;
                PChar->updatemask   = 0;

                PChar->status    = STATUS_TYPE::DISAPPEAR;
                PChar->animation = ANIMATION_NONE;

                PChar->clearPacketList();

                charutils::SendToZone(PChar, 2, zoneutils::GetZoneIPP(PChar->loc.destination));
            }
        }
    }
    */
}

void IPCClient::handleMessage_RPCSend(const IPP& ipp, const ipc::RPCSend& message)
{
    TracyZoneScoped;

    // TODO:
    /*
    // Extract data
    uint8*   data     = (uint8*)extra.data();
    uint16   sendZone = ref<uint16>(data, 2); // here
    uint16   recvZone = ref<uint16>(data, 4); // origin
    uint64_t slotKey  = ref<uint64_t>(data, 6);
    uint16   strSize  = ref<uint16>(data, 14);
    auto     sendStr  = std::string(data + 16, data + 16 + strSize);

    // Execute Lua & collect payload
    std::string payload = "";
    auto        result  = lua.safe_script(sendStr);
    if (result.valid() && result.return_count())
    {
        payload = result.get<std::string>(0);
    }

    // Reply w/ payload
    std::vector<uint8> packetData(16 + payload.size() + 1);

    ref<uint16>(packetData.data(), 2)   = recvZone; // origin
    ref<uint16>(packetData.data(), 4)   = sendZone; // here
    ref<uint64_t>(packetData.data(), 6) = slotKey;

    ref<uint16>(packetData.data(), 14) = (uint16)payload.size();
    std::memcpy(packetData.data() + 16, payload.data(), payload.size());

    packetData[packetData.size() - 1] = '\0';

    message::send(MSG_RPC_RECV, packetData.data(), packetData.size());
    */
}

void IPCClient::handleMessage_RPCRecv(const IPP& ipp, const ipc::RPCRecv& message)
{
    TracyZoneScoped;

    // TODO:
    /*
    uint8* data = (uint8*)extra.data();

    // No need for any of the zone id data now
    uint64_t slotKey = ref<uint64_t>(data, 6);
    uint16   strSize = ref<uint16>(data, 14);
    auto     payload = std::string(data + 16, data + 16 + strSize);

    auto maybeEntry = replyMap.find(slotKey);
    if (maybeEntry != replyMap.end())
    {
        auto& recvFunc = maybeEntry->second;
        auto  result   = recvFunc(payload);
        if (!result.valid())
        {
            sol::error err = result;
            ShowError("message::parse::MSG_RPC_RECV: %s", err.what());
        }
        replyMap.erase(slotKey);
    }
    */
}

void IPCClient::handleUnknownMessage(const IPP& ipp, const std::span<uint8_t> message)
{
    TracyZoneScoped;

    ShowWarningFmt("Received unknown message from {} with code {} and size {}", ipp.toString(), message[0], message.size());
}
