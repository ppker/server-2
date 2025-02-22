/*
===========================================================================

  Copyright (c) 2023 LandSandBoat Dev Teams

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

#include "conquest_system.h"

#include "ipc_server.h"

#include "common/database.h"

ConquestSystem::ConquestSystem(WorldServer& worldServer)
: worldServer_(worldServer)
{
}

bool ConquestSystem::handleMessage(uint8 messageType, HandleableMessage&& message)
{
    const auto conquestMsgType = static_cast<ConquestMessage>(messageType);
    switch (conquestMsgType)
    {
        case ConquestMessage::M2W_GM_WeeklyUpdate:
        {
            updateWeekConquest();
            return true;
        }
        break;
        case ConquestMessage::M2W_GM_ConquestUpdate:
        {
            // Send influence data to the requesting map server
            sendInfluencesMsg(true, message.ipp.getIPP());
            return true;
        }
        break;
        case ConquestMessage::M2W_AddInfluencePoints:
        {
            int32  points = 0;
            uint32 nation = 0;
            uint8  region = 0;
            std::memcpy(&points, message.payload.data() + 2, sizeof(int32));
            std::memcpy(&nation, message.payload.data() + 6, sizeof(uint32));
            std::memcpy(&region, message.payload.data() + 10, sizeof(uint8));

            // We update influence but do not immediately send this update to all map servers
            // Influence updates are sent periodically via time_server instead.
            // It is okay for map servers to be eventually consistent.
            updateInfluencePoints(points, nation, (REGION_TYPE)region);
            return true;
        }
        break;
        default:
        {
            ShowWarningFmt("Message: unknown conquest type message received: {} from {}",
                           conquestMsgType,
                           message.ipp.toString());
        }
        break;
    }

    return false;
}

void ConquestSystem::sendTallyStartMsg()
{
    // 1- Send message to all zones. We are starting update.

    // queue_message_broadcast(MSG_WORLD2MAP_REGIONAL_EVENT, &dataMsg);
    worldServer_.ipcServer_->sendMessage(IPP(),
                                         ipc::RegionalEvent{
                                             .type    = RegionalEventType::Conquest,
                                             .subType = ConquestMessage::W2M_WeeklyUpdateStart,
                                             // No payload
                                         });
}

void ConquestSystem::sendInfluencesMsg(bool shouldUpdateZones, uint64 ipp)
{
    const auto influences = getRegionalInfluences();

    worldServer_.ipcServer_->sendMessage(IPP(ipp),
                                         ipc::RegionalEvent{
                                             .type    = RegionalEventType::Conquest,
                                             .subType = ConquestMessage::W2M_BroadcastInfluencePoints,
                                             .payload = ipc::toBytes(ConquestInfluenceUpdate{
                                                 .shouldUpdateZones = shouldUpdateZones,
                                                 .influences        = influences,
                                             }),
                                         });
}

void ConquestSystem::sendRegionControlsMsg(ConquestMessage msgType, uint64 ipp)
{
    // 2- Serialize regional controls with the following schema:
    // - REGIONALMSGTYPE
    // - CONQUESTMSGTYPE
    // - region controls array size
    // - For N elements we have:
    //      - current control (uint8)
    //      - prev control (uint8)
    const auto regionControls = getRegionControls();

    // Header length is the type + subtype + region control size + size of the size_t
    const std::size_t headerLength = 2 * sizeof(uint8) + sizeof(std::size_t);
    const std::size_t dataLen      = headerLength + sizeof(region_control_t) * regionControls.size();
    const uint8*      data         = new uint8[dataLen];

    // Regional event type + conquest msg type
    ref<uint8>((uint8*)data, 0) = RegionalEventType::Conquest;
    ref<uint8>((uint8*)data, 1) = msgType;

    // Region controls array
    ref<std::size_t>((uint8*)data, 2) = regionControls.size();
    for (std::size_t i = 0; i < regionControls.size(); i++)
    {
        // Everything is offset by i*size of region control struct + headerLength
        const std::size_t offset             = headerLength + sizeof(region_control_t) * i;
        ref<uint8>((uint8*)data, offset)     = regionControls[i].current;
        ref<uint8>((uint8*)data, offset + 1) = regionControls[i].prev;
    }

    // 3- Create ZMQ Message and queue it
    zmq::message_t dataMsg = zmq::message_t(dataLen);
    memcpy(dataMsg.data(), data, dataLen);

    if (ipp == 0xFFFF)
    {
        // queue_message_broadcast(MSG_WORLD2MAP_REGIONAL_EVENT, &dataMsg);
    }
    else
    {
        // queue_message(ipp, MSG_WORLD2MAP_REGIONAL_EVENT, &dataMsg);
    }
}

bool ConquestSystem::updateInfluencePoints(int points, unsigned int nation, REGION_TYPE region)
{
    if (region == REGION_TYPE::UNKNOWN)
    {
        return false;
    }

    std::string query = "SELECT sandoria_influence, bastok_influence, windurst_influence, beastmen_influence FROM conquest_system WHERE region_id = ?";

    auto rset = db::preparedStmt(query.c_str(), static_cast<uint8>(region));
    if (!rset || rset->rowsCount() == 0 || !rset->next())
    {
        return false;
    }

    int influences[4] = {
        rset->get<int>("sandoria_influence"),
        rset->get<int>("bastok_influence"),
        rset->get<int>("windurst_influence"),
        rset->get<int>("beastmen_influence"),
    };

    if (influences[nation] == 5000)
    {
        return false;
    }

    auto lost = 0;
    for (auto i = 0u; i < 4; ++i)
    {
        if (i == nation)
        {
            continue;
        }

        auto loss = std::min<int>(points * influences[i] / (5000 - influences[nation]), influences[i]);
        influences[i] -= loss;
        lost += loss;
    }

    influences[nation] += lost;

    auto rset2 = db::preparedStmt("UPDATE conquest_system SET sandoria_influence = ?, bastok_influence = ?, "
                                  "windurst_influence = ?, beastmen_influence = ? WHERE region_id = ?",
                                  influences[0], influences[1], influences[2], influences[3], static_cast<uint8>(region));

    return !rset2;
}

void ConquestSystem::updateWeekConquest()
{
    TracyZoneScoped;

    // 1- Notify all zones that tally started
    sendTallyStartMsg();

    // 2- Do the actual db update
    auto query = "UPDATE conquest_system SET region_control = "
                 "IF(sandoria_influence > bastok_influence AND sandoria_influence > windurst_influence AND "
                 "sandoria_influence > beastmen_influence, 0, "
                 "IF(bastok_influence > sandoria_influence AND bastok_influence > windurst_influence AND "
                 "bastok_influence > beastmen_influence, 1, "
                 "IF(windurst_influence > bastok_influence AND windurst_influence > sandoria_influence AND "
                 "windurst_influence > beastmen_influence, 2, 3)))";

    auto rset = db::preparedStmt(query);
    if (!rset)
    {
        ShowError("handleWeeklyUpdate() failed");
    }

    // 3- Send tally end Msg
    sendRegionControlsMsg(ConquestMessage::W2M_WeeklyUpdateEnd);
}

void ConquestSystem::updateHourlyConquest()
{
    sendInfluencesMsg(true);
}

void ConquestSystem::updateVanaHourlyConquest()
{
    sendInfluencesMsg(false);
}

auto ConquestSystem::getRegionalInfluences() -> std::vector<influence_t> const
{
    const auto rset = db::preparedStmt("SELECT sandoria_influence, bastok_influence, windurst_influence, beastmen_influence FROM conquest_system");

    std::vector<influence_t> influences;
    if (rset && rset->rowsCount())
    {
        while (rset->next())
        {
            influence_t influence{};
            influence.sandoria_influence = rset->get<uint16>("sandoria_influence");
            influence.bastok_influence   = rset->get<uint16>("bastok_influence");
            influence.windurst_influence = rset->get<uint16>("windurst_influence");
            influence.beastmen_influence = rset->get<uint16>("beastmen_influence");
            influences.emplace_back(influence);
        }
    }

    return influences;
}

auto ConquestSystem::getRegionControls() -> std::vector<region_control_t> const
{
    const auto rset = db::preparedStmt("SELECT region_control, region_control_prev FROM conquest_system");

    std::vector<region_control_t> controllers;
    if (rset && rset->rowsCount())
    {
        while (rset->next())
        {
            region_control_t regionControl{};
            regionControl.current = rset->get<uint8>("region_control");
            regionControl.prev    = rset->get<uint8>("region_control_prev");
            controllers.emplace_back(regionControl);
        }
    }

    return controllers;
}
