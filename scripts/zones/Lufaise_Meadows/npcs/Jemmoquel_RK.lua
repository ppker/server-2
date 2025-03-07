-----------------------------------
-- Area: Lufaise Meadows
--  NPC: Jemmoquel, R.K.
-- Outpost Conquest Guards
-- !pos -542.418 -7.124 -53.521 24
-----------------------------------
---@type TNpcEntity
local entity = {}

local guardNation = xi.nation.SANDORIA
local guardType   = xi.conq.guard.OUTPOST
local guardRegion = xi.region.TAVNAZIANARCH
local guardEvent  = 32763

entity.onTrade = function(player, npc, trade)
    xi.conq.overseerOnTrade(player, npc, trade, guardNation, guardType)
end

entity.onTrigger = function(player, npc)
    xi.conq.overseerOnTrigger(player, npc, guardNation, guardType, guardEvent, guardRegion)
end

entity.onEventUpdate = function(player, csid, option, npc)
    xi.conq.overseerOnEventUpdate(player, csid, option, guardNation)
end

entity.onEventFinish = function(player, csid, option, npc)
    xi.conq.overseerOnEventFinish(player, csid, option, guardNation, guardType, guardRegion)
end

return entity
