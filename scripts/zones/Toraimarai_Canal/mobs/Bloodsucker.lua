-----------------------------------
-- Area: Toraimarai Canal
--  Mob: Bloodsucker
-----------------------------------
---@type TMobEntity
local entity = {}

entity.onMobDeath = function(mob, player, optParams)
    xi.regime.checkRegime(player, mob, 620, 2, xi.regime.type.GROUNDS)
end

return entity
