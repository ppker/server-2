-----------------------------------
-- Area: Northern San d'Oria
--  NPC: Narcheral
-- Starts and Finishes Quest: Messenger from Beyond, Prelude of Black and White (Finish), Pieuje's Decision (Finish)
-- !pos 129 -11 126 231
-----------------------------------
local ID = zones[xi.zone.NORTHERN_SAN_DORIA]
-----------------------------------
---@type TNpcEntity
local entity = {}

entity.onTrade = function(player, npc, trade)
    if player:getQuestStatus(xi.questLog.SANDORIA, xi.quest.id.sandoria.PRELUDE_OF_BLACK_AND_WHITE) == xi.questStatus.QUEST_ACCEPTED then
        if
            trade:hasItemQty(xi.item.CANTEEN_OF_YAGUDO_HOLY_WATER, 1) and
            trade:hasItemQty(xi.item.MOCCASINS, 1) and
            trade:getItemCount() == 2
        then
            -- Trade Yagudo Holy Water & Moccasins
            player:startEvent(691) -- Finish quest "Prelude of Black and White"
        end
    elseif player:getQuestStatus(xi.questLog.SANDORIA, xi.quest.id.sandoria.PIEUJES_DECISION) == xi.questStatus.QUEST_ACCEPTED then
        if
            trade:hasItemQty(xi.item.TAVNAZIAN_MASK, 1) and
            trade:getItemCount() == 1
        then
            player:startEvent(692) -- Finish quest "Pieuje's Decision"
        end
    end
end

entity.onEventFinish = function(player, csid, option, npc)
    if csid == 691 then
        if player:getFreeSlotsCount() == 0 then
            player:messageSpecial(ID.text.ITEM_CANNOT_BE_OBTAINED, xi.item.HEALERS_DUCKBILLS) -- Healer's Duckbills
        else
            player:addItem(xi.item.HEALERS_DUCKBILLS)
            player:messageSpecial(ID.text.ITEM_OBTAINED, xi.item.HEALERS_DUCKBILLS) -- Healer's Duckbills
            player:tradeComplete()
            player:addFame(xi.fameArea.SANDORIA, 40)
            player:completeQuest(xi.questLog.SANDORIA, xi.quest.id.sandoria.PRELUDE_OF_BLACK_AND_WHITE)
        end
    elseif csid == 692 then
        if player:getFreeSlotsCount() == 0 then
            player:messageSpecial(ID.text.ITEM_CANNOT_BE_OBTAINED, xi.item.HEALERS_BLIAUT) -- Healer's Bliaut
        else
            player:addTitle(xi.title.PARAGON_OF_WHITE_MAGE_EXCELLENCE)
            player:setCharVar('pieujesDecisionCS', 0)
            player:addItem(xi.item.HEALERS_BLIAUT)
            player:messageSpecial(ID.text.ITEM_OBTAINED, xi.item.HEALERS_BLIAUT) -- Healer's Bliaut
            player:tradeComplete()
            player:addFame(xi.fameArea.SANDORIA, 60)
            player:completeQuest(xi.questLog.SANDORIA, xi.quest.id.sandoria.PIEUJES_DECISION)
        end
    end
end

return entity
