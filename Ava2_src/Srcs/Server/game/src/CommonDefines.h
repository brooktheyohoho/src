#pragma once

#define FULL_MAX_YANG
#define ENABLE_NEW_NAME_ITEM
#define ENABLE_NEW_PET_SYSTEM
#define NEW_COMMON_DROP
#define ENABLE_SHOW_CHEST_DROP
#define NEW_BONUS
#define ENABLE_BIYOLOG
#define ENABLE_DEFENSAWE_SHIP
#define ENABLE_RARITY
#ifdef ENABLE_RARITY
	// These functions when value 0 which things is close.
	#define CLOSE_RARITY_APPLY_BONUS
	#define CLOSE_RARITY_ATTR_BONUS
	#define CLOSE_RARITY_METIN_BONUS
	//#define GIVING_RANDOMLY_RARITY_VALUE//if this define close giving every time %100 percent.
#endif
#define __CHAT_SETTINGS__
#define ENABLE_EVENT_MANAGER
#define ENABLE_BATTLE_PASS
#define ENABLE_AUTO_SELECT_SKILL
#define ENABLE_DRAGON_LAIR
#define ENABLE_CUBE_RENEWAL_COPY

#define ENABLE_ITEMSHOP
#define ENABLE_ITEMSHOP_TO_INVENTORY//buying item directly from inventory


#define DISABLE_OLD_MINUS
#define ENABLE_CHANNEL_CHANGE

#define dracaryS_DUNGEON_LIB

#define ENABLE_DS_GRADE_MYTH

//////////////////////////////////////////////////////////////////////////
// ### General Features ###
//#define ENABLE_QUEST_CATEGORY
#define ENABLE_D_NJGUILD
#define ENABLE_FULL_NOTICE
#define ENABLE_NEWSTUFF
#define ENABLE_PORT_SECURITY
#define ENABLE_NEW_AUTOPOTION
#define ENABLE_BELT_INVENTORY_EX
enum eCommonDefines {
	MAP_ALLOW_LIMIT = 40, // 32 default
};
// ### General Features ###
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// ### CommonDefines Systems ###
#define ENABLE_WOLFMAN_CHARACTER
#ifdef ENABLE_WOLFMAN_CHARACTER
#define USE_MOB_BLEEDING_AS_POISON
#define USE_MOB_CLAW_AS_DAGGER
// #define USE_ITEM_BLEEDING_AS_POISON
// #define USE_ITEM_CLAW_AS_DAGGER
//#define USE_WOLFMAN_STONES
//#define USE_WOLFMAN_BOOKS
#endif

#define ENABLE_PLAYER_PER_ACCOUNT5
#define ENABLE_DICE_SYSTEM
#define ENABLE_EXTEND_INVEN_SYSTEM
#define ENABLE_2LOOT

#define ENABLE_MOUNT_COSTUME_SYSTEM
#define ENABLE_WEAPON_COSTUME_SYSTEM

// #define ENABLE_MAGIC_REDUCTION_SYSTEM
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
// #define USE_MAGIC_REDUCTION_STONES
#endif
#define ENABLE_MULTILANGUAGE_SYSTEM
#define __SEND_TARGET_INFO__
#define ENABLE_EXTENDED_ITEMNAME_ON_GROUND

//#define WJ_COMBAT_ZONE // Enable the combat zone
//#define WJ_COMBAT_ZONE_HIDE_INFO_USER // If you have this enabled on server, players can't use chat and whisper on combat zone map. (Disable with "//")

//#define ENABLE_FEATURES_REFINE_SYSTEM
#define ENABLE_BIOLOG_SYSTEM
#define WJ_ENABLE_TRADABLE_ICON

#define FULL_YANG // Enable support for yang type long long

/* OFFLINE SHOPS  
#define OFFLINE_SHOP // Offline shops system
#define GIFT_SYSTEM // gift system enable
#define SHOP_TIME_REFRESH 1*60 // time for cycle checking older shops
#define SHOP_BLOCK_GAME99 //Blocking create shops on channel 99
//#define SHOP_DISTANCE // Show shops in pos distance like WoM2
#define SHOP_AUTO_CLOSE //Enable auto closing shop after sell last item
#define SHOP_ONLY_ALLOWED_INDEX //Enable limiting for other map index which is not defined in player.shop_limit
//#define SHOP_HIDE_NAME // Enable hidding shop npc names like "Player's shop"
#define SHOP_GM_PRIVILEGES GM_IMPLEMENTOR //Minimum GM privileges to using Shop GM Panel
// #define NO_YANG_ON_GIFT_SYSTEM // no recoge el yang del sistema de gift, solo del banco
*/


