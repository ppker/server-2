-----------------------------------
-- Area: Misareaux_Coast
--  Mob: Fomor Red Mage
-----------------------------------
mixins = { require('scripts/mixins/fomor_hate') }
-----------------------------------
---@type TMobEntity
local entity = {}

entity.onMobDeath = function(mob, player, optParams)
end

return entity
