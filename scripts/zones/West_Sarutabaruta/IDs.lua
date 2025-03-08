-----------------------------------
-- Area: West_Sarutabaruta
-----------------------------------
zones = zones or {}

zones[xi.zone.WEST_SARUTABARUTA] =
{
    text =
    {
        NOTHING_HAPPENS               = 119,   -- Nothing happens...
        ITEM_CANNOT_BE_OBTAINED       = 6384,  -- You cannot obtain the <item>. Come back after sorting your inventory.
        ITEM_OBTAINED                 = 6390,  -- Obtained: <item>.
        GIL_OBTAINED                  = 6391,  -- Obtained <number> gil.
        KEYITEM_OBTAINED              = 6393,  -- Obtained key item: <keyitem>.
        KEYITEM_LOST                  = 6394,  -- Lost key item: <keyitem>.
        NOTHING_OUT_OF_ORDINARY       = 6404,  -- There is nothing out of the ordinary here.
        FELLOW_MESSAGE_OFFSET         = 6419,  -- I'm ready. I suppose.
        CARRIED_OVER_POINTS           = 7001,  -- You have carried over <number> login point[/s].
        LOGIN_CAMPAIGN_UNDERWAY       = 7002,  -- The [/January/February/March/April/May/June/July/August/September/October/November/December] <number> Login Campaign is currently underway!
        LOGIN_NUMBER                  = 7003,  -- In celebration of your most recent login (login no. <number>), we have provided you with <number> points! You currently have a total of <number> points.
        MEMBERS_LEVELS_ARE_RESTRICTED = 7023,  -- Your party is unable to participate because certain members' levels are restricted.
        FISHING_MESSAGE_OFFSET        = 7067,  -- You can't fish here.
        DIG_THROW_AWAY                = 7080,  -- You dig up <item>, but your inventory is full. You regretfully throw the <item> away.
        FIND_NOTHING                  = 7082,  -- You dig and you dig, but find nothing.
        FOUND_ITEM_WITH_EASE          = 7157,  -- It appears your chocobo found this item with ease.
        CONQUEST_BASE                 = 7167,  -- Tallying conquest results...
        SIGN_1                        = 7396,  -- Northeast: Central Tower, Horutoto Ruins Northwest: Giddeus South: Port Windurst
        SIGN_3                        = 7397,  -- East: East Sarutabaruta West: Giddeus
        SIGN_5                        = 7398,  -- Northeast: Central Tower, Horutoto Ruins East: East Sarutabaruta West: Giddeus
        SIGN_7                        = 7399,  -- South: Windurst East: East Sarutabaruta
        SIGN_9                        = 7400,  -- West: Giddeus North: East Sarutabaruta South: Windurst
        SIGN_11                       = 7401,  -- North: East Sarutabaruta Southeast: Windurst
        SIGN_13                       = 7402,  -- East: Port Windurst West: West Tower, Horutoto Ruins
        SIGN_15                       = 7403,  -- East: East Sarutabaruta West: Giddeus Southeast: Windurst
        SIGN_17                       = 7404,  -- Northwest: Northwest Tower, Horutoto Ruins  East: Outpost Southwest: Giddeus
        PAORE_KUORE_DIALOG            = 7406,  -- Welcome to Windurst! Proceed through this gateway to entaru Port Windurst.
        KOLAPO_OILAPO_DIALOG          = 7407,  -- Hi-diddly-diddly! This is the gateway to Windurst! The grasslands you're on now are known as West Sarutabaruta.
        MAATA_ULAATA                  = 7408,  -- Hello-wello! This is the central tower of the Horutoto Ruins. It's one of the several ancient-wancient magic towers which dot these grasslands.
        IPUPU_DIALOG                  = 7411,  -- I decided to take a little strolly-wolly, but before I realized it, I found myself way out here! Now I am sorta stuck... Woe is me!
        FROST_DEPOSIT_TWINKLES        = 7418,  -- A frost deposit at the base of the tree twinkles in the starlight.
        MELT_BARE_HANDS               = 7420,  -- It looks like it would melt if you touched it with your bare hands...
        HARVESTING_IS_POSSIBLE_HERE   = 7456,  -- Harvesting is possible here if you have <item>.
        CONQUEST                      = 7472,  -- You've earned conquest points!
        GARRISON_BASE                 = 7826,  -- Hm? What is this? %? How do I know this is not some [San d'Orian/Bastokan/Windurstian] trick?
        PLAYER_OBTAINS_ITEM           = 7873,  -- <name> obtains <item>!
        UNABLE_TO_OBTAIN_ITEM         = 7874,  -- You were unable to obtain the item.
        PLAYER_OBTAINS_TEMP_ITEM      = 7875,  -- <name> obtains the temporary item: <item>!
        ALREADY_POSSESS_TEMP          = 7876,  -- You already possess that temporary item.
        NO_COMBINATION                = 7881,  -- You were unable to enter a combination.
        VOIDWALKER_DESPAWN            = 7912,  -- The monster fades before your eyes, a look of disappointment on its face.
        REGIME_REGISTERED             = 10202, -- New training regime registered!
        DONT_SWAP_JOBS                = 10203, -- hanging your job will result in the cancellation of your current training regime.
        REGIME_CANCELED               = 10204, -- Training regime canceled.
        VOIDWALKER_NO_MOB             = 11362, -- The <keyitem> quivers ever so slightly, but emits no light. There seem to be no monsters in the area.
        VOIDWALKER_MOB_TOO_FAR        = 11363, -- The <keyitem> quivers ever so slightly and emits a faint light. There seem to be no monsters in the immediate vicinity.
        VOIDWALKER_MOB_HINT           = 11364, -- The <keyitem> resonates [feebly/softly/solidly/strongly/very strongly/furiously], sending a radiant beam of light lancing towards a spot roughly <number> [yalm/yalms] [east/southeast/south/southwest/west/northwest/north/northeast] of here.
        VOIDWALKER_SPAWN_MOB          = 11365, -- A monster materializes out of nowhere!
        VOIDWALKER_UPGRADE_KI_1       = 11367, -- The <keyitem> takes on a slightly deeper hue and becomes <keyitem>!
        VOIDWALKER_UPGRADE_KI_2       = 11368, -- The <keyitem> takes on a deeper, richer hue and becomes <keyitem>!
        VOIDWALKER_BREAK_KI           = 11369, -- The <keyitem> shatters into tiny fragments.
        VOIDWALKER_OBTAIN_KI          = 11370, -- Obtained key item: <keyitem>!
        COMMON_SENSE_SURVIVAL         = 12353, -- It appears that you have arrived at a new survival guide provided by the Adventurers' Mutual Aid Network. Common sense dictates that you should now be able to teleport here from similar tomes throughout the world.
    },

    mob =
    {
        NUNYENUNC   = GetFirstID('Nunyenunc'),
        TOM_TIT_TAT = GetFirstID('Tom_Tit_Tat'),
        VOIDWALKER  =
        {
            [xi.keyItem.CLEAR_ABYSSITE] =
            {
                17248624, -- Raker bee
                17248623, -- Raker bee
                17248622, -- Raker bee
                17248621, -- Raker bee
                17248620,  -- Rummager beetle
                17248619,  -- Rummager beetle
                17248618,  -- Rummager beetle
                17248617,  -- Rummager beetle
            },

            [xi.keyItem.COLORFUL_ABYSSITE] =
            {
                17248616,  -- Jyeshtha
                17248615, -- Farruca Fly
            },

            [xi.keyItem.BROWN_ABYSSITE] =
            {
                17248614, -- Orcus
            },

            [xi.keyItem.BLACK_ABYSSITE] =
            {
                17248613, -- Yilbegan
            },
        }
    },

    npc =
    {
        SIGNPOST_OFFSET = GetFirstID('Signpost'),
        OVERSEER_BASE   = GetFirstID('Naguipeillont_RK'),
        HARVESTING      = GetTableOfIDs('Harvesting_Point'),
    },
}

return zones[xi.zone.WEST_SARUTABARUTA]
