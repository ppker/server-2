-----------------------------------
-- Area: Arrapago_Reef
-----------------------------------
zones = zones or {}

zones[xi.zone.ARRAPAGO_REEF] =
{
    text =
    {
        NOTHING_HAPPENS               = 119,  -- Nothing happens...
        ITEM_CANNOT_BE_OBTAINED       = 6384, -- You cannot obtain the <item>. Come back after sorting your inventory.
        ITEM_OBTAINED                 = 6390, -- Obtained: <item>.
        GIL_OBTAINED                  = 6391, -- Obtained <number> gil.
        KEYITEM_OBTAINED              = 6393, -- Obtained key item: <keyitem>.
        NOTHING_OUT_OF_ORDINARY       = 6404, -- There is nothing out of the ordinary here.
        CARRIED_OVER_POINTS           = 7001, -- You have carried over <number> login point[/s].
        LOGIN_CAMPAIGN_UNDERWAY       = 7002, -- The [/January/February/March/April/May/June/July/August/September/October/November/December] <number> Login Campaign is currently underway!
        LOGIN_NUMBER                  = 7003, -- In celebration of your most recent login (login no. <number>), we have provided you with <number> points! You currently have a total of <number> points.
        MEMBERS_LEVELS_ARE_RESTRICTED = 7023, -- Your party is unable to participate because certain members' levels are restricted.
        FISHING_MESSAGE_OFFSET        = 7063, -- You can't fish here.
        STAGING_GATE_CLOSER           = 7323, -- You must move closer.
        STAGING_GATE_INTERACT         = 7324, -- This gate guards an area under Imperial control.
        STAGING_GATE_ILRUSI           = 7329, -- Ilrusi Atoll Staging Point.
        CANNOT_LEAVE                  = 7334, -- You cannot leave this area while in the possession of <keyitem>.
        RESPONSE                      = 7343, -- There is no response...
        DOOR_IS_LOCKED                = 7472, -- The door is locked.  You might be able to open it with <item>.
        DOOR_IS_LOCKED2               = 7473, -- The door is locked.  You might be able to open it with <item> or <item>.
        KEY_BREAKS                    = 7474, -- The <item> breaks!
        YOU_UNLOCK_DOOR               = 7477, -- You unlock the door from the inside.
        LOCK_SUCCESS                  = 7478, -- <name> successfully opened the lock with the <item>!
        LOCK_FAIL                     = 7479, -- <name> failed to open the lock with the <item>...
        PARTY_MEMBERS_HAVE_FALLEN     = 7898, -- All party members have fallen in battle. Now leaving the battlefield.
        THE_PARTY_WILL_BE_REMOVED     = 7905, -- If all party members' HP are still zero after # minute[/s], the party will be removed from the battlefield.
        YOU_NO_REQS                   = 7909, -- You do not meet the requirements to enter the battlefield with your party members. Access is denied.
        YOUR_IMPERIAL_STANDING        = 7925, -- Your Imperial Standing has increased!
        SPINE_CHILL                   = 8393, -- You feel a chill run down your spine!
        HAND_OVER_TO_IMMORTAL         = 8451, -- You hand over the % to the Immortal.
        CANNOT_ENTER                  = 8470, -- You cannot enter at this time. Please wait a while before trying again.
        AREA_FULL                     = 8471, -- This area is fully occupied. You were unable to enter.
        MEMBER_NO_REQS                = 8475, -- Not all of your party members meet the requirements for this objective. Unable to enter area.
        MEMBER_TOO_FAR                = 8479, -- One or more party members are too far away from the entrance. Unable to enter area.
        MEDUSA_ENGAGE                 = 8583, -- Foolish two-legs... Have you forgotten the terrible power of the gorgons you created? It is time you were reminded...
        MEDUSA_DEATH                  = 8584, -- No... I cannot leave my sisters...
        GLITTERING_FRAGMENTS          = 8907, -- Minute glittering fragments are scattered all over...
        STIFLING_STENCH               = 8920, -- A stifling stench pervades the air...
        SLIMY_TOUCH                   = 8925, -- The ground here is slimy to the touch...
        FLUTTERY_OBJECTS              = 8931, -- Light, fluttery objects litter the ground...
        DRAWS_NEAR                    = 8936, -- Something draws near!
        COMMON_SENSE_SURVIVAL         = 9807, -- It appears that you have arrived at a new survival guide provided by the Adventurers' Mutual Aid Network. Common sense dictates that you should now be able to teleport here from similar tomes throughout the world.
    },
    mob =
    {
        APKALLU_NPC           = GetFirstID('Arrapago_Apkallu'),
        BLOODY_BONES          = GetFirstID('Bloody_Bones'),
        BUKKI                 = GetFirstID('Bukki'),
        MEDUSA                = GetFirstID('Medusa'),
        LIL_APKALLU           = GetFirstID('Lil_Apkallu'),
        VELIONIS              = GetFirstID('Velionis'),
        ZAREEHKL_THE_JUBILANT = GetFirstID('Zareehkl_the_Jubilant'),
        NUHN                  = GetFirstID('Nuhn'),
    },
    npc =
    {
    },
}

return zones[xi.zone.ARRAPAGO_REEF]