//#define UNLOCK_INVENTORY // item para inventario
//#define CHANGE_CHANNEL
//#define __HIGHLIGHT_SYSTEM__ // disabled because its bug
//#define ENABLE_MOUNT_SYSTEM
#define MOUNT_SYSTEM_BONUS
//#define SEACH_PRIVATE_SHOP
//#define STONE_INTO_COSTUMES // piedras en atuendos (no funciona)
#define ADD_BONUS_INTO_COSTUMES
#ifdef ADD_BONUS_INTO_COSTUMES
	#define ADD_NEW_BONUS_INTO_COSTUME
#endif
#define HORSE_FIX
#define ENABLE_IGNORE_LOWER_BUFFS
//#define ENABLE_SORT_INVEN
#define ENABLE_PLAYERS_ONLINE
//#define __NEWPET_SYSTEM__
#define __SASH_SYSTEM__
#ifdef __SASH_SYSTEM__
	#define FIX_19_PERCENT
#endif
#define __ANTI_EXP__
#define FIX_SyncPosition
//#define ENABLE_MANAGER_BANK_SYSTEM
#define ENABLE_SPECIAL_STORAGE

#define CHANGE_DS_ATRIBUTES
#define ENABLE_SORT_INVENTORY
#define ENABLE_NEW_AFFECT_POTION
#define ENABLE_SUPPORT_SYSTEM
// ### CommonDefines Systems ###
#define ENABLE_CALENDAR_SYSTEM
#define EXCHANGE_LOG
#define SHOW_DROPS_IN_DB
#define SHOUT_LOG_EXPANDED 
#define ENABLE_BUY_ITEMS_WORLDARD
#define ENABLE_DUNGEON_INFO
//////////////////////////////////////////////////////////////////////////
#define __VERSION_162__
#ifdef __VERSION_162__
	#define HEALING_SKILL_VNUM 265
#endif

#define __NEW_EVENTS__
#ifdef __NEW_EVENTS__
	#define __KINGDOMS_WAR__
#endif
#define ENABLE_AUTOMATIC_EVENT
#define ENABLE_NEW_OX_FEATURES
#define ENABLE_AUTOMATIC_PICK_UP_SYSTEM

#define ENABLE_HIDE_COSTUME_SYSTEM
#define ENABLE_HIDE_COSTUME_SYSTEM_ACCE
#define ENABLE_HIDE_COSTUME_SYSTEM_WEAPON_COSTUME
//#define ENABLE_HIDE_COSTUME_SYSTEM_TRANSMUTACION
#define ENABLE_CHANGE_FONT_SYSTEMS

#define LSS_SECURITY_KEY		"1234abcd5678efgh"
#define ENABLE_NEW_BONUS_WORLDARD
#define ENABLE_DROP_ITEM_WORLDARD

#define ENABLE_CUBE_RENEWAL_WORLDARD

//#define ENABLE_VOTE_COINS_WHILE_PLAYING
#define ENABLE_MULTI_REFINE_WORLDARD
//#define ELEMENT_SPELL_WORLDARD
#define ELEMENT_NEW_BONUSES

#define UNLOCK_INVENTORY_WORLDARD
#define __ITEM_SHINING__
#define ENABLE_NEW_RINGS
#define ENABLE_MAINTENANCE_SYSTEM
#define ENABLE_SASH_COSTUME_SYSTEM
#define RENEWAL_DEAD_PACKET
#define BATTLEPASS_WORLDARD
#define CHANGE_EQUIP_WORLDARD
//#define TOURNAMENT_PVP_SYSTEM

//Installed Systems
#define ENABLE_CONQUEROR_LEVEL
#define ENABLE_GLOVE_SYSTEM
#define ITEM_TALISMAN_EQUIPMENT
#define ELEMENT_TARGET
#define ENABLE_78_SKILLS
#define ENABLE_OFFLINESHOP_SYSTEM
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	#define ENABLE_SHOP_SEARCH_SYSTEM
#endif

#define __BL_67_ATTR__
