-----------------------------------
-- Area: Bhaflau_Thickets
-----------------------------------
zones = zones or {}

zones[xi.zone.BHAFLAU_THICKETS] =
{
    text =
    {
        NOTHING_HAPPENS               = 119,  -- Nothing happens...
        ITEM_CANNOT_BE_OBTAINED       = 6384, -- You cannot obtain the <item>. Come back after sorting your inventory.
        ITEM_OBTAINED                 = 6390, -- Obtained: <item>.
        GIL_OBTAINED                  = 6391, -- Obtained <number> gil.
        KEYITEM_OBTAINED              = 6393, -- Obtained key item: <keyitem>.
        WARHORSE_HOOFPRINT            = 6400, -- You find the hoofprint of a gigantic warhorse...
        FELLOW_MESSAGE_OFFSET         = 6419, -- I'm ready. I suppose.
        CARRIED_OVER_POINTS           = 7001, -- You have carried over <number> login point[/s].
        LOGIN_CAMPAIGN_UNDERWAY       = 7002, -- The [/January/February/March/April/May/June/July/August/September/October/November/December] <number> Login Campaign is currently underway!
        LOGIN_NUMBER                  = 7003, -- In celebration of your most recent login (login no. <number>), we have provided you with <number> points! You currently have a total of <number> points.
        MEMBERS_LEVELS_ARE_RESTRICTED = 7023, -- Your party is unable to participate because certain members' levels are restricted.
        FISHING_MESSAGE_OFFSET        = 7063, -- You can't fish here.
        DIG_THROW_AWAY                = 7076, -- You dig up <item>, but your inventory is full. You regretfully throw the <item> away.
        FIND_NOTHING                  = 7078, -- You dig and you dig, but find nothing.
        FOUND_ITEM_WITH_EASE          = 7153, -- It appears your chocobo found this item with ease.
        STAGING_GATE_CLOSER           = 7323, -- You must move closer.
        STAGING_GATE_INTERACT         = 7324, -- This gate guards an area under Imperial control.
        STAGING_GATE_MAMOOL           = 7326, -- Mamool Ja Staging Point.
        CANNOT_LEAVE                  = 7334, -- You cannot leave this area while in the possession of <keyitem>.
        RESPONSE                      = 7343, -- There is no response...
        YOU_HAVE_A_BADGE              = 7356, -- You have a %? Let me have a closer look at that...
        HAND_OVER_TO_IMMORTAL         = 7556, -- You hand over the % to the Immortal.
        YOUR_IMPERIAL_STANDING        = 7557, -- Your Imperial Standing has increased!
        HARVESTING_IS_POSSIBLE_HERE   = 7575, -- Harvesting is possible here if you have <item>.
        CANNOT_ENTER                  = 7598, -- You cannot enter at this time. Please wait a while before trying again.
        AREA_FULL                     = 7599, -- This area is fully occupied. You were unable to enter.
        MEMBER_NO_REQS                = 7603, -- Not all of your party members meet the requirements for this objective. Unable to enter area.
        MEMBER_TOO_FAR                = 7607, -- One or more party members are too far away from the entrance. Unable to enter area.
        WELLSPRING                    = 7661, -- The water in this spring is an unusual color...
        SHED_LEAVES                   = 7670, -- The ground is strewn with shed leaves...
        BLOOD_STAINS                  = 7672, -- The ground is smeared with bloodstains...
        DRAWS_NEAR                    = 7697, -- Something draws near!
        HOMEPOINT_SET                 = 7708, -- Home point set!
    },
    mob =
    {
        CHIGOES              =
        {
            ['Marid']        = utils.slice(GetTableOfIDs('Chigoe'), 1, 5), -- Entries 1-5 of the table (1-indexed, inclusive)
            ['Grand_Marid']  = utils.slice(GetTableOfIDs('Chigoe'), 1, 5), -- Entries 1-5 of the table (1-indexed, inclusive)
        },
        DEA                = GetFirstID('Dea'),
        EMERGENT_ELM       = GetFirstID('Emergent_Elm'),
        HARVESTMAN         = GetFirstID('Harvestman'),
        LIVIDROOT_AMOOSHAH = GetFirstID('Lividroot_Amooshah'),
        MAHISHASURA        = GetFirstID('Mahishasura'),
        NIS_PUK            = GetFirstID('Nis_Puk'),
        PLAGUE_CHIGOE      = GetFirstID('Plague_Chigoe'),
    },
    npc =
    {
        HARVESTING = GetTableOfIDs('Harvesting_Point'),
        HOOFPRINT  = GetFirstID('Warhorse_Hoofprint'),
    },
}

return zones[xi.zone.BHAFLAU_THICKETS]
