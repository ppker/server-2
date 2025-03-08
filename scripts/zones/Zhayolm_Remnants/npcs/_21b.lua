-----------------------------------
-- Door
-- 4th Floor Exit to South Portal
-- !pos -380 -2 -600
-----------------------------------
local ID = zones[xi.zone.ZHAYOLM_REMNANTS]
-----------------------------------

---@type TNpcEntity
local entity = {}

entity.onTrigger = function(player, npc)
    if npc:getLocalVar('unSealed') == 1 then
        player:startEvent(300)
    else
        player:messageSpecial(ID.text.DOOR_IS_SEALED)
    end
end

entity.onEventFinish = function(player, csid, option, npc)
    if csid == 300 and option == 1 then
        if not xi.salvage.onDoorOpen(npc) then
            player:messageSpecial(ID.text.DOOR_IS_SEALED)
        end
    end
end

return entity
