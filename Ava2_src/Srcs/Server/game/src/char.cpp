#include "stdafx.h"

#include "../../common/teen_packet.h"
#include "../../common/VnumHelper.h"

#include "char.h"

#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "char_manager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "item_manager.h"
#include "motion.h"
#include "vector.h"
#include "packet.h"
#include "cmd.h"
#include "fishing.h"
#include "exchange.h"
#include "battle.h"
#include "affect.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "pvp.h"
#include "party.h"
#include "start_position.h"
#include "questmanager.h"
#include "log.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "war_map.h"
#include "xmas_event.h"
#include "banword.h"
#include "target.h"
#include "wedding.h"
#include "mob_manager.h"
#include "mining.h"
#include "monarch.h"
#include "castle.h"
#include "arena.h"
#include "dev_log.h"
#include "horsename_manager.h"
#include "pcbang.h"
#include "gm.h"
#include "map_location.h"
#include "BlueDragon_Binder.h"
#include "HackShield.h"
#include "skill_power.h"
#include "XTrapManager.h"
#include "buff_on_attributes.h"

#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif
#include "DragonSoul.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#include "../../common/CommonDefines.h"

#ifdef __SEND_TARGET_INFO__
#include <algorithm>
#include <iterator>
using namespace std;
#endif
#if defined(WJ_COMBAT_ZONE)
	#include "combat_zone.h"
#endif
#include <boost/algorithm/string.hpp>

#ifdef ENABLE_MOUNT_SYSTEM
#include "MountSystem.h"
#endif
#ifdef __NEWPET_SYSTEM__
#include "New_PetSystem.h"
#endif

#ifdef ENABLE_SUPPORT_SYSTEM
	#include "SupportSystem.h"
#endif

#ifdef ENABLE_DUNGEON_INFO
#include "dungeon_info.h"
#endif
#include "Anticheat_Manager.h"
#ifdef TOURNAMENT_PVP_SYSTEM
	#include "tournament.h"
#endif

#ifdef ENABLE_OFFLINESHOP_SYSTEM
#include "offline_shop.h"
#include "offlineshop_manager.h"
#endif

extern const BYTE g_aBuffOnAttrPoints;
extern bool RaceToJob(unsigned race, unsigned *ret_job);

extern bool IS_SUMMONABLE_ZONE(int map_index); // char_item.cpp
bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index);

bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index)
{
	switch (map_index)
	{
	case 301:
	case 302:
	case 303:
	case 304:
		if (ch->GetLevel() < 80)
			return false;
	}
	return true;
}



#ifdef NEW_ICEDAMAGE_SYSTEM
const DWORD CHARACTER::GetNoDamageRaceFlag()
{
	return m_dwNDRFlag;
}

void CHARACTER::SetNoDamageRaceFlag(DWORD dwRaceFlag)
{
	if (dwRaceFlag>=MAIN_RACE_MAX_NUM) return;
	if (IS_SET(m_dwNDRFlag, 1<<dwRaceFlag)) return;
	SET_BIT(m_dwNDRFlag, 1<<dwRaceFlag);
}

void CHARACTER::UnsetNoDamageRaceFlag(DWORD dwRaceFlag)
{
	if (dwRaceFlag>=MAIN_RACE_MAX_NUM) return;
	if (!IS_SET(m_dwNDRFlag, 1<<dwRaceFlag)) return;
	REMOVE_BIT(m_dwNDRFlag, 1<<dwRaceFlag);
}

void CHARACTER::ResetNoDamageRaceFlag()
{
	m_dwNDRFlag = 0;
}

const std::set<DWORD> & CHARACTER::GetNoDamageAffectFlag()
{
	return m_setNDAFlag;
}

void CHARACTER::SetNoDamageAffectFlag(DWORD dwAffectFlag)
{
	m_setNDAFlag.insert(dwAffectFlag);
}

void CHARACTER::UnsetNoDamageAffectFlag(DWORD dwAffectFlag)
{
	m_setNDAFlag.erase(dwAffectFlag);
}

void CHARACTER::ResetNoDamageAffectFlag()
{
	m_setNDAFlag.clear();
}
#endif

// <Factor> DynamicCharacterPtr member function definitions

LPCHARACTER DynamicCharacterPtr::Get() const {
	LPCHARACTER p = NULL;
	if (is_pc) {
		p = CHARACTER_MANAGER::instance().FindByPID(id);
	} else {
		p = CHARACTER_MANAGER::instance().Find(id);
	}
	return p;
}

DynamicCharacterPtr& DynamicCharacterPtr::operator=(LPCHARACTER character) {
	if (character == NULL) {
		Reset();
		return *this;
	}
	if (character->IsPC()) {
		is_pc = true;
		id = character->GetPlayerID();
	} else {
		is_pc = false;
		id = character->GetVID();
	}
	return *this;
}

CHARACTER::CHARACTER()
{
	m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateIdle, &CHARACTER::EndStateEmpty);
	m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
	m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateBattle, &CHARACTER::EndStateEmpty);

	Initialize();
}

CHARACTER::~CHARACTER()
{
	Destroy();
}

void CHARACTER::Initialize()
{
	CEntity::Initialize(ENTITY_CHARACTER);

	m_bOpeningSafebox = false;

	m_fSyncTime = get_float_time()-3;

#ifdef ENABLE_CONQUEROR_LEVEL
	m_pkCheonunEvent = NULL;
#endif

#ifdef ENABLE_PREVENT_FLOOD_PACKETS
    m_dwLastExchangeTime = 0;
    m_dwLastBuyTime = 0;
    m_dwLastSafeBoxCheckOutTime = 0;
    m_dwLastItemSplitTime = 0;
    m_dwLastOfferExpime = 0;
    dwLastBraveryCapePulse = 0;
#ifdef ENABLE_GUILD_DONATE_ATTENDANCE
    m_dwLastOfferNewExpTime = 0;
#endif
#ifdef ENABLE_SEARCH_ITEM_DROP_ON_MOB
    m_dwLastDropItemSearchTime = 0;
#endif
#endif

#ifdef ENABLE_ANTI_PACKET_FLOOD
	m_dwPacketAntiFloodCount = 0;
	m_dwPacketAntiFloodPulse = 0;
#endif

	m_dwPlayerID = 0;
	m_dwKillerPID = 0;
#ifdef __NEWPET_SYSTEM__
	m_stImmortalSt = 0;

	m_newpetskillcd[0] = 0;
	m_newpetskillcd[1] = 0;
	m_newpetskillcd[2] = 0;
#endif


#ifdef ENABLE_CALENDAR_SYSTEM
	m_dwLastOpenCalendar = 0;
#endif
#ifdef __SEND_TARGET_INFO__
	dwLastTargetInfoPulse = 0;
#endif
	m_iMoveCount = 0;

	m_pkRegen = NULL;
	regen_id_ = 0;
	m_posRegen.x = m_posRegen.y = m_posRegen.z = 0;
	m_posStart.x = m_posStart.y = 0;
	m_posDest.x = m_posDest.y = 0;
	m_fRegenAngle = 0.0f;

	m_pkMobData		= NULL;
	m_pkMobInst		= NULL;

	m_pkShop		= NULL;

	m_pkExchange	= NULL;
	m_pkParty		= NULL;
	m_pkPartyRequestEvent = NULL;
	//m_pkAnticheatEvent = NULL;
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	m_pkOfflineShop = NULL;
	isOfflineShopPanelOpen = false;
	m_protection_Time.clear();
#endif
	m_pGuild = NULL;
#if defined(__BL_67_ATTR__)
	b67Attr = false;
#endif
	m_pkChrTarget = NULL;

#ifdef BATTLEPASS_WORLDARD
	m_dwBattlePassEndTime = 0;
#endif

#ifdef ENABLE_MULTILANGUAGE_SYSTEM
	m_language = LOCALE_DEFAULT;//DEFAULT
#endif

	m_pkMuyeongEvent = NULL;
#if defined(WJ_COMBAT_ZONE)
	m_pkCombatZoneLeaveEvent = NULL;
	m_pkCombatZoneWarpEvent = NULL;
#endif
	m_pkWarpNPCEvent = NULL;
	m_pkDeadEvent = NULL;
	m_pkStunEvent = NULL;
	m_pkSaveEvent = NULL;
	m_pkRecoveryEvent = NULL;
	m_pkTimedEvent = NULL;
	m_pkFishingEvent = NULL;
	m_pkWarpEvent = NULL;

	// MINING
	m_pkMiningEvent = NULL;
	// END_OF_MINING

	m_pkPoisonEvent = NULL;
#ifdef ENABLE_WOLFMAN_CHARACTER
	m_pkBleedingEvent = NULL;
#endif
	m_pkFireEvent = NULL;
	m_pkCheckSpeedHackEvent	= NULL;
	m_speed_hack_count	= 0;

	m_pkAffectEvent = NULL;
	m_afAffectFlag = TAffectFlag(0, 0);

	m_pkDestroyWhenIdleEvent = NULL;
#ifdef ENABLE_BATTLE_PASS
	m_battlePassData.clear();
	m_dwMountStartTime = 0;
#endif
	m_pkChrSyncOwner = NULL;

	memset(&m_points, 0, sizeof(m_points));
	memset(&m_pointsInstant, 0, sizeof(m_pointsInstant));
	memset(&m_quickslot, 0, sizeof(m_quickslot));
	memset(&character_cards, 0, sizeof(character_cards));
	memset(&randomized_cards, 0, sizeof(randomized_cards));
	m_bCharType = CHAR_TYPE_MONSTER;

	SetPosition(POS_STANDING);

	m_dwPlayStartTime = m_dwLastMoveTime = get_dword_time();

	GotoState(m_stateIdle);
	m_dwStateDuration = 1;

	m_dwLastAttackTime = get_dword_time() - 20000;

	m_bAddChrState = 0;

	m_pkChrStone = NULL;

	m_pkSafebox = NULL;
	m_iSafeboxSize = -1;
	m_iSafeboxLoadTime = 0;

	m_pkMall = NULL;
	m_iMallLoadTime = 0;

	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;
	m_lWarpMapIndex = 0;

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;

	m_pSkillLevels = NULL;

	m_dwMoveStartTime = 0;
	m_dwMoveDuration = 0;

	m_dwFlyTargetID = 0;

	m_dwNextStatePulse = 0;

	m_dwLastDeadTime = get_dword_time()-180000;

	m_bSkipSave = false;

	m_bItemLoaded = false;

	m_bHasPoisoned = false;
#ifdef ENABLE_WOLFMAN_CHARACTER
	m_bHasBled = false;
#endif
	m_pkDungeon = NULL;
	m_iEventAttr = 0;

#ifdef ENABLE_MOUNT_SYSTEM
	bMounting = false;
	bMountingS = false;
	bMountVnumM = 0;
#endif

	m_kAttackLog.dwVID = 0;
	m_kAttackLog.dwTime = 0;

	m_bNowWalking = m_bWalking = false;
	ResetChangeAttackPositionTime();

	m_bDetailLog = false;
	m_bMonsterLog = false;

	m_bDisableCooltime = false;

	m_iAlignment = 0;
	m_iRealAlignment = 0;
#if defined(WJ_COMBAT_ZONE)
	m_iCombatZonePoints = 0;
	m_iCombatZoneDeaths = 0;
	m_pkNoticeCombatZoneEvent = NULL;
#endif
	m_iKillerModePulse = 0;
	m_bPKMode = PK_MODE_PEACE;

	m_dwQuestNPCVID = 0;
	m_dwQuestByVnum = 0;
	m_pQuestItem = NULL;

	m_szMobileAuth[0] = '\0';

	m_dwUnderGuildWarInfoMessageTime = get_dword_time()-60000;

	m_bUnderRefine = false;

	// REFINE_NPC
	m_dwRefineNPCVID = 0;
	// END_OF_REFINE_NPC

	m_dwPolymorphRace = 0;

	m_bStaminaConsume = false;

	ResetChainLightningIndex();

	m_dwMountVnum = 0;
	m_chHorse = NULL;
	m_chRider = NULL;

	m_pWarMap = NULL;
	m_pWeddingMap = NULL;
	m_bChatCounter = 0;

	ResetStopTime();

	m_dwLastVictimSetTime = get_dword_time() - 3000;
	m_iMaxAggro = -100;

	m_bSendHorseLevel = 0;
	m_bSendHorseHealthGrade = 0;
	m_bSendHorseStaminaGrade = 0;

	m_dwLoginPlayTime = 0;

	m_pkChrMarried = NULL;

	m_posSafeboxOpen.x = -1000;
	m_posSafeboxOpen.y = -1000;

	// EQUIP_LAST_SKILL_DELAY
	m_dwLastSkillTime = get_dword_time();
	// END_OF_EQUIP_LAST_SKILL_DELAY

	// MOB_SKILL_COOLTIME
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
	// END_OF_MOB_SKILL_COOLTIME

	m_isinPCBang = false;

	// ARENA
	m_pArena = NULL;
	m_nPotionLimit = quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count");
	// END_ARENA

	//PREVENT_TRADE_WINDOW
	m_isOpenSafebox = 0;
	//END_PREVENT_TRADE_WINDOW

	//PREVENT_REFINE_HACK
	m_iRefineTime = 0;
	//END_PREVENT_REFINE_HACK

	//RESTRICT_USE_SEED_OR_MOONBOTTLE
	m_iSeedTime = 0;
	//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
	//PREVENT_PORTAL_AFTER_EXCHANGE
	m_iExchangeTime = 0;
	//END_PREVENT_PORTAL_AFTER_EXCHANGE
	//
	m_iSafeboxLoadTime = 0;

	m_iMyShopTime = 0;

	InitMC();

	m_deposit_pulse = 0;

	SET_OVER_TIME(this, OT_NONE);

	m_strNewName = "";

	m_known_guild.clear();

	m_dwLogOffInterval = 0;

	m_bComboSequence = 0;
	m_dwLastComboTime = 0;
	m_bComboIndex = 0;
	m_iComboHackCount = 0;
	m_dwSkipComboAttackByTime = 0;

#ifdef ENABLE_CHECK_PICKUP_HACK
	m_dwLastPickupTime = 0;
#endif
#ifdef ENABLE_CHECK_WALLHACK
	m_dwCountWallhackPoint = 0;
#endif
#ifdef ENABLE_CHECK_GHOSTMODE
	m_dwCountGhostmodePoint = 0;
#endif
#ifdef ENABLE_CSHIELD
	m_pkCShieldEvent = NULL;
	m_pkCShieldDataEvent = NULL;
	m_cshield = nullptr;
#endif

	m_dwMountTime = 0;

	m_dwLastGoldDropTime = 0;
#ifdef ENABLE_NEWSTUFF
	m_dwLastItemDropTime = 0;
	m_dwLastBoxUseTime = 0;
	m_dwLastBuySellTime = 0;
#endif

	m_HackShieldCheckEvent = NULL;
	m_HackShieldCheckMode = false;

	m_bIsLoadedAffect = false;
	cannot_dead = false;
	
#ifdef ENABLE_SUPPORT_SYSTEM
	m_supportSystem = 0;
	m_dwSupportHair = 0;
	m_dwSupportWeapon = 0;
	m_dwSupportCostume = 0;
	m_bIsSupport = false;
	m_Owner = NULL;
#endif

#ifdef __PET_SYSTEM__
	m_petSystem = 0;
	m_bIsPet = false;
#endif
#ifdef __NEWPET_SYSTEM__	
	m_newpetSystem = 0;
	m_bIsNewPet = false;
	m_eggvid = 0;
#endif
#ifdef ENABLE_MOUNT_SYSTEM
	m_MountSystem = 0;
	m_bIsMountSystem = false;
#endif
#ifdef NEW_ICEDAMAGE_SYSTEM
	m_dwNDRFlag = 0;
	m_setNDAFlag.clear();
#endif

	m_fAttMul = 1.0f;
	m_fDamMul = 1.0f;
#ifdef ENABLE_NEW_PET_SYSTEM
	m_bIsNewPet = false;
#endif
	m_pointsInstant.iDragonSoulActiveDeck = -1;
#ifdef ENABLE_ANTI_CMD_FLOOD
	m_dwCmdAntiFloodCount = 0;
	m_dwCmdAntiFloodPulse = 0;
#endif
	memset(&m_tvLastSyncTime, 0, sizeof(m_tvLastSyncTime));
	m_iSyncHackCount = 0;
#ifdef __SASH_SYSTEM__
	m_bSashCombination = false;
	m_bSashAbsorption = false;
#endif
#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
	dwPickUPMode = 0;
	dwRarityMode = 0;
#endif
#ifdef UNLOCK_INVENTORY
	m_dwUnlockSlots = 0;
#endif
#ifdef ENABLE_SORT_INVENTORY
    m_sortInventoryPulse = 0;
    m_sortSpecialStoragePulse = 0;
#endif

#ifdef ENABLE_CALENDAR_SYSTEM
	m_isOpenCalendar = 0;
#endif

#ifdef ENABLE_DUNGEON_INFO
	CDungeonInfoExtern::instance().LoadDateDungeon();
	//CDungeonInfoExtern::instance().LoadDateDungeonRanking();
	damage_done_dungeon_info = 0;
	damage_received_dungeon_info = 0;
#endif
#ifdef ELEMENT_SPELL_WORLDARD
	m_OpenElementsSpell = false;
#endif	
#ifdef ENABLE_VOTE_COINS_WHILE_PLAYING
	m_pkVoteCoinsWhilePlaying = NULL;
#endif

#ifdef UNLOCK_INVENTORY_WORLDARD
	memset(inventory_unlock, 0, sizeof(inventory_unlock));
#endif
#ifdef ENABLE_MINI_GAME_CATCH_KING
	m_vecCatchKingFieldCards.clear();
	bCatchKingHandCard = 0;
	bCatchKingHandCardLeft = 0;
	bCatchKingBetSetNumber = 0;
	dwCatchKingTotalScore = 0;
	dwCatchKingGameStatus = false;
#endif
}

void CHARACTER::Create(const char * c_pszName, DWORD vid, bool isPC)
{
	static int s_crc = 172814;

	char crc_string[128+1];
	snprintf(crc_string, sizeof(crc_string), "%s%p%d", c_pszName, this, ++s_crc);
	m_vid = VID(vid, GetCRC32(crc_string, strlen(crc_string)));

	if (isPC)
	{
		m_stName = c_pszName;
#ifdef ENABLE_CSHIELD
		m_cshield = std::make_shared<CShield>();
#endif
	}
#ifdef ENABLE_MULTILANGUAGE_SYSTEM
	else
		m_stName = "M" + std::to_string(GetRaceNum());
#endif
}

void CHARACTER::Destroy()
{
	event_cancel(&m_pkNoticeCombatZoneEvent);

	if (m_pkRegen)
	{
		if (m_pkDungeon) {
			// Dungeon regen may not be valid at this point
			if (m_pkDungeon->IsValidRegen(m_pkRegen, regen_id_)) {
				--m_pkRegen->count;
			}
		} else {
			// Is this really safe?
			--m_pkRegen->count;
		}
		m_pkRegen = NULL;
	}

	if (m_pkDungeon)
	{
		SetDungeon(NULL);
	}
#ifdef ENABLE_SUPPORT_SYSTEM
	if (m_supportSystem)
	{
		m_supportSystem->Destroy();
		delete m_supportSystem;

		m_supportSystem = 0;
	}
#endif

#ifdef __PET_SYSTEM__
	if (m_petSystem)
	{
		m_petSystem->Destroy();
		delete m_petSystem;
		m_petSystem = NULL;
	}
#endif

#ifdef __NEWPET_SYSTEM__
	if (m_newpetSystem)
	{
		m_newpetSystem->Destroy();
		delete m_newpetSystem;
		m_newpetSystem = NULL;
	}
#endif
#ifdef ENABLE_MOUNT_SYSTEM
	if (m_MountSystem)
	{
		m_MountSystem->Destroy();
		delete m_MountSystem;

		m_MountSystem = 0;
	}
#endif
	HorseSummon(false);

	if (GetRider())
		GetRider()->ClearHorseInfo();

	
	if (IsPC())
	{
		CAnticheatManager::instance().DeleteClientHandle(GetPlayerID());
	}
	

	if( IsPC() )
	{
		if (isHackShieldEnable)
		{
			CHackShieldManager::instance().DeleteClientHandle(GetPlayerID());
		}
	}

	if (GetDesc())
	{
		GetDesc()->BindCharacter(NULL);
//		BindDesc(NULL);
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	if (GetOfflineShop())
		COfflineShopManager::Instance().StopShopping(this);
#endif

	SetVictim(NULL);

	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

#ifdef TOURNAMENT_PVP_SYSTEM
	if (IsPC())
	{	
		CTournamentPvP::instance().OnDisconnect(this);
	}
#endif

#if defined(WJ_COMBAT_ZONE)
	if (IsPC())
	{	
		CCombatZoneManager::instance().OnLogout(this);
	}
#endif
	ClearStone();
	ClearSync();
	ClearTarget();

	if (NULL == m_pkMobData)
	{
		DragonSoul_CleanUp();
		ClearItem();
	}

	// <Factor> m_pkParty becomes NULL after CParty destructor call!
	LPPARTY party = m_pkParty;
	if (party)
	{
		if (party->GetLeaderPID() == GetVID() && !IsPC())
		{
			M2_DELETE(party);
		}
		else
		{
			party->Unlink(this);

			if (!IsPC())
				party->Quit(GetVID());
		}

		SetParty(NULL); // 안해도 되지만 안전하게.
	}

	if (m_pkMobInst)
	{
		M2_DELETE(m_pkMobInst);
		m_pkMobInst = NULL;
	}

	m_pkMobData = NULL;
#ifdef ENABLE_MULTI_FARM_BLOCK
	m_bmultiFarmStatus = false;
#endif

	if (m_pkSafebox)
	{
		M2_DELETE(m_pkSafebox);
		m_pkSafebox = NULL;
	}

	if (m_pkMall)
	{
		M2_DELETE(m_pkMall);
		m_pkMall = NULL;
	}

	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin();  it != m_map_buff_on_attrs.end(); it++)
	{
		if (NULL != it->second)
		{
			M2_DELETE(it->second);
		}
	}
	m_map_buff_on_attrs.clear();

	m_set_pkChrSpawnedBy.clear();

	StopMuyeongEvent();
#ifdef ENABLE_CONQUEROR_LEVEL
	StopCheonunEvent();
#endif

	event_cancel(&m_pkWarpNPCEvent);
	event_cancel(&m_pkRecoveryEvent);
	event_cancel(&m_pkDeadEvent);
	event_cancel(&m_pkSaveEvent);
#if defined(WJ_COMBAT_ZONE)
	event_cancel(&m_pkCombatZoneLeaveEvent);
	event_cancel(&m_pkCombatZoneWarpEvent);
#endif
	event_cancel(&m_pkTimedEvent);
	event_cancel(&m_pkStunEvent);
	event_cancel(&m_pkFishingEvent);
	event_cancel(&m_pkPoisonEvent);
#ifdef ENABLE_WOLFMAN_CHARACTER
	event_cancel(&m_pkBleedingEvent);
#endif
	event_cancel(&m_pkFireEvent);
	event_cancel(&m_pkPartyRequestEvent);
	//DELAYED_WARP
	event_cancel(&m_pkWarpEvent);
	event_cancel(&m_pkCheckSpeedHackEvent);
	//END_DELAYED_WARP

	event_cancel(&m_pkAnticheatEvent);


	// RECALL_DELAY
	//event_cancel(&m_pkRecallEvent);
	// END_OF_RECALL_DELAY

	// MINING
	event_cancel(&m_pkMiningEvent);
	// END_OF_MINING

#ifdef ENABLE_CSHIELD
	event_cancel(&m_pkCShieldEvent);
	event_cancel(&m_pkCShieldDataEvent);
#endif

#ifdef ENABLE_VOTE_COINS_WHILE_PLAYING
	event_cancel(&m_pkVoteCoinsWhilePlaying);
#endif

	StopHackShieldCheckCycle();

	for (itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.begin(); it != m_mapMobSkillEvent.end(); ++it)
	{
		LPEVENT pkEvent = it->second;
		event_cancel(&pkEvent);
	}
	m_mapMobSkillEvent.clear();

	//event_cancel(&m_pkAffectEvent);
	ClearAffect();

	event_cancel(&m_pkDestroyWhenIdleEvent);

	if (m_pSkillLevels)
	{
		M2_DELETE_ARRAY(m_pSkillLevels);
		m_pSkillLevels = NULL;
	}

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

	if (m_bMonsterLog)
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);
}

const char * CHARACTER::GetName() const
{
	return (m_stName.empty() || (IsWarp() || IsGoto()))  ? (m_pkMobData ? m_pkMobData->m_table.szLocaleName : "") : m_stName.c_str();
}

void EncodeMovePacket(TPacketGCMove & pack, DWORD dwVID, BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, BYTE bRot)
{
	pack.bHeader = HEADER_GC_MOVE;
	pack.bFunc   = bFunc;
	pack.bArg    = bArg;
	pack.dwVID   = dwVID;
	pack.dwTime  = dwTime ? dwTime : get_dword_time();
	pack.bRot    = bRot;
	pack.lX		= x;
	pack.lY		= y;
	pack.dwDuration	= dwDuration;
}

void CHARACTER::RestartAtSamePos()
{
	if (m_bIsObserver)
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		if (!m_bIsObserver)
			EncodeInsertPacket(entity);

		if( entity->IsType(ENTITY_CHARACTER) )
		{
			LPCHARACTER lpChar = (LPCHARACTER)entity;
			if( lpChar->IsPC() || lpChar->IsNPC() || lpChar->IsMonster() )
			{
				if (!entity->IsObserverMode())
					entity->EncodeInsertPacket(this);
			}
		}
		else
		{
			if( !entity->IsObserverMode())
			{
				entity->EncodeInsertPacket(this);
			}
		}
	}
}

// #define ENABLE_SHOWNPCLEVEL
// Entity에 내가 나타났다고 패킷을 보낸다.
void CHARACTER::EncodeInsertPacket(LPENTITY entity)
{

	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	// 길드이름 버그 수정 코드
	LPCHARACTER ch = (LPCHARACTER) entity;
	ch->SendGuildName(GetGuild());
	// 길드이름 버그 수정 코드

	TPacketGCCharacterAdd pack;

	pack.header		= HEADER_GC_CHARACTER_ADD;
	pack.dwVID		= m_vid;
	pack.bType		= GetCharType();
	pack.angle		= GetRotation();
	pack.x		= GetX();
	pack.y		= GetY();
	pack.z		= GetZ();
	pack.wRaceNum	= GetRaceNum();
#ifdef ENABLE_MOUNT_SYSTEM
	if (IsPet() || IsMountSystem() || IsNewPet() || IsSupport())
#else
	if (IsPet())
#endif
	{
		pack.bMovingSpeed	= 150;
	}
	else
	{
		pack.bMovingSpeed	= GetLimitPoint(POINT_MOV_SPEED);
	}
	pack.bAttackSpeed	= GetLimitPoint(POINT_ATT_SPEED);
	pack.dwAffectFlag[0] = m_afAffectFlag.bits[0];
	pack.dwAffectFlag[1] = m_afAffectFlag.bits[1];

	pack.bStateFlag = m_bAddChrState;

	int iDur = 0;

	if (m_posDest.x != pack.x || m_posDest.y != pack.y)
	{
		iDur = (m_dwMoveStartTime + m_dwMoveDuration) - get_dword_time();

		if (iDur <= 0)
		{
			pack.x = m_posDest.x;
			pack.y = m_posDest.y;
		}
	}

	d->Packet(&pack, sizeof(pack));


#ifdef ENABLE_NEW_PET_SYSTEM
	if (IsPC() == true || m_bCharType == CHAR_TYPE_NPC || m_bCharType == CHAR_TYPE_PET)
#else
	if (IsPC() == true || m_bCharType == CHAR_TYPE_NPC)
#endif
	{
		TPacketGCCharacterAdditionalInfo addPacket;
		memset(&addPacket, 0, sizeof(TPacketGCCharacterAdditionalInfo));

		addPacket.header = HEADER_GC_CHAR_ADDITIONAL_INFO;
		addPacket.dwVID = m_vid;
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		SetHideCostumeUpdate();
#endif
		addPacket.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
		addPacket.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
		addPacket.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
		addPacket.awPart[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);
		#ifdef __SASH_SYSTEM__
		addPacket.awPart[CHR_EQUIPPART_SASH] = GetPart(PART_SASH);
		#endif
		addPacket.awPart[CHR_PART_ARROW_TYPE] = GetPart(PART_ARROW_TYPE);

#ifdef __ITEM_SHINING__
		addPacket.adwShining[CHR_SHINING_WEAPON_1] = GetWear(WEAR_SHINING_WEAPON_1) ? GetWear(WEAR_SHINING_WEAPON_1)->GetVnum() : 0;
		// addPacket.adwShining[CHR_SHINING_WEAPON_2] = GetWear(WEAR_SHINING_WEAPON_2) ? GetWear(WEAR_SHINING_WEAPON_2)->GetVnum() : 0;
		// addPacket.adwShining[CHR_SHINING_WEAPON_3] = GetWear(WEAR_SHINING_WEAPON_3) ? GetWear(WEAR_SHINING_WEAPON_3)->GetVnum() : 0;
		addPacket.adwShining[CHR_SHINING_ARMOR_1] = GetWear(WEAR_SHINING_ARMOR_1) ? GetWear(WEAR_SHINING_ARMOR_1)->GetVnum() : 0;
		// addPacket.adwShining[CHR_SHINING_ARMOR_2] = GetWear(WEAR_SHINING_ARMOR_2) ? GetWear(WEAR_SHINING_ARMOR_2)->GetVnum() : 0;
		// addPacket.adwShining[CHR_SHINING_ARMOR_3] = GetWear(WEAR_SHINING_ARMOR_3) ? GetWear(WEAR_SHINING_ARMOR_3)->GetVnum() : 0;
		addPacket.adwShining[CHR_SHINING_SPECIAL] = GetWear(WEAR_SHINING_SPECIAL) ? GetWear(WEAR_SHINING_SPECIAL)->GetVnum() : 0;
#endif

		addPacket.bPKMode = m_bPKMode;
		addPacket.dwMountVnum = GetMountVnum();
		addPacket.bEmpire = m_bEmpire;
#ifdef ELEMENT_SPELL_WORLDARD
		addPacket.dwElementsEffect = GetElementsEffect();
#endif
#ifdef ENABLE_SHOWNPCLEVEL
		if (1)
#else
		if (IsPC() == true || IsNewPet() == true)
#endif
		{
			addPacket.dwLevel = GetLevel();
		}
		else
		{
			addPacket.dwLevel = 0;
		}
#ifdef ENABLE_CONQUEROR_LEVEL
		if (IsPC() == true)
			addPacket.dwConquerorLevel = GetConquerorLevel();
#endif

		if (false)
		{
			LPCHARACTER ch = (LPCHARACTER) entity;

			if (GetEmpire() == ch->GetEmpire() || ch->GetGMLevel() > GM_PLAYER || m_bCharType == CHAR_TYPE_NPC)
			{
				goto show_all_info;
			}
			else
			{
				memset(addPacket.name, 0, CHARACTER_NAME_MAX_LEN);
				addPacket.dwGuildID = 0;
				addPacket.sAlignment = 0;
			}
		}
		else
		{
		show_all_info:
			strlcpy(addPacket.name, GetName(), sizeof(addPacket.name));
#ifdef ENABLE_MULTILANGUAGE_SYSTEM
			addPacket.language = GetCharacterLanguage();
#endif
			if (GetGuild() != NULL)
				addPacket.dwGuildID = GetGuild()->GetID();
			else
				addPacket.dwGuildID = 0;

			addPacket.sAlignment = m_iAlignment / 10;
#if defined(WJ_COMBAT_ZONE)
			addPacket.combat_zone_rank = GetCombatZoneRank();
			addPacket.combat_zone_points = GetCombatZonePoints();
#endif
		}

		d->Packet(&addPacket, sizeof(TPacketGCCharacterAdditionalInfo));
	}

	if (iDur)
	{
		TPacketGCMove pack;
		EncodeMovePacket(pack, GetVID(), FUNC_MOVE, 0, m_posDest.x, m_posDest.y, iDur, 0, (BYTE) (GetRotation() / 5));
		d->Packet(&pack, sizeof(pack));

		TPacketGCWalkMode p;
		p.vid = GetVID();
		p.header = HEADER_GC_WALK_MODE;
		p.mode = m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;

		d->Packet(&p, sizeof(p));
	}

	if (entity->IsType(ENTITY_CHARACTER) && GetDesc())
	{
		LPCHARACTER ch = (LPCHARACTER) entity;
		if (ch->IsWalking())
		{
			TPacketGCWalkMode p;
			p.vid = ch->GetVID();
			p.header = HEADER_GC_WALK_MODE;
			p.mode = ch->m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;
			GetDesc()->Packet(&p, sizeof(p));
		}
	}


#ifdef ENABLE_OFFLINESHOP_SYSTEM
	if (IsOfflineShopNPC())
	{
		LPOFFLINESHOP shop = GetOfflineShop();
		if (shop)
		{
			TPacketGCShopSign p;
			p.bHeader = HEADER_GC_OFFLINE_SHOP_SIGN;
			p.dwVID = GetVID();
			strlcpy(p.szSign, shop->GetShopSign(), sizeof(p.szSign));
			d->Packet(&p, sizeof(TPacketGCShopSign));
		}
	}
#endif

	if (entity->IsType(ENTITY_CHARACTER))
	{
		sys_log(3, "EntityInsert %s (RaceNum %d) (%d %d) TO %s",
				GetName(), GetRaceNum(), GetX() / SECTREE_SIZE, GetY() / SECTREE_SIZE, ((LPCHARACTER)entity)->GetName());
	}
}

void CHARACTER::EncodeRemovePacket(LPENTITY entity)
{
	if (entity->GetType() != ENTITY_CHARACTER)
		return;

	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	TPacketGCCharacterDelete pack;

	pack.header	= HEADER_GC_CHARACTER_DEL;
	pack.id	= m_vid;

	d->Packet(&pack, sizeof(TPacketGCCharacterDelete));

	if (entity->IsType(ENTITY_CHARACTER))
		sys_log(3, "EntityRemove %s(%d) FROM %s", GetName(), (DWORD) m_vid, ((LPCHARACTER) entity)->GetName());
}

void CHARACTER::UpdatePacket()
{
	if (GetSectree() == NULL) return;

	TPacketGCCharacterUpdate pack;
	TPacketGCCharacterUpdate pack2;

	pack.header = HEADER_GC_CHARACTER_UPDATE;
	pack.dwVID = m_vid;
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	SetHideCostumeUpdate();
#endif
	pack.awPart[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
	pack.awPart[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
	pack.awPart[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
	pack.awPart[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);
#ifdef __SASH_SYSTEM__
	pack.awPart[CHR_EQUIPPART_SASH] = GetPart(PART_SASH);
#endif

#ifdef __ITEM_SHINING__
	pack.adwShining[CHR_SHINING_WEAPON_1] = GetWear(WEAR_SHINING_WEAPON_1) ? GetWear(WEAR_SHINING_WEAPON_1)->GetVnum() : 0;
	// pack.adwShining[CHR_SHINING_WEAPON_2] = GetWear(WEAR_SHINING_WEAPON_2) ? GetWear(WEAR_SHINING_WEAPON_2)->GetVnum() : 0;
	// pack.adwShining[CHR_SHINING_WEAPON_3] = GetWear(WEAR_SHINING_WEAPON_3) ? GetWear(WEAR_SHINING_WEAPON_3)->GetVnum() : 0;
	pack.adwShining[CHR_SHINING_ARMOR_1] = GetWear(WEAR_SHINING_ARMOR_1) ? GetWear(WEAR_SHINING_ARMOR_1)->GetVnum() : 0;
	// pack.adwShining[CHR_SHINING_ARMOR_2] = GetWear(WEAR_SHINING_ARMOR_2) ? GetWear(WEAR_SHINING_ARMOR_2)->GetVnum() : 0;
	// pack.adwShining[CHR_SHINING_ARMOR_3] = GetWear(WEAR_SHINING_ARMOR_3) ? GetWear(WEAR_SHINING_ARMOR_3)->GetVnum() : 0;
	pack.adwShining[CHR_SHINING_SPECIAL] = GetWear(WEAR_SHINING_SPECIAL) ? GetWear(WEAR_SHINING_SPECIAL)->GetVnum() : 0;
#endif

	pack.awPart[CHR_PART_ARROW_TYPE] = GetPart(PART_ARROW_TYPE);

	pack.bMovingSpeed	= GetLimitPoint(POINT_MOV_SPEED);
	pack.bAttackSpeed	= GetLimitPoint(POINT_ATT_SPEED);
	pack.bStateFlag	= m_bAddChrState;
	pack.dwAffectFlag[0] = m_afAffectFlag.bits[0];
	pack.dwAffectFlag[1] = m_afAffectFlag.bits[1];
	pack.dwGuildID	= 0;
	pack.sAlignment	= m_iAlignment / 10;
#ifdef ENABLE_CONQUEROR_LEVEL
	pack.dwConquerorLevel = GetConquerorLevel();
#endif

#ifdef ENABLE_SUPPORT_SYSTEM
	if(IsSupport())
	{
		pack.awPart[CHR_EQUIPPART_ARMOR] = GetSupportArmor();
		pack.awPart[CHR_EQUIPPART_WEAPON] = GetSupportWeapon();
		pack.awPart[CHR_EQUIPPART_HAIR] = GetSupportHair();
	}	
#endif
#ifdef __NEWPET_SYSTEM__
	pack.dwLevel = GetLevel();
#endif
	pack.bPKMode	= m_bPKMode;
#ifdef ELEMENT_SPELL_WORLDARD
	pack.dwElementsEffect = GetElementsEffect();
#endif
#if defined(WJ_COMBAT_ZONE)
	pack.combat_zone_rank = GetCombatZoneRank();
	pack.combat_zone_points	= GetCombatZonePoints();
#endif
	if (GetGuild())
		pack.dwGuildID = GetGuild()->GetID();

	pack.dwMountVnum	= GetMountVnum();

	pack2 = pack;
	pack2.dwGuildID = 0;
	pack2.sAlignment = 0;
#ifdef __NEWPET_SYSTEM__
	pack2.dwLevel = 0;
#endif
	if (false)
	{
		if (m_bIsObserver != true)
		{
			for (ENTITY_MAP::iterator iter = m_map_view.begin(); iter != m_map_view.end(); iter++)
			{
				LPENTITY pEntity = iter->first;

				if (pEntity != NULL)
				{
					if (pEntity->IsType(ENTITY_CHARACTER) == true)
					{
						if (pEntity->GetDesc() != NULL)
						{
							LPCHARACTER pChar = (LPCHARACTER)pEntity;

							if (GetEmpire() == pChar->GetEmpire() || pChar->GetGMLevel() > GM_PLAYER)
							{
								pEntity->GetDesc()->Packet(&pack, sizeof(pack));
							}
							else
							{
								pEntity->GetDesc()->Packet(&pack2, sizeof(pack2));
							}
						}
					}
					else
					{
						if (pEntity->GetDesc() != NULL)
						{
							pEntity->GetDesc()->Packet(&pack, sizeof(pack));
						}
					}
				}
			}
		}

		if (GetDesc() != NULL)
		{
			GetDesc()->Packet(&pack, sizeof(pack));
		}
	}
	else
	{
		PacketAround(&pack, sizeof(pack));
	}
}

LPCHARACTER CHARACTER::FindCharacterInView(const char * c_pszName, bool bFindPCOnly)
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	for (; it != m_map_view.end(); ++it)
	{
		if (!it->first->IsType(ENTITY_CHARACTER))
			continue;

		LPCHARACTER tch = (LPCHARACTER) it->first;

		if (bFindPCOnly && tch->IsNPC())
			continue;

		if (!strcasecmp(tch->GetName(), c_pszName))
			return (tch);
	}

	return NULL;
}

void CHARACTER::SetPosition(int pos)
{
	if (pos == POS_STANDING)
	{
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

		event_cancel(&m_pkDeadEvent);
		event_cancel(&m_pkStunEvent);
	}
	else if (pos == POS_DEAD)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);

	if (!IsStone())
	{
		switch (pos)
		{
			case POS_FIGHTING:
				if (!IsState(m_stateBattle))
					MonsterLog("[BATTLE] 싸우는 상태");

				GotoState(m_stateBattle);
				break;

			default:
				if (!IsState(m_stateIdle))
					MonsterLog("[IDLE] 쉬는 상태");

				GotoState(m_stateIdle);
				break;
		}
	}

	m_pointsInstant.position = pos;
}

void CHARACTER::Save()
{
	if (!m_bSkipSave)
		CHARACTER_MANAGER::instance().DelayedSave(this);
}

void CHARACTER::CreatePlayerProto(TPlayerTable & tab)
{
	memset(&tab, 0, sizeof(TPlayerTable));

	if (GetNewName().empty())
	{
		strlcpy(tab.name, GetName(), sizeof(tab.name));
	}
	else
	{
		strlcpy(tab.name, GetNewName().c_str(), sizeof(tab.name));
	}

	strlcpy(tab.ip, GetDesc()->GetHostName(), sizeof(tab.ip));

	tab.id			= m_dwPlayerID;
	tab.voice		= GetPoint(POINT_VOICE);
	tab.level		= GetLevel();
	tab.level_step	= GetPoint(POINT_LEVEL_STEP);
	tab.exp			= GetExp();
	tab.gold		= GetGold();
#ifdef __GEM_SYSTEM__
	tab.gem = GetGem();
#endif
	tab.job			= m_points.job;
	tab.part_base	= m_pointsInstant.bBasePart;
	tab.skill_group	= m_points.skill_group;
#ifdef ENABLE_NEW_DETAILS_GUI
	thecore_memcpy(tab.kill_log, m_points.kill_log, sizeof(tab.kill_log));
#endif
	DWORD dwPlayedTime = (get_dword_time() - m_dwPlayStartTime);

	if (dwPlayedTime > 60000)
	{
#ifdef ENABLE_BATTLE_PASS
		CHARACTER_MANAGER::Instance().DoMission(this, MISSION_PLAYTIME, dwPlayedTime / 1000, 0);
#endif

		if (GetSectree() && !GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK))
		{
			if (GetRealAlignment() < 0)
			{
				if (IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_TIME))
					UpdateAlignment(120 * (dwPlayedTime / 60000));
				else
					UpdateAlignment(60 * (dwPlayedTime / 60000));
			}
			else
				UpdateAlignment(5 * (dwPlayedTime / 60000));
		}

		SetRealPoint(POINT_PLAYTIME, GetRealPoint(POINT_PLAYTIME) + dwPlayedTime / 60000);
		SetPoint(POINT_PLAYTIME,GetRealPoint(POINT_PLAYTIME) + dwPlayedTime / 60000);
		ResetPlayTime(dwPlayedTime % 60000);
	}

#ifdef ENABLE_BATTLE_PASS
	if(m_dwMountStartTime > 0)
	{
		DWORD dwMountPlayedTime = (get_dword_time() - m_dwMountStartTime);
		if (dwMountPlayedTime > 1000)
		{
			CHARACTER_MANAGER::Instance().DoMission(this, MISSION_MOUNT_TIME, dwMountPlayedTime / 1000, 0);
			dwMountPlayedTime = get_dword_time();
		}
	}
#endif

	tab.playtime = GetRealPoint(POINT_PLAYTIME);
	tab.lAlignment = m_iRealAlignment;

	if (m_posWarp.x != 0 || m_posWarp.y != 0)
	{
		tab.x = m_posWarp.x;
		tab.y = m_posWarp.y;
		tab.z = 0;
		tab.lMapIndex = m_lWarpMapIndex;
	}
	else
	{
		tab.x = GetX();
		tab.y = GetY();
		tab.z = GetZ();
		tab.lMapIndex	= GetMapIndex();
	}

	if (m_lExitMapIndex == 0)
	{
		tab.lExitMapIndex	= tab.lMapIndex;
		tab.lExitX		= tab.x;
		tab.lExitY		= tab.y;
	}
	else
	{
		tab.lExitMapIndex	= m_lExitMapIndex;
		tab.lExitX		= m_posExit.x;
		tab.lExitY		= m_posExit.y;
	}

	sys_log(0, "SAVE: %s %dx%d", GetName(), tab.x, tab.y);

	tab.st = GetRealPoint(POINT_ST);
	tab.ht = GetRealPoint(POINT_HT);
	tab.dx = GetRealPoint(POINT_DX);
	tab.iq = GetRealPoint(POINT_IQ);

	tab.stat_point = GetPoint(POINT_STAT);
	tab.skill_point = GetPoint(POINT_SKILL);
	tab.sub_skill_point = GetPoint(POINT_SUB_SKILL);
	tab.horse_skill_point = GetPoint(POINT_HORSE_SKILL);

	tab.stat_reset_count = GetPoint(POINT_STAT_RESET_COUNT);

	tab.hp = GetHP();
	tab.sp = GetSP();

	tab.stamina = GetStamina();

	tab.sRandomHP = m_points.iRandomHP;
	tab.sRandomSP = m_points.iRandomSP;

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
		tab.quickslot[i] = m_quickslot[i];

#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
	tab.dwPickUPMode = GetPickUPMode();
	tab.dwRarityMode = GetRarityMode();
#endif

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < UNLOCK_INVENTORY_MAX; ++i)
		tab.inventory_unlock[i] = GetUnlockSlotsW(i);
#endif


#ifdef BATTLEPASS_WORLDARD
	tab.dwBattlePassEndTime = m_dwBattlePassEndTime;
#endif

	if (m_stMobile.length() && !*m_szMobileAuth)
		strlcpy(tab.szMobile, m_stMobile.c_str(), sizeof(tab.szMobile));

	thecore_memcpy(tab.parts, m_pointsInstant.parts, sizeof(tab.parts));

	// REMOVE_REAL_SKILL_LEVLES
	thecore_memcpy(tab.skills, m_pSkillLevels, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	// END_OF_REMOVE_REAL_SKILL_LEVLES
#ifdef ENABLE_CONQUEROR_LEVEL
	tab.conqueror_exp	= GetConquerorExp();
	tab.conquerorlevel = GetConquerorLevel();
	tab.conqueror_level_step = GetPoint(POINT_CONQUEROR_LEVEL_STEP);
	tab.sungma_str = GetRealPoint(POINT_SUNGMA_STR);
	tab.sungma_hp = GetRealPoint(POINT_SUNGMA_HP);
	tab.sungma_move = GetRealPoint(POINT_SUNGMA_MOVE);
	tab.sungma_inmune = GetRealPoint(POINT_SUNGMA_INMUNE);
	tab.conqueror_point = GetPoint(POINT_CONQUEROR_POINT);
#endif

	//sys_err("SUNGMA: %d, %d, %d, %d", tab.sungma_str, tab.sungma_hp, tab.sungma_move, tab.sungma_inmune);
	tab.horse = GetHorseData();
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	tab.shopFlag = GetOfflineShopFlag();
#endif
#ifdef ENABLE_BATTLE_PASS
	memset(&tab.battlePassData, 0, sizeof(tab.battlePassData));
	//if (m_battlePassData.size() && GetBattlePassStatus())
	if (m_battlePassData.size())
	{
		for (auto it = m_battlePassData.begin(); it != m_battlePassData.end(); ++it)
		{
			if(it->second > 0)
				tab.battlePassData[it->first] = it->second;
		}
	}
#endif
#ifdef ENABLE_ANTI_EXP
	tab.anti_exp = GetAntiExp();
#endif
}


void CHARACTER::SaveReal()
{
	if (m_bSkipSave)
		return;

	if (!GetDesc())
	{
		sys_err("Character::Save : no descriptor when saving (name: %s)", GetName());
		return;
	}

	TPlayerTable table;
	CreatePlayerProto(table);

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_SAVE, GetDesc()->GetHandle(), &table, sizeof(TPlayerTable));

	quest::PC * pkQuestPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	if (!pkQuestPC)
		sys_err("CHARACTER::Save : null quest::PC pointer! (name %s)", GetName());
	else
	{
		pkQuestPC->Save();
	}

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
	if (pMarriage)
		pMarriage->Save();
}

void CHARACTER::FlushDelayedSaveItem()
{
	// 저장 안된 소지품을 전부 저장시킨다.
	LPITEM item;

	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
			ITEM_MANAGER::instance().FlushDelayedSave(item);
	}
}

void CHARACTER::Disconnect(const char * c_pszReason)
{
	assert(GetDesc() != NULL);

	sys_log(0, "DISCONNECT: %s (%s)", GetName(), c_pszReason ? c_pszReason : "unset" );
#ifdef RENEWAL_MISSION_BOOKS
	SaveMissionData();
#endif
	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

#if defined(WJ_COMBAT_ZONE)
	if (IsPC())
	{	
		CCombatZoneManager::instance().OnLogout(this);
	}
#endif
	if (GetArena() != NULL)
	{
		GetArena()->OnDisconnect(GetPlayerID());
	}

	if (GetParty() != NULL)
	{
		GetParty()->UpdateOfflineState(GetPlayerID());
	}
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	if (GetOfflineShop())
	{
		GetOfflineShop()->RemoveGuest(this);
		SetOfflineShop(NULL);
	}
#endif
	marriage::CManager::instance().Logout(this);
#ifdef ENABLE_MULTI_FARM_BLOCK
	CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(GetDesc() ? GetDesc()->GetHostName() : "", GetPlayerID(), GetName(), false);
#endif
	// P2P Logout
	TPacketGGLogout p;
	p.bHeader = HEADER_GG_LOGOUT;
	strlcpy(p.szName, GetName(), sizeof(p.szName));
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogout));
	LogManager::instance().CharLog(this, 0, "LOGOUT", "");

#ifdef ENABLE_PCBANG_FEATURE // @warme006
	{
		long playTime = GetRealPoint(POINT_PLAYTIME) - m_dwLoginPlayTime;
		LogManager::instance().LoginLog(false, GetDesc()->GetAccountTable().id, GetPlayerID(), GetLevel(), GetJob(), playTime);

		if (0)
			CPCBangManager::instance().Log(GetDesc()->GetHostName(), GetPlayerID(), playTime);
	}
#endif

	if (m_pWarMap)
		SetWarMap(NULL);

	if (m_pWeddingMap)
	{
		SetWeddingMap(NULL);
	}

	if (GetGuild())
		GetGuild()->LogoutMember(this);

	quest::CQuestManager::instance().LogoutPC(this);

	if (GetParty())
		GetParty()->Unlink(this);

	// 죽었을 때 접속끊으면 경험치 줄게 하기
	if (IsStun() || IsDead())
	{
		DeathPenalty(0);
		PointChange(POINT_HP, 50 - GetHP());
	}

	if (m_petSystem)
	{
		LPITEM item = GetWear(WEAR_PET);
		if (item)
			m_petSystem->DeletePet(item->GetID());
		item = GetWear(WEAR_OLD_PET);
		if (item)
			m_petSystem->DeletePet(item->GetID());
	}

	if (!CHARACTER_MANAGER::instance().FlushDelayedSave(this))
	{
		SaveReal();
	}

	FlushDelayedSaveItem();

	SaveAffect();
	m_bIsLoadedAffect = false;

	m_bSkipSave = true; // 이 이후에는 더이상 저장하면 안된다.

	quest::CQuestManager::instance().DisconnectPC(this);

	CloseSafebox();

	CloseMall();

	CPVPManager::instance().Disconnect(this);

	CTargetManager::instance().Logout(GetPlayerID());

	MessengerManager::instance().Logout(GetName());

	if (g_TeenDesc)
	{
		int		offset = 0;
		char	buf[245] = {0};

		buf[0] = HEADER_GT_LOGOUT;
		offset += 1;

		memset(buf+offset, 0x00, 2);
		offset += 2;

		TAccountTable	&acc_table = GetDesc()->GetAccountTable();
		memcpy(buf+offset, &acc_table.id, 4);
		offset += 4;

		g_TeenDesc->Packet(buf, offset);
	}

	if (GetDesc())
	{
		GetDesc()->BindCharacter(NULL);
//		BindDesc(NULL);
	}

	CXTrapManager::instance().DestroyClientSession(this);

	M2_DESTROY_CHARACTER(this);
}

bool CHARACTER::Show(long lMapIndex, long x, long y, long z, bool bShowSpawnMotion/* = false */)
{
	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "cannot find sectree by %dx%d mapindex %d", x, y, lMapIndex);
		return false;
	}

	SetMapIndex(lMapIndex);

	bool bChangeTree = false;

	if (!GetSectree() || GetSectree() != sectree)
		bChangeTree = true;

	if (bChangeTree)
	{
		if (GetSectree())
			GetSectree()->RemoveEntity(this);

		ViewCleanup();
	}

	if (!IsNPC())
	{
		sys_log(0, "SHOW: %s %dx%dx%d", GetName(), x, y, z);
#ifdef ENABLE_CHECK_MOVESPEED_HACK
		GetCShield()->ResetMoveSpeedhack(get_dword_time());
#endif
		if (GetStamina() < GetMaxStamina())
			StartAffectEvent();
	}
	else if (m_pkMobData)
	{
		m_pkMobInst->m_posLastAttacked.x = x;
		m_pkMobInst->m_posLastAttacked.y = y;
		m_pkMobInst->m_posLastAttacked.z = z;
	}

	if (bShowSpawnMotion)
	{
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
		m_afAffectFlag.Set(AFF_SPAWN);
	}

	SetXYZ(x, y, z);

	m_posDest.x = x;
	m_posDest.y = y;
	m_posDest.z = z;

	m_posStart.x = x;
	m_posStart.y = y;
	m_posStart.z = z;

	if (bChangeTree)
	{
		EncodeInsertPacket(this);
		sectree->InsertEntity(this);

		UpdateSectree();
	}
	else
	{
		ViewReencode();
		sys_log(0, "      in same sectree");
	}

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	SetValidComboInterval(0);
	return true;
}

// BGM_INFO
struct BGMInfo
{
	std::string	name;
	float		vol;
};

typedef std::map<unsigned, BGMInfo> BGMInfoMap;

static BGMInfoMap 	gs_bgmInfoMap;
static bool		gs_bgmVolEnable = false;

void CHARACTER_SetBGMVolumeEnable()
{
	gs_bgmVolEnable = true;
	sys_log(0, "bgm_info.set_bgm_volume_enable");
}

void CHARACTER_AddBGMInfo(unsigned mapIndex, const char* name, float vol)
{
	BGMInfo newInfo;
	newInfo.name = name;
	newInfo.vol = vol;

	gs_bgmInfoMap[mapIndex] = newInfo;

	sys_log(0, "bgm_info.add_info(%d, '%s', %f)", mapIndex, name, vol);
}

const BGMInfo& CHARACTER_GetBGMInfo(unsigned mapIndex)
{
	BGMInfoMap::iterator f = gs_bgmInfoMap.find(mapIndex);
	if (gs_bgmInfoMap.end() == f)
	{
		static BGMInfo s_empty = {"", 0.0f};
		return s_empty;
	}
	return f->second;
}

bool CHARACTER_IsBGMVolumeEnable()
{
	return gs_bgmVolEnable;
}
// END_OF_BGM_INFO

void CHARACTER::MainCharacterPacket()
{
	const unsigned mapIndex = GetMapIndex();
	const BGMInfo& bgmInfo = CHARACTER_GetBGMInfo(mapIndex);

	// SUPPORT_BGM
	if (!bgmInfo.name.empty())
	{
		if (CHARACTER_IsBGMVolumeEnable())
		{
			sys_log(1, "bgm_info.play_bgm_vol(%d, name='%s', vol=%f)", mapIndex, bgmInfo.name.c_str(), bgmInfo.vol);
			TPacketGCMainCharacter4_BGM_VOL mainChrPacket;
			mainChrPacket.header = HEADER_GC_MAIN_CHARACTER4_BGM_VOL;
			mainChrPacket.dwVID = m_vid;
			mainChrPacket.wRaceNum = GetRaceNum();
			mainChrPacket.lx = GetX();
			mainChrPacket.ly = GetY();
			mainChrPacket.lz = GetZ();
			mainChrPacket.empire = GetDesc()->GetEmpire();
			mainChrPacket.skill_group = GetSkillGroup();
			strlcpy(mainChrPacket.szChrName, GetName(), sizeof(mainChrPacket.szChrName));

			mainChrPacket.fBGMVol = bgmInfo.vol;
			strlcpy(mainChrPacket.szBGMName, bgmInfo.name.c_str(), sizeof(mainChrPacket.szBGMName));
			GetDesc()->Packet(&mainChrPacket, sizeof(TPacketGCMainCharacter4_BGM_VOL));
		}
		else
		{
			sys_log(1, "bgm_info.play(%d, '%s')", mapIndex, bgmInfo.name.c_str());
			TPacketGCMainCharacter3_BGM mainChrPacket;
			mainChrPacket.header = HEADER_GC_MAIN_CHARACTER3_BGM;
			mainChrPacket.dwVID = m_vid;
			mainChrPacket.wRaceNum = GetRaceNum();
			mainChrPacket.lx = GetX();
			mainChrPacket.ly = GetY();
			mainChrPacket.lz = GetZ();
			mainChrPacket.empire = GetDesc()->GetEmpire();
			mainChrPacket.skill_group = GetSkillGroup();
			strlcpy(mainChrPacket.szChrName, GetName(), sizeof(mainChrPacket.szChrName));
			strlcpy(mainChrPacket.szBGMName, bgmInfo.name.c_str(), sizeof(mainChrPacket.szBGMName));
			GetDesc()->Packet(&mainChrPacket, sizeof(TPacketGCMainCharacter3_BGM));
		}
		//if (m_stMobile.length())
		//		ChatPacket(CHAT_TYPE_COMMAND, "sms");
	}
	// END_OF_SUPPORT_BGM
	else
	{
		sys_log(0, "bgm_info.play(%d, DEFAULT_BGM_NAME)", mapIndex);

		TPacketGCMainCharacter pack;
		pack.header = HEADER_GC_MAIN_CHARACTER;
		pack.dwVID = m_vid;
		pack.wRaceNum = GetRaceNum();
		pack.lx = GetX();
		pack.ly = GetY();
		pack.lz = GetZ();
		pack.empire = GetDesc()->GetEmpire();
		pack.skill_group = GetSkillGroup();
		strlcpy(pack.szName, GetName(), sizeof(pack.szName));
		GetDesc()->Packet(&pack, sizeof(TPacketGCMainCharacter));

		if (m_stMobile.length())
			ChatPacket(CHAT_TYPE_COMMAND, "sms");
	}
}

void CHARACTER::PointsPacket()
{
	if (!GetDesc())
		return;

	TPacketGCPoints pack;

	pack.header	= HEADER_GC_CHARACTER_POINTS;

	pack.points[POINT_LEVEL]		= GetLevel();
	pack.points[POINT_EXP]		= GetExp();
	pack.points[POINT_NEXT_EXP]		= GetNextExp();
	pack.points[POINT_HP]		= GetHP();
	pack.points[POINT_MAX_HP]		= GetMaxHP();
	pack.points[POINT_SP]		= GetSP();
	pack.points[POINT_MAX_SP]		= GetMaxSP();
	pack.points[POINT_GOLD]		= GetGold();
#ifdef __GEM_SYSTEM__
	pack.points[POINT_GEM] = GetGem();
#endif
	pack.points[POINT_STAMINA]		= GetStamina();
	pack.points[POINT_MAX_STAMINA]	= GetMaxStamina();

	for (int i = POINT_ST; i < POINT_MAX_NUM; ++i)
		pack.points[i] = GetPoint(i);

#ifdef ENABLE_CONQUEROR_LEVEL
	pack.points[POINT_CONQUEROR_LEVEL]		= GetConquerorLevel();
	pack.points[POINT_CONQUEROR_EXP]		= GetConquerorExp();
	pack.points[POINT_CONQUEROR_NEXT_EXP]		= GetConquerorNextExp();
#endif

	GetDesc()->Packet(&pack, sizeof(TPacketGCPoints));
}

bool CHARACTER::ChangeSex()
{
	int src_race = GetRaceNum();

	switch (src_race)
	{
		case MAIN_RACE_WARRIOR_M:
			m_points.job = MAIN_RACE_WARRIOR_W;
			break;

		case MAIN_RACE_WARRIOR_W:
			m_points.job = MAIN_RACE_WARRIOR_M;
			break;

		case MAIN_RACE_ASSASSIN_M:
			m_points.job = MAIN_RACE_ASSASSIN_W;
			break;

		case MAIN_RACE_ASSASSIN_W:
			m_points.job = MAIN_RACE_ASSASSIN_M;
			break;

		case MAIN_RACE_SURA_M:
			m_points.job = MAIN_RACE_SURA_W;
			break;

		case MAIN_RACE_SURA_W:
			m_points.job = MAIN_RACE_SURA_M;
			break;

		case MAIN_RACE_SHAMAN_M:
			m_points.job = MAIN_RACE_SHAMAN_W;
			break;

		case MAIN_RACE_SHAMAN_W:
			m_points.job = MAIN_RACE_SHAMAN_M;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case MAIN_RACE_WOLFMAN_M:
			m_points.job = MAIN_RACE_WOLFMAN_M;
			break;
#endif
		default:
			sys_err("CHANGE_SEX: %s unknown race %d", GetName(), src_race);
			return false;
	}

	sys_log(0, "CHANGE_SEX: %s (%d -> %d)", GetName(), src_race, m_points.job);
	return true;
}

WORD CHARACTER::GetRaceNum() const
{
	if (m_dwPolymorphRace)
		return m_dwPolymorphRace;

	if (m_pkMobData)
		return m_pkMobData->m_table.dwVnum;

	return m_points.job;
}

void CHARACTER::SetRace(BYTE race)
{
	if (race >= MAIN_RACE_MAX_NUM)
	{
		sys_err("CHARACTER::SetRace(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
		return;
	}

	m_points.job = race;
}

BYTE CHARACTER::GetJob() const
{
	unsigned race = m_points.job;
	unsigned job;

	if (RaceToJob(race, &job))
		return job;

	sys_err("CHARACTER::GetJob(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
	return JOB_WARRIOR;
}

void CHARACTER::SetLevel(BYTE level)
{
	m_points.level = level;
#if defined(WJ_COMBAT_ZONE)	
	if (IsPC() && CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	if (IsPC() && GetMapIndex() == TOURNAMENT_MAP_INDEX)
		return;
#endif
	if (IsPC())
	{
		if (level < PK_PROTECT_LEVEL)
			SetPKMode(PK_MODE_PROTECT);
		else if (GetGMLevel() != GM_PLAYER)
			SetPKMode(PK_MODE_PROTECT);
		else if (m_bPKMode == PK_MODE_PROTECT)
			SetPKMode(PK_MODE_PEACE);
	}
}

#ifdef ENABLE_CONQUEROR_LEVEL
void CHARACTER::SetConquerorLevel(BYTE level)
{
	m_points.conquerorlevel = level;
}
#endif


void CHARACTER::SetEmpire(BYTE bEmpire)
{
	m_bEmpire = bEmpire;
}

#define ENABLE_GM_FLAG_IF_TEST_SERVER
#define ENABLE_GM_FLAG_FOR_LOW_WIZARD
void CHARACTER::SetPlayerProto(const TPlayerTable * t)
{
	if (!GetDesc() || !*GetDesc()->GetHostName())
		sys_err("cannot get desc or hostname");
	else
		SetGMLevel();

	m_bCharType = CHAR_TYPE_PC;

	m_dwPlayerID = t->id;

	m_iAlignment = t->lAlignment;
	m_iRealAlignment = t->lAlignment;

	m_points.voice = t->voice;

	m_points.skill_group = t->skill_group;

	m_pointsInstant.bBasePart = t->part_base;
	SetPart(PART_HAIR, t->parts[PART_HAIR]);

#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
	SetPickUPMode(t->dwPickUPMode);
	SetRarityMode(t->dwRarityMode);
#endif

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < UNLOCK_INVENTORY_MAX; ++i)
		SetUnlockSlotsW(t->inventory_unlock[i],i);
#endif
#ifdef BATTLEPASS_WORLDARD
	m_dwBattlePassEndTime = t->dwBattlePassEndTime;
#endif
	#ifdef __SASH_SYSTEM__
	SetPart(PART_SASH, t->parts[PART_SASH]);
	#endif

	m_points.iRandomHP = t->sRandomHP;
	m_points.iRandomSP = t->sRandomSP;

	// REMOVE_REAL_SKILL_LEVLES
	if (m_pSkillLevels)
		M2_DELETE_ARRAY(m_pSkillLevels);

	m_pSkillLevels = M2_NEW TPlayerSkill[SKILL_MAX_NUM];
	thecore_memcpy(m_pSkillLevels, t->skills, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	// END_OF_REMOVE_REAL_SKILL_LEVLES


	if (t->lMapIndex >= 10000)
	{
		m_posWarp.x = t->lExitX;
		m_posWarp.y = t->lExitY;
		m_lWarpMapIndex = t->lExitMapIndex;
	}

	SetRealPoint(POINT_PLAYTIME, t->playtime);
	SetPoint(POINT_PLAYTIME, t->playtime);
	m_dwLoginPlayTime = t->playtime;
	SetRealPoint(POINT_ST, t->st);
	SetRealPoint(POINT_HT, t->ht);
	SetRealPoint(POINT_DX, t->dx);
	SetRealPoint(POINT_IQ, t->iq);

	SetPoint(POINT_ST, t->st);
	SetPoint(POINT_HT, t->ht);
	SetPoint(POINT_DX, t->dx);
	SetPoint(POINT_IQ, t->iq);

	SetPoint(POINT_STAT, t->stat_point);
	SetPoint(POINT_SKILL, t->skill_point);
	SetPoint(POINT_SUB_SKILL, t->sub_skill_point);
	SetPoint(POINT_HORSE_SKILL, t->horse_skill_point);

	SetPoint(POINT_STAT_RESET_COUNT, t->stat_reset_count);

	SetPoint(POINT_LEVEL_STEP, t->level_step);
	SetRealPoint(POINT_LEVEL_STEP, t->level_step);

	SetRace(t->job);

	SetLevel(t->level);
	SetExp(t->exp);
	SetGold(t->gold);
#ifdef __GEM_SYSTEM__
	SetGem(t->gem);
#endif
#ifdef ENABLE_CONQUEROR_LEVEL
	SetConquerorLevel(t->conquerorlevel);
	SetConquerorExp(t->conqueror_exp);

	SetPoint(POINT_CONQUEROR_LEVEL_STEP, t->conqueror_level_step);
	SetRealPoint(POINT_CONQUEROR_LEVEL_STEP, t->conqueror_level_step);

	SetRealPoint(POINT_SUNGMA_STR, t->sungma_str);
	SetRealPoint(POINT_SUNGMA_HP, t->sungma_hp);
	SetRealPoint(POINT_SUNGMA_MOVE, t->sungma_move);
	SetRealPoint(POINT_SUNGMA_INMUNE, t->sungma_inmune);

	SetPoint(POINT_SUNGMA_STR, t->sungma_str);
	SetPoint(POINT_SUNGMA_HP, t->sungma_hp);
	SetPoint(POINT_SUNGMA_MOVE, t->sungma_move);
	SetPoint(POINT_SUNGMA_INMUNE, t->sungma_inmune);
	SetPoint(POINT_CONQUEROR_POINT, t->conqueror_point);
#endif

	//sys_err("SUNGMA 2: %d, %d, %d, %d", t->sungma_str, t->sungma_hp, t->sungma_move, t->sungma_inmune);
	
	SetMapIndex(t->lMapIndex);
	SetXYZ(t->x, t->y, t->z);
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	SetOfflineShopFlag(t->shopFlag);
#endif
	ComputePoints();
#ifdef ENABLE_ANTI_EXP
	SetAntiExp(t->anti_exp);
#endif
	SetHP(t->hp);
	SetSP(t->sp);
	SetStamina(t->stamina);

	//GM일때 보호모드
#ifndef ENABLE_GM_FLAG_IF_TEST_SERVER
	if (!test_server)
#endif
	{
#ifdef ENABLE_GM_FLAG_FOR_LOW_WIZARD
		if (GetGMLevel() > GM_PLAYER)
#else
		if (GetGMLevel() > GM_LOW_WIZARD)
#endif
		{
			m_afAffectFlag.Set(AFF_YMIR);
			m_bPKMode = PK_MODE_PROTECT;
		}
	}

	if (GetLevel() < PK_PROTECT_LEVEL)
		m_bPKMode = PK_MODE_PROTECT;

	m_stMobile = t->szMobile;

	SetHorseData(t->horse);

	if (GetHorseLevel() > 0)
		UpdateHorseDataByLogoff(t->logoff_interval);

	thecore_memcpy(m_aiPremiumTimes, t->aiPremiumTimes, sizeof(t->aiPremiumTimes));
#ifdef ENABLE_BATTLE_PASS
	for (BYTE j = 0; j < MISSION_MAX; ++j)
	{
		//sys_err("load: index %d value %lld ",j,t->battlePassData[j]);
		if (t->battlePassData[j] > 0)
			m_battlePassData.emplace(j, t->battlePassData[j]);
	}
#endif
#ifdef ENABLE_NEW_DETAILS_GUI
	thecore_memcpy(m_points.kill_log, t->kill_log, sizeof(m_points.kill_log));
#endif
	m_dwLogOffInterval = t->logoff_interval;

	sys_log(0, "PLAYER_LOAD: %s PREMIUM %d %d, LOGGOFF_INTERVAL %u PTR: %p", t->name, m_aiPremiumTimes[0], m_aiPremiumTimes[1], t->logoff_interval, this);

	if (GetGMLevel() != GM_PLAYER)
	{
		LogManager::instance().CharLog(this, GetGMLevel(), "GM_LOGIN", "");
		sys_log(0, "GM_LOGIN(gmlevel=%d, name=%s(%d), pos=(%d, %d)", GetGMLevel(), GetName(), GetPlayerID(), GetX(), GetY());
	}

#ifdef ENABLE_SUPPORT_SYSTEM
	if (m_supportSystem)
	{
		m_supportSystem->Destroy();
		delete m_supportSystem;
	}

	m_supportSystem = M2_NEW CSupportSystem(this);
#endif

#ifdef __PET_SYSTEM__
	// NOTE: 일단 캐릭터가 PC인 경우에만 PetSystem을 갖도록 함. 유럽 머신당 메모리 사용률때문에 NPC까지 하긴 좀..
	if (m_petSystem)
	{
		m_petSystem->Destroy();
		delete m_petSystem;
	}

	m_petSystem = M2_NEW CPetSystem(this);
#endif
#ifdef __NEWPET_SYSTEM__
	// NOTE: AI´U A³¸?AI°¡ PCAI °æ¿i¿¡¸¸ PetSystemA≫ °®μμ·I CO. A?·´ ¸O½A´c ¸Þ¸ð¸® ≫c¿e·u¶§¹®¿¡ NPC±iAo CI±a A≫..
	if (m_newpetSystem)
	{
		m_newpetSystem->Destroy();
		delete m_newpetSystem;
	}

	m_newpetSystem = M2_NEW CNewPetSystem(this);
#endif
#ifdef ENABLE_MOUNT_SYSTEM
	if (m_MountSystem)
	{
		m_MountSystem->Destroy();
		delete m_MountSystem;
	}

	m_MountSystem = M2_NEW CMountSystem(this);
#endif
}

EVENTFUNC(kill_ore_load_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "kill_ore_load_even> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::SetProto(const CMob * pkMob)
{
	if (m_pkMobInst)
		M2_DELETE(m_pkMobInst);

	m_pkMobData = pkMob;
	m_pkMobInst = M2_NEW CMobInstance;

	m_bPKMode = PK_MODE_FREE;

	const TMobTable * t = &m_pkMobData->m_table;

	m_bCharType = t->bType;

	SetLevel(t->bLevel);
	SetEmpire(t->bEmpire);

	SetExp(t->dwExp);
	SetRealPoint(POINT_ST, t->bStr);
	SetRealPoint(POINT_DX, t->bDex);
	SetRealPoint(POINT_HT, t->bCon);
	SetRealPoint(POINT_IQ, t->bInt);

	ComputePoints();

	SetHP(GetMaxHP());
	SetSP(GetMaxSP());

	////////////////////
	m_pointsInstant.dwAIFlag = t->dwAIFlag;
	SetImmuneFlag(t->dwImmuneFlag);

	AssignTriggers(t);

	ApplyMobAttribute(t);

	if (IsStone())
	{
		DetermineDropMetinStone();
	}

	if (IsWarp() || IsGoto())
	{
		StartWarpNPCEvent();
	}

	CHARACTER_MANAGER::instance().RegisterRaceNumMap(this);

	// XXX X-mas santa hardcoding
	if (GetRaceNum() == xmas::MOB_SANTA_VNUM)
	{
		SetPoint(POINT_ATT_GRADE_BONUS, 10);
		SetPoint(POINT_DEF_GRADE_BONUS, 6);

		//산타용
		//m_dwPlayStartTime = get_dword_time() + 10 * 60 * 1000;
		//신선자 노해
		m_dwPlayStartTime = get_dword_time() + 30 * 1000;
		if (test_server)
			m_dwPlayStartTime = get_dword_time() + 30 * 1000;
	}

	// XXX CTF GuildWar hardcoding
	if (warmap::IsWarFlag(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
	}

	if (warmap::IsWarFlagBase(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
	}
#ifdef ENABLE_NEW_PET_SYSTEM
	if (m_bCharType == CHAR_TYPE_PET)
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StatePet, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StatePet, &CHARACTER::EndStateEmpty);
	}
#endif

	if (m_bCharType == CHAR_TYPE_HORSE ||
			GetRaceNum() == 20101 ||
			GetRaceNum() == 20102 ||
			GetRaceNum() == 20103 ||
			GetRaceNum() == 20104 ||
			GetRaceNum() == 20105 ||
			GetRaceNum() == 20106 ||
			GetRaceNum() == 20107 ||
			GetRaceNum() == 20108 ||
			GetRaceNum() == 20109
	  )
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
	}

	// MINING
	if (mining::IsVeinOfOre (GetRaceNum()))
	{
		char_event_info* info = AllocEventInfo<char_event_info>();

		info->ch = this;

		m_pkMiningEvent = event_create(kill_ore_load_event, info, PASSES_PER_SEC(number(7 * 60, 15 * 60)));
	}
	// END_OF_MINING
}

const TMobTable & CHARACTER::GetMobTable() const
{
	return m_pkMobData->m_table;
}

bool CHARACTER::IsRaceFlag(DWORD dwBit) const
{
	return m_pkMobData ? IS_SET(m_pkMobData->m_table.dwRaceFlag, dwBit) : 0;
}

DWORD CHARACTER::GetMobDamageMin() const
{
	return m_pkMobData->m_table.dwDamageRange[0];
}

DWORD CHARACTER::GetMobDamageMax() const
{
	return m_pkMobData->m_table.dwDamageRange[1];
}

float CHARACTER::GetMobDamageMultiply() const
{
	float fDamMultiply = GetMobTable().fDamMultiply;

	if (IsBerserk())
		fDamMultiply = fDamMultiply * 2.0f; // BALANCE: 광폭화 시 두배

	return fDamMultiply;
}

DWORD CHARACTER::GetMobDropItemVnum() const
{
	return m_pkMobData->m_table.dwDropItemVnum;
}

bool CHARACTER::IsSummonMonster() const
{
	return GetSummonVnum() != 0;
}

DWORD CHARACTER::GetSummonVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwSummonVnum : 0;
}

DWORD CHARACTER::GetPolymorphItemVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwPolymorphItemVnum : 0;
}

DWORD CHARACTER::GetMonsterDrainSPPoint() const
{
	return m_pkMobData ? m_pkMobData->m_table.dwDrainSP : 0;
}

BYTE CHARACTER::GetMobRank() const
{
	if (!m_pkMobData)
		return MOB_RANK_KNIGHT;	// PC일 경우 KNIGHT급

	return m_pkMobData->m_table.bRank;
}

BYTE CHARACTER::GetMobSize() const
{
	if (!m_pkMobData)
		return MOBSIZE_MEDIUM;

	return m_pkMobData->m_table.bSize;
}

WORD CHARACTER::GetMobAttackRange() const
{
	switch (GetMobBattleType())
	{
		case BATTLE_TYPE_RANGE:
		case BATTLE_TYPE_MAGIC:
			return m_pkMobData->m_table.wAttackRange + GetPoint(POINT_BOW_DISTANCE);
		default:
			return m_pkMobData->m_table.wAttackRange;
	}
}

BYTE CHARACTER::GetMobBattleType() const
{
	if (!m_pkMobData)
		return BATTLE_TYPE_MELEE;

	return (m_pkMobData->m_table.bBattleType);
}

void CHARACTER::ComputeBattlePoints()
{
	if (IsPolymorphed())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);
		int iAtt = 0;
		int iDef = 0;

		if (pMob)
		{
			iAtt = GetLevel() * 2 + GetPolymorphPoint(POINT_ST) * 2;
			// lev + con
			iDef = GetLevel() + GetPolymorphPoint(POINT_HT) + pMob->m_table.wDef;
		}

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
	else if (IsPC())
	{
		SetPoint(POINT_ATT_GRADE, 0);
		SetPoint(POINT_DEF_GRADE, 0);
		SetPoint(POINT_CLIENT_DEF_GRADE, 0);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));

		//
		// 기본 ATK = 2lev + 2str, 직업에 마다 2str은 바뀔 수 있음
		//
		int iAtk = GetLevel() * 2;
		int iStatAtk = 0;

		switch (GetJob())
		{
			case JOB_WARRIOR:
			case JOB_SURA:
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;

			case JOB_ASSASSIN:
				iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_DX)) / 3;
				break;

			case JOB_SHAMAN:
				iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_IQ)) / 3;
				break;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case JOB_WOLFMAN:
				// TODO: 수인족 공격력 공식 기획자에게 요청
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;
#endif
			default:
				sys_err("invalid job %d", GetJob());
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;
		}

		// 말을 타고 있고, 스탯으로 인한 공격력이 ST*2 보다 낮으면 ST*2로 한다.
		// 스탯을 잘못 찍은 사람 공격력이 더 낮지 않게 하기 위해서다.
		if (GetMountVnum() && iStatAtk < 2 * GetPoint(POINT_ST))
			iStatAtk = (2 * GetPoint(POINT_ST));

		iAtk += iStatAtk;

		// 승마(말) : 검수라 데미지 감소
		if (GetMountVnum())
		{
			if (GetJob() == JOB_SURA && GetSkillGroup() == 1)
			{
				iAtk += (iAtk * GetHorseLevel()) / 60;
			}
			else
			{
				iAtk += (iAtk * GetHorseLevel()) / 30;
			}
		}

		//
		// ATK Setting
		//
		iAtk += GetPoint(POINT_ATT_GRADE_BONUS);

		PointChange(POINT_ATT_GRADE, iAtk);

		// DEF = LEV + CON + ARMOR
		int iShowDef = GetLevel() + GetPoint(POINT_HT); // For Ymir(천마)
		int iDef = GetLevel() + (int) (GetPoint(POINT_HT) / 1.25); // For Other
		int iArmor = 0;

		LPITEM pkItem;

		for (int i = 0; i < WEAR_MAX_NUM; ++i)
			if ((pkItem = GetWear(i)) && pkItem->GetType() == ITEM_ARMOR)
			{
				if (pkItem->GetSubType() == ARMOR_BODY || pkItem->GetSubType() == ARMOR_HEAD || pkItem->GetSubType() == ARMOR_FOOTS || pkItem->GetSubType() == ARMOR_SHIELD)
				{
					iArmor += pkItem->GetValue(1);
					iArmor += (2 * pkItem->GetValue(5));
#ifdef ENABLE_RARITY
					if (pkItem->IsRarityItem() && pkItem->GetSocket(RARITY_VALUE_INDEX) > 0)
					{
						int rarityiArmor = GetRarityPoint(-4, pkItem->GetSocket(RARITY_TYPE_INDEX)) * (float(pkItem->GetValue(1)+(2 * pkItem->GetValue(5)))/100.0);
						iArmor += rarityiArmor;
					}
#endif
				}
			}

		// 말 타고 있을 때 방어력이 말의 기준 방어력보다 낮으면 기준 방어력으로 설정
		if( true == IsHorseRiding() )
		{
			if (iArmor < GetHorseArmor())
				iArmor = GetHorseArmor();

			const char* pHorseName = CHorseNameManager::instance().GetHorseName(GetPlayerID());

			if (pHorseName != NULL && strlen(pHorseName))
			{
				iArmor += 20;
			}
		}

		iArmor += GetPoint(POINT_DEF_GRADE_BONUS);
		iArmor += GetPoint(POINT_PARTY_DEFENDER_BONUS);

		// INTERNATIONAL_VERSION
		PointChange(POINT_DEF_GRADE, iDef + iArmor);
		PointChange(POINT_CLIENT_DEF_GRADE, (iShowDef + iArmor) - GetPoint(POINT_DEF_GRADE));
		// END_OF_INTERNATIONAL_VERSION

		PointChange(POINT_MAGIC_ATT_GRADE, GetLevel() * 2 + GetPoint(POINT_IQ) * 2 + GetPoint(POINT_MAGIC_ATT_GRADE_BONUS));
		PointChange(POINT_MAGIC_DEF_GRADE, GetLevel() + (GetPoint(POINT_IQ) * 3 + GetPoint(POINT_HT)) / 3 + iArmor / 2 + GetPoint(POINT_MAGIC_DEF_GRADE_BONUS));
#ifdef ENABLE_CONQUEROR_LEVEL
		//PointChange(POINT_MAX_HP, 0);
		PointChange(POINT_MOV_SPEED, 0);
		//if (GetMaxHP() < GetHP())
		//	PointChange(POINT_HP, GetMaxHP() - GetHP());
#endif

	}
	else
	{
		// 2lev + str * 2
		int iAtt = GetLevel() * 2 + GetPoint(POINT_ST) * 2;
		// lev + con
		int iDef = GetLevel() + GetPoint(POINT_HT) + GetMobTable().wDef;

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
}

void CHARACTER::ComputePoints()
{
	long lStat = GetPoint(POINT_STAT);
	long lStatResetCount = GetPoint(POINT_STAT_RESET_COUNT);
	long lSkillActive = GetPoint(POINT_SKILL);
	long lSkillSub = GetPoint(POINT_SUB_SKILL);
	long lSkillHorse = GetPoint(POINT_HORSE_SKILL);
	long lLevelStep = GetPoint(POINT_LEVEL_STEP);

	long lAttackerBonus = GetPoint(POINT_PARTY_ATTACKER_BONUS);
	long lTankerBonus = GetPoint(POINT_PARTY_TANKER_BONUS);
	long lBufferBonus = GetPoint(POINT_PARTY_BUFFER_BONUS);
	long lSkillMasterBonus = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
	long lHasteBonus = GetPoint(POINT_PARTY_HASTE_BONUS);
	long lDefenderBonus = GetPoint(POINT_PARTY_DEFENDER_BONUS);

	long lHPRecovery = GetPoint(POINT_HP_RECOVERY);
	long lSPRecovery = GetPoint(POINT_SP_RECOVERY);
#ifdef ENABLE_CONQUEROR_LEVEL
	long lConquerorStat = GetPoint(POINT_CONQUEROR_POINT);
	long lConquerorLevelStep = GetPoint(POINT_CONQUEROR_LEVEL_STEP);
#endif


	memset(m_pointsInstant.points, 0, sizeof(m_pointsInstant.points));
	BuffOnAttr_ClearAll();
	m_SkillDamageBonus.clear();

	SetPoint(POINT_STAT, lStat);
	SetPoint(POINT_SKILL, lSkillActive);
	SetPoint(POINT_SUB_SKILL, lSkillSub);
	SetPoint(POINT_HORSE_SKILL, lSkillHorse);
	SetPoint(POINT_LEVEL_STEP, lLevelStep);
	SetPoint(POINT_STAT_RESET_COUNT, lStatResetCount);

	SetPoint(POINT_ST, GetRealPoint(POINT_ST));
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));
#ifdef ENABLE_CONQUEROR_LEVEL
	SetPoint(POINT_CONQUEROR_POINT, lConquerorStat);
	SetPoint(POINT_CONQUEROR_LEVEL_STEP, lConquerorLevelStep);

	SetPoint(POINT_SUNGMA_STR, GetRealPoint(POINT_SUNGMA_STR));
	SetPoint(POINT_SUNGMA_HP, GetRealPoint(POINT_SUNGMA_HP));
	SetPoint(POINT_SUNGMA_MOVE, GetRealPoint(POINT_SUNGMA_MOVE));
	SetPoint(POINT_SUNGMA_INMUNE, GetRealPoint(POINT_SUNGMA_INMUNE));
	
#endif

	SetPart(PART_MAIN, GetOriginalPart(PART_MAIN));
	SetPart(PART_WEAPON, GetOriginalPart(PART_WEAPON));
	SetPart(PART_HEAD, GetOriginalPart(PART_HEAD));
	SetPart(PART_HAIR, GetOriginalPart(PART_HAIR));
	#ifdef __SASH_SYSTEM__
	SetPart(PART_SASH, GetOriginalPart(PART_SASH));
	#endif
	SetPart(PART_ARROW_TYPE, GetOriginalPart(PART_ARROW_TYPE));
	
	SetPoint(POINT_PARTY_ATTACKER_BONUS, lAttackerBonus);
	SetPoint(POINT_PARTY_TANKER_BONUS, lTankerBonus);
	SetPoint(POINT_PARTY_BUFFER_BONUS, lBufferBonus);
	SetPoint(POINT_PARTY_SKILL_MASTER_BONUS, lSkillMasterBonus);
	SetPoint(POINT_PARTY_HASTE_BONUS, lHasteBonus);
	SetPoint(POINT_PARTY_DEFENDER_BONUS, lDefenderBonus);

	SetPoint(POINT_HP_RECOVERY, lHPRecovery);
	SetPoint(POINT_SP_RECOVERY, lSPRecovery);

	// PC_BANG_ITEM_ADD
	SetPoint(POINT_PC_BANG_EXP_BONUS, 0);
	SetPoint(POINT_PC_BANG_DROP_BONUS, 0);
	// END_PC_BANG_ITEM_ADD

	int iMaxHP, iMaxSP;
	int iMaxStamina;

	if (IsPC())
	{
		// 최대 생명력/정신력
		iMaxHP = JobInitialPoints[GetJob()].max_hp + m_points.iRandomHP + GetPoint(POINT_HT) * JobInitialPoints[GetJob()].hp_per_ht;
		iMaxSP = JobInitialPoints[GetJob()].max_sp + m_points.iRandomSP + GetPoint(POINT_IQ) * JobInitialPoints[GetJob()].sp_per_iq;
		iMaxStamina = JobInitialPoints[GetJob()].max_stamina + GetPoint(POINT_HT) * JobInitialPoints[GetJob()].stamina_per_con;

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_ADD_HP);

			if (NULL != pkSk)
			{
				pkSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_ADD_HP) / 100.0f);

				iMaxHP += static_cast<int>(pkSk->kPointPoly.Eval());
			}
		}

		// 기본 값들
		//SetPoint(POINT_MOV_SPEED,	100);
		SetPoint(POINT_MOV_SPEED,	220);
		SetPoint(POINT_ATT_SPEED,	100);
		PointChange(POINT_ATT_SPEED, GetPoint(POINT_PARTY_HASTE_BONUS));
		SetPoint(POINT_CASTING_SPEED,	100);
#ifdef __GEM_SYSTEM__
		SetPoint(POINT_GEM, GetGem());
#endif
	}
	else
	{
		iMaxHP = m_pkMobData->m_table.dwMaxHP;
		iMaxSP = 0;
		iMaxStamina = 0;

		SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.sAttackSpeed);
		SetPoint(POINT_MOV_SPEED, m_pkMobData->m_table.sMovingSpeed);
		SetPoint(POINT_CASTING_SPEED, m_pkMobData->m_table.sAttackSpeed);
	}

	if (IsPC())
	{
		// 말 타고 있을 때는 기본 스탯이 말의 기준 스탯보다 낮으면 높게 만든다.
		// 따라서 말의 기준 스탯이 무사 기준이므로, 수라/무당은 전체 스탯 합이
		// 대채적으로 더 올라가게 될 것이다.
		if (GetMountVnum())
		{
			if (GetHorseST() > GetPoint(POINT_ST))
				PointChange(POINT_ST, GetHorseST() - GetPoint(POINT_ST));

			if (GetHorseDX() > GetPoint(POINT_DX))
				PointChange(POINT_DX, GetHorseDX() - GetPoint(POINT_DX));

			if (GetHorseHT() > GetPoint(POINT_HT))
				PointChange(POINT_HT, GetHorseHT() - GetPoint(POINT_HT));

			if (GetHorseIQ() > GetPoint(POINT_IQ))
				PointChange(POINT_IQ, GetHorseIQ() - GetPoint(POINT_IQ));
		}

	}

	ComputeBattlePoints();

	// 기본 HP/SP 설정
	if (iMaxHP != GetMaxHP())
	{
		SetRealPoint(POINT_MAX_HP, iMaxHP); // 기본HP를 RealPoint에 저장해 놓는다.
	}

	PointChange(POINT_MAX_HP, 0);

	if (iMaxSP != GetMaxSP())
	{
		SetRealPoint(POINT_MAX_SP, iMaxSP); // 기본SP를 RealPoint에 저장해 놓는다.
	}

	PointChange(POINT_MAX_SP, 0);

	SetMaxStamina(iMaxStamina);
	// @fixme118 part1
	int iCurHP = this->GetHP();
	int iCurSP = this->GetSP();

	m_pointsInstant.dwImmuneFlag = 0;

	for (int i = 0 ; i < WEAR_MAX_NUM; i++)
	{
		LPITEM pItem = GetWear(i);
		if (pItem)
		{
			pItem->ModifyPoints(true);
			SET_BIT(m_pointsInstant.dwImmuneFlag, GetWear(i)->GetImmuneFlag());
		}
	}
#ifdef ENABLE_EVENT_MANAGER
	CHARACTER_MANAGER::Instance().CheckBonusEvent(this);
#endif
	// 용혼석 시스템
	// ComputePoints에서는 케릭터의 모든 속성값을 초기화하고,
	// 아이템, 버프 등에 관련된 모든 속성값을 재계산하기 때문에,
	// 용혼석 시스템도 ActiveDeck에 있는 모든 용혼석의 속성값을 다시 적용시켜야 한다.
	if (DragonSoul_IsDeckActivated())
	{
		for (int i = WEAR_MAX_NUM + DS_SLOT_MAX * DragonSoul_GetActiveDeck();
			i < WEAR_MAX_NUM + DS_SLOT_MAX * (DragonSoul_GetActiveDeck() + 1); i++)
		{
			LPITEM pItem = GetWear(i);
			if (pItem)
			{
				if (DSManager::instance().IsTimeLeftDragonSoul(pItem))
					pItem->ModifyPoints(true);
			}
		}
	}

	if (GetHP() > GetMaxHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetSP() > GetMaxSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());

	ComputeSkillPoints();

	RefreshAffect();
	// @fixme118 part2 (before petsystem stuff)
	if (IsPC())
	{
		if (this->GetHP() != iCurHP)
			this->PointChange(POINT_HP, iCurHP-this->GetHP());
		if (this->GetSP() != iCurSP)
			this->PointChange(POINT_SP, iCurSP-this->GetSP());
	}

#ifndef ENABLE_NEW_PET_SYSTEM
	CPetSystem* pPetSystem = GetPetSystem();
	if (NULL != pPetSystem)
		pPetSystem->RefreshBuff();
#endif
// #if defined(ENABLE_MOUNT_SYSTEM) and defined(MOUNT_SYSTEM_BONUS)
	// CMountSystem* pMountSystem = GetMountSystem();
	// if (NULL != pMountSystem)
	// {
		// pMountSystem->RefreshBuff();
	// }
// #endif

	UpdatePacket();
}

// m_dwPlayStartTime의 단위는 milisecond다. 데이터베이스에는 분단위로 기록하기
// 때문에 플레이시간을 계산할 때 / 60000 으로 나눠서 하는데, 그 나머지 값이 남았
// 을 때 여기에 dwTimeRemain으로 넣어서 제대로 계산되도록 해주어야 한다.
void CHARACTER::ResetPlayTime(DWORD dwTimeRemain)
{
	m_dwPlayStartTime = get_dword_time() - dwTimeRemain;
}

const int aiRecoveryPercents[10] = { 1, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

EVENTFUNC(recovery_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "recovery_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->IsPC())
	{
		//
		// 몬스터 회복
		//
		if (ch->IsAffectFlag(AFF_POISON))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
#ifdef ENABLE_WOLFMAN_CHARACTER
		if (ch->IsAffectFlag(AFF_BLEEDING))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
#endif
		if (2493 == ch->GetMobTable().dwVnum)
		{
			int regenPct = BlueDragon_GetRangeFactor("hp_regen", ch->GetHPPct());
			regenPct += ch->GetMobTable().bRegenPercent;

			for (int i=1 ; i <= 4 ; ++i)
			{
				if (REGEN_PECT_BONUS == BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"))
				{
					DWORD dwDragonStoneID = BlueDragon_GetIndexFactor("DragonStone", i, "vnum");
					size_t val = BlueDragon_GetIndexFactor("DragonStone", i, "val");
					size_t cnt = SECTREE_MANAGER::instance().GetMonsterCountInMap( ch->GetMapIndex(), dwDragonStoneID );

					regenPct += (val*cnt);

					break;
				}
			}

			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * regenPct) / 100));
		}
		else if (!ch->IsDoor())
		{
			ch->MonsterLog("HP_REGEN +%d", MAX(1, (ch->GetMaxHP() * ch->GetMobTable().bRegenPercent) / 100));
			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * ch->GetMobTable().bRegenPercent) / 100));
		}

		if (ch->GetHP() >= ch->GetMaxHP())
		{
			ch->m_pkRecoveryEvent = NULL;
			return 0;
		}

		if (2493 == ch->GetMobTable().dwVnum)
		{
			for (int i=1 ; i <= 4 ; ++i)
			{
				if (REGEN_TIME_BONUS == BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"))
				{
					DWORD dwDragonStoneID = BlueDragon_GetIndexFactor("DragonStone", i, "vnum");
					size_t val = BlueDragon_GetIndexFactor("DragonStone", i, "val");
					size_t cnt = SECTREE_MANAGER::instance().GetMonsterCountInMap( ch->GetMapIndex(), dwDragonStoneID );

					return PASSES_PER_SEC(MAX(1, (ch->GetMobTable().bRegenCycle - (val*cnt))));
				}
			}
		}

		return PASSES_PER_SEC(MAX(1, ch->GetMobTable().bRegenCycle));
	}
	else
	{
		//
		// PC 회복
		//
		ch->CheckTarget();
		//ch->UpdateSectree(); // 여기서 이걸 왜하지?
		ch->UpdateKillerMode();

		if (ch->IsAffectFlag(AFF_POISON) == true)
		{
			// 중독인 경우 자동회복 금지
			// 파법술인 경우 자동회복 금지
			return 3;
		}
#ifdef ENABLE_WOLFMAN_CHARACTER
		if (ch->IsAffectFlag(AFF_BLEEDING))
			return 3;
#endif
		int iSec = (get_dword_time() - ch->GetLastMoveTime()) / 3000;

		// SP 회복 루틴.
		// 왜 이걸로 해서 함수로 빼놨는가 ?!
		ch->DistributeSP(ch);

		if (ch->GetMaxHP() <= ch->GetHP())
			return PASSES_PER_SEC(3);

		int iPercent = 0;
		int iAmount = 0;

		{
			iPercent = aiRecoveryPercents[MIN(9, iSec)];
			iAmount = 15 + (ch->GetMaxHP() * iPercent) / 100;
		}

		iAmount += (iAmount * ch->GetPoint(POINT_HP_REGEN)) / 100;

		sys_log(1, "RECOVERY_EVENT: %s %d HP_REGEN %d HP +%d", ch->GetName(), iPercent, ch->GetPoint(POINT_HP_REGEN), iAmount);

		ch->PointChange(POINT_HP, iAmount, false);
		return PASSES_PER_SEC(3);
	}
}

void CHARACTER::StartRecoveryEvent()
{
	if (m_pkRecoveryEvent)
		return;

	if (IsDead() || IsStun())
		return;

	if (IsNPC() && GetHP() >= GetMaxHP()) // 몬스터는 체력이 다 차있으면 시작 안한다.
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	int iSec = IsPC() ? 3 : (MAX(1, GetMobTable().bRegenCycle));
	m_pkRecoveryEvent = event_create(recovery_event, info, PASSES_PER_SEC(iSec));
}

void CHARACTER::Standup()
{
	struct packet_position pack_position;

	if (!IsPosition(POS_SITTING))
		return;

	SetPosition(POS_STANDING);

	sys_log(1, "STANDUP: %s", GetName());

	pack_position.header	= HEADER_GC_CHARACTER_POSITION;
	pack_position.vid		= GetVID();
	pack_position.position	= POSITION_GENERAL;

	PacketAround(&pack_position, sizeof(pack_position));
}

void CHARACTER::Sitdown(int is_ground)
{
	struct packet_position pack_position;

	if (IsPosition(POS_SITTING))
		return;

	SetPosition(POS_SITTING);
	sys_log(1, "SITDOWN: %s", GetName());

	pack_position.header	= HEADER_GC_CHARACTER_POSITION;
	pack_position.vid		= GetVID();
	pack_position.position	= POSITION_SITTING_GROUND;
	PacketAround(&pack_position, sizeof(pack_position));
}

void CHARACTER::SetRotation(float fRot)
{
	m_pointsInstant.fRot = fRot;
}

// x, y 방향으로 보고 선다.
void CHARACTER::SetRotationToXY(long x, long y)
{
	SetRotation(GetDegreeFromPositionXY(GetX(), GetY(), x, y));
}

bool CHARACTER::CannotMoveByAffect() const
{
	return (IsAffectFlag(AFF_STUN));
}

bool CHARACTER::CanMove() const
{
	if (CannotMoveByAffect())
		return false;

	// 0.2초 전이라면 움직일 수 없다.
	/*
	   if (get_float_time() - m_fSyncTime < 0.2f)
	   return false;
	 */
	return true;
}

// 무조건 x, y 위치로 이동 시킨다.
bool CHARACTER::Sync(long x, long y)
{
	if (!GetSectree())
		return false;

	LPSECTREE new_tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), x, y);

	if (!new_tree)
	{
		if (GetDesc())
		{
			sys_err("cannot find tree at %d %d (name: %s)", x, y, GetName());
			GetDesc()->SetPhase(PHASE_CLOSE);
		}
		else
		{
			sys_err("no tree: %s %d %d %d", GetName(), x, y, GetMapIndex());
			Dead();
		}

		return false;
	}

#ifdef ENABLE_CHECK_MOVESPEED_HACK
	if (IsPC())
	{
		if (GetCShield()->CheckMoveSpeedhack(x, y, get_dword_time(), IsRiding(), GetMoveSpeed()))
		{
			LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: Movespeed Hack", this);
			Show(GetMapIndex(), GetX(), GetY(), 0);
			Stop();
		}
	}
#endif

	SetRotationToXY(x, y);
	SetXYZ(x, y, 0);

	if (GetDungeon())
	{
		// 던젼용 이벤트 속성 변화
		int iLastEventAttr = m_iEventAttr;
		m_iEventAttr = new_tree->GetEventAttribute(x, y);

		if (m_iEventAttr != iLastEventAttr)
		{
			if (GetParty())
			{
				quest::CQuestManager::instance().AttrOut(GetParty()->GetLeaderPID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetParty()->GetLeaderPID(), this, m_iEventAttr);
			}
			else
			{
				quest::CQuestManager::instance().AttrOut(GetPlayerID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetPlayerID(), this, m_iEventAttr);
			}
		}
	}

	if (GetSectree() != new_tree)
	{
		if (!IsNPC())
		{
			SECTREEID id = new_tree->GetID();
			SECTREEID old_id = GetSectree()->GetID();

			const float fDist = DISTANCE_SQRT(id.coord.x - old_id.coord.x, id.coord.y - old_id.coord.y);
			sys_log(0, "SECTREE DIFFER: %s %dx%d was %dx%d dist %.1fm",
					GetName(),
					id.coord.x,
					id.coord.y,
					old_id.coord.x,
					old_id.coord.y,
					fDist);
		}

		new_tree->InsertEntity(this);
	}

	return true;
}

void CHARACTER::Stop()
{
	if (!IsState(m_stateIdle))
		MonsterLog("[IDLE] 정지");

	GotoState(m_stateIdle);

	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
}

bool CHARACTER::Goto(long x, long y)
{
	// TODO 거리체크 필요
	// 같은 위치면 이동할 필요 없음 (자동 성공)
	if (GetX() == x && GetY() == y)
		return false;

	if (m_posDest.x == x && m_posDest.y == y)
	{
		if (!IsState(m_stateMove))
		{
			m_dwStateDuration = 4;
			GotoState(m_stateMove);
		}
		return false;
	}

	m_posDest.x = x;
	m_posDest.y = y;

	CalculateMoveDuration();

	m_dwStateDuration = 4;


	if (!IsState(m_stateMove))
	{
		MonsterLog("[MOVE] %s", GetVictim() ? "대상추적" : "그냥이동");

		if (GetVictim())
		{
			//MonsterChat(MONSTER_CHAT_CHASE);
			MonsterChat(MONSTER_CHAT_ATTACK);
		}
	}

	GotoState(m_stateMove);

	return true;
}


DWORD CHARACTER::GetMotionMode() const
{
	DWORD dwMode = MOTION_MODE_GENERAL;

	if (IsPolymorphed())
		return dwMode;

	LPITEM pkItem;

	if ((pkItem = GetWear(WEAR_WEAPON)))
	{
		switch (pkItem->GetProto()->bSubType)
		{
			case WEAPON_SWORD:
				dwMode = MOTION_MODE_ONEHAND_SWORD;
				break;

			case WEAPON_TWO_HANDED:
				dwMode = MOTION_MODE_TWOHAND_SWORD;
				break;

			case WEAPON_DAGGER:
				dwMode = MOTION_MODE_DUALHAND_SWORD;
				break;

			case WEAPON_BOW:
				dwMode = MOTION_MODE_BOW;
				break;

			case WEAPON_BELL:
				dwMode = MOTION_MODE_BELL;
				break;

			case WEAPON_FAN:
				dwMode = MOTION_MODE_FAN;
				break;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case WEAPON_CLAW:
				dwMode = MOTION_MODE_CLAW;
				break;
#endif
		}
	}
	return dwMode;
}

float CHARACTER::GetMoveMotionSpeed() const
{
	DWORD dwMode = GetMotionMode();

	const CMotion * pkMotion = NULL;

	if (!GetMountVnum())
		pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(dwMode, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));
	else
	{
		pkMotion = CMotionManager::instance().GetMotion(GetMountVnum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));

		if (!pkMotion)
			pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_HORSE, (IsWalking() && IsPC()) ? MOTION_WALK : MOTION_RUN));
	}

	if (pkMotion)
		return -pkMotion->GetAccumVector().y / pkMotion->GetDuration();
	else
	{
		sys_err("cannot find motion (name %s race %d mode %d)", GetName(), GetRaceNum(), dwMode);
		return 300.0f;
	}
}

float CHARACTER::GetMoveSpeed() const
{
	return GetMoveMotionSpeed() * 10000 / CalculateDuration(GetLimitPoint(POINT_MOV_SPEED), 10000);
}

void CHARACTER::CalculateMoveDuration()
{
	m_posStart.x = GetX();
	m_posStart.y = GetY();

	float fDist = DISTANCE_SQRT(m_posStart.x - m_posDest.x, m_posStart.y - m_posDest.y);

	float motionSpeed = GetMoveMotionSpeed();

	m_dwMoveDuration = CalculateDuration(GetLimitPoint(POINT_MOV_SPEED),
			(int) ((fDist / motionSpeed) * 1000.0f));

	if (IsNPC())
		sys_log(1, "%s: GOTO: distance %f, spd %u, duration %u, motion speed %f pos %d %d -> %d %d",
				GetName(), fDist, GetLimitPoint(POINT_MOV_SPEED), m_dwMoveDuration, motionSpeed,
				m_posStart.x, m_posStart.y, m_posDest.x, m_posDest.y);

	m_dwMoveStartTime = get_dword_time();
}

// x y 위치로 이동 한다. (이동할 수 있는 가 없는 가를 확인 하고 Sync 메소드로 실제 이동 한다)
// 서버는 char의 x, y 값을 바로 바꾸지만,
// 클라에서는 이전 위치에서 바꾼 x, y까지 interpolation한다.
// 걷거나 뛰는 것은 char의 m_bNowWalking에 달려있다.
// Warp를 의도한 것이라면 Show를 사용할 것.
bool CHARACTER::Move(long x, long y)
{
	// 같은 위치면 이동할 필요 없음 (자동 성공)
	if (GetX() == x && GetY() == y)
		return true;

	if (test_server)
		if (m_bDetailLog)
			sys_log(0, "%s position %u %u", GetName(), x, y);

	OnMove();
	return Sync(x, y);
}
#ifdef ENABLE_SUPPORT_SYSTEM
void CHARACTER::SendSupportSkillPacket(DWORD skill_vnum, DWORD skill_level)
{
	TPacketGCSupportUseSkill pack;
	pack.bHeader = HEADER_GC_SUPPORT_SKILL;
	pack.dwVnum = skill_vnum;
	pack.dwVid = GetVID();
	pack.dwLevel = skill_level;
	
	PacketView(&pack, sizeof(TPacketGCSupportUseSkill), this);
}
#endif
void CHARACTER::SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, int iRot)
{
	TPacketGCMove pack;

	if (bFunc == FUNC_WAIT)
	{
		x = m_posDest.x;
		y = m_posDest.y;
		dwDuration = m_dwMoveDuration;
	}

	EncodeMovePacket(pack, GetVID(), bFunc, bArg, x, y, dwDuration, dwTime, iRot == -1 ? (int) GetRotation() / 5 : iRot);
	PacketView(&pack, sizeof(TPacketGCMove), this);
}

int CHARACTER::GetRealPoint(BYTE type) const
{
	return m_points.points[type];
}

void CHARACTER::SetRealPoint(BYTE type, int val)
{
	m_points.points[type] = val;
}

int CHARACTER::GetPolymorphPoint(BYTE type) const
{
	if (IsPolymorphed() && !IsPolyMaintainStat())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);
		int iPower = GetPolymorphPower();

		if (pMob)
		{
			switch (type)
			{
				case POINT_ST:
					if ((GetJob() == JOB_SHAMAN) || ((GetJob() == JOB_SURA) && (GetSkillGroup() == 2)))
						return pMob->m_table.bStr * iPower / 100 + GetPoint(POINT_IQ);
					return pMob->m_table.bStr * iPower / 100 + GetPoint(POINT_ST);

				case POINT_HT:
					return pMob->m_table.bCon * iPower / 100 + GetPoint(POINT_HT);

				case POINT_IQ:
					return pMob->m_table.bInt * iPower / 100 + GetPoint(POINT_IQ);

				case POINT_DX:
					return pMob->m_table.bDex * iPower / 100 + GetPoint(POINT_DX);
			}
		}
	}

	return GetPoint(type);
}

long long CHARACTER::GetPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	long long val = m_pointsInstant.points[type];
	long long max_val = LLONG_MAX;

	switch (type)
	{
		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			max_val = 50;
			break;
		case POINT_GOLD:
			max_val = GOLD_MAX;
			break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR: %s type %d val %lld (max: %lld)", GetName(), val, max_val);

	return (val);
}

int CHARACTER::GetLimitPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	int val = m_pointsInstant.points[type];
#ifdef ENABLE_MOB_MOVE_SPEED
	if (IsMonster() && GetMobRank() < MOB_RANK_KING && POINT_MOV_SPEED == type)
		val = GetProtectTime("is_aggregate_monster") == 1 ? 300 : 150;
#endif
	int max_val = INT_MAX;
	int limit = INT_MAX;
	int min_limit = -INT_MAX;

	switch (type)
	{
		case POINT_ATT_SPEED:
			min_limit = 0;

			if (IsPC())
				limit = 170;
			else
				limit = 250;
			break;

		case POINT_MOV_SPEED:
			min_limit = 0;

			if (IsPC())
				limit = 200;
			else
#ifdef ENABLE_MOB_MOVE_SPEED
				limit = (IsMonster() && GetMobRank() < MOB_RANK_KING) ? 350 : 250;
#else
				limit = 250;
#endif
			break;

		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			limit = 50;
			max_val = 50;
			break;

		case POINT_MALL_ATTBONUS:
		case POINT_MALL_DEFBONUS:
			limit = 20;
			max_val = 50;
			break;

		//SERGIO-CRITICAL
		//case POINT_CRITICAL_PCT:
		//	if(IsPC())
		//		limit = 100;
		//		max_val = 100;
		//	break;
	}

	if (val > max_val)
		//sys_err("POINT_ERROR: %s type %d val %d (max: %d)", GetName(), val, max_val);
		sys_err("POINT_ERROR: %s type %d val %d (max: %d)", GetName(), type, val, max_val);

	if (val > limit)
		val = limit;

	if (val < min_limit)
		val = min_limit;

	return (val);
}

void CHARACTER::SetPoint(BYTE type, long long val)
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return;
	}

	m_pointsInstant.points[type] = val;

	// 아직 이동이 다 안끝났다면 이동 시간 계산을 다시 해야 한다.
	if (type == POINT_MOV_SPEED && get_dword_time() < m_dwMoveStartTime + m_dwMoveDuration)
	{
		CalculateMoveDuration();
	}
}

long long CHARACTER::GetAllowedGold() const
{
	if (GetLevel() <= 10)
		return 100000;
	else if (GetLevel() <= 20)
		return 500000;
	else
		return 50000000;
}

void CHARACTER::CheckMaximumPoints()
{
	if (GetMaxHP() < GetHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetMaxSP() < GetSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());
}

void CHARACTER::PointChange(BYTE type, long long amount, bool bAmount, bool bBroadcast)
{
	long long val = 0;

	//sys_log(0, "PointChange %d %d | %d -> %d cHP %d mHP %d", type, amount, GetPoint(type), GetPoint(type)+amount, GetHP(), GetMaxHP());

	switch (type)
	{
		case POINT_NONE:
		case POINT_BATTLE_PASS_ID:
			return;

#ifdef ENABLE_CONQUEROR_LEVEL
		case POINT_CONQUEROR_LEVEL:
			if ((GetConquerorLevel() + amount) > gPlayerConquerorMaxLevel)
				return;

			SetConquerorLevel(GetConquerorLevel() + amount);
			val = GetConquerorLevel();

			sys_log(0, "CONQUEROR_LEVELUP: %s %d NEXT EXP %d", GetName(), GetConquerorLevel(), GetConquerorNextExp());
#ifdef ENABLE_WOLFMAN_CHARACTER
			if (GetJob() == JOB_WOLFMAN)
			{
				if ((5 <= val) && (GetSkillGroup()!=1))
				{
					ClearSkill();
					SetSkillGroup(1);
					SetRealPoint(POINT_SKILL, GetLevel()-1);
					SetPoint(POINT_SKILL, GetRealPoint(POINT_SKILL));
					PointChange(POINT_SKILL, 0);
				}
			}
#endif
			PointChange(POINT_CONQUEROR_NEXT_EXP,	GetConquerorNextExp(), false);
			break;
		case POINT_CONQUEROR_NEXT_EXP:
			val = GetConquerorNextExp();
			bAmount = false;
			break;

		case POINT_CONQUEROR_EXP:
			{
				DWORD exp = GetConquerorExp();
				DWORD next_exp = GetConquerorNextExp();
				
				if (block_exp)
					return;

				if ((amount < 0) && (exp < (DWORD)(-amount)))
				{
					sys_log(1, "%s AMOUNT < 0 %d, CUR CONQUEROR EXP: %d", GetName(), -amount, exp);
					amount = -exp;

					SetConquerorExp(exp + amount);
					val = GetExp();
				}
				else
				{
					if (gPlayerConquerorMaxLevel <= GetConquerorLevel())
						return;

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "You have gained %d conqueror exp.", amount);

					DWORD iExpBalance = 0;

					// ?? ?!
					if (exp + amount >= next_exp)
					{
						iExpBalance = (exp + amount) - next_exp;
						amount = next_exp - exp;

						SetConquerorExp(0);
						exp = next_exp;
					}
					else
					{
						SetConquerorExp(exp + amount);
						exp = GetConquerorExp();
					}

					DWORD q = DWORD(next_exp / 4.0f);
					int iLevStep = GetRealPoint(POINT_CONQUEROR_LEVEL_STEP);

					// iLevStep? 4 ???? ??? ???? ??? ??? ? ? ?? ???.
					if (iLevStep >= 4)
					{
						sys_err("%s CONQUEROR_LEVEL_STEP bigger than 4! (%d)", GetName(), iLevStep);
						iLevStep = 4;
					}

					if (exp >= next_exp && iLevStep < 4)
					{
						for (int i = 0; i < 4 - iLevStep; ++i)
							PointChange(POINT_CONQUEROR_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 3 && iLevStep < 3)
					{
						for (int i = 0; i < 3 - iLevStep; ++i)
							PointChange(POINT_CONQUEROR_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 2 && iLevStep < 2)
					{
						for (int i = 0; i < 2 - iLevStep; ++i)
							PointChange(POINT_CONQUEROR_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q && iLevStep < 1)
						PointChange(POINT_CONQUEROR_LEVEL_STEP, 1);

					if (iExpBalance)
					{
						PointChange(POINT_CONQUEROR_EXP, iExpBalance);
					}

					val = GetConquerorExp();
				}
			}
			break;
		case POINT_CONQUEROR_LEVEL_STEP:

			if (amount > 0)
			{
				val = GetPoint(POINT_CONQUEROR_LEVEL_STEP) + amount;
				switch (val)
				{
					case 1:
					case 2:
					case 3:
						{
							if ((GetConquerorLevel() <= g_iStatusPointGetLevelLimit) && (GetConquerorLevel() <= gPlayerConquerorMaxLevel) )
								PointChange(POINT_CONQUEROR_POINT, 1);
						}
						break;
					case 4:
						{
							PointChange(POINT_CONQUEROR_LEVEL, 1, false, true);
							val = 0;
						}
						break;

				}

				SetPoint(POINT_CONQUEROR_LEVEL_STEP, val);
				SetRealPoint(POINT_CONQUEROR_LEVEL_STEP, val);

				Save();
			}
			else
				val = GetPoint(POINT_CONQUEROR_LEVEL_STEP);

			break;

		case POINT_SUNGMA_STR:
		case POINT_SUNGMA_HP:
		case POINT_SUNGMA_MOVE:
		case POINT_SUNGMA_INMUNE:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;
		case POINT_CONQUEROR_POINT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			SetRealPoint(type, val);
			break;
#endif

		case POINT_LEVEL:
			if ((GetLevel() + amount) > gPlayerMaxLevel)
				return;

			SetLevel(GetLevel() + amount);
			val = GetLevel();

			sys_log(0, "LEVELUP: %s %d NEXT EXP %d", GetName(), GetLevel(), GetNextExp());
#ifdef ENABLE_WOLFMAN_CHARACTER
			if (GetJob() == JOB_WOLFMAN)
			{
				if ((5 <= val) && (GetSkillGroup()!=1))
				{
					ClearSkill();
					// set skill group
					SetSkillGroup(1);
					// set skill points
					SetRealPoint(POINT_SKILL, GetLevel()-1);
					SetPoint(POINT_SKILL, GetRealPoint(POINT_SKILL));
					PointChange(POINT_SKILL, 0);
					// update points (not required)
					// ComputePoints();
					// PointsPacket();
				}
			}
#endif
			PointChange(POINT_NEXT_EXP,	GetNextExp(), false);

			if (amount)
			{
				quest::CQuestManager::instance().LevelUp(GetPlayerID());
#ifdef ENABLE_BIYOLOG
				CheckBio();
#endif
#ifdef ENABLE_BATTLE_PASS
				CHARACTER_MANAGER::Instance().DoMission(this, MISSION_LEVEL_UP, amount, 0);
#endif
#ifdef ENABLE_AUTO_SELECT_SKILL
				if(GetSkillGroup() == 0 && GetLevel() >= 5)
					ChatPacket(CHAT_TYPE_COMMAND, "OpenAutoSkill");
#endif
				LogManager::instance().LevelLog(this, val, GetRealPoint(POINT_PLAYTIME) + (get_dword_time() - m_dwPlayStartTime) / 60000);

				if (GetGuild())
				{
					GetGuild()->LevelChange(GetPlayerID(), GetLevel());
				}

				if (GetParty())
				{
					GetParty()->RequestSetMemberLevel(GetPlayerID(), GetLevel());
				}
			}
			break;

		case POINT_NEXT_EXP:
			val = GetNextExp();
			bAmount = false;	// 무조건 bAmount는 false 여야 한다.
			break;

		case POINT_EXP:
			{
				DWORD exp = GetExp();
				DWORD next_exp = GetNextExp();

				// 청소년보호
				if (g_bChinaIntoxicationCheck)
				{
					if (IsOverTime(OT_NONE))
					{
						dev_log(LOG_DEB0, "<EXP_LOG> %s = NONE", GetName());
					}
					else if (IsOverTime(OT_3HOUR))
					{
						amount = (amount / 2);
						dev_log(LOG_DEB0, "<EXP_LOG> %s = 3HOUR", GetName());
					}
					else if (IsOverTime(OT_5HOUR))
					{
						amount = 0;
						dev_log(LOG_DEB0, "<EXP_LOG> %s = 5HOUR", GetName());
					}
				}

				// exp가 0 이하로 가지 않도록 한다
				if ((amount < 0) && (exp < (DWORD)(-amount)))
				{
					sys_log(1, "%s AMOUNT < 0 %d, CUR EXP: %d", GetName(), -amount, exp);
					amount = -exp;

					SetExp(exp + amount);
					val = GetExp();
				}
				else
				{
					if (gPlayerMaxLevel <= GetLevel())
						return;
#ifdef __ANTI_EXP__
					if (GetQuestFlag("anti_exp.block_exp") == 1)
					{
						return;
					}
#endif

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "You have gained %d exp.", amount);

					DWORD iExpBalance = 0;

					// 레벨 업!
					if (exp + amount >= next_exp)
					{
						iExpBalance = (exp + amount) - next_exp;
						amount = next_exp - exp;

						SetExp(0);
						exp = next_exp;
					}
					else
					{
						SetExp(exp + amount);
						exp = GetExp();
					}

					DWORD q = DWORD(next_exp / 4.0f);
					int iLevStep = GetRealPoint(POINT_LEVEL_STEP);

					// iLevStep이 4 이상이면 레벨이 올랐어야 하므로 여기에 올 수 없는 값이다.
					if (iLevStep >= 4)
					{
						sys_err("%s LEVEL_STEP bigger than 4! (%d)", GetName(), iLevStep);
						iLevStep = 4;
					}

					if (exp >= next_exp && iLevStep < 4)
					{
						for (int i = 0; i < 4 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 3 && iLevStep < 3)
					{
						for (int i = 0; i < 3 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 2 && iLevStep < 2)
					{
						for (int i = 0; i < 2 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q && iLevStep < 1)
						PointChange(POINT_LEVEL_STEP, 1);

					if (iExpBalance)
					{
						PointChange(POINT_EXP, iExpBalance);
					}

					val = GetExp();
				}
			}
			break;

		case POINT_LEVEL_STEP:
			if (amount > 0)
			{
				val = GetPoint(POINT_LEVEL_STEP) + amount;

				switch (val)
				{
					case 1:
					case 2:
					case 3:
						if ((GetLevel() <= g_iStatusPointGetLevelLimit) &&
							(GetLevel() <= gPlayerMaxLevel) ) // @fixme104
							PointChange(POINT_STAT, 1);
						break;

					case 4:
						{
							int iHP = number(JobInitialPoints[GetJob()].hp_per_lv_begin, JobInitialPoints[GetJob()].hp_per_lv_end);
							int iSP = number(JobInitialPoints[GetJob()].sp_per_lv_begin, JobInitialPoints[GetJob()].sp_per_lv_end);

							m_points.iRandomHP += iHP;
							m_points.iRandomSP += iSP;

							if (GetSkillGroup())
							{
								if (GetLevel() >= 5)
									PointChange(POINT_SKILL, 1);

								if (GetLevel() >= 9)
									PointChange(POINT_SUB_SKILL, 1);
							}

							PointChange(POINT_MAX_HP, iHP);
							PointChange(POINT_MAX_SP, iSP);
							PointChange(POINT_LEVEL, 1, false, true);

							val = 0;
						}
						break;
				}

				// if (GetLevel() <= 10)
					// AutoGiveItem(27001, 2);
				// else if (GetLevel() <= 30)
					// AutoGiveItem(27002, 2);
				// else
				// {
					// AutoGiveItem(27002, 2);
				// }

				PointChange(POINT_HP, GetMaxHP() - GetHP());
				PointChange(POINT_SP, GetMaxSP() - GetSP());
				PointChange(POINT_STAMINA, GetMaxStamina() - GetStamina());

				SetPoint(POINT_LEVEL_STEP, val);
				SetRealPoint(POINT_LEVEL_STEP, val);

				Save();
			}
			else
				val = GetPoint(POINT_LEVEL_STEP);

			break;

		case POINT_HP:
			{
				if (IsDead() || IsStun())
					return;

				int prev_hp = GetHP();

				amount = MIN(GetMaxHP() - GetHP(), amount);
				SetHP(GetHP() + amount);
				val = GetHP();

				BroadcastTargetPacket();

				if (GetParty() && IsPC() && val != prev_hp)
					GetParty()->SendPartyInfoOneToAll(this);
			}
			break;

		case POINT_SP:
			{
				if (IsDead() || IsStun())
					return;

				amount = MIN(GetMaxSP() - GetSP(), amount);
				SetSP(GetSP() + amount);
				val = GetSP();
			}
			break;

		case POINT_STAMINA:
			{
				if (IsDead() || IsStun())
					return;

				int prev_val = GetStamina();
				amount = MIN(GetMaxStamina() - GetStamina(), amount);
				SetStamina(GetStamina() + amount);
				val = GetStamina();

				if (val == 0)
				{
					// Stamina가 없으니 걷자!
					SetNowWalking(true);
				}
				else if (prev_val == 0)
				{
					// 없던 스테미나가 생겼으니 이전 모드 복귀
					ResetWalking();
				}

				if (amount < 0 && val != 0) // 감소는 보내지않는다.
					return;
			}
			break;

		case POINT_MAX_HP:
			{
				SetPoint(type, GetPoint(type) + amount);

				//SetMaxHP(GetMaxHP() + amount);
				// 최대 생명력 = (기본 최대 생명력 + 추가) * 최대생명력%
				int hp = GetRealPoint(POINT_MAX_HP);
				int add_hp = MIN(3500, hp * GetPoint(POINT_MAX_HP_PCT) / 100);
				add_hp += GetPoint(POINT_MAX_HP);
				add_hp += GetPoint(POINT_PARTY_TANKER_BONUS);

				SetMaxHP(hp + add_hp);
#ifdef ENABLE_CONQUEROR_LEVEL
				if (IsConquerorMap(GetMapIndex()) && IsPC())
				{
					int value = 0; 
					int aValue = 0;

					value = GetPoint(POINT_SUNGMA_HP);
					aValue = SECTREE_MANAGER::instance().GetSungmaValueAffectByRegion(GetMapIndex(), AFFECT_SUNGMA_HP); 
					if(value < aValue)
						SetMaxHP(hp + add_hp/2);
				}
#endif

				val = GetMaxHP();
			}
			break;

		case POINT_MAX_SP:
			{
				SetPoint(type, GetPoint(type) + amount);

				//SetMaxSP(GetMaxSP() + amount);
				// 최대 정신력 = (기본 최대 정신력 + 추가) * 최대정신력%
				int sp = GetRealPoint(POINT_MAX_SP);
				int add_sp = MIN(800, sp * GetPoint(POINT_MAX_SP_PCT) / 100);
				add_sp += GetPoint(POINT_MAX_SP);
				add_sp += GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);

				SetMaxSP(sp + add_sp);

				val = GetMaxSP();
			}
			break;

		case POINT_MAX_HP_PCT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			PointChange(POINT_MAX_HP, 0);
			break;

		case POINT_MAX_SP_PCT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			PointChange(POINT_MAX_SP, 0);
			break;

		case POINT_MAX_STAMINA:
			SetMaxStamina(GetMaxStamina() + amount);
			val = GetMaxStamina();
			break;
#ifdef __GEM_SYSTEM__
		case POINT_GEM:
		{
			const int64_t nTotalGem = static_cast<int64_t>(GetGem()) + static_cast<long long>(amount);
			if (GEM_MAX <= nTotalGem)
			{
				sys_err("[OVERFLOW_GEM] OriGem %d AddedGem %lld id %u Name %s ", GetGem(), amount, GetPlayerID(), GetName());
				return;
			}
			SetGem(GetGem() + amount);
			val = GetGem();
		}
		break;
#endif
		case POINT_GOLD:
			{
				//const long long nTotalMoney = static_cast<long long>(GetGold()) + static_cast<long long>(amount);
				//if (GOLD_MAX <= nTotalMoney)
				if (amount > 0 && GetGold() >= GOLD_MAX)
				{
					sys_err("[OVERFLOW_GOLD] OriGold %lld AddedGold %lld id %u Name %s ", GetGold(), amount, GetPlayerID(), GetName());
					LogManager::instance().CharLog(this, GetGold() + amount, "OVERFLOW_GOLD", "");
					return;
				}

				// 청소년보호
				if (g_bChinaIntoxicationCheck && amount > 0)
				{
					if (IsOverTime(OT_NONE))
					{
						dev_log(LOG_DEB0, "<GOLD_LOG> %s = NONE", GetName());
					}
					else if (IsOverTime(OT_3HOUR))
					{
						amount = (amount / 2);
						dev_log(LOG_DEB0, "<GOLD_LOG> %s = 3HOUR", GetName());
					}
					else if (IsOverTime(OT_5HOUR))
					{
						amount = 0;
						dev_log(LOG_DEB0, "<GOLD_LOG> %s = 5HOUR", GetName());
					}
				}
#ifdef ENABLE_BATTLE_PASS
				if(amount > 0)
					CHARACTER_MANAGER::Instance().DoMission(this, MISSION_EARN_MONEY, amount, 0);
				else if (amount < 0)
					CHARACTER_MANAGER::Instance().DoMission(this, MISSION_SPEND_MONEY, amount, 0);
#endif
				long long newTotalMoney = static_cast<long long>(GetGold()) + static_cast<long long>(amount);
				if(newTotalMoney < 0)
					newTotalMoney = 0;
				else if(newTotalMoney > GOLD_MAX)
					newTotalMoney = GOLD_MAX;

				SetGold(newTotalMoney);
				val = GetGold();
			}
			break;

		case POINT_SKILL:
		case POINT_STAT:
		case POINT_SUB_SKILL:
		case POINT_STAT_RESET_COUNT:
		case POINT_HORSE_SKILL:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			SetRealPoint(type, val);
			break;

		case POINT_DEF_GRADE:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);

			PointChange(POINT_CLIENT_DEF_GRADE, amount);
			break;

		case POINT_CLIENT_DEF_GRADE:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;
		case POINT_MOV_SPEED:
		{
			#if defined(WJ_COMBAT_ZONE)	
			if (FindAffect(AFFECT_COMBAT_ZONE_MOVEMENT))
			{
				SetPoint(type, COMBAT_ZONE_REDUCTION_MAX_MOVEMENT_SPEED);
				val = GetPoint(type);
				break;
			}
			#endif

#ifdef ENABLE_CONQUEROR_LEVEL
			if (IsConquerorMap(GetMapIndex()) && IsPC())
			{
				int value = 0; 
				int aValue = 0;
				value = GetPoint(POINT_SUNGMA_MOVE);
				aValue = SECTREE_MANAGER::instance().GetSungmaValueAffectByRegion(GetMapIndex(), AFFECT_SUNGMA_MOVE); 
				if(value < aValue)
					SetPoint(type, 140);
				else
					SetPoint(type, 209);
			}


			val = GetPoint(type);
#endif		
			
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
		}
		break;

		case POINT_ST:
		case POINT_HT:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HP_REGEN:
		case POINT_SP_REGEN:
		case POINT_ATT_SPEED:
		case POINT_ATT_GRADE:
		case POINT_CASTING_SPEED:
		case POINT_MAGIC_ATT_GRADE:
		case POINT_MAGIC_DEF_GRADE:
		case POINT_BOW_DISTANCE:
		case POINT_HP_RECOVERY:
		case POINT_SP_RECOVERY:

		case POINT_ATTBONUS_HUMAN:	// 42 인간에게 강함
		case POINT_ATTBONUS_ANIMAL:	// 43 동물에게 데미지 % 증가
		case POINT_ATTBONUS_ORC:		// 44 웅귀에게 데미지 % 증가
		case POINT_ATTBONUS_MILGYO:	// 45 밀교에게 데미지 % 증가
		case POINT_ATTBONUS_UNDEAD:	// 46 시체에게 데미지 % 증가
		case POINT_ATTBONUS_DEVIL:	// 47 마귀(악마)에게 데미지 % 증가

		case POINT_ATTBONUS_MONSTER:
		case POINT_ATTBONUS_SURA:
		case POINT_ATTBONUS_ASSASSIN:
		case POINT_ATTBONUS_WARRIOR:
		case POINT_ATTBONUS_SHAMAN:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_WOLFMAN:
#endif

		case POINT_POISON_PCT:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_PCT:
#endif
		case POINT_STUN_PCT:
		case POINT_SLOW_PCT:

		case POINT_BLOCK:
		case POINT_DODGE:

		case POINT_CRITICAL_PCT:
		case POINT_RESIST_CRITICAL:
		case POINT_PENETRATE_PCT:
		case POINT_RESIST_PENETRATE:
		case POINT_CURSE_PCT:

		case POINT_STEAL_HP:		// 48 생명력 흡수
		case POINT_STEAL_SP:		// 49 정신력 흡수

		case POINT_MANA_BURN_PCT:	// 50 마나 번
		case POINT_DAMAGE_SP_RECOVER:	// 51 공격당할 시 정신력 회복 확률
		case POINT_RESIST_NORMAL_DAMAGE:
		case POINT_RESIST_SWORD:
		case POINT_RESIST_TWOHAND:
		case POINT_RESIST_DAGGER:
		case POINT_RESIST_BELL:
		case POINT_RESIST_FAN:
		case POINT_RESIST_BOW:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW:
#endif
		case POINT_RESIST_FIRE:
		case POINT_RESIST_ELEC:
		case POINT_RESIST_MAGIC:
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case POINT_ACCEDRAIN_RATE:
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case POINT_RESIST_MAGIC_REDUCTION:
#endif
		case POINT_RESIST_WIND:
		case POINT_RESIST_ICE:
		case POINT_RESIST_EARTH:
		case POINT_RESIST_DARK:
		case POINT_REFLECT_MELEE:	// 67 공격 반사
		case POINT_REFLECT_CURSE:	// 68 저주 반사
		case POINT_POISON_REDUCE:	// 69 독데미지 감소
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_REDUCE:
#endif
		case POINT_KILL_SP_RECOVER:	// 70 적 소멸시 MP 회복
		case POINT_KILL_HP_RECOVERY:	// 75
		case POINT_HIT_HP_RECOVERY:
		case POINT_HIT_SP_RECOVERY:
		case POINT_MANASHIELD:
		case POINT_ATT_BONUS:
		case POINT_DEF_BONUS:
		case POINT_SKILL_DAMAGE_BONUS:
		case POINT_NORMAL_HIT_DAMAGE_BONUS:
#ifdef ELEMENT_SPELL_WORLDARD
		case POINT_ATT_ELEMENTS:
#endif
#ifdef ELEMENT_NEW_BONUSES
		case POINT_ATTBONUS_ELEC:
		case POINT_ATTBONUS_FIRE:
		case POINT_ATTBONUS_ICE:
		case POINT_ATTBONUS_WIND:
		case POINT_ATTBONUS_EARTH:
		case POINT_ATTBONUS_DARK:
#endif
			// DEPEND_BONUS_ATTRIBUTES
		case POINT_SKILL_DEFEND_BONUS:
		case POINT_NORMAL_HIT_DEFEND_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;
			// END_OF_DEPEND_BONUS_ATTRIBUTES

		case POINT_PARTY_ATTACKER_BONUS:
		case POINT_PARTY_TANKER_BONUS:
		case POINT_PARTY_BUFFER_BONUS:
		case POINT_PARTY_SKILL_MASTER_BONUS:
		case POINT_PARTY_HASTE_BONUS:
		case POINT_PARTY_DEFENDER_BONUS:

		case POINT_RESIST_WARRIOR :
		case POINT_RESIST_ASSASSIN :
		case POINT_RESIST_SURA :
		case POINT_RESIST_SHAMAN :
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_WOLFMAN :
#endif

#ifdef ENABLE_NEW_BONUS_WORLDARD
		case  POINT_RESIST_HUMAN:
#endif
#ifdef ELEMENT_NEW_BONUSES
		case POINT_ENCHANT_ELEC:
		case POINT_ENCHANT_FIRE:
		case POINT_ENCHANT_ICE:
		case POINT_ENCHANT_WIND:
		case POINT_ENCHANT_EARTH:
		case POINT_ENCHANT_DARK:

		case POINT_ATTBONUS_SWORD:		
		case POINT_ATTBONUS_TWOHAND:	
		case POINT_ATTBONUS_DAGGER:	
		case POINT_ATTBONUS_BELL:		
		case POINT_ATTBONUS_FAN:		
		case POINT_ATTBONUS_BOW:

		case POINT_ATTBONUS_ZODIAC:
		case POINT_ATTBONUS_DESERT:
		case POINT_ATTBONUS_INSECT:	
#endif

#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_CLAW:
#endif
#ifdef NEW_BONUS
		case POINT_ATTBONUS_STONE:
		case POINT_ATTBONUS_BOSS:
#endif


			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

		case POINT_MALL_ATTBONUS:
		case POINT_MALL_DEFBONUS:
		case POINT_MALL_EXPBONUS:
		case POINT_MALL_ITEMBONUS:
		case POINT_MALL_GOLDBONUS:
		case POINT_MELEE_MAGIC_ATT_BONUS_PER:
			if (GetPoint(type) + amount > 100)
			{
				sys_err("MALL_BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
				amount = 100 - GetPoint(type);
			}

			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

			// PC_BANG_ITEM_ADD
		case POINT_PC_BANG_EXP_BONUS :
		case POINT_PC_BANG_DROP_BONUS :
		case POINT_RAMADAN_CANDY_BONUS_EXP:
			SetPoint(type, amount);
			val = GetPoint(type);
			break;
			// END_PC_BANG_ITEM_ADD

		case POINT_EXP_DOUBLE_BONUS:	// 71
		case POINT_GOLD_DOUBLE_BONUS:	// 72
		case POINT_ITEM_DROP_BONUS:	// 73
		case POINT_POTION_BONUS:	// 74
			//SERGIO-GOLD_MAX 
			if (GetPoint(type) + amount > 100)
			{
				sys_err("BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
				amount = 100 - GetPoint(type);
			}

			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			break;

		case POINT_IMMUNE_STUN:		// 76
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			if (val)
			{
				// ChatPacket(CHAT_TYPE_INFO, "IMMUNE_STUN SET_BIT type(%u) amount(%d)", type, amount);
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
			}
			else
			{
				// ChatPacket(CHAT_TYPE_INFO, "IMMUNE_STUN REMOVE_BIT type(%u) amount(%d)", type, amount);
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
			}
			break;

		case POINT_IMMUNE_SLOW:		// 77
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
			}
			break;

		case POINT_IMMUNE_FALL:	// 78
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
			}
			break;

		case POINT_ATT_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_ATT_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_DEF_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_DEF_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_MAGIC_ATT_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_MAGIC_ATT_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_MAGIC_DEF_GRADE_BONUS:
			SetPoint(type, GetPoint(type) + amount);
			PointChange(POINT_MAGIC_DEF_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_VOICE:
		case POINT_EMPIRE_POINT:
			//sys_err("CHARACTER::PointChange: %s: point cannot be changed. use SetPoint instead (type: %d)", GetName(), type);
			val = GetRealPoint(type);
			break;

		case POINT_POLYMORPH:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			SetPolymorph(val);
			break;

		case POINT_MOUNT:
			SetPoint(type, GetPoint(type) + amount);
			val = GetPoint(type);
			MountVnum(val);
			break;

		case POINT_ENERGY:
		case POINT_COSTUME_ATTR_BONUS:
			{
				int old_val = GetPoint(type);
				SetPoint(type, old_val + amount);
				val = GetPoint(type);
				BuffOnAttr_ValueChange(type, old_val, val);
			}
			break;

		default:
			sys_err("CHARACTER::PointChange: %s: unknown point change type %d", GetName(), type);
			return;
	}

	switch (type)
	{
		case POINT_LEVEL:
		case POINT_ST:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HT:
#ifdef ENABLE_CONQUEROR_LEVEL
		case POINT_CONQUEROR_LEVEL:
		case POINT_SUNGMA_STR:
		case POINT_SUNGMA_HP:
		case POINT_SUNGMA_MOVE:
		case POINT_SUNGMA_INMUNE:
#endif

			ComputeBattlePoints();
			break;
		case POINT_MAX_HP:
		case POINT_MAX_SP:
		case POINT_MAX_STAMINA:
			break;
	}

	if (type == POINT_HP && amount == 0)
		return;

	if (GetDesc())
	{
		struct packet_point_change pack;

		pack.header = HEADER_GC_CHARACTER_POINT_CHANGE;
		pack.dwVID = m_vid;
		pack.type = type;
		pack.value = val;

		if (bAmount)
			pack.amount = amount;
		else
			pack.amount = 0;

		if (!bBroadcast)
			GetDesc()->Packet(&pack, sizeof(struct packet_point_change));
		else
			PacketAround(&pack, sizeof(pack));
	}
}
#ifdef __NEWPET_SYSTEM__
void CHARACTER::SendPetLevelUpEffect(int vid, int type, int value, int amount) {
	struct packet_point_change pack;

	pack.header = HEADER_GC_CHARACTER_POINT_CHANGE;
	pack.dwVID = vid;
	pack.type = type;
	pack.value = value;
	pack.amount = amount;
	PacketAround(&pack, sizeof(pack));
}
#endif
void CHARACTER::ApplyPoint(BYTE bApplyType, int iVal)
{
	switch (bApplyType)
	{
		case APPLY_NONE:			// 0
			break;;

		case APPLY_CON:
			PointChange(POINT_HT, iVal);
			PointChange(POINT_MAX_HP, (iVal * JobInitialPoints[GetJob()].hp_per_ht));
			PointChange(POINT_MAX_STAMINA, (iVal * JobInitialPoints[GetJob()].stamina_per_con));
			break;

		case APPLY_INT:
			PointChange(POINT_IQ, iVal);
			PointChange(POINT_MAX_SP, (iVal * JobInitialPoints[GetJob()].sp_per_iq));
			break;

		case APPLY_SKILL:
			// SKILL_DAMAGE_BONUS
			{
				// 최상위 비트 기준으로 8비트 vnum, 9비트 add, 15비트 change
				// 00000000 00000000 00000000 00000000
				// ^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^
				// vnum     ^ add       change
				BYTE bSkillVnum = (BYTE) (((DWORD)iVal) >> 24);
				int iAdd = iVal & 0x00800000;
				int iChange = iVal & 0x007fffff;

				sys_log(1, "APPLY_SKILL skill %d add? %d change %d", bSkillVnum, iAdd ? 1 : 0, iChange);

				if (0 == iAdd)
					iChange = -iChange;

				boost::unordered_map<BYTE, int>::iterator iter = m_SkillDamageBonus.find(bSkillVnum);

				if (iter == m_SkillDamageBonus.end())
					m_SkillDamageBonus.insert(std::make_pair(bSkillVnum, iChange));
				else
					iter->second += iChange;
			}
			// END_OF_SKILL_DAMAGE_BONUS
			break;

		// NOTE: 아이템에 의한 최대HP 보너스나 퀘스트 보상 보너스가 똑같은 방식을 사용하므로
		// 그냥 MAX_HP만 계산하면 퀘스트 보상의 경우 문제가 생김. 사실 원래 이쪽이 합리적이기도 하고..
		// 바꾼 공식은 현재 최대 hp와 보유 hp의 비율을 구한 뒤 바뀔 최대 hp를 기준으로 hp를 보정한다.
		// 원래 PointChange에서 하는게 좋을것 같은데 설계 문제로 어려워서 skip..
		// SP도 똑같이 계산한다.
		// Mantis : 101460			~ ity ~
		case APPLY_MAX_HP:
		case APPLY_MAX_HP_PCT:
			{
				int i = GetMaxHP(); if(i == 0) break;
				PointChange(aApplyInfo[bApplyType].bPointType, iVal);
				float fRatio = (float)GetMaxHP() / (float)i;
				PointChange(POINT_HP, GetHP() * fRatio - GetHP());
			}
			break;

		case APPLY_MAX_SP:
		case APPLY_MAX_SP_PCT:
			{
				int i = GetMaxSP(); if(i == 0) break;
				PointChange(aApplyInfo[bApplyType].bPointType, iVal);
				float fRatio = (float)GetMaxSP() / (float)i;
				PointChange(POINT_SP, GetSP() * fRatio - GetSP());
			}
			break;

		case APPLY_STR:
		case APPLY_DEX:
		case APPLY_ATT_SPEED:
		case APPLY_MOV_SPEED:
		case APPLY_CAST_SPEED:
		case APPLY_HP_REGEN:
		case APPLY_SP_REGEN:
		case APPLY_POISON_PCT:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_BLEEDING_PCT:
#endif
		case APPLY_STUN_PCT:
		case APPLY_SLOW_PCT:
		case APPLY_CRITICAL_PCT:
		case APPLY_PENETRATE_PCT:
		case APPLY_ATTBONUS_HUMAN:
		case APPLY_ATTBONUS_ANIMAL:
		case APPLY_ATTBONUS_ORC:
		case APPLY_ATTBONUS_MILGYO:
		case APPLY_ATTBONUS_UNDEAD:
		case APPLY_ATTBONUS_DEVIL:
		case APPLY_ATTBONUS_WARRIOR:	// 59
		case APPLY_ATTBONUS_ASSASSIN:	// 60
		case APPLY_ATTBONUS_SURA:	// 61
		case APPLY_ATTBONUS_SHAMAN:	// 62
#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_ATTBONUS_WOLFMAN:
#endif
		case APPLY_ATTBONUS_MONSTER:	// 63
		case APPLY_STEAL_HP:
		case APPLY_STEAL_SP:
		case APPLY_MANA_BURN_PCT:
		case APPLY_DAMAGE_SP_RECOVER:
		case APPLY_BLOCK:
		case APPLY_DODGE:
		case APPLY_RESIST_SWORD:
		case APPLY_RESIST_TWOHAND:
		case APPLY_RESIST_DAGGER:
		case APPLY_RESIST_BELL:
		case APPLY_RESIST_FAN:
		case APPLY_RESIST_BOW:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_RESIST_CLAW:
#endif
		case APPLY_RESIST_FIRE:
		case APPLY_RESIST_ELEC:
		case APPLY_RESIST_MAGIC:
		case APPLY_RESIST_WIND:
		case APPLY_RESIST_ICE:
		case APPLY_RESIST_EARTH:
		case APPLY_RESIST_DARK:
		case APPLY_REFLECT_MELEE:
		case APPLY_REFLECT_CURSE:
		case APPLY_ANTI_CRITICAL_PCT:
		case APPLY_ANTI_PENETRATE_PCT:
		case APPLY_POISON_REDUCE:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_BLEEDING_REDUCE:
#endif
		case APPLY_KILL_SP_RECOVER:
		case APPLY_EXP_DOUBLE_BONUS:
		case APPLY_GOLD_DOUBLE_BONUS:
		case APPLY_ITEM_DROP_BONUS:
		case APPLY_POTION_BONUS:
		case APPLY_KILL_HP_RECOVER:
		case APPLY_IMMUNE_STUN:
		case APPLY_IMMUNE_SLOW:
		case APPLY_IMMUNE_FALL:
		case APPLY_BOW_DISTANCE:
		case APPLY_ATT_GRADE_BONUS:
		case APPLY_DEF_GRADE_BONUS:
		case APPLY_MAGIC_ATT_GRADE:
		case APPLY_MAGIC_DEF_GRADE:
		case APPLY_CURSE_PCT:
		case APPLY_MAX_STAMINA:
		case APPLY_MALL_ATTBONUS:
		case APPLY_MALL_DEFBONUS:
		case APPLY_MALL_EXPBONUS:
		case APPLY_MALL_ITEMBONUS:
		case APPLY_MALL_GOLDBONUS:
		case APPLY_SKILL_DAMAGE_BONUS:
		case APPLY_NORMAL_HIT_DAMAGE_BONUS:
			// DEPEND_BONUS_ATTRIBUTES
		case APPLY_SKILL_DEFEND_BONUS:
		case APPLY_NORMAL_HIT_DEFEND_BONUS:
			// END_OF_DEPEND_BONUS_ATTRIBUTES

		case APPLY_PC_BANG_EXP_BONUS :
		case APPLY_PC_BANG_DROP_BONUS :

		case APPLY_RESIST_WARRIOR :
		case APPLY_RESIST_ASSASSIN :
		case APPLY_RESIST_SURA :
		case APPLY_RESIST_SHAMAN :
#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_RESIST_WOLFMAN :
#endif
		case APPLY_ENERGY:					// 82 기력
		case APPLY_DEF_GRADE:				// 83 방어력. DEF_GRADE_BONUS는 클라에서 두배로 보여지는 의도된 버그(...)가 있다.
		case APPLY_COSTUME_ATTR_BONUS:		// 84 코스튬 아이템에 붙은 속성치 보너스
		case APPLY_MAGIC_ATTBONUS_PER:		// 85 마법 공격력 +x%
		case APPLY_MELEE_MAGIC_ATTBONUS_PER:			// 86 마법 + 밀리 공격력 +x%
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case APPLY_ACCEDRAIN_RATE:			//97
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case APPLY_RESIST_MAGIC_REDUCTION:	//98
#endif

#ifdef ENABLE_NEW_BONUS_WORLDARD
		case APPLY_RESIST_HUMAN:
#endif
#ifdef ELEMENT_NEW_BONUSES
		case APPLY_ATTBONUS_ELEC:
		case APPLY_ATTBONUS_FIRE:
		case APPLY_ATTBONUS_ICE:
		case APPLY_ATTBONUS_WIND:
		case APPLY_ATTBONUS_EARTH:
		case APPLY_ATTBONUS_DARK:
#endif
#ifdef ENABLE_CONQUEROR_LEVEL
		case APPLY_SUNGMA_STR:
		case APPLY_SUNGMA_HP:
		case APPLY_SUNGMA_MOVE:
		case APPLY_SUNGMA_INMUNE:
#endif
#ifdef ELEMENT_NEW_BONUSES
		case APPLY_ENCHANT_ELEC: // 99
		case APPLY_ENCHANT_FIRE: // 100
		case APPLY_ENCHANT_ICE: // 101
		case APPLY_ENCHANT_WIND: // 102
		case APPLY_ENCHANT_EARTH: // 103
		case APPLY_ENCHANT_DARK: // 104

		case APPLY_ATTBONUS_SWORD: //106		
		case APPLY_ATTBONUS_TWOHAND: //107
		case APPLY_ATTBONUS_DAGGER:	//108
		case APPLY_ATTBONUS_BELL: //109		
		case APPLY_ATTBONUS_FAN: //110		
		case APPLY_ATTBONUS_BOW: //111

		case APPLY_ATTBONUS_ZODIAC: //112
		case APPLY_ATTBONUS_DESERT: //113
		case APPLY_ATTBONUS_INSECT: //114	
#endif
#ifdef ENABLE_WOLFMAN_CHARACTER
		case APPLY_ATTBONUS_CLAW: //115
#endif

#ifdef NEW_BONUS
		case APPLY_ATTBONUS_STONE:
		case APPLY_ATTBONUS_BOSS:
#endif

			PointChange(aApplyInfo[bApplyType].bPointType, iVal);
			break;

		default:
			sys_err("Unknown apply type %d name %s", bApplyType, GetName());
			break;
	}
}

void CHARACTER::MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion * packet)
{
	packet->header	= HEADER_GC_MOTION;
	packet->vid		= m_vid;
	packet->motion	= motion;

	if (victim)
		packet->victim_vid = victim->GetVID();
	else
		packet->victim_vid = 0;
}

void CHARACTER::Motion(BYTE motion, LPCHARACTER victim)
{
	struct packet_motion pack_motion;
	MotionPacketEncode(motion, victim, &pack_motion);
	PacketAround(&pack_motion, sizeof(struct packet_motion));
}

EVENTFUNC(save_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "save_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	sys_log(1, "SAVE_EVENT: %s", ch->GetName());
	ch->Save();
	ch->FlushDelayedSaveItem();
	return (save_event_second_cycle);
}

void CHARACTER::StartSaveEvent()
{
	if (m_pkSaveEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkSaveEvent = event_create(save_event, info, save_event_second_cycle);
}

void CHARACTER::MonsterLog(const char* format, ...)
{
	if (!test_server)
		return;

	if (IsPC())
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	int len = snprintf(chatbuf, sizeof(chatbuf), "%u)", (DWORD)GetVID());

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	va_list args;

	va_start(args, format);

	int len2 = vsnprintf(chatbuf + len, sizeof(chatbuf) - len, format, args);

	if (len2 < 0 || len2 >= (int) sizeof(chatbuf) - len)
		len += (sizeof(chatbuf) - len) - 1;
	else
		len += len2;

	// \0 문자 포함
	++len;

	va_end(args);

	TPacketGCChat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size		= sizeof(TPacketGCChat) + len;
	pack_chat.type      = CHAT_TYPE_TALKING;
	pack_chat.id        = (DWORD)GetVID();
	pack_chat.bEmpire	= 0;

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(TPacketGCChat));
	buf.write(chatbuf, len);

	CHARACTER_MANAGER::instance().PacketMonsterLog(this, buf.read_peek(), buf.size());
}

void CHARACTER::ChatPacket(BYTE type, const char * format, ...)
{
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	struct packet_chat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size      = sizeof(struct packet_chat) + len;
	pack_chat.type      = type;
	pack_chat.id        = 0;
	pack_chat.bEmpire   = d->GetEmpire();

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(chatbuf, len);

	d->Packet(buf.read_peek(), buf.size());

	if (type == CHAT_TYPE_COMMAND && test_server)
		sys_log(0, "SEND_COMMAND %s %s", GetName(), chatbuf);
}

// MINING
void CHARACTER::mining_take()
{
	m_pkMiningEvent = NULL;
}

void CHARACTER::mining_cancel()
{
	if (m_pkMiningEvent)
	{
		sys_log(0, "XXX MINING CANCEL");
		event_cancel(&m_pkMiningEvent);
		ChatPacket(CHAT_TYPE_INFO, "39");
	}
}

void CHARACTER::mining(LPCHARACTER chLoad)
{
	if (m_pkMiningEvent)
	{
		mining_cancel();
		return;
	}

	if (!chLoad)
		return;

	// @fixme128
	if (GetMapIndex() != chLoad->GetMapIndex() || DISTANCE_APPROX(GetX() - chLoad->GetX(), GetY() - chLoad->GetY()) > 1000)
		return;

	if (mining::GetRawOreFromLoad(chLoad->GetRaceNum()) == 0)
		return;

	LPITEM pick = GetWear(WEAR_WEAPON);

	if (!pick || pick->GetType() != ITEM_PICK)
	{
		ChatPacket(CHAT_TYPE_INFO, "40");
		return;
	}

	int count = number(5, 15); // 동작 횟수, 한 동작당 2초

	// 채광 동작을 보여줌
	TPacketGCDigMotion p;
	p.header = HEADER_GC_DIG_MOTION;
	p.vid = GetVID();
	p.target_vid = chLoad->GetVID();
	p.count = count;

	PacketAround(&p, sizeof(p));

	m_pkMiningEvent = mining::CreateMiningEvent(this, chLoad, count);
}
// END_OF_MINING
#ifdef ENABLE_CSHIELD
bool CHARACTER::IsNearWater() const
{
	if (!GetSectree())
		return false;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			if (IS_SET(GetSectree()->GetAttribute(GetX() + x * 100, GetY() + y * 100), ATTR_WATER))
				return true;
		}
	}

	return false;
}
#endif
void CHARACTER::fishing()
{
#ifdef ENABLE_CSHIELD
	if (!IsNearWater())
		return;
#endif
	if (m_pkFishingEvent)
	{
		fishing_take();
		return;
	}

	// 못감 속성에서 낚시를 시도한다?
	{
		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

		int	x = GetX();
		int y = GetY();

		LPSECTREE tree = pkSectreeMap->Find(x, y);
		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK))
		{
			ChatPacket(CHAT_TYPE_INFO, "41");
			return;
		}
	}

	LPITEM rod = GetWear(WEAR_WEAPON);

	// 낚시대 장착
	if (!rod || rod->GetType() != ITEM_ROD)
	{
		ChatPacket(CHAT_TYPE_INFO, "42");
		return;
	}

	if (0 == rod->GetSocket(2))
	{
		ChatPacket(CHAT_TYPE_INFO, "43");
		return;
	}

	float fx, fy;
	GetDeltaByDegree(GetRotation(), 400.0f, &fx, &fy);

	m_pkFishingEvent = fishing::CreateFishingEvent(this);
}

void CHARACTER::fishing_take()
{
	LPITEM rod = GetWear(WEAR_WEAPON);
	if (rod && rod->GetType() == ITEM_ROD)
	{
		using fishing::fishing_event_info;
		if (m_pkFishingEvent)
		{
			struct fishing_event_info* info = dynamic_cast<struct fishing_event_info*>(m_pkFishingEvent->info);

			if (info)
				fishing::Take(info, this);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, "44");
	}

	event_cancel(&m_pkFishingEvent);
}

bool CHARACTER::StartStateMachine(int iNextPulse)
{
	if (CHARACTER_MANAGER::instance().AddToStateList(this))
	{
		m_dwNextStatePulse = thecore_heart->pulse + iNextPulse;
		return true;
	}

	return false;
}

void CHARACTER::StopStateMachine()
{
	CHARACTER_MANAGER::instance().RemoveFromStateList(this);
}

void CHARACTER::UpdateStateMachine(DWORD dwPulse)
{
	if (dwPulse < m_dwNextStatePulse)
		return;

	if (IsDead())
		return;

	Update();
	m_dwNextStatePulse = dwPulse + m_dwStateDuration;
}

void CHARACTER::SetNextStatePulse(int iNextPulse)
{
	CHARACTER_MANAGER::instance().AddToStateList(this);
	m_dwNextStatePulse = iNextPulse;

	if (iNextPulse < 10)
		MonsterLog("다음상태로어서가자");
}


// 캐릭터 인스턴스 업데이트 함수.
void CHARACTER::UpdateCharacter(DWORD dwPulse)
{
	CFSM::Update();
}

void CHARACTER::SetShop(LPSHOP pkShop)
{
	if ((m_pkShop = pkShop))
		SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
	else
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
}

void CHARACTER::SetExchange(CExchange * pkExchange)
{
	m_pkExchange = pkExchange;
}

void CHARACTER::SetPart(BYTE bPartPos, WORD wVal)
{
	assert(bPartPos < PART_MAX_NUM);
	m_pointsInstant.parts[bPartPos] = wVal;
}

WORD CHARACTER::GetPart(BYTE bPartPos) const
{
	assert(bPartPos < PART_MAX_NUM);
	return m_pointsInstant.parts[bPartPos];
}

WORD CHARACTER::GetOriginalPart(BYTE bPartPos) const
{
	switch (bPartPos)
	{
		case PART_MAIN:
			if (!IsPC()) // PC가 아닌 경우 현재 파트를 그대로 리턴
				return GetPart(PART_MAIN);
			else
				return m_pointsInstant.bBasePart;

		case PART_HAIR:
			return GetPart(PART_HAIR);
#ifdef __SASH_SYSTEM__
		case PART_SASH:
			return GetPart(PART_SASH);
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		case PART_WEAPON:
			return GetPart(PART_WEAPON);
#endif

		default:
			return 0;
	}
}

BYTE CHARACTER::GetCharType() const
{
	return m_bCharType;
}

bool CHARACTER::SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList)
{
	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		return false;
	// END_OF_TRENT_MONSTER
	
	if (ch) // @fixme131
	{
		if (!battle_is_attackable(ch, this))
		{
			SendDamagePacket(ch, 0, DAMAGE_BLOCK);
			return false;
		}
	}

	if (ch == this)
	{
		sys_err("SetSyncOwner owner == this (%p)", this);
		return false;
	}

	if (!ch)
	{
		if (bRemoveFromList && m_pkChrSyncOwner)
		{
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
		}

		if (m_pkChrSyncOwner)
			sys_log(1, "SyncRelease %s %p from %s", GetName(), this, m_pkChrSyncOwner->GetName());

		// 리스트에서 제거하지 않더라도 포인터는 NULL로 셋팅되어야 한다.
		m_pkChrSyncOwner = NULL;
	}
	else
	{
		if (!IsSyncOwner(ch))
			return false;

		// 거리가 200 이상이면 SyncOwner가 될 수 없다.
		if (DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY()) > 250)
		{
			sys_log(1, "SetSyncOwner distance over than 250 %s %s", GetName(), ch->GetName());

			// SyncOwner일 경우 Owner로 표시한다.
			if (m_pkChrSyncOwner == ch)
				return true;

			return false;
		}

		if (m_pkChrSyncOwner != ch)
		{
			if (m_pkChrSyncOwner)
			{
				sys_log(1, "SyncRelease %s %p from %s", GetName(), this, m_pkChrSyncOwner->GetName());
				m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
			}

			m_pkChrSyncOwner = ch;
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.push_back(this);

			// SyncOwner가 바뀌면 LastSyncTime을 초기화한다.
			static const timeval zero_tv = {0, 0};
			SetLastSyncTime(zero_tv);

			sys_log(1, "SetSyncOwner set %s %p to %s", GetName(), this, ch->GetName());
		}

		m_fSyncTime = get_float_time();
	}

	// TODO: Sync Owner가 같더라도 계속 패킷을 보내고 있으므로,
	//       동기화 된 시간이 3초 이상 지났을 때 풀어주는 패킷을
	//       보내는 방식으로 하면 패킷을 줄일 수 있다.
	TPacketGCOwnership pack;

	pack.bHeader	= HEADER_GC_OWNERSHIP;
	pack.dwOwnerVID	= ch ? ch->GetVID() : 0;
	pack.dwVictimVID	= GetVID();

	PacketAround(&pack, sizeof(TPacketGCOwnership));
	return true;
}

struct FuncClearSync
{
	void operator () (LPCHARACTER ch)
	{
		assert(ch != NULL);
		ch->SetSyncOwner(NULL, false);	// false 플래그로 해야 for_each 가 제대로 돈다.
	}
};

void CHARACTER::ClearSync()
{
	SetSyncOwner(NULL);

	// 아래 for_each에서 나를 m_pkChrSyncOwner로 가진 자들의 포인터를 NULL로 한다.
	std::for_each(m_kLst_pkChrSyncOwned.begin(), m_kLst_pkChrSyncOwned.end(), FuncClearSync());
	m_kLst_pkChrSyncOwned.clear();
}

bool CHARACTER::IsSyncOwner(LPCHARACTER ch) const
{
	if (m_pkChrSyncOwner == ch)
		return true;

	// 마지막으로 동기화 된 시간이 3초 이상 지났다면 소유권이 아무에게도
	// 없다. 따라서 아무나 SyncOwner이므로 true 리턴
	if (get_float_time() - m_fSyncTime >= 3.0f)
		return true;

	return false;
}

void CHARACTER::SetParty(LPPARTY pkParty)
{
	if (pkParty == m_pkParty)
		return;

	if (pkParty && m_pkParty)
		sys_err("%s is trying to reassigning party (current %p, new party %p)", GetName(), get_pointer(m_pkParty), get_pointer(pkParty));

	sys_log(1, "PARTY set to %p", get_pointer(pkParty));

	//if (m_pkDungeon && IsPC())
	//SetDungeon(NULL);
	m_pkParty = pkParty;

	if (IsPC())
	{
		if (m_pkParty)
			SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);
		else
			REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);

		UpdatePacket();
	}
}

// PARTY_JOIN_BUG_FIX
/// 파티 가입 event 정보
EVENTINFO(TPartyJoinEventInfo)
{
	DWORD	dwGuestPID;		///< 파티에 참여할 캐릭터의 PID
	DWORD	dwLeaderPID;		///< 파티 리더의 PID

	TPartyJoinEventInfo()
	: dwGuestPID( 0 )
	, dwLeaderPID( 0 )
	{
	}
} ;

EVENTFUNC(party_request_event)
{
	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(  event->info );

	if ( info == NULL )
	{
		sys_err( "party_request_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->dwGuestPID);

	if (ch)
	{
		sys_log(0, "PartyRequestEvent %s", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
		ch->SetPartyRequestEvent(NULL);
	}

	return 0;
}

bool CHARACTER::RequestToParty(LPCHARACTER leader)
{
	if (leader->GetParty())
		leader = leader->GetParty()->GetLeaderCharacter();

	if (!leader)
	{
		ChatPacket(CHAT_TYPE_INFO, "45");
		return false;
	}

	if (m_pkPartyRequestEvent)
		return false;

	if (!IsPC() || !leader->IsPC())
		return false;

	if (leader->IsBlockMode(BLOCK_PARTY_REQUEST))
		return false;
#if defined(WJ_COMBAT_ZONE)	
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return false;
#endif
	PartyJoinErrCode errcode = IsPartyJoinableCondition(leader, this);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			ChatPacket(CHAT_TYPE_INFO, "46");
			return false;

		case PERR_DIFFEMPIRE:
			ChatPacket(CHAT_TYPE_INFO, "47");
			return false;

		case PERR_DUNGEON:
			ChatPacket(CHAT_TYPE_INFO, "48");
			return false;

		case PERR_OBSERVER:
			ChatPacket(CHAT_TYPE_INFO, "49");
			return false;

		case PERR_LVBOUNDARY:
			ChatPacket(CHAT_TYPE_INFO, "50");
			return false;

		case PERR_LOWLEVEL:
			ChatPacket(CHAT_TYPE_INFO, "51");
			return false;

		case PERR_HILEVEL:
			ChatPacket(CHAT_TYPE_INFO, "52");
			return false;

		case PERR_ALREADYJOIN:
			return false;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, "53");
			return false;

		default:
			sys_err("Do not process party join error(%d)", errcode);
			return false;
	}

	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = GetPlayerID();
	info->dwLeaderPID = leader->GetPlayerID();

	SetPartyRequestEvent(event_create(party_request_event, info, PASSES_PER_SEC(10)));

	leader->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequest %u", (DWORD) GetVID());
	ChatPacket(CHAT_TYPE_INFO, "54 %s", leader->GetName());
	return true;
}

void CHARACTER::DenyToParty(LPCHARACTER member)
{
	sys_log(1, "DenyToParty %s member %s %p", GetName(), member->GetName(), get_pointer(member->m_pkPartyRequestEvent));

	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err( "CHARACTER::DenyToParty> <Factor> Null pointer" );
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

void CHARACTER::AcceptToParty(LPCHARACTER member)
{
	sys_log(1, "AcceptToParty %s member %s %p", GetName(), member->GetName(), get_pointer(member->m_pkPartyRequestEvent));

	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err( "CHARACTER::AcceptToParty> <Factor> Null pointer" );
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	if (!GetParty())
		member->ChatPacket(CHAT_TYPE_INFO, "55");
	else
	{
		if (GetPlayerID() != GetParty()->GetLeaderPID())
			return;

		PartyJoinErrCode errcode = IsPartyJoinableCondition(this, member);
		switch (errcode)
		{
			case PERR_NONE: 		member->PartyJoin(this); return;
			case PERR_SERVER:		member->ChatPacket(CHAT_TYPE_INFO, "46"); break;
			case PERR_DUNGEON:		member->ChatPacket(CHAT_TYPE_INFO, "48"); break;
			case PERR_OBSERVER: 	member->ChatPacket(CHAT_TYPE_INFO, "49"); break;
			case PERR_LVBOUNDARY:	member->ChatPacket(CHAT_TYPE_INFO, "50"); break;
			case PERR_LOWLEVEL: 	member->ChatPacket(CHAT_TYPE_INFO, "51"); break;
			case PERR_HILEVEL: 		member->ChatPacket(CHAT_TYPE_INFO, "52"); break;
			case PERR_ALREADYJOIN: 	break;
			case PERR_PARTYISFULL: {
									   ChatPacket(CHAT_TYPE_INFO, "53");
									   member->ChatPacket(CHAT_TYPE_INFO, "56");
									   break;
								   }
			default: sys_err("Do not process party join error(%d)", errcode);
		}
	}

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

/**
 * 파티 초대 event callback 함수.
 * event 가 발동하면 초대 거절로 처리한다.
 */
EVENTFUNC(party_invite_event)
{
	TPartyJoinEventInfo * pInfo = dynamic_cast<TPartyJoinEventInfo *>(  event->info );

	if ( pInfo == NULL )
	{
		sys_err( "party_invite_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER pchInviter = CHARACTER_MANAGER::instance().FindByPID(pInfo->dwLeaderPID);

	if (pchInviter)
	{
		sys_log(1, "PartyInviteEvent %s", pchInviter->GetName());
		pchInviter->PartyInviteDeny(pInfo->dwGuestPID);
	}

	return 0;
}

void CHARACTER::PartyInvite(LPCHARACTER pchInvitee)
{
#if defined(WJ_COMBAT_ZONE)	
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif

#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsTournamentMap(this, TOURNAMENT_BLOCK_PARTY))
		return;
#endif


	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		ChatPacket(CHAT_TYPE_INFO, "57");
		return;
	}
	else if (pchInvitee->IsBlockMode(BLOCK_PARTY_INVITE))
	{
		ChatPacket(CHAT_TYPE_INFO, "58 %s", pchInvitee->GetName());
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableCondition(this, pchInvitee);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			ChatPacket(CHAT_TYPE_INFO, "46");
			return;

		case PERR_DIFFEMPIRE:
			ChatPacket(CHAT_TYPE_INFO, "47");
			return;

		case PERR_DUNGEON:
			ChatPacket(CHAT_TYPE_INFO, "48");
			return;

		case PERR_OBSERVER:
			ChatPacket(CHAT_TYPE_INFO, "49");
			return;

		case PERR_LVBOUNDARY:
			ChatPacket(CHAT_TYPE_INFO, "50");
			return;

		case PERR_LOWLEVEL:
			ChatPacket(CHAT_TYPE_INFO, "51");
			return;

		case PERR_HILEVEL:
			ChatPacket(CHAT_TYPE_INFO, "52");
			return;

		case PERR_ALREADYJOIN:
			ChatPacket(CHAT_TYPE_INFO, "59 %s", pchInvitee->GetName());
			return;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, "53");
			return;

		default:
			sys_err("Do not process party join error(%d)", errcode);
			return;
	}

	if (m_PartyInviteEventMap.end() != m_PartyInviteEventMap.find(pchInvitee->GetPlayerID()))
		return;

	//
	// EventMap 에 이벤트 추가
	//
	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = pchInvitee->GetPlayerID();
	info->dwLeaderPID = GetPlayerID();

	m_PartyInviteEventMap.insert(EventMap::value_type(pchInvitee->GetPlayerID(), event_create(party_invite_event, info, PASSES_PER_SEC(10))));

	//
	// 초대 받는 character 에게 초대 패킷 전송
	//

	TPacketGCPartyInvite p;
	p.header = HEADER_GC_PARTY_INVITE;
	p.leader_vid = GetVID();
	pchInvitee->GetDesc()->Packet(&p, sizeof(p));
}

void CHARACTER::PartyInviteAccept(LPCHARACTER pchInvitee)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(pchInvitee->GetPlayerID());

	if (itFind == m_PartyInviteEventMap.end())
	{
		sys_log(1, "PartyInviteAccept from not invited character(%s)", pchInvitee->GetName());
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		ChatPacket(CHAT_TYPE_INFO, "57");
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableMutableCondition(this, pchInvitee);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "46");
			return;

		case PERR_DUNGEON:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "60");
			return;

		case PERR_OBSERVER:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "49");
			return;

		case PERR_LVBOUNDARY:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "50");
			return;

		case PERR_LOWLEVEL:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "51");
			return;

		case PERR_HILEVEL:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "52");
			return;

		case PERR_ALREADYJOIN:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "61");
			return;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, "53");
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, "56");
			return;

		default:
			sys_err("ignore party join error(%d)", errcode);
			return;
	}

	//
	// 파티 가입 처리
	//

	if (GetParty())
		pchInvitee->PartyJoin(this);
	else
	{
		LPPARTY pParty = CPartyManager::instance().CreateParty(this);

		pParty->Join(pchInvitee->GetPlayerID());
		pParty->Link(pchInvitee);
		pParty->SendPartyInfoAllToOne(this);
	}
}

void CHARACTER::PartyInviteDeny(DWORD dwPID)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(dwPID);

	if (itFind == m_PartyInviteEventMap.end())
	{
		sys_log(1, "PartyInviteDeny to not exist event(inviter PID: %d, invitee PID: %d)", GetPlayerID(), dwPID);
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	LPCHARACTER pchInvitee = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (pchInvitee)
		ChatPacket(CHAT_TYPE_INFO, "62", pchInvitee->GetName());
}

void CHARACTER::PartyJoin(LPCHARACTER pLeader)
{
	pLeader->ChatPacket(CHAT_TYPE_INFO, "63 %s", GetName());
	ChatPacket(CHAT_TYPE_INFO, "64 %s", pLeader->GetName());

	pLeader->GetParty()->Join(GetPlayerID());
	pLeader->GetParty()->Link(this);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (pchLeader->GetEmpire() != pchGuest->GetEmpire())
		return PERR_DIFFEMPIRE;

	return IsPartyJoinableMutableCondition(pchLeader, pchGuest);
}

static bool __party_can_join_by_level(LPCHARACTER leader, LPCHARACTER quest)
{
	int	level_limit = 30;
	return (abs(leader->GetLevel() - quest->GetLevel()) <= level_limit);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (!CPartyManager::instance().IsEnablePCParty())
		return PERR_SERVER;
	else if (pchLeader->GetDungeon())
		return PERR_DUNGEON;
	else if (pchGuest->IsObserverMode())
		return PERR_OBSERVER;
	else if (false == __party_can_join_by_level(pchLeader, pchGuest))
		return PERR_LVBOUNDARY;
	else if (pchGuest->GetParty())
		return PERR_ALREADYJOIN;
	else if (pchLeader->GetParty())
   	{
	   	if (pchLeader->GetParty()->GetMemberCount() == PARTY_MAX_MEMBER)
			return PERR_PARTYISFULL;
	}

	return PERR_NONE;
}
// END_OF_PARTY_JOIN_BUG_FIX

void CHARACTER::SetDungeon(LPDUNGEON pkDungeon)
{
	if (pkDungeon && m_pkDungeon)
		sys_err("%s is trying to reassigning dungeon (current %p, new party %p)", GetName(), get_pointer(m_pkDungeon), get_pointer(pkDungeon));

	if (m_pkDungeon == pkDungeon) {
		return;
	}

	if (m_pkDungeon)
	{
		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->DecPartyMember(GetParty(), this);
			else
				m_pkDungeon->DecMember(this);
		}
		else if (IsMonster() || IsStone())
		{
			m_pkDungeon->DecMonster();
		}
	}

	m_pkDungeon = pkDungeon;

	if (pkDungeon)
	{
		sys_log(0, "%s DUNGEON set to %p, PARTY is %p", GetName(), get_pointer(pkDungeon), get_pointer(m_pkParty));

		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->IncPartyMember(GetParty(), this);
			else
				m_pkDungeon->IncMember(this);
		}
		else if (IsMonster() || IsStone())
		{
			m_pkDungeon->IncMonster();
		}
	}
}

void CHARACTER::SetWarMap(CWarMap * pWarMap)
{
	if (m_pWarMap)
		m_pWarMap->DecMember(this);

	m_pWarMap = pWarMap;

	if (m_pWarMap)
		m_pWarMap->IncMember(this);
}

void CHARACTER::SetWeddingMap(marriage::WeddingMap* pMap)
{
	if (m_pWeddingMap)
		m_pWeddingMap->DecMember(this);

	m_pWeddingMap = pMap;

	if (m_pWeddingMap)
		m_pWeddingMap->IncMember(this);
}

void CHARACTER::SetRegen(LPREGEN pkRegen)
{
	m_pkRegen = pkRegen;
	if (pkRegen != NULL) {
		regen_id_ = pkRegen->id;
	}
	m_fRegenAngle = GetRotation();
	m_posRegen = GetXYZ();
}

bool CHARACTER::OnIdle()
{
	return false;
}

void CHARACTER::OnMove(bool bIsAttack)
{
	m_dwLastMoveTime = get_dword_time();

	if (bIsAttack)
	{
		m_dwLastAttackTime = m_dwLastMoveTime;

		if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
			RemoveAffect(AFFECT_REVIVE_INVISIBLE);

		if (IsAffectFlag(AFF_EUNHYUNG))
		{
			RemoveAffect(SKILL_EUNHYUNG);
			SetAffectedEunhyung();
		}
		else
		{
			ClearAffectedEunhyung();
		}

		/*if (IsAffectFlag(AFF_JEONSIN))
		  RemoveAffect(SKILL_JEONSINBANGEO);*/
	}

	/*if (IsAffectFlag(AFF_GUNGON))
	  RemoveAffect(SKILL_GUNGON);*/

	// MINING
	mining_cancel();
	// END_OF_MINING
}

void CHARACTER::OnClick(LPCHARACTER pkChrCauser)
{
	if (!pkChrCauser)
	{
		sys_err("OnClick %s by NULL", GetName());
		return;
	}
	else if (pkChrCauser->IsPC() && pkChrCauser->GetExchange())
	{
		sys_err("OnClick Fail (%s->%s) - pc is exchanging", pkChrCauser->GetName(), GetName());
		return;
	}

	sys_log(0, "OnClick %s[vnum %d ServerUniqueID %d, pid %d] by %s", GetName(), GetRaceNum(), GetVID(), GetPlayerID(), pkChrCauser->GetName());

	pkChrCauser->SetQuestNPCID(GetVID());
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	if (IsOfflineShopNPC())
	{
		if (pkChrCauser->CanOpenOfflineShop())
			return;
		GetOfflineShop()->AddGuest(pkChrCauser, this);
		return;
	}
#endif
	if (quest::CQuestManager::instance().Click(pkChrCauser->GetPlayerID(), this))
		return;

	if (!IsPC())
	{
		if (!m_triggerOnClick.pFunc)
			return;
		m_triggerOnClick.pFunc(this, pkChrCauser);
	}

}

BYTE CHARACTER::GetGMLevel() const
{
	if (test_server)
		return GM_IMPLEMENTOR;
	return m_pointsInstant.gm_level;
}

void CHARACTER::SetGMLevel()
{
	if (GetDesc())
	{
	    m_pointsInstant.gm_level =  gm_get_level(GetName(), GetDesc()->GetHostName(), GetDesc()->GetAccountTable().login);
	}
	else
	{
	    m_pointsInstant.gm_level = GM_PLAYER;
	}
}

BOOL CHARACTER::IsGM() const
{
	if (m_pointsInstant.gm_level != GM_PLAYER)
		return true;
	if (test_server)
		return true;
	return false;
}

void CHARACTER::SetStone(LPCHARACTER pkChrStone)
{
	m_pkChrStone = pkChrStone;

	if (m_pkChrStone)
	{
		if (pkChrStone->m_set_pkChrSpawnedBy.find(this) == pkChrStone->m_set_pkChrSpawnedBy.end())
			pkChrStone->m_set_pkChrSpawnedBy.insert(this);
	}
}

struct FuncDeadSpawnedByStone
{
	void operator () (LPCHARACTER ch)
	{
		ch->Dead(NULL);
		ch->SetStone(NULL);
	}
};

void CHARACTER::ClearStone()
{
	if (!m_set_pkChrSpawnedBy.empty())
	{
		// 내가 스폰시킨 몬스터들을 모두 죽인다.
		FuncDeadSpawnedByStone f;
		std::for_each(m_set_pkChrSpawnedBy.begin(), m_set_pkChrSpawnedBy.end(), f);
		m_set_pkChrSpawnedBy.clear();
	}

	if (!m_pkChrStone)
		return;

	m_pkChrStone->m_set_pkChrSpawnedBy.erase(this);
	m_pkChrStone = NULL;
}

void CHARACTER::ClearTarget()
{
	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);
		m_pkChrTarget = NULL;
	}

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = 0;
	p.bHPPercent = 0;
#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	memcpy (p.dwCostumeHideID, "", sizeof(p.dwCostumeHideID));
#endif
	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *(it++);
		pkChr->m_pkChrTarget = NULL;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
	}

	m_set_pkChrTargetedBy.clear();
}

void CHARACTER::SetTarget(LPCHARACTER pkChrTarget)
{
	if (m_pkChrTarget == pkChrTarget)
		return;

	// CASTLE
	if (IS_CASTLE_MAP(GetMapIndex()) && !IsGM())
		return;
	// CASTLE

	if (m_pkChrTarget)
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);

	m_pkChrTarget = pkChrTarget;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;

	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.insert(this);

		p.dwVID	= m_pkChrTarget->GetVID();

#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		if (m_pkChrTarget->IsPC()){
			char c_hide_show[255+1];
			snprintf(c_hide_show, sizeof(c_hide_show), "%d|%d|%d|%d|", m_pkChrTarget->GetQuestFlag("hide_costume_w.hide_costume_0"),m_pkChrTarget->GetQuestFlag("hide_costume_w.hide_costume_1"),m_pkChrTarget->GetQuestFlag("hide_costume_w.hide_costume_2"),m_pkChrTarget->GetQuestFlag("hide_costume_w.hide_costume_3"));
			memcpy (p.dwCostumeHideID, c_hide_show, sizeof(p.dwCostumeHideID));
		}
#endif

		if ((m_pkChrTarget->GetMaxHP() <= 0))
		{
			p.bHPPercent = 0;
		}

		else if (m_pkChrTarget->IsPC() && !m_pkChrTarget->IsPolymorphed())
		{
			p.bHPPercent = MINMAX(0, (m_pkChrTarget->GetHP() * 100) / m_pkChrTarget->GetMaxHP(), 100);
		}

		else
		{
			if (m_pkChrTarget->GetRaceNum() == 20101 ||
					m_pkChrTarget->GetRaceNum() == 20102 ||
					m_pkChrTarget->GetRaceNum() == 20103 ||
					m_pkChrTarget->GetRaceNum() == 20104 ||
					m_pkChrTarget->GetRaceNum() == 20105 ||
					m_pkChrTarget->GetRaceNum() == 20106 ||
					m_pkChrTarget->GetRaceNum() == 20107 ||
					m_pkChrTarget->GetRaceNum() == 20108 ||
					m_pkChrTarget->GetRaceNum() == 20109)
			{
				LPCHARACTER owner = m_pkChrTarget->GetVictim();

				if (owner)
				{
					int iHorseHealth = owner->GetHorseHealth();
					int iHorseMaxHealth = owner->GetHorseMaxHealth();

					if (iHorseMaxHealth)
						p.bHPPercent = MINMAX(0,  iHorseHealth * 100 / iHorseMaxHealth, 100);
					else
						p.bHPPercent = 100;
				}
				else
					p.bHPPercent = 100;
			}
			else
			{
				if (m_pkChrTarget->GetMaxHP() <= 0) // @fixme136
					p.bHPPercent = 0;
				else
					p.bHPPercent = MINMAX(0, (m_pkChrTarget->GetHP() * 100) / m_pkChrTarget->GetMaxHP(), 100);
			}
		}
	}
	else
	{
		p.dwVID = 0;
		p.bHPPercent = 0;
	}

#ifdef ELEMENT_TARGET
	const int ELEMENT_BASE = 11;
	DWORD curElementBase = ELEMENT_BASE;
	DWORD raceFlag;
	if (m_pkChrTarget && m_pkChrTarget->IsMonster() && (raceFlag = m_pkChrTarget->GetMobTable().dwRaceFlag) >= RACE_FLAG_ATT_ELEC)
	{
		for (int i = RACE_FLAG_ATT_ELEC; i <= RACE_FLAG_ATT_DARK; i *= 2)
		{
			curElementBase++;
			int diff = raceFlag - i;
			if (abs(diff) <= 1024)
				break;
		}
		p.bElement = curElementBase - ELEMENT_BASE;
	}
	else if(m_pkChrTarget && m_pkChrTarget->IsPC())
	{
		LPITEM pkElement = m_pkChrTarget->GetWear(WEAR_PENDANT);
		if (!pkElement) // NONE
			p.bElement = 0;
		else if (pkElement && (pkElement->GetVnum() >= 9600 && pkElement->GetVnum() <= 9800))	
			p.bElement = 2;
		else if (pkElement && (pkElement->GetVnum() >= 9830 && pkElement->GetVnum() <= 10030))	
			p.bElement = 3;
		else if (pkElement && (pkElement->GetVnum() >= 10060 && pkElement->GetVnum() <= 10260))
			p.bElement = 5;
		else if (pkElement && (pkElement->GetVnum() >= 10290 && pkElement->GetVnum() <= 10490))	
			p.bElement = 6;
		else if (pkElement && (pkElement->GetVnum() >= 10520 && pkElement->GetVnum() <= 10720))	
			p.bElement = 4;
		else if (pkElement && (pkElement->GetVnum() >= 10750 && pkElement->GetVnum() <= 10950))
			p.bElement = 1;
	}
	else
	{
		p.bElement = 0;
	}

#endif

	GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
}

void CHARACTER::BroadcastTargetPacket()
{
	if (m_set_pkChrTargetedBy.empty())
		return;

	TPacketGCTarget p;

	p.header = HEADER_GC_TARGET;
	p.dwVID = GetVID();


	if (GetMaxHP() <= 0)
	{
		p.bHPPercent = 0;
	}else{
		p.bHPPercent = MINMAX(0, (GetHP() * 100) / GetMaxHP(), 100);
	}

	/*
	if (IsPC())
		p.bHPPercent = 0;
	else if (GetMaxHP() <= 0) // @fixme136
		p.bHPPercent = 0;
	else
		p.bHPPercent = MINMAX(0, (GetHP() * 100) / GetMaxHP(), 100);*/

#ifdef ENABLE_HIDE_COSTUME_SYSTEM
		if (IsPC()){
			char c_hide_show[255+1];
			snprintf(c_hide_show, sizeof(c_hide_show), "%d|%d|%d|%d|", GetQuestFlag("hide_costume_w.hide_costume_0"),GetQuestFlag("hide_costume_w.hide_costume_1"),GetQuestFlag("hide_costume_w.hide_costume_2"),GetQuestFlag("hide_costume_w.hide_costume_3"));
			memcpy (p.dwCostumeHideID, c_hide_show, sizeof(p.dwCostumeHideID));
		}
#endif

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *it++;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(&p, sizeof(TPacketGCTarget));
	}
}

void CHARACTER::CheckTarget()
{
	if (!m_pkChrTarget)
		return;

	if (DISTANCE_APPROX(GetX() - m_pkChrTarget->GetX(), GetY() - m_pkChrTarget->GetY()) >= 4800)
		SetTarget(NULL);
}

void CHARACTER::SetWarpLocation(long lMapIndex, long x, long y)
{
	m_posWarp.x = x * 100;
	m_posWarp.y = y * 100;
	m_lWarpMapIndex = lMapIndex;
}

void CHARACTER::SaveExitLocation()
{
	m_posExit = GetXYZ();
	m_lExitMapIndex = GetMapIndex();
}

void CHARACTER::ExitToSavedLocation()
{
	sys_log (0, "ExitToSavedLocation");
	WarpSet(m_posWarp.x, m_posWarp.y, m_lWarpMapIndex);

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;
}

// fixme
// 지금까진 privateMapIndex 가 현재 맵 인덱스와 같은지 체크 하는 것을 외부에서 하고,
// 다르면 warpset을 불렀는데
// 이를 warpset 안으로 넣자.
bool CHARACTER::WarpSet(long x, long y, long lPrivateMapIndex)
{
	if (!IsPC())
		return false;

	long lAddr;
	long lMapIndex;
	WORD wPort;

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
	{
		sys_err("cannot find map location index %d x %d y %d name %s", lMapIndex, x, y, GetName());
		return false;
	}

	//Send Supplementary Data Block if new map requires security packages in loading this map
	{
		long lCurAddr;
		long lCurMapIndex = 0;
		WORD wCurPort;

		CMapLocation::instance().Get(GetX(), GetY(), lCurMapIndex, lCurAddr, wCurPort);

		//do not send SDB files if char is in the same map
		if( lCurMapIndex != lMapIndex )
		{
			const TMapRegion * rMapRgn = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex);
			{
				DESC_MANAGER::instance().SendClientPackageSDBToLoadMap( GetDesc(), rMapRgn->strMapName.c_str() );
			}
		}
	}

	if (lPrivateMapIndex >= 10000)
	{
		if (lPrivateMapIndex / 10000 != lMapIndex)
		{
			sys_err("Invalid map index %d, must be child of %d", lPrivateMapIndex, lMapIndex);
			return false;
		}

		lMapIndex = lPrivateMapIndex;
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();

		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	sys_log(0, "WarpSet %s %d %d current map %d target map %d", GetName(), x, y, GetMapIndex(), lMapIndex);

	TPacketGCWarp p;

	p.bHeader	= HEADER_GC_WARP;
	p.lX	= x;
	p.lY	= y;
	p.lAddr	= lAddr;
	p.wPort	= wPort;
	
#ifdef ENABLE_SWITCHBOT
	CSwitchbotManager::Instance().SetIsWarping(GetPlayerID(), true);

	if (p.wPort != mother_port)
	{
		CSwitchbotManager::Instance().P2PSendSwitchbot(GetPlayerID(), p.wPort);
	}
#endif

	GetDesc()->Packet(&p, sizeof(TPacketGCWarp));

	char buf[256];
	snprintf(buf, sizeof(buf), "%s MapIdx %ld DestMapIdx%ld DestX%ld DestY%ld Empire%d", GetName(), GetMapIndex(), lPrivateMapIndex, x, y, GetEmpire());
	LogManager::instance().CharLog(this, 0, "WARP", buf);
	
#ifdef ENABLE_VOTE_COINS_WHILE_PLAYING
	SetQuestFlag("vote_coins_event.warp_cache", get_global_time());
#endif

	return true;
}

#define ENABLE_GOHOME_IF_MAP_NOT_ALLOWED
void CHARACTER::WarpEnd()
{
	if (test_server)
		sys_log(0, "WarpEnd %s", GetName());

	if (m_posWarp.x == 0 && m_posWarp.y == 0)
		return;

	int index = m_lWarpMapIndex;

	if (index > 10000)
		index /= 10000;

	if (!map_allow_find(index))
	{
		// 이 곳으로 워프할 수 없으므로 워프하기 전 좌표로 되돌리자.
		sys_err("location %d %d not allowed to login this server", m_posWarp.x, m_posWarp.y);
#ifdef ENABLE_GOHOME_IF_MAP_NOT_ALLOWED
		GoHome();
#else
		GetDesc()->SetPhase(PHASE_CLOSE);
#endif
		return;
	}

	sys_log(0, "WarpEnd %s %d %u %u", GetName(), m_lWarpMapIndex, m_posWarp.x, m_posWarp.y);

	Show(m_lWarpMapIndex, m_posWarp.x, m_posWarp.y, 0);
	Stop();

	m_lWarpMapIndex = 0;
	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;

	{
		// P2P Login
		TPacketGGLogin p;
		p.bHeader = HEADER_GG_LOGIN;
		strlcpy(p.szName, GetName(), sizeof(p.szName));
		p.dwPID = GetPlayerID();
		p.bEmpire = GetEmpire();
		p.lMapIndex = SECTREE_MANAGER::instance().GetMapIndex(GetX(), GetY());
		p.bChannel = g_bChannel;
		P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGLogin));
	}
}

bool CHARACTER::Return()
{
	if (!IsNPC())
		return false;

	int x, y;
	/*
	   float fDist = DISTANCE_SQRT(m_pkMobData->m_posLastAttacked.x - GetX(), m_pkMobData->m_posLastAttacked.y - GetY());
	   float fx, fy;
	   GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);
	   x = GetX() + (int) fx;
	   y = GetY() + (int) fy;
	 */
	SetVictim(NULL);

	x = m_pkMobInst->m_posLastAttacked.x;
	y = m_pkMobInst->m_posLastAttacked.y;

	SetRotationToXY(x, y);

	if (!Goto(x, y))
		return false;

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	if (test_server)
		sys_log(0, "%s %p 포기하고 돌아가자! %d %d", GetName(), this, x, y);

	if (GetParty())
		GetParty()->SendMessage(this, PM_RETURN, x, y);

	return true;
}

bool CHARACTER::Follow(LPCHARACTER pkChr, float fMinDistance)
{
	if (IsPC())
	{
		sys_err("CHARACTER::Follow : PC cannot use this method", GetName());
		return false;
	}

	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (pkChr->IsPC()) // 쫓아가는 상대가 PC일 때
		{
			// If i'm in a party. I must obey party leader's AI.
			if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
			{
				if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000) // 마지막으로 공격받은지 15초가 지났고
				{
					// 마지막 맞은 곳으로 부터 50미터 이상 차이나면 포기하고 돌아간다.
					if (m_pkMobData->m_table.wAttackRange < DISTANCE_APPROX(pkChr->GetX() - GetX(), pkChr->GetY() - GetY()))
						if (Return())
							return true;
				}
			}
		}
		return false;
	}
	// END_OF_TRENT_MONSTER

	long x = pkChr->GetX();
	long y = pkChr->GetY();

	if (pkChr->IsPC()) // 쫓아가는 상대가 PC일 때
	{
		// If i'm in a party. I must obey party leader's AI.
		if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
		{
			if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000) // 마지막으로 공격받은지 15초가 지났고
			{
				// 마지막 맞은 곳으로 부터 50미터 이상 차이나면 포기하고 돌아간다.
				if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
					if (Return())
						return true;
			}
		}
	}

	if (IsGuardNPC())
	{
		if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
			if (Return())
				return true;
	}

#ifdef ENABLE_MOUNT_SYSTEM
	if (pkChr->IsState(pkChr->m_stateMove) &&
		GetMobBattleType() != BATTLE_TYPE_RANGE &&
		GetMobBattleType() != BATTLE_TYPE_MAGIC &&
		false == IsPet() && false == IsMountSystem() && false == IsNewPet() && false == IsSupport())
#else
	if (pkChr->IsState(pkChr->m_stateMove) && 
		GetMobBattleType() != BATTLE_TYPE_RANGE && 
		GetMobBattleType() != BATTLE_TYPE_MAGIC &&
		false == IsPet())
#endif
	{
		// 대상이 이동중이면 예측 이동을 한다
		// 나와 상대방의 속도차와 거리로부터 만날 시간을 예상한 후
		// 상대방이 그 시간까지 직선으로 이동한다고 가정하여 거기로 이동한다.
		float rot = pkChr->GetRotation();
		float rot_delta = GetDegreeDelta(rot, GetDegreeFromPositionXY(GetX(), GetY(), pkChr->GetX(), pkChr->GetY()));

		float yourSpeed = pkChr->GetMoveSpeed();
		float mySpeed = GetMoveSpeed();

		float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());
		float fFollowSpeed = mySpeed - yourSpeed * cos(rot_delta * M_PI / 180);

		if (fFollowSpeed >= 0.1f)
		{
			float fMeetTime = fDist / fFollowSpeed;
			float fYourMoveEstimateX, fYourMoveEstimateY;

			if( fMeetTime * yourSpeed <= 100000.0f )
			{
				GetDeltaByDegree(pkChr->GetRotation(), fMeetTime * yourSpeed, &fYourMoveEstimateX, &fYourMoveEstimateY);

				x += (long) fYourMoveEstimateX;
				y += (long) fYourMoveEstimateY;

				float fDistNew = sqrt(((double)x - GetX())*(x-GetX())+((double)y - GetY())*(y-GetY()));
				if (fDist < fDistNew)
				{
					x = (long)(GetX() + (x - GetX()) * fDist / fDistNew);
					y = (long)(GetY() + (y - GetY()) * fDist / fDistNew);
				}
			}
		}
	}

	// 가려는 위치를 바라봐야 한다.
	SetRotationToXY(x, y);

	float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());

	if (fDist <= fMinDistance)
		return false;

	float fx, fy;

	if (IsChangeAttackPosition(pkChr) && GetMobRank() < MOB_RANK_BOSS)
	{
		// 상대방 주변 랜덤한 곳으로 이동
		SetChangeAttackPositionTime();

		int retry = 16;
		int dx, dy;
		int rot = (int) GetDegreeFromPositionXY(x, y, GetX(), GetY());

		while (--retry)
		{
			if (fDist < 500.0f)
				GetDeltaByDegree((rot + number(-90, 90) + number(-90, 90)) % 360, fMinDistance, &fx, &fy);
			else
				GetDeltaByDegree(number(0, 359), fMinDistance, &fx, &fy);

			dx = x + (int) fx;
			dy = y + (int) fy;

			LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), dx, dy);

			if (NULL == tree)
				break;

			if (0 == (tree->GetAttribute(dx, dy) & (ATTR_BLOCK | ATTR_OBJECT)))
				break;
		}

		//sys_log(0, "근처 어딘가로 이동 %s retry %d", GetName(), retry);
		if (!Goto(dx, dy))
			return false;
	}
	else
	{
		// 직선 따라가기
		float fDistToGo = fDist - fMinDistance;
		GetDeltaByDegree(GetRotation(), fDistToGo, &fx, &fy);

		//sys_log(0, "직선으로 이동 %s", GetName());
		if (!Goto(GetX() + (int) fx, GetY() + (int) fy))
			return false;
	}

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	//MonsterLog("쫓아가기; %s", pkChr->GetName());
	return true;
}

float CHARACTER::GetDistanceFromSafeboxOpen() const
{
	return DISTANCE_APPROX(GetX() - m_posSafeboxOpen.x, GetY() - m_posSafeboxOpen.y);
}

void CHARACTER::SetSafeboxOpenPosition()
{
	m_posSafeboxOpen = GetXYZ();
}

CSafebox * CHARACTER::GetSafebox() const
{
	return m_pkSafebox;
}

void CHARACTER::ReqSafeboxLoad(const char* pszPassword)
{
	if (!*pszPassword || strlen(pszPassword) > SAFEBOX_PASSWORD_MAX_LEN)
	{
		ChatPacket(CHAT_TYPE_INFO, "65");
		return;
	}
	else if (m_pkSafebox)
	{
		ChatPacket(CHAT_TYPE_INFO, "66");
		return;
	}

	int iPulse = thecore_pulse();

	if (iPulse - GetSafeboxLoadTime()  < PASSES_PER_SEC(10))// need timer.
	{
		ChatPacket(CHAT_TYPE_INFO, "67");
		return;
	}
	//else if (GetDistanceFromSafeboxOpen() > 1000)
	//{
	//	ChatPacket(CHAT_TYPE_INFO, "68");
	//	return;
	//}
	else if (m_bOpeningSafebox)
	{
		sys_log(0, "Overlapped safebox load request from %s", GetName());
		return;
	}

	SetSafeboxLoadTime();
	m_bOpeningSafebox = true;

	TSafeboxLoadPacket p;
	p.dwID = GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, pszPassword, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_LOAD, GetDesc()->GetHandle(), &p, sizeof(p));
}

void CHARACTER::LoadSafebox(int iSize, DWORD dwGold, int iItemCount, TPlayerItem * pItems)
{
	bool bLoaded = false;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(true);
	//END_PREVENT_TRADE_WINDOW

	if (m_pkSafebox)
		bLoaded = true;

	if (!m_pkSafebox)
		m_pkSafebox = M2_NEW CSafebox(this, iSize, dwGold);
	else
		m_pkSafebox->ChangeSize(iSize);

	m_iSafeboxSize = iSize;

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_SAFEBOX_SIZE;
	p.bSize = iSize;

	GetDesc()->Packet(&p, sizeof(TPacketCGSafeboxSize));

	if (!bLoaded)
	{
		for (int i = 0; i < iItemCount; ++i, ++pItems)
		{
			if (!m_pkSafebox->IsValidPosition(pItems->pos))
				continue;

#ifdef __SOULBINDING_SYSTEM__
			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id, false, -1, false, pItems->bind);
#else
			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id);
#endif

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", pItems->vnum, pItems->id, GetName());
				continue;
			}

			item->SetSkipSave(true);
			item->SetSockets(pItems->alSockets);
			item->SetAttributes(pItems->aAttr);
#ifdef ENABLE_GLOVE_SYSTEM
			item->SetRandomAttributes(pItems->randomAttr);
#endif
#ifdef ENABLE_NEW_NAME_ITEM
			item->SetNewName(pItems->name);
#endif
#ifdef ELEMENT_SPELL_WORLDARD
			item->SetElement(pItems->grade_element,pItems->attack_element,pItems->element_type_bonus,pItems->elements_value_bonus);
#endif
			if (!m_pkSafebox->Add(pItems->pos, item))
			{
				M2_DESTROY_ITEM(item);
			}
			else
				item->SetSkipSave(false);
		}
	}
}

void CHARACTER::ChangeSafeboxSize(BYTE bSize)
{
	//if (!m_pkSafebox)
	//return;

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_SAFEBOX_SIZE;
	p.bSize = bSize;

	GetDesc()->Packet(&p, sizeof(TPacketCGSafeboxSize));

	if (m_pkSafebox)
		m_pkSafebox->ChangeSize(bSize);

	m_iSafeboxSize = bSize;
}

void CHARACTER::CloseSafebox()
{
	if (!m_pkSafebox)
		return;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(false);
	//END_PREVENT_TRADE_WINDOW

	m_pkSafebox->Save();

	M2_DELETE(m_pkSafebox);
	m_pkSafebox = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseSafebox");

	SetSafeboxLoadTime();
	m_bOpeningSafebox = false;

	Save();
}

CSafebox * CHARACTER::GetMall() const
{
	return m_pkMall;
}

void CHARACTER::LoadMall(int iItemCount, TPlayerItem * pItems)
{
	bool bLoaded = false;

	//if (m_pkMall)//i think here...
	//	bLoaded = true;

	if (!m_pkMall)
		m_pkMall = M2_NEW CSafebox(this, 3 * SAFEBOX_PAGE_SIZE, 0);
	else
		m_pkMall->ChangeSize(3 * SAFEBOX_PAGE_SIZE);

	m_pkMall->SetWindowMode(MALL);

	TPacketCGSafeboxSize p;

	p.bHeader = HEADER_GC_MALL_OPEN;
	p.bSize = 3 * SAFEBOX_PAGE_SIZE;

	GetDesc()->Packet(&p, sizeof(TPacketCGSafeboxSize));

	if (!bLoaded)
	{
		for (int i = 0; i < iItemCount; ++i, ++pItems)
		{
			if (!m_pkMall->IsValidPosition(pItems->pos))
				continue;

#ifdef __SOULBINDING_SYSTEM__
			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id, false, -1, false, pItems->bind);
#else
			LPITEM item = ITEM_MANAGER::instance().CreateItem(pItems->vnum, pItems->count, pItems->id);
#endif

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", pItems->vnum, pItems->id, GetName());
				continue;
			}

			item->SetSkipSave(true);
			item->SetSockets(pItems->alSockets);
			item->SetAttributes(pItems->aAttr);
#ifdef ENABLE_GLOVE_SYSTEM
			item->SetRandomAttributes(pItems->randomAttr);
#endif


			if (!m_pkMall->Add(pItems->pos, item))
				M2_DESTROY_ITEM(item);
			else
				item->SetSkipSave(false);
		}
	}
}

void CHARACTER::CloseMall()
{
	if (!m_pkMall)
		return;

	m_pkMall->Save();

	M2_DELETE(m_pkMall);
	m_pkMall = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseMall");
}

bool CHARACTER::BuildUpdatePartyPacket(TPacketGCPartyUpdate & out)
{
	if (!GetParty())
		return false;

	memset(&out, 0, sizeof(out));

	out.header		= HEADER_GC_PARTY_UPDATE;
	out.pid		= GetPlayerID();
	if (GetMaxHP() <= 0) // @fixme136
		out.percent_hp	= 0;
	else
		out.percent_hp	= MINMAX(0, GetHP() * 100 / GetMaxHP(), 100);
	out.role		= GetParty()->GetRole(GetPlayerID());

	sys_log(1, "PARTY %s role is %d", GetName(), out.role);

	LPCHARACTER l = GetParty()->GetLeaderCharacter();

	if (l && DISTANCE_APPROX(GetX() - l->GetX(), GetY() - l->GetY()) < PARTY_DEFAULT_RANGE)
	{
		out.affects[0] = GetParty()->GetPartyBonusExpPercent();
		out.affects[1] = GetPoint(POINT_PARTY_ATTACKER_BONUS);
		out.affects[2] = GetPoint(POINT_PARTY_TANKER_BONUS);
		out.affects[3] = GetPoint(POINT_PARTY_BUFFER_BONUS);
		out.affects[4] = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
		out.affects[5] = GetPoint(POINT_PARTY_HASTE_BONUS);
		out.affects[6] = GetPoint(POINT_PARTY_DEFENDER_BONUS);
	}

	return true;
}

int CHARACTER::GetLeadershipSkillLevel() const
{
	return GetSkillLevel(SKILL_LEADERSHIP);
}

void CHARACTER::QuerySafeboxSize()
{
	if (m_iSafeboxSize == -1)
	{
		DBManager::instance().ReturnQuery(QID_SAFEBOX_SIZE,
				GetPlayerID(),
				NULL,
				"SELECT size FROM safebox%s WHERE account_id = %u",
				get_table_postfix(),
				GetDesc()->GetAccountTable().id);
	}
}

void CHARACTER::SetSafeboxSize(int iSize)
{
	sys_log(1, "SetSafeboxSize: %s %d", GetName(), iSize);
	m_iSafeboxSize = iSize;
	DBManager::instance().Query("UPDATE safebox%s SET size = %d WHERE account_id = %u", get_table_postfix(), iSize / SAFEBOX_PAGE_SIZE, GetDesc()->GetAccountTable().id);
}

int CHARACTER::GetSafeboxSize() const
{
	return m_iSafeboxSize;
}

void CHARACTER::SetNowWalking(bool bWalkFlag)
{
	//if (m_bNowWalking != bWalkFlag || IsNPC())
	if (m_bNowWalking != bWalkFlag)
	{
		if (bWalkFlag)
		{
			m_bNowWalking = true;
			m_dwWalkStartTime = get_dword_time();
		}
		else
		{
			m_bNowWalking = false;
		}

		//if (m_bNowWalking)
		{
			TPacketGCWalkMode p;
			p.vid = GetVID();
			p.header = HEADER_GC_WALK_MODE;
			p.mode = m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN;

			PacketView(&p, sizeof(p));
		}

		if (IsNPC())
		{
			if (m_bNowWalking)
				MonsterLog("걷는다");
			else
				MonsterLog("뛴다");
		}

		//sys_log(0, "%s is now %s", GetName(), m_bNowWalking?"walking.":"running.");
	}
}

void CHARACTER::StartStaminaConsume()
{
	if (m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = true;
	//ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
	if (IsStaminaHalfConsume())
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec / 2, GetStamina());
	else
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
}

void CHARACTER::StopStaminaConsume()
{
	if (!m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = false;
	ChatPacket(CHAT_TYPE_COMMAND, "StopStaminaConsume %d", GetStamina());
}

bool CHARACTER::IsStaminaConsume() const
{
	return m_bStaminaConsume;
}

bool CHARACTER::IsStaminaHalfConsume() const
{
	return IsEquipUniqueItem(UNIQUE_ITEM_HALF_STAMINA);
}

void CHARACTER::ResetStopTime()
{
	m_dwStopTime = get_dword_time();
}

DWORD CHARACTER::GetStopTime() const
{
	return m_dwStopTime;
}

void CHARACTER::ResetPoint(int iLv)
{
	BYTE bJob = GetJob();

	PointChange(POINT_LEVEL, iLv - GetLevel());

	SetRealPoint(POINT_ST, JobInitialPoints[bJob].st);
	SetPoint(POINT_ST, GetRealPoint(POINT_ST));

	SetRealPoint(POINT_HT, JobInitialPoints[bJob].ht);
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));

	SetRealPoint(POINT_DX, JobInitialPoints[bJob].dx);
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));

	SetRealPoint(POINT_IQ, JobInitialPoints[bJob].iq);
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	SetRandomHP((iLv - 1) * number(JobInitialPoints[GetJob()].hp_per_lv_begin, JobInitialPoints[GetJob()].hp_per_lv_end));
	SetRandomSP((iLv - 1) * number(JobInitialPoints[GetJob()].sp_per_lv_begin, JobInitialPoints[GetJob()].sp_per_lv_end));

	// @fixme104
	PointChange(POINT_STAT, (MINMAX(1, iLv, g_iStatusPointGetLevelLimit) * 3) + GetPoint(POINT_LEVEL_STEP) - GetPoint(POINT_STAT));

	ComputePoints();

	// 회복
	PointChange(POINT_HP, GetMaxHP() - GetHP());
	PointChange(POINT_SP, GetMaxSP() - GetSP());

	PointsPacket();

	LogManager::instance().CharLog(this, 0, "RESET_POINT", "");
}

bool CHARACTER::IsChangeAttackPosition(LPCHARACTER target) const
{
	if (!IsNPC())
		return true;

	DWORD dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_NEAR;

	if (DISTANCE_APPROX(GetX() - target->GetX(), GetY() - target->GetY()) >
		AI_CHANGE_ATTACK_POISITION_DISTANCE + GetMobAttackRange())
		dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_FAR;

	return get_dword_time() - m_dwLastChangeAttackPositionTime > dwChangeTime;
}

void CHARACTER::GiveRandomSkillBook()
{
	LPITEM item = AutoGiveItem(50300);

	if (NULL != item)
	{
		extern const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);
		DWORD dwSkillVnum = 0;
		// 50% of getting random books or getting one of the same player's race
		if (!number(0, 1))
			dwSkillVnum = GetRandomSkillVnum(GetJob());
		else
			dwSkillVnum = GetRandomSkillVnum();
		item->SetSocket(0, dwSkillVnum);
	}
}

void CHARACTER::ReviveInvisible(int iDur)
{
	AddAffect(AFFECT_REVIVE_INVISIBLE, POINT_NONE, 0, AFF_REVIVE_INVISIBLE, iDur, 0, true);
}

void CHARACTER::ToggleMonsterLog()
{
	m_bMonsterLog = !m_bMonsterLog;

	if (m_bMonsterLog)
	{
		CHARACTER_MANAGER::instance().RegisterForMonsterLog(this);
	}
	else
	{
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);
	}
}

void CHARACTER::SetGuild(CGuild* pGuild)
{
	if (m_pGuild != pGuild)
	{
		m_pGuild = pGuild;
		UpdatePacket();
	}
}

void CHARACTER::SendGreetMessage()
{
	__typeof(DBManager::instance().GetGreetMessage()) v = DBManager::instance().GetGreetMessage();

	for (itertype(v) it = v.begin(); it != v.end(); ++it)
	{
		ChatPacket(CHAT_TYPE_NOTICE, it->c_str());
	}
}

void CHARACTER::BeginStateEmpty()
{
	MonsterLog("!");
}

void CHARACTER::EffectPacket(int enumEffectType)
{
	TPacketGCSpecialEffect p;

	p.header = HEADER_GC_SEPCIAL_EFFECT;
	p.type = enumEffectType;
	p.vid = GetVID();

	PacketAround(&p, sizeof(TPacketGCSpecialEffect));
}

void CHARACTER::SpecificEffectPacket(const char filename[MAX_EFFECT_FILE_NAME])
{
	TPacketGCSpecificEffect p;

	p.header = HEADER_GC_SPECIFIC_EFFECT;
	p.vid = GetVID();
	memcpy (p.effect_file, filename, MAX_EFFECT_FILE_NAME);

	PacketAround(&p, sizeof(TPacketGCSpecificEffect));
}

void CHARACTER::MonsterChat(BYTE bMonsterChatType)
{
	if (IsPC())
		return;

	char sbuf[256+1];

	if (IsMonster())
	{
		if (number(0, 60))
			return;

		snprintf(sbuf, sizeof(sbuf),
				"(locale.monster_chat[%i] and locale.monster_chat[%i][%d] or '')",
				GetRaceNum(), GetRaceNum(), bMonsterChatType*3 + number(1, 3));
	}
	else
	{
		if (bMonsterChatType != MONSTER_CHAT_WAIT)
			return;

		if (IsGuardNPC())
		{
			if (number(0, 6))
				return;
		}
		else
		{
			if (number(0, 30))
				return;
		}

		snprintf(sbuf, sizeof(sbuf), "(locale.monster_chat[%i] and locale.monster_chat[%i][number(1, table.getn(locale.monster_chat[%i]))] or '')", GetRaceNum(), GetRaceNum(), GetRaceNum());
	}

	std::string text = quest::ScriptToString(sbuf);

	if (text.empty())
		return;

	struct packet_chat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size	= sizeof(struct packet_chat) + text.size() + 1;
	pack_chat.type      = CHAT_TYPE_TALKING;
	pack_chat.id        = GetVID();
	pack_chat.bEmpire	= 0;

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(text.c_str(), text.size() + 1);

	PacketAround(buf.read_peek(), buf.size());
}

void CHARACTER::SetQuestNPCID(DWORD vid)
{
	m_dwQuestNPCVID = vid;
}

LPCHARACTER CHARACTER::GetQuestNPC() const
{
	return CHARACTER_MANAGER::instance().Find(m_dwQuestNPCVID);
}

void CHARACTER::SetQuestItemPtr(LPITEM item)
{
	m_pQuestItem = item;
}

void CHARACTER::ClearQuestItemPtr()
{
	m_pQuestItem = NULL;
}

LPITEM CHARACTER::GetQuestItemPtr() const
{
	return m_pQuestItem;
}

LPDUNGEON CHARACTER::GetDungeonForce() const
{
	if (m_lWarpMapIndex > 10000)
		return CDungeonManager::instance().FindByMapIndex(m_lWarpMapIndex);

	return m_pkDungeon;
}

void CHARACTER::SetBlockMode(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;

	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);

	SetQuestFlag("game_option.block_exchange", bFlag & BLOCK_EXCHANGE ? 1 : 0);
	SetQuestFlag("game_option.block_party_invite", bFlag & BLOCK_PARTY_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_guild_invite", bFlag & BLOCK_GUILD_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_whisper", bFlag & BLOCK_WHISPER ? 1 : 0);
	SetQuestFlag("game_option.block_messenger_invite", bFlag & BLOCK_MESSENGER_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_party_request", bFlag & BLOCK_PARTY_REQUEST ? 1 : 0);
}

void CHARACTER::SetBlockModeForce(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;
	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);
}

bool CHARACTER::IsGuardNPC() const
{
	return IsNPC() && (GetRaceNum() == 11000 || GetRaceNum() == 11002 || GetRaceNum() == 11004);
}

int CHARACTER::GetPolymorphPower() const
{
	if (test_server)
	{
		int value = quest::CQuestManager::instance().GetEventFlag("poly");
		if (value)
			return value;
	}
	return aiPolymorphPowerByLevel[MINMAX(0, GetSkillLevel(SKILL_POLYMORPH), 40)];
}

void CHARACTER::SetPolymorph(DWORD dwRaceNum, bool bMaintainStat)
{
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (dwRaceNum < MAIN_RACE_MAX_NUM)
#else
	if (dwRaceNum < JOB_MAX_NUM)
#endif
	{
		dwRaceNum = 0;
		bMaintainStat = false;
	}

	if (m_dwPolymorphRace == dwRaceNum)
		return;

	m_bPolyMaintainStat = bMaintainStat;
	m_dwPolymorphRace = dwRaceNum;

	sys_log(0, "POLYMORPH: %s race %u ", GetName(), dwRaceNum);

	if (dwRaceNum != 0)
		StopRiding();

	SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
	m_afAffectFlag.Set(AFF_SPAWN);

	ViewReencode();

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	if (!bMaintainStat)
	{
		PointChange(POINT_ST, 0);
		PointChange(POINT_DX, 0);
		PointChange(POINT_IQ, 0);
		PointChange(POINT_HT, 0);
	}

	// 폴리모프 상태에서 죽는 경우, 폴리모프가 풀리게 되는데
	// 폴리 모프 전후로 valid combo interval이 다르기 때문에
	// Combo 핵 또는 Hacker로 인식하는 경우가 있다.
	// 따라서 폴리모프를 풀거나 폴리모프 하게 되면,
	// valid combo interval을 reset한다.
	SetValidComboInterval(0);
	SetComboSequence(0);

	ComputeBattlePoints();
}

int CHARACTER::GetQuestFlag(const std::string& flag) const
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());

	if(!pPC) {
        return 0;
    }  

	return pPC->GetFlag(flag);
}

void CHARACTER::SetQuestFlag(const std::string& flag, int value)
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	if(!pPC) {
        return;
    } 

	pPC->SetFlag(flag, value);
}

void CHARACTER::DetermineDropMetinStone()
{
#ifdef ENABLE_NEWSTUFF
	if (g_NoDropMetinStone)
	{
		m_dwDropMetinStone = 0;
		return;
	}
#endif

	static const DWORD c_adwMetin[] =
	{
// #if defined(ENABLE_WOLFMAN_CHARACTER) && defined(USE_WOLFMAN_STONES)
		// 28012,
// #endif
		28030,
		28031,
		28032,
		28033,
		28034,
		28035,
		28036,
		28037,
		28038,
		28039,
		28040,
		28041,
		28042,
		28043,
		28044,
		28045,
		// 28046,
		// 28047,
		// 28048,
		// 28049,
// #if defined(ENABLE_MAGIC_REDUCTION_SYSTEM) && defined(USE_MAGIC_REDUCTION_STONES)
		// 28044,
		// 28045,
// #endif
	};
	DWORD stone_num = GetRaceNum();
	int idx = std::lower_bound(aStoneDrop, aStoneDrop+STONE_INFO_MAX_NUM, stone_num) - aStoneDrop;
	if (idx >= STONE_INFO_MAX_NUM || aStoneDrop[idx].dwMobVnum != stone_num)
	{
		m_dwDropMetinStone = 0;
	}
	else
	{
		const SStoneDropInfo & info = aStoneDrop[idx];
		m_bDropMetinStonePct = info.iDropPct;
		{
			m_dwDropMetinStone = c_adwMetin[number(0, sizeof(c_adwMetin)/sizeof(DWORD) - 1)];
			int iGradePct = number(1, 100);
			for (int iStoneLevel = 0; iStoneLevel < STONE_LEVEL_MAX_NUM; iStoneLevel ++)
			{
				int iLevelGradePortion = info.iLevelPct[iStoneLevel];
				if (iGradePct <= iLevelGradePortion)
				{
					break;
				}
				else
				{
					iGradePct -= iLevelGradePortion;
					m_dwDropMetinStone += 100; // 돌 +a -> +(a+1)이 될때마다 100씩 증가
				}
			}
		}
	}
}

void CHARACTER::SendEquipment(LPCHARACTER ch)
{
	TPacketViewEquip p;
	p.header = HEADER_GC_VIEW_EQUIP;
	p.vid    = GetVID();
	for (int i = 0; i<WEAR_MAX_NUM; i++)
	{
		LPITEM item = GetWear(i);
		if (item)
		{
			p.equips[i].vnum = item->GetVnum();
			p.equips[i].count = item->GetCount();

			thecore_memcpy(p.equips[i].alSockets, item->GetSockets(), sizeof(p.equips[i].alSockets));
			thecore_memcpy(p.equips[i].aAttr, item->GetAttributes(), sizeof(p.equips[i].aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
			thecore_memcpy(p.equips[i].randomAttr, item->GetRandomAttributes(), sizeof(p.equips[i].randomAttr));
#endif

		}
		else
		{
			p.equips[i].vnum = 0;
		}
	}
	ch->GetDesc()->Packet(&p, sizeof(p));
}

bool CHARACTER::CanSummon(int iLeaderShip)
{
	return ((iLeaderShip >= 20) || ((iLeaderShip >= 12) && ((m_dwLastDeadTime + 180) > get_dword_time())));
}
#ifdef ENABLE_BIOLOG_SYSTEM
int CHARACTER::GetBiologState(const std::string& state) const
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	if (!pPC)
		return 0;
	return pPC->GetFlag(state);
}

void CHARACTER::SetBiologState(const std::string& state, int szValue)
{
	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	if (!pPC)
		return;
	pPC->SetFlag(state, szValue);
}
#endif


void CHARACTER::MountVnum(DWORD vnum)
{
	if (m_dwMountVnum == vnum)
		return;
	if ((m_dwMountVnum != 0)&&(vnum!=0)) //@fixme108 set recursively to 0 for eventuality
		MountVnum(0);

	m_dwMountVnum = vnum;
	m_dwMountTime = get_dword_time();
#if defined(WJ_COMBAT_ZONE)	
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif
	if (m_bIsObserver)
		return;

	//NOTE : Mount한다고 해서 Client Side의 객체를 삭제하진 않는다.
	//그리고 서버Side에서 탔을때 위치 이동은 하지 않는다. 왜냐하면 Client Side에서 Coliision Adjust를 할수 있는데
	//객체를 소멸시켰다가 서버위치로 이동시키면 이때 collision check를 하지는 않으므로 배경에 끼거나 뚫고 나가는 문제가 존재한다.
	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
	//EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		//Mount한다고 해서 Client Side의 객체를 삭제하진 않는다.
		//EncodeRemovePacket(entity);
		//if (!m_bIsObserver)
		EncodeInsertPacket(entity);

		//if (!entity->IsObserverMode())
		//	entity->EncodeInsertPacket(this);
	}

	SetValidComboInterval(0);
	SetComboSequence(0);

	ComputePoints();
}

namespace {
	class FuncCheckWarp
	{
		public:
			FuncCheckWarp(LPCHARACTER pkWarp)
			{
				m_lTargetY = 0;
				m_lTargetX = 0;

				m_lX = pkWarp->GetX();
				m_lY = pkWarp->GetY();

				m_bInvalid = false;
				m_bEmpire = pkWarp->GetEmpire();

				char szTmp[64];

				if (3 != sscanf(pkWarp->GetName(), " %s %ld %ld ", szTmp, &m_lTargetX, &m_lTargetY))
				{
					if (number(1, 100) < 5)
						sys_err("Warp NPC name wrong : vnum(%d) name(%s)", pkWarp->GetRaceNum(), pkWarp->GetName());

					m_bInvalid = true;

					return;
				}

				m_lTargetX *= 100;
				m_lTargetY *= 100;

				m_bUseWarp = true;

				if (pkWarp->IsGoto())
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(pkWarp->GetMapIndex());
					m_lTargetX += pkSectreeMap->m_setting.iBaseX;
					m_lTargetY += pkSectreeMap->m_setting.iBaseY;
					m_bUseWarp = false;
				}
			}

			bool Valid()
			{
				return !m_bInvalid;
			}

			void operator () (LPENTITY ent)
			{
				if (!Valid())
					return;

				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX() - m_lX, pkChr->GetY() - m_lY);

				if (iDist > 300)
					return;

				if (m_bEmpire && pkChr->GetEmpire() && m_bEmpire != pkChr->GetEmpire())
					return;

				if (pkChr->IsHack())
					return;

				if (!pkChr->CanHandleItem(false, true))
					return;

				if (m_bUseWarp)
					pkChr->WarpSet(m_lTargetX, m_lTargetY);
				else
				{
					pkChr->Show(pkChr->GetMapIndex(), m_lTargetX, m_lTargetY);
					pkChr->Stop();
				}
			}

			bool m_bInvalid;
			bool m_bUseWarp;

			long m_lX;
			long m_lY;
			long m_lTargetX;
			long m_lTargetY;

			BYTE m_bEmpire;
	};
}

EVENTFUNC(warp_npc_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "warp_npc_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->GetSectree())
	{
		ch->m_pkWarpNPCEvent = NULL;
		return 0;
	}

	FuncCheckWarp f(ch);
	if (f.Valid())
		ch->GetSectree()->ForEachAround(f);

	return passes_per_sec / 2;
}

#ifdef CHANGE_CHANNEL
void CHARACTER::ChannelSwitch(int new_ch){
	long lAddr;
	long lMapIndex;
	WORD wPort;
	long x = this->GetX();
	long y = this->GetY();

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
	{
			sys_err("cannot find map location index %d x %d y %d name %s", lMapIndex, x, y, GetName());
			return;
	}
	if(lMapIndex >= 10000){
		return;
	}
	
	std::map<WORD, int>micha;
	for(int i = 0; i < 4; i++){ //replace with maximum channels -1   actual 1300x - 1330x
		for(int i2 = 2; i2 < 6; i2++){ //replace with your core values actual  13x02 - 13x03
			micha[13*1000 + i*100 + i2] = i+1;
		}
	}

	int chan;
	if(micha.find(wPort) != micha.end()){
		chan = micha[wPort];
	}
	else{
		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, "Change Channel cant find the new port %d, contact with the admin.", wPort);
		return;
	}
	if (test_server)
		ChatPacket(CHAT_TYPE_INFO, "new_ch: %d, lAddr: %d, lMapIndex: %d, wPort: %d, GetName: %s, NewwPort: %d, chan: %d", new_ch, lAddr, lMapIndex, wPort, GetName(), (wPort - 100*(chan-1) + 100*(new_ch-1)), chan);
	Stop();
	Save();
	
	if(GetSectree()){
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}
	
	
	TPacketGCWarp p;

	p.bHeader	= HEADER_GC_WARP;
	p.lX	= x;
	p.lY	= y;
	p.lAddr	= lAddr;
	p.wPort	= (wPort - 100*(chan-1) + 100*(new_ch-1));
	
	sys_err("x : %d y: %d, addr: %d, port: %d , newPort: %d", x, y, lAddr, wPort, (wPort - 100*chan + 100*new_ch));

	GetDesc()->Packet(&p, sizeof(TPacketGCWarp));
}
#endif

void CHARACTER::StartWarpNPCEvent()
{
	if (m_pkWarpNPCEvent)
		return;

	if (!IsWarp() && !IsGoto())
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkWarpNPCEvent = event_create(warp_npc_event, info, passes_per_sec / 2);
}

void CHARACTER::SyncPacket()
{
	TEMP_BUFFER buf;

	TPacketCGSyncPositionElement elem;

	elem.dwVID = GetVID();
	elem.lX = GetX();
	elem.lY = GetY();

	TPacketGCSyncPosition pack;

	pack.bHeader = HEADER_GC_SYNC_POSITION;
	pack.wSize = sizeof(TPacketGCSyncPosition) + sizeof(elem);

	buf.write(&pack, sizeof(pack));
	buf.write(&elem, sizeof(elem));

	PacketAround(buf.read_peek(), buf.size());
}

LPCHARACTER CHARACTER::GetMarryPartner() const
{
	return m_pkChrMarried;
}

void CHARACTER::SetMarryPartner(LPCHARACTER ch)
{
	m_pkChrMarried = ch;
}

int CHARACTER::GetMarriageBonus(DWORD dwItemVnum, bool bSum)
{
	if (IsNPC())
		return 0;

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());

	if (!pMarriage)
		return 0;

	return pMarriage->GetBonus(dwItemVnum, bSum, this);
}

void CHARACTER::ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID)
{
	if (!IsPC())
		return;

	TPacketGCQuestConfirm p;

	p.header = HEADER_GC_QUEST_CONFIRM;
	p.requestPID = dwRequestPID;
	p.timeout = iTimeout;
	strlcpy(p.msg, szMsg, sizeof(p.msg));

	GetDesc()->Packet(&p, sizeof(p));
}

int CHARACTER::GetPremiumRemainSeconds(BYTE bType) const
{
	if (bType >= PREMIUM_MAX_NUM)
		return 0;

	return m_aiPremiumTimes[bType] - get_global_time();
}

bool CHARACTER::WarpToPID(DWORD dwPID)
{
	LPCHARACTER victim;
	if ((victim = (CHARACTER_MANAGER::instance().FindByPID(dwPID))))
	{
		int mapIdx = victim->GetMapIndex();
		if (IS_SUMMONABLE_ZONE(mapIdx))
		{
			if (CAN_ENTER_ZONE(this, mapIdx))
			{
				WarpSet(victim->GetX(), victim->GetY());
			}
			else
			{
				ChatPacket(CHAT_TYPE_INFO, "69");
				return false;
			}
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, "69");
			return false;
		}
	}
	else
	{
		// 다른 서버에 로그인된 사람이 있음 -> 메시지 보내 좌표를 받아오자
		// 1. A.pid, B.pid 를 뿌림
		// 2. B.pid를 가진 서버가 뿌린서버에게 A.pid, 좌표 를 보냄
		// 3. 워프
		CCI * pcci = P2P_MANAGER::instance().FindByPID(dwPID);

		if (!pcci)
		{
			ChatPacket(CHAT_TYPE_INFO, "70");
			return false;
		}

		if (pcci->bChannel != g_bChannel)
		{
			ChatPacket(CHAT_TYPE_INFO, "71 %d %d", pcci->bChannel, g_bChannel);
			return false;
		}
		else if (false == IS_SUMMONABLE_ZONE(pcci->lMapIndex))
		{
			ChatPacket(CHAT_TYPE_INFO, "69");
			return false;
		}
		else
		{
			if (!CAN_ENTER_ZONE(this, pcci->lMapIndex))
			{
				ChatPacket(CHAT_TYPE_INFO, "69");
				return false;
			}

			TPacketGGFindPosition p;
			p.header = HEADER_GG_FIND_POSITION;
			p.dwFromPID = GetPlayerID();
			p.dwTargetPID = dwPID;
			pcci->pkDesc->Packet(&p, sizeof(TPacketGGFindPosition));

			if (test_server)
				ChatPacket(CHAT_TYPE_PARTY, "sent find position packet for teleport");
		}
	}
	return true;
}

// ADD_REFINE_BUILDING
CGuild* CHARACTER::GetRefineGuild() const
{
	LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);

	return (chRefineNPC ? chRefineNPC->GetGuild() : NULL);
}

bool CHARACTER::IsRefineThroughGuild() const
{
	return GetRefineGuild() != NULL;
}

int CHARACTER::ComputeRefineFee(int iCost, int iMultiply) const
{
	CGuild* pGuild = GetRefineGuild();
	if (pGuild)
	{
		if (pGuild == GetGuild())
			return iCost * iMultiply * 9 / 10;

		// 다른 제국 사람이 시도하는 경우 추가로 3배 더
		LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);
		if (chRefineNPC && chRefineNPC->GetEmpire() != GetEmpire())
			return iCost * iMultiply * 3;

		return iCost * iMultiply;
	}
	else
		return iCost;
}

void CHARACTER::PayRefineFee(int iTotalMoney)
{
	int iFee = iTotalMoney / 10;
	CGuild* pGuild = GetRefineGuild();

	int iRemain = iTotalMoney;

	if (pGuild)
	{
		// 자기 길드이면 iTotalMoney에 이미 10%가 제외되어있다
		if (pGuild != GetGuild())
		{
			pGuild->RequestDepositMoney(this, iFee);
			iRemain -= iFee;
		}
	}

	PointChange(POINT_GOLD, -iRemain);
}
// END_OF_ADD_REFINE_BUILDING

//Hack 방지를 위한 체크.
bool CHARACTER::IsHack(bool bSendMsg, bool bCheckShopOwner, int limittime)
{
	const int iPulse = thecore_pulse();

	if (test_server)
		bSendMsg = true;

	//창고 연후 체크
	if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, "72 %d", limittime);

		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(limittime));
		return true;
	}

	//거래관련 창 체크
	if (bCheckShopOwner)
	{
		if (GetExchange() || GetShop() || IsOpenSafebox() || IsCubeOpen()
#if defined(__BL_67_ATTR__)
			|| Is67AttrOpen()
#endif
			)
		{
			if (bSendMsg)
				ChatPacket(CHAT_TYPE_INFO, "73");

			return true;
		}
	}
	else
	{
		if (GetExchange() || GetShop() || IsOpenSafebox() || IsCubeOpen()
#if defined(__BL_67_ATTR__)
			|| Is67AttrOpen()
#endif
			)
		{
			if (bSendMsg)
				ChatPacket(CHAT_TYPE_INFO, "73");

			return true;
		}
	}

	//PREVENT_PORTAL_AFTER_EXCHANGE
	//교환 후 시간체크
	if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, "74 %d", limittime );
		return true;
	}
	//END_PREVENT_PORTAL_AFTER_EXCHANGE

	//PREVENT_ITEM_COPY
	if (iPulse - GetMyShopTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, "74 %d", limittime);
		return true;
	}

	if (iPulse - GetRefineTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, "75", limittime);
		return true;
	}
	//END_PREVENT_ITEM_COPY

	return false;
}

BOOL CHARACTER::IsMonarch() const
{
	//MONARCH_LIMIT
	if (CMonarch::instance().IsMonarch(GetPlayerID(), GetEmpire()))
		return true;

	return false;

	//END_MONARCH_LIMIT
}
void CHARACTER::Say(const std::string & s)
{
	struct ::packet_script packet_script;

	packet_script.header = HEADER_GC_SCRIPT;
	packet_script.skin = 1;
	packet_script.src_size = s.size();
	packet_script.size = packet_script.src_size + sizeof(struct packet_script);

	TEMP_BUFFER buf;

	buf.write(&packet_script, sizeof(struct packet_script));
	buf.write(&s[0], s.size());

	if (IsPC())
	{
		GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}

//
// Monarch
//
void CHARACTER::InitMC()
{
	for (int n = 0; n < MI_MAX; ++n)
	{
		m_dwMonarchCooltime[n] = thecore_pulse();
	}

	m_dwMonarchCooltimelimit[MI_HEAL] = PASSES_PER_SEC(MC_HEAL);
	m_dwMonarchCooltimelimit[MI_WARP] = PASSES_PER_SEC(MC_WARP);
	m_dwMonarchCooltimelimit[MI_TRANSFER] = PASSES_PER_SEC(MC_TRANSFER);
	m_dwMonarchCooltimelimit[MI_TAX] = PASSES_PER_SEC(MC_TAX);
	m_dwMonarchCooltimelimit[MI_SUMMON] = PASSES_PER_SEC(MC_SUMMON);

	m_dwMonarchCooltime[MI_HEAL] -= PASSES_PER_SEC(GetMCL(MI_HEAL));
	m_dwMonarchCooltime[MI_WARP] -= PASSES_PER_SEC(GetMCL(MI_WARP));
	m_dwMonarchCooltime[MI_TRANSFER] -= PASSES_PER_SEC(GetMCL(MI_TRANSFER));
	m_dwMonarchCooltime[MI_TAX] -= PASSES_PER_SEC(GetMCL(MI_TAX));
	m_dwMonarchCooltime[MI_SUMMON] -= PASSES_PER_SEC(GetMCL(MI_SUMMON));
}

DWORD CHARACTER::GetMC(enum MONARCH_INDEX e) const
{
	return m_dwMonarchCooltime[e];
}

void CHARACTER::SetMC(enum MONARCH_INDEX e)
{
	m_dwMonarchCooltime[e] = thecore_pulse();
}

bool CHARACTER::IsMCOK(enum MONARCH_INDEX e) const
{
	int iPulse = thecore_pulse();

	if ((iPulse -  GetMC(e)) <  GetMCL(e))
	{
		if (test_server)
			sys_log(0, " Pulse %d cooltime %d, limit %d", iPulse, GetMC(e), GetMCL(e));

		return false;
	}

	if (test_server)
		sys_log(0, " Pulse %d cooltime %d, limit %d", iPulse, GetMC(e), GetMCL(e));

	return true;
}

DWORD CHARACTER::GetMCL(enum MONARCH_INDEX e) const
{
	return m_dwMonarchCooltimelimit[e];
}

DWORD CHARACTER::GetMCLTime(enum MONARCH_INDEX e) const
{
	int iPulse = thecore_pulse();

	if (test_server)
		sys_log(0, " Pulse %d cooltime %d, limit %d", iPulse, GetMC(e), GetMCL(e));

	return  (GetMCL(e)) / passes_per_sec   -  (iPulse - GetMC(e)) / passes_per_sec;
}

bool CHARACTER::IsSiegeNPC() const
{
	return IsNPC() && (GetRaceNum() == 11000 || GetRaceNum() == 11002 || GetRaceNum() == 11004);
}

//------------------------------------------------
void CHARACTER::UpdateDepositPulse()
{
	m_deposit_pulse = thecore_pulse() + PASSES_PER_SEC(60*5);	// 5분
}

bool CHARACTER::CanDeposit() const
{
	return (m_deposit_pulse == 0 || (m_deposit_pulse < thecore_pulse()));
}
//------------------------------------------------

ESex GET_SEX(LPCHARACTER ch)
{
	switch (ch->GetRaceNum())
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_SHAMAN_M:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case MAIN_RACE_WOLFMAN_M:
#endif
			return SEX_MALE;

		case MAIN_RACE_ASSASSIN_W:
		case MAIN_RACE_SHAMAN_W:
		case MAIN_RACE_WARRIOR_W:
		case MAIN_RACE_SURA_W:
			return SEX_FEMALE;
	}

	/* default sex = male */
	return SEX_MALE;
}

int CHARACTER::GetHPPct() const
{
	if (GetMaxHP() <= 0) // @fixme136
		return 0;
	return (GetHP() * 100) / GetMaxHP();
}

bool CHARACTER::IsBerserk() const
{
	if (m_pkMobInst != NULL)
		return m_pkMobInst->m_IsBerserk;
	else
		return false;
}

void CHARACTER::SetBerserk(bool mode)
{
	if (m_pkMobInst != NULL)
		m_pkMobInst->m_IsBerserk = mode;
}

bool CHARACTER::IsGodSpeed() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsGodSpeed;
	}
	else
	{
		return false;
	}
}

void CHARACTER::SetGodSpeed(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsGodSpeed = mode;

		if (mode == true)
		{
			SetPoint(POINT_ATT_SPEED, 250);
		}
		else
		{
			SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.sAttackSpeed);
		}
	}
}

bool CHARACTER::IsDeathBlow() const
{
	if (number(1, 100) <= m_pkMobData->m_table.bDeathBlowPoint)
	{
		return true;
	}
	else
	{
		return false;
	}
}

struct FFindReviver
{
	FFindReviver()
	{
		pChar = NULL;
		HasReviver = false;
	}

	void operator() (LPCHARACTER ch)
	{
		if (ch->IsMonster() != true)
		{
			return;
		}

		if (ch->IsReviver() == true && pChar != ch && ch->IsDead() != true)
		{
			if (number(1, 100) <= ch->GetMobTable().bRevivePoint)
			{
				HasReviver = true;
				pChar = ch;
			}
		}
	}

	LPCHARACTER pChar;
	bool HasReviver;
};

bool CHARACTER::HasReviverInParty() const
{
	LPPARTY party = GetParty();

	if (party != NULL)
	{
		if (party->GetMemberCount() == 1) return false;

		FFindReviver f;
		party->ForEachMemberPtr(f);
		return f.HasReviver;
	}

	return false;
}

bool CHARACTER::IsRevive() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsRevive;
	}

	return false;
}

void CHARACTER::SetRevive(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsRevive = mode;
	}
}

#define IS_SPEED_HACK_PLAYER(ch) (ch->m_speed_hack_count > SPEEDHACK_LIMIT_COUNT)

EVENTFUNC(check_speedhack_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "check_speedhack_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;

	if (IS_SPEED_HACK_PLAYER(ch))
	{
		// write hack log
		LogManager::instance().SpeedHackLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), ch->m_speed_hack_count);

		if (g_bEnableSpeedHackCrash)
		{
			// close connection
			LPDESC desc = ch->GetDesc();

			if (desc)
			{
				DESC_MANAGER::instance().DestroyDesc(desc);
				return 0;
			}
		}
	}

	ch->m_speed_hack_count = 0;

	ch->ResetComboHackCount();
	return PASSES_PER_SEC(60);
}

void CHARACTER::StartCheckSpeedHackEvent()
{
	if (m_pkCheckSpeedHackEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkCheckSpeedHackEvent = event_create(check_speedhack_event, info, PASSES_PER_SEC(60));	// 1분
}

void CHARACTER::GoHome()
{
	WarpSet(EMPIRE_START_X(GetEmpire()), EMPIRE_START_Y(GetEmpire()));
}

void CHARACTER::SendGuildName(CGuild* pGuild)
{
	if (NULL == pGuild) return;

	DESC	*desc = GetDesc();

	if (NULL == desc) return;
	if (m_known_guild.find(pGuild->GetID()) != m_known_guild.end()) return;

	m_known_guild.insert(pGuild->GetID());

	TPacketGCGuildName	pack;
	memset(&pack, 0x00, sizeof(pack));

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_GUILD_NAME;
	pack.size		= sizeof(TPacketGCGuildName);
	pack.guildID	= pGuild->GetID();
	memcpy(pack.guildName, pGuild->GetName(), GUILD_NAME_MAX_LEN);

	desc->Packet(&pack, sizeof(pack));
}

void CHARACTER::SendGuildName(DWORD dwGuildID)
{
	SendGuildName(CGuildManager::instance().FindGuild(dwGuildID));
}

EVENTFUNC(destroy_when_idle_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "destroy_when_idle_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (ch->GetVictim())
	{
		return PASSES_PER_SEC(300);
	}

	sys_log(1, "DESTROY_WHEN_IDLE: %s", ch->GetName());

	ch->m_pkDestroyWhenIdleEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::StartDestroyWhenIdleEvent()
{
	if (m_pkDestroyWhenIdleEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkDestroyWhenIdleEvent = event_create(destroy_when_idle_event, info, PASSES_PER_SEC(300));
}

void CHARACTER::SetComboSequence(BYTE seq)
{
	m_bComboSequence = seq;
}

BYTE CHARACTER::GetComboSequence() const
{
	return m_bComboSequence;
}

void CHARACTER::SetLastComboTime(DWORD time)
{
	m_dwLastComboTime = time;
}

DWORD CHARACTER::GetLastComboTime() const
{
	return m_dwLastComboTime;
}

void CHARACTER::SetValidComboInterval(int interval)
{
	m_iValidComboInterval = interval;
}

int CHARACTER::GetValidComboInterval() const
{
	return m_iValidComboInterval;
}

BYTE CHARACTER::GetComboIndex() const
{
	return m_bComboIndex;
}

void CHARACTER::IncreaseComboHackCount(int k)
{
	m_iComboHackCount += k;

	if (m_iComboHackCount >= 10)
	{
		if (GetDesc())
			if (GetDesc()->DelayedDisconnect(number(2, 7)))
			{
				sys_log(0, "COMBO_HACK_DISCONNECT: %s count: %d", GetName(), m_iComboHackCount);
				LogManager::instance().HackLog("Combo", this);
			}
	}
}

void CHARACTER::ResetComboHackCount()
{
	m_iComboHackCount = 0;
}

void CHARACTER::SkipComboAttackByTime(int interval)
{
	m_dwSkipComboAttackByTime = get_dword_time() + interval;
}

DWORD CHARACTER::GetSkipComboAttackByTime() const
{
	return m_dwSkipComboAttackByTime;
}

void CHARACTER::ResetChatCounter()
{
	m_bChatCounter = 0;
}

BYTE CHARACTER::IncreaseChatCounter()
{
	return ++m_bChatCounter;
}

BYTE CHARACTER::GetChatCounter() const
{
	return m_bChatCounter;
}
#if defined(WJ_COMBAT_ZONE)
void CHARACTER::UpdateCombatZoneRankings(const char* memberName, DWORD memberEmpire, DWORD memberPoints)
{
	DBManager::instance().DirectQuery("INSERT INTO player.combat_zone_ranking_weekly (memberName, memberEmpire, memberPoints) VALUES('%s', '%d', '%d') ON DUPLICATE KEY UPDATE memberPoints = memberPoints + '%d'", memberName, memberEmpire, memberPoints, memberPoints);
	DBManager::instance().DirectQuery("INSERT INTO player.combat_zone_ranking_general (memberName, memberEmpire, memberPoints) VALUES('%s', '%d', '%d') ON DUPLICATE KEY UPDATE memberPoints = memberPoints + '%d'", memberName, memberEmpire, memberPoints, memberPoints);
}

BYTE CHARACTER::GetCombatZoneRank()
{
	if (GetDesc() != NULL)
	{
		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT combat_zone_rank FROM player.player WHERE id = %u", GetPlayerID()));
		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		return atoi(row[0]);
	}
	
	return 0;
}

DWORD CHARACTER::GetRealCombatZonePoints()
{
	return CCombatZoneManager::Instance().GetValueByType(this, COMBAT_ZONE_GET_POINTS);	
}

void CHARACTER::SetRealCombatZonePoints(DWORD iValue)
{
	DBManager::instance().Query("UPDATE player.player SET combat_zone_points = %d WHERE id = %u", iValue, GetPlayerID());
}
#endif
// 말이나 다른것을 타고 있나?
bool CHARACTER::IsRiding() const
{
	return IsHorseRiding() || GetMountVnum();
}

bool CHARACTER::CanWarp() const
{
	const int iPulse = thecore_pulse();
	const int limit_time = PASSES_PER_SEC(g_nPortalLimitTime);

	if ((iPulse - GetSafeboxLoadTime()) < limit_time)
		return false;

	if ((iPulse - GetExchangeTime()) < limit_time)
		return false;

	if ((iPulse - GetMyShopTime()) < limit_time)
		return false;

	if ((iPulse - GetRefineTime()) < limit_time)
		return false;

	if (GetExchange() || GetShop() || IsOpenSafebox() || IsCubeOpen()
#if defined(__BL_67_ATTR__)
			|| Is67AttrOpen()
#endif
		)
		return false;

	return true;
}

DWORD CHARACTER::GetNextExp() const
{
	if (PLAYER_MAX_LEVEL_CONST < GetLevel())
		return 2500000000u;
	else
		return exp_table[GetLevel()];
}

#ifdef ENABLE_CONQUEROR_LEVEL
DWORD CHARACTER::GetConquerorNextExp() const
{
	if (PLAYER_MAX_LEVEL_CONST < GetConquerorLevel())
		return 2500000000u;
	else
		return exp_coqueror_table[GetConquerorLevel()];
}

#endif

#ifdef ENABLE_SUPPORT_SYSTEM
DWORD CHARACTER::SupportGetNextExp() const
{
	return 0;
}
#endif


EVENTFUNC(anticheat_check_routine)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("anticheat_check_routine <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER Player = info->ch;

	if (!Player)
		return 0;

	LPDESC d = Player->GetDesc();

	if (!d)
		return 0;

	if (d->IsPhase(PHASE_HANDSHAKE) || d->IsPhase(PHASE_LOGIN) || d->IsPhase(PHASE_SELECT) || d->IsPhase(PHASE_DEAD) || d->IsPhase(PHASE_LOADING))
		return PASSES_PER_SEC(1);

	sys_log(2, "ANTICHEAT_CHECK: %s", Player->GetName());

	LPSECTREE CurrentTree = Player->GetSectree();
	if (!CurrentTree)
		return PASSES_PER_SEC(1);

	CAnticheatManager::Instance().SendCheckPacket(Player);

	return PASSES_PER_SEC(60);
}

void CHARACTER::StartAnticheatCommunication()
{
	if (m_pkAnticheatEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkAnticheatEvent = event_create(anticheat_check_routine, info, PASSES_PER_SEC(60));
}


int	CHARACTER::GetSkillPowerByLevel(int level, bool bMob) const
{
	return CTableBySkill::instance().GetSkillPowerByLevelFromType(GetJob(), GetSkillGroup(), MINMAX(0, level, SKILL_MAX_LEVEL), bMob);
}
#ifdef __NEWPET_SYSTEM__
DWORD CHARACTER::PetGetNextExp() const
{
	if (IsNewPet()) {
		if (120 < GetLevel())
			return 2500000000;
		else
			return exppet_table[GetLevel()];
	}
}
#endif

#ifdef __SASH_SYSTEM__
void CHARACTER::OpenSash(bool bCombination)
{
	if (isSashOpened(bCombination))
	{
		ChatPacket(CHAT_TYPE_INFO, "76");
		return;
	}
	
	if (bCombination)
	{
		if (m_bSashAbsorption)
		{
			ChatPacket(CHAT_TYPE_INFO, "77");
			return;
		}
		
		m_bSashCombination = true;
	}
	else
	{
		if (m_bSashCombination)
		{
			ChatPacket(CHAT_TYPE_INFO, "78");
			return;
		}
		
		m_bSashAbsorption = true;
	}
	
	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;
	
	TPacketSash sPacket;
	sPacket.header = HEADER_GC_SASH;
	sPacket.subheader = SASH_SUBHEADER_GC_OPEN;
	sPacket.bWindow = bCombination;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketSash));
	
	ClearSashMaterials();
}

void CHARACTER::CloseSash()
{
	if ((!m_bSashCombination) && (!m_bSashAbsorption))
		return;
	
	bool bWindow = (m_bSashCombination == true ? true : false);
	
	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;
	
	TPacketSash sPacket;
	sPacket.header = HEADER_GC_SASH;
	sPacket.subheader = SASH_SUBHEADER_GC_CLOSE;
	sPacket.bWindow = bWindow;
	sPacket.dwPrice = 0;
	sPacket.bPos = 0;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketSash));
	
	if (bWindow)
		m_bSashCombination = false;
	else
		m_bSashAbsorption = false;
	
	ClearSashMaterials();
}

void CHARACTER::ClearSashMaterials()
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	for (int i = 0; i < SASH_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			continue;
		
		pkItemMaterial[i]->Lock(false);
		pkItemMaterial[i] = NULL;
	}
}

bool CHARACTER::SashIsSameGrade(long lGrade)
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	if (!pkItemMaterial[0])
		return false;
	
	bool bReturn = (pkItemMaterial[0]->GetValue(SASH_GRADE_VALUE_FIELD) == lGrade ? true : false);
	return bReturn;
}

DWORD CHARACTER::GetSashCombinePrice(long lGrade)
{
	DWORD dwPrice = 0;
	switch (lGrade)
	{
		case 2:
			{
				dwPrice = SASH_GRADE_2_PRICE;
			}
			break;
		case 3:
			{
				dwPrice = SASH_GRADE_3_PRICE;
			}
			break;
		case 4:
			{
				dwPrice = SASH_GRADE_4_PRICE;
			}
			break;
		default:
			{
				dwPrice = SASH_GRADE_1_PRICE;
			}
			break;
	}
	
	return dwPrice;
}

BYTE CHARACTER::CheckEmptyMaterialSlot()
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	for (int i = 0; i < SASH_WINDOW_MAX_MATERIALS; ++i)
	{
		if (!pkItemMaterial[i])
			return i;
	}
	
	return 255;
}

void CHARACTER::GetSashCombineResult(DWORD & dwItemVnum, DWORD & dwMinAbs, DWORD & dwMaxAbs)
{
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	
	if (m_bSashCombination)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			long lVal = pkItemMaterial[0]->GetValue(SASH_GRADE_VALUE_FIELD);
			if (lVal == 4)
			{
				dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
				dwMinAbs = pkItemMaterial[0]->GetSocket(SASH_ABSORPTION_SOCKET);
				DWORD dwMaxAbsCalc = (dwMinAbs + SASH_GRADE_4_ABS_RANGE > SASH_GRADE_4_ABS_MAX ? SASH_GRADE_4_ABS_MAX : (dwMinAbs + SASH_GRADE_4_ABS_RANGE));
				dwMaxAbs = dwMaxAbsCalc;
			}
			else
			{
				DWORD dwMaskVnum = pkItemMaterial[0]->GetOriginalVnum();
				TItemTable * pTable = ITEM_MANAGER::instance().GetTable(dwMaskVnum + 1);
				if (pTable)
					dwMaskVnum += 1;
				
				dwItemVnum = dwMaskVnum;
				switch (lVal)
				{
					case 2:
						{
							dwMinAbs = SASH_GRADE_3_ABS;
							dwMaxAbs = SASH_GRADE_3_ABS;
						}
						break;
					case 3:
						{
							dwMinAbs = SASH_GRADE_4_ABS_MIN;
							dwMaxAbs = SASH_GRADE_4_ABS_MAX_COMB;
						}
						break;
					default:
						{
							dwMinAbs = SASH_GRADE_2_ABS;
							dwMaxAbs = SASH_GRADE_2_ABS;
						}
						break;
				}
			}
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
	else
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			dwItemVnum = pkItemMaterial[0]->GetOriginalVnum();
			dwMinAbs = pkItemMaterial[0]->GetSocket(SASH_ABSORPTION_SOCKET);
			dwMaxAbs = dwMinAbs;
		}
		else
		{
			dwItemVnum = 0;
			dwMinAbs = 0;
			dwMaxAbs = 0;
		}
	}
}

void CHARACTER::AddSashMaterial(TItemPos tPos, BYTE bPos)
{
	if (bPos >= SASH_WINDOW_MAX_MATERIALS)
	{
		if (bPos == 255)
		{
			bPos = CheckEmptyMaterialSlot();
			if (bPos >= SASH_WINDOW_MAX_MATERIALS)
				return;
		}
		else
			return;
	}
	
	LPITEM pkItem = GetItem(tPos);
	if (!pkItem)
		return;
	else if ((pkItem->GetCell() >= INVENTORY_MAX_NUM) || (pkItem->IsEquipped()) || (tPos.IsBeltInventoryPosition()) || (pkItem->IsDragonSoul()))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME) && (m_bSashCombination))
		return;
	else if ((pkItem->GetType() != ITEM_COSTUME) && (bPos == 0) && (m_bSashAbsorption))
		return;
	else if (pkItem->isLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, "79");
		return;
	}
	#ifdef __SOULBINDING_SYSTEM__
	else if ((pkItem->IsBind()) || (pkItem->IsUntilBind()))
	{
		ChatPacket(CHAT_TYPE_INFO, "80");
		return;
	}
	#endif
	else if ((m_bSashCombination) && (bPos == 1) && (!SashIsSameGrade(pkItem->GetValue(SASH_GRADE_VALUE_FIELD))))
	{
		ChatPacket(CHAT_TYPE_INFO, "81");
		return;
	}
	else if ((m_bSashCombination) && (pkItem->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_GRADE_4_ABS_MAX))
	{
		ChatPacket(CHAT_TYPE_INFO, "82");
		return;
	}
	else if ((bPos == 1) && (m_bSashAbsorption))
	{
		if ((pkItem->GetType() != ITEM_WEAPON) && (pkItem->GetType() != ITEM_ARMOR))
		{
			ChatPacket(CHAT_TYPE_INFO, "83");
			return;
		}
		else if ((pkItem->GetType() == ITEM_ARMOR) && (pkItem->GetSubType() != ARMOR_BODY))
		{
			ChatPacket(CHAT_TYPE_INFO, "83");
			return;
		}
	}
	else if ((pkItem->GetSubType() != COSTUME_SASH) && (m_bSashCombination))
		return;
	else if ((pkItem->GetSubType() != COSTUME_SASH) && (bPos == 0) && (m_bSashAbsorption))
		return;
	else if ((pkItem->GetSocket(SASH_ABSORBED_SOCKET) > 0) && (bPos == 0) && (m_bSashAbsorption))
		return;
	
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	if ((bPos == 1) && (!pkItemMaterial[0]))
		return;
	
	if (pkItemMaterial[bPos])
		return;
	
	pkItemMaterial[bPos] = pkItem;
	pkItemMaterial[bPos]->Lock(true);
	
	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetSashCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);
	
	TPacketSash sPacket;
	sPacket.header = HEADER_GC_SASH;
	sPacket.subheader = SASH_SUBHEADER_GC_ADDED;
	sPacket.bWindow = m_bSashCombination == true ? true : false;
	sPacket.dwPrice = GetSashCombinePrice(pkItem->GetValue(SASH_GRADE_VALUE_FIELD));
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = dwItemVnum;
	sPacket.dwMinAbs = dwMinAbs;
	sPacket.dwMaxAbs = dwMaxAbs;
	GetDesc()->Packet(&sPacket, sizeof(TPacketSash));
}

void CHARACTER::RemoveSashMaterial(BYTE bPos)
{
	if (bPos >= SASH_WINDOW_MAX_MATERIALS)
		return;
	
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	
	DWORD dwPrice = 0;
	
	if (bPos == 1)
	{
		if (pkItemMaterial[bPos])
		{
			pkItemMaterial[bPos]->Lock(false);
			pkItemMaterial[bPos] = NULL;
		}
		
		if (pkItemMaterial[0])
			dwPrice = GetSashCombinePrice(pkItemMaterial[0]->GetValue(SASH_GRADE_VALUE_FIELD));
	}
	else
		ClearSashMaterials();
	
	TItemPos tPos;
	tPos.window_type = INVENTORY;
	tPos.cell = 0;
	
	TPacketSash sPacket;
	sPacket.header = HEADER_GC_SASH;
	sPacket.subheader = SASH_SUBHEADER_GC_REMOVED;
	sPacket.bWindow = m_bSashCombination == true ? true : false;
	sPacket.dwPrice = dwPrice;
	sPacket.bPos = bPos;
	sPacket.tPos = tPos;
	sPacket.dwItemVnum = 0;
	sPacket.dwMinAbs = 0;
	sPacket.dwMaxAbs = 0;
	GetDesc()->Packet(&sPacket, sizeof(TPacketSash));
}

BYTE CHARACTER::CanRefineSashMaterials()
{
	BYTE bReturn = 0;
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	if (m_bSashCombination)
	{
		for (int i = 0; i < SASH_WINDOW_MAX_MATERIALS; ++i)
		{
			if (pkItemMaterial[i])
			{
				if ((pkItemMaterial[i]->GetType() == ITEM_COSTUME) && (pkItemMaterial[i]->GetSubType() == COSTUME_SASH))
					bReturn = 1;
				else
				{
					bReturn = 0;
					break;
				}
			}
			else
			{
				bReturn = 0;
				break;
			}
		}
	}
	else if (m_bSashAbsorption)
	{
		if ((pkItemMaterial[0]) && (pkItemMaterial[1]))
		{
			if ((pkItemMaterial[0]->GetType() == ITEM_COSTUME) && (pkItemMaterial[0]->GetSubType() == COSTUME_SASH))
				bReturn = 2;
			else
				bReturn = 0;
			
			if ((pkItemMaterial[1]->GetType() == ITEM_WEAPON) || ((pkItemMaterial[1]->GetType() == ITEM_ARMOR) && (pkItemMaterial[1]->GetSubType() == ARMOR_BODY)))
				bReturn = 2;
			else
				bReturn = 0;
			
			if (pkItemMaterial[0]->GetSocket(SASH_ABSORBED_SOCKET) > 0)
				bReturn = 0;
		}
		else
			bReturn = 0;
	}
	
	return bReturn;
}

void CHARACTER::RefineSashMaterials()
{
	BYTE bCan = CanRefineSashMaterials();
	if (bCan == 0)
		return;
	
	LPITEM * pkItemMaterial;
	pkItemMaterial = GetSashMaterials();
	
	DWORD dwItemVnum, dwMinAbs, dwMaxAbs;
	GetSashCombineResult(dwItemVnum, dwMinAbs, dwMaxAbs);
	long long dwPrice = GetSashCombinePrice(pkItemMaterial[0]->GetValue(SASH_GRADE_VALUE_FIELD));
	
	if (bCan == 1)
	{
		int iSuccessChance = 0;
		long lVal = pkItemMaterial[0]->GetValue(SASH_GRADE_VALUE_FIELD);
		switch (lVal)
		{
			case 2:
				{
					iSuccessChance = SASH_COMBINE_GRADE_2;
				}
				break;
			case 3:
				{
					iSuccessChance = SASH_COMBINE_GRADE_3;
				}
				break;
			case 4:
				{
					iSuccessChance = SASH_COMBINE_GRADE_4;
				}
				break;
			default:
				{
					iSuccessChance = SASH_COMBINE_GRADE_1;
				}
				break;
		}
		
		if (GetGold() < dwPrice)
		{
			ChatPacket(CHAT_TYPE_INFO, "84");
			return;
		}
		
		int iChance = number(1, 100);
		bool bSucces = (iChance <= iSuccessChance ? true : false);
		if (bSucces)
		{
			LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(dwItemVnum, 1, 0, false);
			if (!pkItem)
			{
				sys_err("%d can't be created.", dwItemVnum);
				return;
			}
			
			ITEM_MANAGER::CopyAllAttrTo(pkItemMaterial[0], pkItem);
			LogManager::instance().ItemLog(this, pkItem, "COMBINE SUCCESS", pkItem->GetName());
			DWORD dwAbs = (dwMinAbs == dwMaxAbs ? dwMinAbs : number(dwMinAbs + 1, dwMaxAbs));
			pkItem->SetSocket(SASH_ABSORPTION_SOCKET, dwAbs);
			pkItem->SetSocket(SASH_ABSORBED_SOCKET, pkItemMaterial[0]->GetSocket(SASH_ABSORBED_SOCKET));
			
			PointChange(POINT_GOLD, -dwPrice);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, pkItemMaterial[0]->GetVnum(), -dwPrice);
			
			WORD wCell = pkItemMaterial[0]->GetCell();
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[0], "COMBINE (REFINE SUCCESS)");
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "COMBINE (REFINE SUCCESS)");
			
			pkItem->AddToCharacter(this, TItemPos(INVENTORY, wCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
			pkItem->AttrLog();
			
			if (lVal == 4)
				ChatPacket(CHAT_TYPE_INFO, "85 %d%", dwAbs);
			else
				ChatPacket(CHAT_TYPE_INFO, "86");
			
			EffectPacket(SE_EFFECT_SASH_SUCCEDED);
			LogManager::instance().SashLog(GetPlayerID(), GetX(), GetY(), dwItemVnum, pkItem->GetID(), 1, dwAbs, 1);
			
			ClearSashMaterials();
		}
		else
		{
			PointChange(POINT_GOLD, -dwPrice);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, pkItemMaterial[0]->GetVnum(), -dwPrice);
			
			ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "COMBINE (REFINE FAIL)");
			
			if (lVal == 4)
				ChatPacket(CHAT_TYPE_INFO, "85 %d%", pkItemMaterial[0]->GetSocket(SASH_ABSORPTION_SOCKET));
			else
				ChatPacket(CHAT_TYPE_INFO, "87");
			
			LogManager::instance().SashLog(GetPlayerID(), GetX(), GetY(), dwItemVnum, 0, 0, 0, 0);
			
			pkItemMaterial[1] = NULL;
		}
		
#ifdef ENABLE_BATTLE_PASS
		CHARACTER_MANAGER::Instance().DoMission(this, MISSION_SASH, 1, 0);
#endif
		
		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;
		
		TPacketSash sPacket;
		sPacket.header = HEADER_GC_SASH;
		sPacket.subheader = SASH_SUBHEADER_CG_REFINED;
		sPacket.bWindow = m_bSashCombination == true ? true : false;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 0;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		if (bSucces)
			sPacket.dwMaxAbs = 100;
		else
			sPacket.dwMaxAbs = 0;
		
		GetDesc()->Packet(&sPacket, sizeof(TPacketSash));
	}
	else
	{
		pkItemMaterial[1]->CopyAttributeTo(pkItemMaterial[0]);

#ifdef ELEMENT_SPELL_WORLDARD
		pkItemMaterial[0]->SetElement(pkItemMaterial[1]->GetElementGrade(),pkItemMaterial[1]->GetElementAttacks(),pkItemMaterial[1]->GetElementsType(),pkItemMaterial[1]->GetElementsValues());
#endif	

		LogManager::instance().ItemLog(this, pkItemMaterial[0], "ABSORB (REFINE SUCCESS)", pkItemMaterial[0]->GetName());
		pkItemMaterial[0]->SetSocket(SASH_ABSORBED_SOCKET, pkItemMaterial[1]->GetOriginalVnum());
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (pkItemMaterial[0]->GetAttributeValue(i) < 0)
				pkItemMaterial[0]->SetForceAttribute(i, pkItemMaterial[0]->GetAttributeType(i), 0);
		}
		
		ITEM_MANAGER::instance().RemoveItem(pkItemMaterial[1], "ABSORBED (REFINE SUCCESS)");
		
		ITEM_MANAGER::instance().FlushDelayedSave(pkItemMaterial[0]);
		pkItemMaterial[0]->AttrLog();
		
		ChatPacket(CHAT_TYPE_INFO, "86");
		
		ClearSashMaterials();
		
		TItemPos tPos;
		tPos.window_type = INVENTORY;
		tPos.cell = 0;
		
		TPacketSash sPacket;
		sPacket.header = HEADER_GC_SASH;
		sPacket.subheader = SASH_SUBHEADER_CG_REFINED;
		sPacket.bWindow = m_bSashCombination == true ? true : false;
		sPacket.dwPrice = dwPrice;
		sPacket.bPos = 255;
		sPacket.tPos = tPos;
		sPacket.dwItemVnum = 0;
		sPacket.dwMinAbs = 0;
		sPacket.dwMaxAbs = 1;
		GetDesc()->Packet(&sPacket, sizeof(TPacketSash));
	}
}

bool CHARACTER::CleanSashAttr(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;
	else if ((!pkItem) || (!pkTarget))
		return false;
	else if ((pkTarget->GetType() != ITEM_COSTUME) && (pkTarget->GetSubType() != COSTUME_SASH))
		return false;
	
	if (pkTarget->GetSocket(SASH_ABSORBED_SOCKET) <= 0)
		return false;
	
	pkTarget->SetSocket(SASH_ABSORBED_SOCKET, 0);
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		pkTarget->SetForceAttribute(i, 0, 0);
	
	pkItem->SetCount(pkItem->GetCount() - 1);
	LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT (CLEAN ATTR)", pkTarget->GetName());
	return true;
}
#endif

#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
bool CHARACTER::CheckItemCanGet(const LPITEM item)
{
	const BYTE itemType = item->GetType();
	const BYTE itemSubType = item->GetSubType();
	if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_YANG) && item->GetVnum() == 1)
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_WEAPON) && (itemType == ITEM_WEAPON && itemSubType != WEAPON_ARROW))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_ARMOR) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_BODY))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_SHIELD) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_SHIELD))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_RINGS) && itemType == ITEM_NEW_RINGS)
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_HELMETS) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_HEAD))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_BRACELETS) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_WRIST))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_NECKLACE) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_NECK))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_EARRINGS) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_EAR))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_SHOES) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_FOOTS))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_COSTUMES) && (itemType == ITEM_COSTUME && !(itemSubType == COSTUME_MOUNT_SKIN || itemSubType == COSTUME_PET || itemSubType == COSTUME_MOUNT || itemSubType == COSTUME_NEW_SASH || itemSubType == COSTUME_SASH)))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_PET_MOUNT) && (itemType == ITEM_COSTUME && (itemSubType == COSTUME_MOUNT_SKIN || itemSubType == COSTUME_PET || itemSubType == COSTUME_MOUNT)))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_SASH) && (itemType == ITEM_COSTUME && (itemSubType == COSTUME_NEW_SASH || itemSubType == COSTUME_SASH)))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_TALISMAN) && (itemType == ITEM_ARMOR && itemSubType == ARMOR_PENDANT))
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_BOOKS) && itemType == ITEM_SKILLBOOK)
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_STONES) && itemType == ITEM_METIN)
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_CHESTS) && itemType == ITEM_METIN)
		return false;
	else if(IS_SET(dwPickUPMode, AUTOMATIC_PICK_UP_BELTS) && itemType == ITEM_BELT)
		return false;
	if(item->IsRarityItem())
	{
		if(IS_SET(dwRarityMode, (1 << item->GetSocket(RARITY_TYPE_INDEX))))
			return false;
	}
	return true;
}
void CHARACTER::ChangePickUPMode(DWORD dwMode, DWORD dwRMode)
{
	dwPickUPMode = dwMode;
	dwRarityMode = dwRMode;
	ChatPacket(CHAT_TYPE_COMMAND, "PickUPMode %u %u", dwPickUPMode, dwRarityMode);
}
#endif

#ifdef ENABLE_HIDE_COSTUME_SYSTEM

int CHARACTER::GetSlotsCostume(){
	int slot_default = 1;

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_ACCE
	slot_default += 1;
#endif
#ifdef ENABLE_HIDE_COSTUME_SYSTEM_WEAPON_COSTUME
	slot_default += 1;
#endif

	slot_default += 1;

	slot_default += 2;

	return slot_default;
}

int CHARACTER::GetSlotDefault(int slot){
	int slot_amor = 0;
	int slot_hair = 1;
	int slot_weapon = 2;
	int slot_acce = 3;
	int slot_skin_acce = 4;
	int slot_shining_weapon = 5;
	int slot_shining_armor = 6;

	if(slot == 0){return slot_amor;}
	if(slot == 1){return slot_hair;}
	if(slot == 2){return slot_weapon;}
	if(slot == 3){return slot_acce;}
	if(slot == 4){return slot_skin_acce;}
	if(slot == 5){return slot_shining_weapon;}
	if(slot == 6){return slot_shining_armor;}

	return 0;
}

void CHARACTER::FuncHideCostume(int slot)
{

	if (slot <0 || slot > GetSlotsCostume())
		return;

	// 0 = show, 1 = hide
	int part_value = 0;
	int part_acce = 0;
	bool bAdd = false;

	char costume_hide[1024];	
	snprintf(costume_hide, sizeof(costume_hide), "hide_costume_w.hide_costume_%d", slot);

	const CItem* pArmor;
	const CItem * pWeapon;
	LPITEM pkItem;


	if(slot == GetSlotDefault(0))
	{
		pkItem = GetWear(WEAR_COSTUME_BODY);
		if(!pkItem)
			return;

		if (GetQuestFlag(costume_hide) == 0) 
		{
			SetQuestFlag(costume_hide,1);
			pArmor = GetWear(WEAR_BODY);
		}
		else{
			SetQuestFlag(costume_hide,0);
			pArmor = GetWear(WEAR_COSTUME_BODY);
		}

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_TRANSMUTACION
		if (NULL != pArmor){
			if (pArmor->GetTransmutation() > 0)
			{
				part_value = pArmor->GetTransmutation();
			}else{
				part_value = pArmor->GetVnum();
			}
		}else{
			part_value = GetOriginalPart(PART_MAIN);
		}
#else
		part_value = (NULL != pArmor) ? pArmor->GetVnum() : GetOriginalPart(PART_MAIN);
#endif

		SetPart(PART_MAIN, part_value);
	}

	else if(slot == GetSlotDefault(1))
	{

		pkItem = GetWear(WEAR_COSTUME_HAIR);
		if(!pkItem)
			return;

		if (GetQuestFlag(costume_hide) == 0) 
		{
			SetQuestFlag(costume_hide,1);
			bAdd = false;
		}
		else{
			SetQuestFlag(costume_hide,0);
			bAdd = true;
		}

		LPITEM pkItem = GetWear(WEAR_COSTUME_HAIR);
		if (pkItem){

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_TRANSMUTACION
			if (pkItem->GetTransmutation() > 0){
				TItemTable * pHair = ITEM_MANAGER::instance().GetTable(pkItem->GetTransmutation());
				if (!pHair)
				{
					part_value = (true == bAdd) ? pkItem->GetValue(3) : 0;
				}
				else
				{
					part_value = (true == bAdd) ? pHair->alValues[3] : 0;
				}
			}else{
				part_value = (true == bAdd) ? pkItem->GetValue(3) : 0;
			}
#else
			part_value = (true == bAdd) ? pkItem->GetValue(3) : 0;
#endif
		}
		else{
			part_value = 0;
		}

		SetPart(PART_HAIR, part_value);
	}

	//Acce Slot 2
#ifdef ENABLE_HIDE_COSTUME_SYSTEM_ACCE
	else if(slot == GetSlotDefault(3))
	{

		pkItem = GetWear(WEAR_COSTUME_SASH);
		if(!pkItem)
			return;

		if (GetQuestFlag(costume_hide) == 0) 
		{
			SetQuestFlag(costume_hide,1);
			bAdd = false;
		}
		else{
			SetQuestFlag(costume_hide,0);
			bAdd = true;
		}

		if (pkItem){

			LPITEM itemSash = GetWear(WEAR_COSTUME_NEW_SASH);
			if(NULL != itemSash  && GetHideCostume(GetSlotDefault(4)) == 0)
			{
				part_value = itemSash->GetVnum() - 85000;
				//if (itemSash->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
				//	part_value += 1000;
			}else{
				if(bAdd)
				{
					part_value = pkItem->GetVnum();			
					part_value -= 85000;

#ifndef FIX_19_PERCENT
					if (pkItem->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
					{
						part_value += 1000;
					}
#endif

				}else{
					part_value = 0;
				}
			}
		}

		SetPart(PART_SASH, part_value);
	}
#endif

#ifdef	ENABLE_HIDE_COSTUME_SYSTEM_WEAPON_COSTUME
	//Weapon Slot 3
	else if(slot == GetSlotDefault(2))
	{

		pkItem = GetWear(WEAR_COSTUME_WEAPON);
		if(!pkItem)
			return;
		
		if (GetQuestFlag(costume_hide) == 0) 
		{
			SetQuestFlag(costume_hide,1);
			pWeapon = GetWear(WEAR_WEAPON);
		}
		else{
			SetQuestFlag(costume_hide,0);
			pWeapon = GetWear(WEAR_COSTUME_WEAPON);
		}

		if (NULL != pWeapon){

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_TRANSMUTACION
			if (pWeapon->GetTransmutation() > 0)
			{
				part_value = pWeapon->GetTransmutation();
			}else{
				part_value = pWeapon->GetVnum();
			}
#else
			part_value = pWeapon->GetVnum();
#endif
		}else{
			part_value = GetOriginalPart(PART_WEAPON);
		}

		SetPart(PART_WEAPON, part_value);
	}
#endif

	else if(slot == GetSlotDefault(4))
	{

		pkItem = GetWear(WEAR_COSTUME_NEW_SASH);
		if(!pkItem)
			return;

		if (GetQuestFlag(costume_hide) == 0) 
		{
			SetQuestFlag(costume_hide,1);
			bAdd = false;
		}
		else{
			SetQuestFlag(costume_hide,0);
			bAdd = true;
		}

		if (pkItem){

			if(!bAdd)
			{
				LPITEM itemSash = GetWear(WEAR_COSTUME_SASH);
				if(NULL != itemSash && GetHideCostume(GetSlotDefault(3)) == 0 )
				{
					part_value = itemSash->GetVnum() - 85000;
					if (itemSash->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
						part_value += 1000;
				}else{
					part_value = 0;
				}
			}else{
				part_value = pkItem->GetVnum();			
				part_value -= 85000;
			}
		}

		SetPart(PART_SASH, part_value);
	}

	else if(slot == GetSlotDefault(5) || slot == GetSlotDefault(6))
	{
		pkItem = GetWear(WEAR_SHINING_WEAPON_1);
		if(!pkItem)
			return;

		if (GetQuestFlag(costume_hide) == 0) 
		{
			SetQuestFlag(costume_hide,1);
		}
		else{
			SetQuestFlag(costume_hide,0);
		}
	}

	BroadcastTargetPacket();
	FuncHideCostumeLoad();
	UpdatePacket();
	
}

void CHARACTER::FuncHideCostumeLoad()
{
	char costume_hide[1024];
	ChatPacket(CHAT_TYPE_COMMAND,"COSTUME_HIDE_CLEAR");

	for (int i = 0; i < GetSlotsCostume()+1; ++i)
	{
		snprintf(costume_hide, sizeof(costume_hide), "hide_costume_w.hide_costume_%d", i);
		ChatPacket(CHAT_TYPE_COMMAND,"COSTUME_HIDE_LIST %d %d",i,GetQuestFlag(costume_hide));
	}
	ChatPacket(CHAT_TYPE_COMMAND,"COSTUME_HIDE_LOAD");
}


DWORD CHARACTER::SetHideCostumeUpdateShining(DWORD id)
{
	CItem* shining_weapon = GetWear(WEAR_SHINING_WEAPON_1);
	CItem* shining_armor  = GetWear(WEAR_SHINING_ARMOR_1);
	if(id == 0)
	{
		if(shining_weapon)
		{
			if (GetHideCostume(GetSlotDefault(5)) == 1){
				return 0;
			}else{
				return shining_weapon->GetVnum();
			}
		}else{
			return 0;
		}

	}else{
		if(shining_armor)
		{
			if (GetHideCostume(GetSlotDefault(6)) == 1){
				return 0;
			}else{
				return shining_armor->GetVnum();
			}
		}else{
			return 0;
		}
	}

	return 0;
}

void CHARACTER::SetHideCostumeUpdate()
{
	CItem* body_costume 	= GetWear(WEAR_COSTUME_BODY);

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_ACCE
	CItem* acce_costume 	= GetWear(WEAR_COSTUME_SASH);
#endif
	CItem* hair_costume 	= GetWear(WEAR_COSTUME_HAIR);

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_WEAPON_COSTUME
	CItem* weapon_costume 	= GetWear(WEAR_COSTUME_WEAPON);
#endif

	CItem* skin_acce_costume 	= GetWear(WEAR_COSTUME_NEW_SASH);

	if (GetHideCostume(GetSlotDefault(0)) == 1){
		CItem* body_armor = GetWear(WEAR_BODY);

		if (body_costume){
			if (body_armor){

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_TRANSMUTACION
				if (body_armor->GetTransmutation() > 0){
					SetPart(PART_MAIN, body_armor->GetTransmutation());
				}else{
					SetPart(PART_MAIN, body_armor->GetVnum());
				}
#else
				SetPart(PART_MAIN, body_armor->GetVnum());
#endif
			}
			else
			{
				SetPart(PART_MAIN, GetOriginalPart(PART_MAIN));
			}
		}
	}

	if (GetHideCostume(GetSlotDefault(1)) == 1){
		if (hair_costume){
			SetPart(PART_HAIR, 0);
		}
	}

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_ACCE
	if (GetHideCostume(GetSlotDefault(3)) == 1){
		if (acce_costume){
			if(skin_acce_costume && GetHideCostume(GetSlotDefault(4)) == 0)
			{
				DWORD vnum_skin =  skin_acce_costume->GetVnum()-85000;
				//if (skin_acce_costume->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
				//	vnum_skin += 1000;
				SetPart(PART_SASH, vnum_skin);
			}else{
				SetPart(PART_SASH, 0);
			}
			
		}
	}
#endif

#ifdef ENABLE_HIDE_COSTUME_SYSTEM_WEAPON_COSTUME
	if (GetHideCostume(GetSlotDefault(2)) == 1){
		CItem* body_weapon = GetWear(WEAR_WEAPON);
		if (weapon_costume){
			if (body_weapon){
#ifdef ENABLE_HIDE_COSTUME_SYSTEM_TRANSMUTACION
				if (body_weapon->GetTransmutation() > 0){
					SetPart(PART_WEAPON, body_weapon->GetTransmutation());
				}else{
					SetPart(PART_WEAPON, body_weapon->GetVnum());
				}
#else
				SetPart(PART_WEAPON, body_weapon->GetVnum());
#endif
			}
			else
			{
				SetPart(PART_WEAPON, GetOriginalPart(PART_WEAPON));
			}
		}
	}
#endif


	if (GetHideCostume(GetSlotDefault(4)) == 1){

		if (skin_acce_costume){
			if(acce_costume && GetHideCostume(GetSlotDefault(3)) == 0)
			{
				DWORD vnum_acce =  acce_costume->GetVnum()-85000;
				if (acce_costume->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
					vnum_acce += 1000;
				SetPart(PART_SASH, vnum_acce);
			}else{
				SetPart(PART_SASH, 0);
			}
			
		}
	}


}

int CHARACTER::GetHideCostume(int wear) const
{
	char costume_hide[1024];
	snprintf(costume_hide, sizeof(costume_hide), "hide_costume_w.hide_costume_%d", wear);
	return GetQuestFlag(costume_hide);
}
#endif

#ifdef ENABLE_VOTE_COINS_WHILE_PLAYING
const int VOTE_COINS_WHILE_PLAYING_TIME = 60;

EVENTINFO(SVoteCoinsWhilePlaying)
{
	DWORD dwID;

	SVoteCoinsWhilePlaying() : dwID(0)
	{
	}
};

EVENTFUNC(VoteCoinsWhilePlayingEvent)
{
	SVoteCoinsWhilePlaying* info = dynamic_cast<SVoteCoinsWhilePlaying*>(event->info);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->dwID);

	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "88");

		int iAccountID = ch->GetDesc()->GetAccountTable().id;

		// Update coins.
		DBManager::instance().Query("UPDATE account.account SET drs = drs + 100 WHERE id = %d", ch->GetDesc()->GetAccountTable().id);
		
		// Send the coins to the client.
		std::auto_ptr<SQLMsg>pMsg (DBManager::instance().DirectQuery("SELECT coins, drs FROM account.account WHERE id = %d", iAccountID));
		if (pMsg->Get()->uiNumRows > 0)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "ITEMSHOP COINS/%d/%d", atoi(row[0]), atoi(row[1]));
		}

		ch->SetVoteCoinsEventLastTime();

		int iLastTime = ch->GetQuestFlag("vote_coins_event.last_time_new");
		DWORD convert_time = (iLastTime-ch->GetRealPoint(POINT_PLAYTIME))*60;

		return PASSES_PER_SEC(convert_time);
	}

	return 0;
}

void CHARACTER::StartVoteCoinsEvent()
{
	if (m_pkVoteCoinsWhilePlaying)
		return;

	SVoteCoinsWhilePlaying* pEventInfo = AllocEventInfo<SVoteCoinsWhilePlaying>();
	pEventInfo->dwID = GetPlayerID();

	int iTime = VOTE_COINS_WHILE_PLAYING_TIME;
	int iLastTime = GetQuestFlag("vote_coins_event.last_time_new");

	if (iLastTime <= 0)
	{
		SetVoteCoinsEventLastTime();
		iLastTime = GetQuestFlag("vote_coins_event.last_time_new");
	}
	else if(GetRealPoint(POINT_PLAYTIME) >= iLastTime)
	{
		iLastTime = 5;
	}

	DWORD convert_time = (iLastTime-GetRealPoint(POINT_PLAYTIME))*60;

	m_pkVoteCoinsWhilePlaying = event_create(VoteCoinsWhilePlayingEvent, pEventInfo, PASSES_PER_SEC(convert_time));
	
}

void CHARACTER::SetVoteCoinsEventLastTime()
{
	SetQuestFlag("vote_coins_event.last_time_new", GetRealPoint(POINT_PLAYTIME) + VOTE_COINS_WHILE_PLAYING_TIME);
}
#endif

#ifdef ELEMENT_SPELL_WORLDARD	
BYTE CHARACTER::GetElementsEffect()
{
	LPITEM w_weapon = GetWear(WEAR_WEAPON);
	if (w_weapon)
	{
		if(w_weapon->GetElementGrade() == 3)
		{
			return w_weapon->GetElementsType();
		}
	}
	return 0;
}
#endif

#if defined(WJ_COMBAT_ZONE)
EVENTFUNC(combat_zone_notice_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("combat_zone_notice_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;

	if(CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()) && CCombatZoneManager::instance().IsRunning())
	{
		ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "PARA GUARDAR LOS PUNTOS OBTENIDOS DEBES ABANDONAR LA TOTAL WAR ANTES DE QUE ACABE!");
		return PASSES_PER_SEC(60*10);
	}

	return 0;
}

void CHARACTER::StartNoticeMapCombatZone()
{
	if (m_pkNoticeCombatZoneEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkNoticeCombatZoneEvent = event_create(combat_zone_notice_event, info, PASSES_PER_SEC(60*10));

}
#endif

#ifdef RENEWAL_DEAD_PACKET
#include "threeway_war.h"
DWORD CHARACTER::CalculateDeadTime(BYTE type)
{
	//generated from do_restart(cmd_general.cpp)
	if (!m_pkDeadEvent)
		return 0;
	int iTimeToDead = (event_time(m_pkDeadEvent) / passes_per_sec);

	if (type == REVIVE_TYPE_AUTO_TOWN)
		return iTimeToDead-7;
	if (!test_server && type == REVIVE_TYPE_HERE && (!GetWarMap() || GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG)) {
		if (IsHack(false) && !CThreeWayWar::instance().IsSungZiMapIndex(GetMapIndex()))
			return iTimeToDead - (180 - g_nPortalLimitTime);
		if (iTimeToDead > 170)
			return iTimeToDead - 170;
	}
	if (IsHack(false) && ((!GetWarMap() || GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG) || !CThreeWayWar::instance().IsSungZiMapIndex(GetMapIndex())))
		return iTimeToDead - (180 - g_nPortalLimitTime);
	if (iTimeToDead > 173) 
		return iTimeToDead - 173;
	return 0;
}
#endif

#ifdef BATTLEPASS_WORLDARD

BYTE CHARACTER::GetBattlePassId()
{
	CAffect* pAffect = FindAffect(AFFECT_BATTLE_PASS, POINT_BATTLE_PASS_ID);

	if (!pAffect)
		return 0;

	return pAffect->lApplyValue;
}

int CHARACTER::GetSecondsTillNextMonth()
{
	time_t iTime;
	time(&iTime);
	struct tm endTime = *localtime(&iTime);

	int iCurrentMonth = endTime.tm_mon;

	endTime.tm_hour = 0; 
	endTime.tm_min = 0; 
	endTime.tm_sec = 0;
	endTime.tm_mday = 1;

	if (iCurrentMonth == 11)
	{
		endTime.tm_mon = 0;
		endTime.tm_year = endTime.tm_year + 1;
	}
	else
	{
		endTime.tm_mon = iCurrentMonth + 1;
	}

	int seconds = difftime(mktime(&endTime), iTime);
	
	return seconds;
}

#endif

#ifdef ENABLE_CONQUEROR_LEVEL
bool CHARACTER::IsConquerorMap(int iMapIndex)
{
	if (iMapIndex == EASTPLAIN_01)
		return true;
	else if(iMapIndex == EASTPLAIN_02)
		return true;
	else if(iMapIndex == EASTPLAIN_03)
		return true;
	else if(iMapIndex == EMPIRECASTLE)
		return true;		
	return false;
}
#endif

#ifdef ENABLE_OFFLINESHOP_SYSTEM
void CHARACTER::SetProtectTime(const std::string& flagname, int value)
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		it->second = value;
	else
		m_protection_Time.insert(make_pair(flagname, value));
}
int CHARACTER::GetProtectTime(const std::string& flagname) const
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		return it->second;
	return 0;
}


bool CHARACTER::CheckWindows(bool bChat)
{
	if (GetExchange() || GetShop() || GetSafebox() || GetOfflineShop() || GetOfflineShopPanel())
	{
		if(bChat)
			ChatPacket(CHAT_TYPE_INFO, "89");
		return true;
	}
	return false;
}

bool CHARACTER::CanAddItemShop()
{
	time_t now = get_global_time();
	if (GetProtectTime("offlineshop.additem") > get_global_time())
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.additem") - now);
		return true;
	}
	SetProtectTime("offlineshop.additem", now + 1);

	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		sys_log(0, "COfflineShopManager::AddItem: adding item with timed event xD %s:%d", GetName(), GetPlayerID());
		return true;
	}
	else if (!GetOfflineShop())
	{
		sys_log(0, "COfflineShopManager::AddItem: adding item without panel <?> %s:%d", GetName(), GetPlayerID());
		return true;
	}
	return false;
}

bool CHARACTER::CanDestroyShop()
{
	time_t now = get_global_time();
	if (GetProtectTime("offlineshop.destroy") > get_global_time())
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.destroy") - now);
		return true;
	}
	
	SetProtectTime("offlineshop.destroy", now + 3);
	if (!GetOfflineShop())
		return true;
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	return false;
}

bool CHARACTER::CanRemoveItemShop()
{
	time_t now = get_global_time();
	if (GetProtectTime("offlineshop.remove") > get_global_time())
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.remove")-now);
		return true;
	}
	SetProtectTime("offlineshop.remove", now + 1);
	if (!GetOfflineShop())
		return true;
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	return false;
}


bool CHARACTER::CanCreateShop()
{
	if (!GetOfflineShopPanel())
		return true;
	SetOfflineShopPanel(false);
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.create") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.create")-time);
		return true;
	}
	SetProtectTime("offlineshop.create", time + 10);
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	if (GetGold() < 2000000)
	{
		ChatPacket(CHAT_TYPE_INFO, "92 %d",2000000);
		return true;
	}
	else if (GetDungeon())
	{
		ChatPacket(CHAT_TYPE_INFO, "93");
		return true;
	}
	else if (COfflineShopManager::Instance().HasOfflineShop(this) == true)
	{
		ChatPacket(CHAT_TYPE_INFO, "94");
		return true;
	}
	
	switch(GetMapIndex())
	{
		//offlineshop can create map indexes!
		case 90:
		case 91:
			return false;
		default:
			ChatPacket(CHAT_TYPE_INFO, "93");
			return true;
	}
	return true;
}

bool CHARACTER::CanRemoveLogShop()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.log") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.log") - time);
		return true;
	}
	SetProtectTime("offlineshop.log", time + 30);

	return false;
}

bool CHARACTER::CanWithdrawMoney()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.money") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.money") - time);
		return true;
	}
	SetProtectTime("offlineshop.money", time + 5);
	return false;
}

bool CHARACTER::CanOpenShopPanel()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.panel") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.panel") - time);
		return true;
	}
	SetProtectTime("offlineshop.panel", time + 5);
	if (CheckWindows(true))
		return true;
	return false;
}

bool CHARACTER::CanOpenOfflineShop()
{
	if (m_pkTimedEvent)
		return true;
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.offlineshop") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.offlineshop") - time);
		return true;
	}
	SetProtectTime("offlineshop.offlineshop", time + 1);
	if (CheckWindows(true))
		return true;
	return false;
}

bool CHARACTER::CanChangeTitle()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.title") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.title") - time);
		return true;
	}
	SetProtectTime("offlineshop.title", time + 10);
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	if (!FindAffect(AFFECT_DECORATION))
	{
		ChatPacket(CHAT_TYPE_INFO, "95");
		return true;
	}
		
	return false;
}

bool CHARACTER::CanBuyItemOfflineShop()
{
	//time_t time = get_global_time();
	//if (GetProtectTime("offlineshop.buy") > time)
	//{
	//	ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.buy") - time);
	//	return true;
	//}
	//SetProtectTime("offlineshop.buy", time + 1);
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	return false;
}
bool CHARACTER::CanChangeDecoration()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.decoration") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.decoration") - time);
		return true;
	}
	SetProtectTime("offlineshop.decoration", time + 10);
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	if (!FindAffect(AFFECT_DECORATION))
	{
		ChatPacket(CHAT_TYPE_INFO, "95");
		return true;
	}
	return false;
}

bool CHARACTER::CanGetBackItems()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.back") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.back") - time);
		return true;
	}
	SetProtectTime("offlineshop.back", time + 5);
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	return false;
}

bool CHARACTER::CanAddTimeShop()
{
	time_t time = get_global_time();
	if (GetProtectTime("offlineshop.time") > time)
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.time") - time);
		return true;
	}
	SetProtectTime("offlineshop.time", time + 10);
	if (m_pkTimedEvent)
	{
		ChatPacket(CHAT_TYPE_INFO, "91");
		event_cancel(&m_pkTimedEvent);
		return true;
	}
	if (GetGold() < 20000000)
	{
		ChatPacket(CHAT_TYPE_INFO, "92 %d",2000000);
		return true;
	}
		
	return false;
}

#endif

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
bool CHARACTER::CanSearch()
{
	time_t now = get_global_time();
	if (GetProtectTime("offlineshop.search") > get_global_time())
	{
		ChatPacket(CHAT_TYPE_INFO, "90 %d", GetProtectTime("offlineshop.search")-now);
		return true;
	}
	SetProtectTime("offlineshop.search", now + 1);
	return false;
}
#endif


#ifdef ENABLE_NEW_PET_SYSTEM
bool CHARACTER::PetBlockMap()
{
	if (IsObserverMode())
		return true;
	switch (GetMapIndex())
	{
		case 999:
			return true;
	}
	return false;
}
/*
void CHARACTER::SetProtectTime(const std::string& flagname, int value)
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		it->second = value;
	else
		m_protection_Time.insert(make_pair(flagname, value));
}
int CHARACTER::GetProtectTime(const std::string& flagname) const
{
	itertype(m_protection_Time) it = m_protection_Time.find(flagname);
	if (it != m_protection_Time.end())
		return it->second;
	return 0;
}
*/
#endif

#if defined(__BL_67_ATTR__)
void CHARACTER::Open67Attr()
{
	LPDESC d = GetDesc();
	if (!d)
		return;
	
	if (GetExchange() || IsOpenSafebox() || GetShop() || IsCubeOpen())
	{
		ChatPacket(CHAT_TYPE_INFO, "89");
		return;
	}

	Set67Attr(true);

	TPacket67AttrOpenClose p;
	p.bHeader = HEADER_GC_OPEN_67_ATTR;
	d->Packet(&p, sizeof(p));
}
#endif

#ifdef ENABLE_BIYOLOG
void CHARACTER::CheckBio()
{
	int level = GetQuestFlag("bio.level");
	if (level == 0)
	{
		level += 1;
		SetQuestFlag("bio.level", level);
	}
	else if (level >= bio_max)
		return;

	int count = GetQuestFlag("bio.count");
	if (count >= bio_data[level][2])
	{
		if (count == bio_data[level][2])
		{
			if (bio_data[level][5] != 0)
				ChatPacket(CHAT_TYPE_COMMAND, "biostone %d", level);
			else
				if (bio_data[level][14] == 1)
					ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
		}
		else
			if (bio_data[level][14] == 1)
				ChatPacket(CHAT_TYPE_COMMAND, "bioodul %d", level);
	}
	else
	{
		ChatPacket(CHAT_TYPE_COMMAND, "biodata %d %d %d", level, count, GetQuestFlag("bio.time"));
	}
}
#endif


#ifdef ENABLE_CHANNEL_CHANGE
void CHARACTER::MoveChannel(DWORD channelIndex)
{
	if(GetMapIndex() >= 10000)//dungeon check.
	{
		ChatPacket(CHAT_TYPE_INFO, "999");
		return;
	}

	int oldPort = mother_port-(g_bChannel > 1 ? (100*(g_bChannel-1)):0);
	int newPort = oldPort+(100*(channelIndex-1));
	long lAddr;
	long lMapIndex;
	WORD wPort;
	long x = this->GetX();
	long y = this->GetY();
	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
		return;
	Stop();
	Save();
	if(GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}
	
	ChatPacket(CHAT_TYPE_COMMAND, "UpdateServerInfo %d", channelIndex);
	TPacketGCWarp p;
	p.bHeader = HEADER_GC_WARP;
	p.lX = x;
	p.lY = y;
	p.lAddr = lAddr;
	p.wPort = newPort;
	GetDesc()->Packet(&p, sizeof(TPacketGCWarp));
}
#endif

#ifdef ENABLE_BATTLE_PASS
int CHARACTER::GetBattlePassStatus()
{
	time_t cur_Time = time(NULL);
	struct tm vKey = *localtime(&cur_Time);
	if (GetQuestFlag("battlePass.monthIndex") != (vKey.tm_mon + 1))
		return 0;
	return GetQuestFlag("battlePass.status");
}
bool CHARACTER::IsBattlePassActive()
{
	return GetBattlePassStatus() == 1;
}
void CHARACTER::ClearBattlePassData()
{
	m_battlePassData.clear();
}
void CHARACTER::SetBattlePassMissonValue(BYTE missionIndex, long long missionValue)
{
	auto it = m_battlePassData.find(missionIndex);
	if (it != m_battlePassData.end())
		it->second = missionValue;
	else
		m_battlePassData.emplace(missionIndex, missionValue);
}
long long CHARACTER::GetBattlePassMissonValue(BYTE missionIndex)
{
	auto it = m_battlePassData.find(missionIndex);
	if (it != m_battlePassData.end())
		return it->second;
	return 0;
}
#endif


#ifdef ENABLE_ITEMSHOP
long long CHARACTER::GetDragonCoin()
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT coins FROM account.account WHERE id = '%d';", GetDesc()->GetAccountTable().id));
	if (pMsg->Get()->uiNumRows == 0)
		return 0;
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	long long dc = 0;
	str_to_number(dc, row[0]);
	return dc;
}
void CHARACTER::SetDragonCoin(long long amount)
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("UPDATE account.account SET coins = '%lld' WHERE id = '%d';", amount, GetDesc()->GetAccountTable().id));
}
#endif

#ifdef ENABLE_DEFENSAWE_SHIP
bool CHARACTER::IsHydraMob() const
{
	switch(GetRaceNum())
	{
		case 6802:
		case 6803:
		case 6804:
		case 6805:
		case 6806:
		case 6807:
		case 6808:
		case 6809:
		case 6812:
			return true;
	}
	return false;
}
bool CHARACTER::IsHydraMobLP(LPCHARACTER ch) const
{
	switch(ch->GetRaceNum())
	{
		case 6802:
		case 6803:
		case 6804:
		case 6805:
		case 6806:
		case 6807:
		case 6808:
		case 6809:
		case 6812:
			return true;
	}
	return false;
}
bool CHARACTER::IsHydraNPC() const
{
	switch(GetRaceNum())
	{
		case 6801:
			return true;
	}
	return false;
}
bool CHARACTER::IsHydra() const
{
	switch(GetRaceNum())
	{
		case 6812:
			return true;
	}
	return false;
}
#endif

#ifdef ENABLE_RARITY
void CHARACTER::SetRarityValue(long decreaseValue, bool setWeapon, bool setArmor)
{
	std::vector<WORD> m_WearIndex;
	if(setWeapon)
		m_WearIndex.emplace_back(WEAR_WEAPON);
	if(setArmor)
	{
		m_WearIndex.emplace_back(WEAR_BODY);
		m_WearIndex.emplace_back(WEAR_HEAD);
		m_WearIndex.emplace_back(WEAR_FOOTS);
		m_WearIndex.emplace_back(WEAR_WRIST);
		m_WearIndex.emplace_back(WEAR_NECK);
		m_WearIndex.emplace_back(WEAR_EAR);
		m_WearIndex.emplace_back(WEAR_SHIELD);
		m_WearIndex.emplace_back(WEAR_RINGS_1);
		m_WearIndex.emplace_back(WEAR_RINGS_2);
	}
	for (WORD j = 0; j < m_WearIndex.size(); ++j)
	{
		LPITEM pItem = GetWear(m_WearIndex[j]);
		if (pItem)
		{
			if (pItem->IsRarityItem())
				pItem->SetRarityValue(this, decreaseValue);
		}
	}
}
#endif


#ifdef ENABLE_NEW_DETAILS_GUI
void CHARACTER::SendKillLog()
{
	if(!GetDesc())
		return;
	TPacketGCKillLOG p;
	p.header = HEADER_GC_KILL_LOG;
	thecore_memcpy(p.kill_log, m_points.kill_log,sizeof(p.kill_log));
	GetDesc()->Packet(&p,sizeof(TPacketGCKillLOG));
}
#endif

#ifdef ENABLE_CSHIELD
EVENTFUNC(cshield_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("cshield_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;

	DWORD cShield = ch->GetCShield()->VerifyCShield(ch->GetName());

	if (cShield > 1)
	{
		LPDESC d = ch->GetDesc();
		if (d)
		{
			if (d->DelayedDisconnect(3))
			{
				if (cShield == 2)
					LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: CShield Patched", ch);
				else
				{
					LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: CShield Async", ch);
				}
				
#ifdef ENABLE_AUTOBAN
				std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT login FROM account.account WHERE id='%d' LIMIT 1", ch->GetAID()));
				if (pMsg->Get()->uiNumRows == 0)
					return 0;
				MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
				
				std::unique_ptr<SQLMsg> pMsg2(DBManager::instance().DirectQuery("SELECT count(*) FROM log.hack_log WHERE (why = 'CShield Async' OR why = 'CShield Patched') AND login = '%s' AND time BETWEEN DATE_ADD(NOW(), INTERVAL -20 MINUTE) AND NOW()", row[0]));
				if (pMsg2->Get()->uiNumRows == 0)
					return 0;

				MYSQL_ROW row2 = mysql_fetch_row(pMsg2->Get()->pSQLResult);
				if (atoi(row2[0]) >= 5)
				{
					DBManager::instance().DirectQuery("UPDATE account.account set status='BLOCK' where id='%d'", ch->GetAID());
					LogManager::instance().HackLog("CShield-ServerSide-ErrorCode: CShield Autoban", ch);
				}
#endif
			}
		}
	}

	return PASSES_PER_SEC(20);
}

EVENTFUNC(cshield_data_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("cshield_data_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;

	if (!ch->GetCShield()->GetCShieldInit())
	{
		ch->GetCShield()->SetCShieldTime();
		ch->SendCShieldPacket(true);
		ch->GetCShield()->SetCShieldIinit();
	}
	else
		ch->SendCShieldPacket(false);

	return PASSES_PER_SEC(3);
}

void CHARACTER::StartCShieldEvent()
{
	if (m_pkCShieldEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkCShieldEvent = event_create(cshield_event, info, PASSES_PER_SEC(15));
}

void CHARACTER::StartCShieldDataEvent()
{
	if (m_pkCShieldDataEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkCShieldDataEvent = event_create(cshield_data_event, info, PASSES_PER_SEC(7));
}

void CHARACTER::SendCShieldPacket(bool start)
{
	LPDESC d = GetDesc();
	if (!d)
		return;

	SPacketGCCShield packet;
	packet.header = HEADER_GC_CSHIELD;
	packet.start = start;
	packet.num = GetCShield()->GetCShieldNumber();
	d->Packet(&packet, sizeof(SPacketGCCShield));
}
#endif
void CHARACTER::StopAttacksOnLogout()
{
	//ChatPacket(CHAT_TYPE_INFO, "WTF1");
    for (auto it : m_map_kDamage)
    {
        auto vid = it.first;
        CHARACTER * mob = CHARACTER_MANAGER::instance().Find(vid);
		//ChatPacket(CHAT_TYPE_INFO, "The mob vid is %s, name" , GetVID(), GetName()); 
        if (mob)
        {
            mob->SetVictim(NULL); 
        }

    }
}

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
bool CHARACTER::IsLookingSearchItem(DWORD itemID)
{
	return (std::find(m_vecSearchLooking.begin(), m_vecSearchLooking.end(), itemID) != m_vecSearchLooking.end());
}
void CHARACTER::SetLookingSearch(const std::vector<const OFFLINE_SHOP_ITEM*>& dataVector)
{
	m_vecSearchLooking.clear();
	for (WORD j = 0; j < dataVector.size(); ++j)
		m_vecSearchLooking.emplace_back(dataVector[j]->id);
}
#endif

#ifdef ENABLE_ZODIAC_MISSION
void CHARACTER::BeadCount(LPCHARACTER ch)
{
	int bCount = ch->GetQuestFlag("Bead.Count");
	ch->ChatPacket(CHAT_TYPE_COMMAND, "SetBeadCount %d",bCount);
}
#endif

#ifdef ENABLE_MELEY_LAIR_DUNGEON
bool CHARACTER::IsMeley()
{
	switch(GetRaceNum())
	{
		case 6231:
		case 9563:
		case 6230:
		case 6119:
			return true;
			break;
	}
	return false;
}
LPCHARACTER CHARACTER::GetQuestNPCAttack() const
{
	return CHARACTER_MANAGER::instance().Find(m_dwQuestNPCVIDAttack);
}
#endif


#ifdef ENABLE_TRACK_WINDOW
void CHARACTER::GetDungeonCooldownTest(WORD bossIndex, int value, bool isCooldown)
{
	//HERE SET DUNGEONS FLAGS
	//BossVnum - (cooldownFlag.backDungeonFlag)
	const std::map<WORD, std::string> m_vecQuestCooldowns = {
		{1093,isCooldown ? "deviltower_zone.cooldown" : "deviltower_zone.time"},
		{2598,isCooldown ? "devilcatacomb_zone.cooldown" : "devilcatacomb_zone.time"},
		{2493,isCooldown ? "dragonlair.cooldown" : "dragonlair.time"},
		{6091,isCooldown ? "flame_dungeon.cooldown" : "flame_dungeon.time"},
		{9018,isCooldown ? "ShipDefense.cooldown" : "ShipDefense.time"},
		{6191,isCooldown ? "snow_dungeon.cooldown" : "snow_dungeon.time"},
		{20442,isCooldown ? "MeleyDungeon.cooldown" : "MeleyDungeon.time"},
		{2092,isCooldown ? "SpiderDungeon.cooldown" : "SpiderDungeon.time"},
	};
	const auto it = m_vecQuestCooldowns.find(bossIndex);
	if (it != m_vecQuestCooldowns.end())
	{
		SetQuestFlag(it->second.c_str(), time(0) + value);
		GetDungeonCooldown(bossIndex);
	}
}
void CHARACTER::GetDungeonCooldown(WORD bossIndex)
{
	//HERE SET DUNGEONS FLAGS
	//BossVnum - (cooldownFlag.backDungeonFlag)
	const std::map<WORD, std::pair<std::string, std::string>> m_vecQuestCooldowns = {
		{1093,{"deviltower_zone.cooldown", "deviltower_zone.time"}},
		{2598,{"devilcatacomb_zone.cooldown", "devilcatacomb_zone.time"}},
		{2493,{"dragonlair.cooldown", "dragonlair.time"}},
		{6091,{"flame_dungeon.cooldown", "flame_dungeon.time"}},
		{9018,{"ShipDefense.cooldown", "ShipDefense.time"}},
		{6191,{"snow_dungeon.cooldown", "snow_dungeon.time"}},
		{20442,{"MeleyDungeon.cooldown", "MeleyDungeon.time"}},
		{2092,{"SpiderDungeon.cooldown", "SpiderDungeon.time"}},
	};
	std::string cmdText("");
	if (bossIndex == WORD_MAX)
	{
#if __cplusplus < 199711L
		for (const auto& [bossVnum, cooldown] : m_vecQuestCooldowns)
		{
			const int leftTime = GetQuestFlag(cooldown.c_str()) - time(0);
#else
		for (auto it = m_vecQuestCooldowns.begin(); it != m_vecQuestCooldowns.end(); ++it)
		{
			const WORD bossVnum = it->first;
			int leftTime = GetQuestFlag(it->second.first.c_str()) - time(0);
#endif
			if (leftTime > 0)
			{
				cmdText += std::to_string(bossVnum);
				cmdText += "|";
				cmdText += std::to_string(leftTime);
				cmdText += "|";
				cmdText += "0";
				cmdText += "#";

				if (cmdText.length() + 50 > CHAT_MAX_LEN)
				{
					ChatPacket(CHAT_TYPE_COMMAND, "TrackDungeonInfo %s", cmdText.c_str());
					cmdText.clear();
				}
				continue;
			}

			leftTime = GetQuestFlag(it->second.second.c_str()) - time(0);
			if (leftTime > 0)
			{
				cmdText += std::to_string(bossVnum);
				cmdText += "|";
				cmdText += std::to_string(leftTime);
				cmdText += "|";
				cmdText += "1";
				cmdText += "#";
				if (cmdText.length() + 50 > CHAT_MAX_LEN)
				{
					ChatPacket(CHAT_TYPE_COMMAND, "TrackDungeonInfo %s", cmdText.c_str());
					cmdText.clear();
				}

				continue;
			}
		}

		
	}
	else
	{
		const auto it = m_vecQuestCooldowns.find(bossIndex);
		if (it != m_vecQuestCooldowns.end())
		{
			int leftTime = GetQuestFlag(it->second.first.c_str()) - time(0);
			if (leftTime > 0)
			{
				cmdText += std::to_string(it->first);
				cmdText += "|";
				cmdText += std::to_string(leftTime);
				cmdText += "|";
				cmdText += "0";
				cmdText += "#";
			}
			leftTime = GetQuestFlag(it->second.second.c_str()) - time(0);
			if (leftTime > 0)
			{
				cmdText += std::to_string(it->first);
				cmdText += "|";
				cmdText += std::to_string(leftTime);
				cmdText += "|";
				cmdText += "1";
				cmdText += "#";
			}
		}
	}
	if (cmdText.length())
		ChatPacket(CHAT_TYPE_COMMAND, "TrackDungeonInfo %s", cmdText.c_str());
}
#endif

#ifdef ENABLE_EXCHANGE_LOG
bool CHARACTER::LoadExchangeLogItem(DWORD logID)
{
	const auto it = m_mapExchangeLog.find(logID);
	if (it != m_mapExchangeLog.end())
	{
		if (!it->second.first.itemsLoaded)
		{
			it->second.second.clear();
			char szQuery[124];
			snprintf(szQuery, sizeof(szQuery), "SELECT * FROM log.exchange_log_items WHERE id = %u AND (pid = %u OR pid = %u)", logID, it->second.first.ownerPID, it->second.first.targetPID);
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
			if (pMsg && pMsg->Get()->uiNumRows != 0)
			{
				std::vector<TExchangeLogItem> logItemVector;
				MYSQL_ROW row;
				while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
				{
					DWORD itemOwnerPID;
					TExchangeLogItem logItem;
					BYTE col = 0;
					col++;//log_id
					str_to_number(itemOwnerPID, row[col++]);
					col++;//item_id
					str_to_number(logItem.pos, row[col++]);
					str_to_number(logItem.vnum, row[col++]);
					str_to_number(logItem.count, row[col++]);
					for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						str_to_number(logItem.alSockets[j], row[col++]);
					for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j) {
						str_to_number(logItem.aAttr[j].bType, row[col++]);
						str_to_number(logItem.aAttr[j].sValue, row[col++]);
					}
					logItem.isOwnerItem = it->second.first.ownerPID == itemOwnerPID;
					it->second.second.emplace_back(logItem);
				}
			}

			const WORD logItemCount = it->second.second.size();
			const BYTE subHeader = SUB_EXCHANGELOG_LOAD_ITEM;

			TPacketGCExchangeLog p;
			p.header = HEADER_GC_EXCHANGE_LOG;
			p.size = sizeof(TPacketGCExchangeLog) + sizeof(BYTE) + sizeof(WORD) + sizeof(DWORD) + (logItemCount*sizeof(TExchangeLogItem));

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(TPacketGCExchangeLog));
			buf.write(&subHeader, sizeof(BYTE));
			buf.write(&logItemCount, sizeof(WORD));
			buf.write(&logID, sizeof(DWORD));
			if(logItemCount)
				buf.write(it->second.second.data(), logItemCount * sizeof(TExchangeLogItem));
			GetDesc()->Packet(buf.read_peek(), buf.size());

			return true;
		}
	}
	return false;
}
void CHARACTER::DeleteExchangeLog(DWORD logID)
{
	if (logID == 0)
	{
		for (auto it = m_mapExchangeLog.begin(); it != m_mapExchangeLog.end(); ++it)
		{
			char szQuery[124];
			snprintf(szQuery, sizeof(szQuery), "UPDATE log.exchange_log SET %s = 1 WHERE id = %u", it->second.first.ownerPID == GetPlayerID() ? "owner_delete" : "target_delete", it->first);
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
		}
		m_mapExchangeLog.clear();
	}
	else
	{
		auto it = m_mapExchangeLog.find(logID);
		if (it != m_mapExchangeLog.end())
		{
			char szQuery[124];
			snprintf(szQuery, sizeof(szQuery), "UPDATE log.exchange_log SET %s = 1 WHERE id = %u", it->second.first.ownerPID == GetPlayerID() ? "owner_delete" : "target_delete", logID);
			std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
			m_mapExchangeLog.erase(it);
		}
	}
}
void CHARACTER::LoadExchangeLog()
{
	if (GetProtectTime("ExchangeLogLoaded") == 1)
		return;
	m_mapExchangeLog.clear();

	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT * FROM log.exchange_log WHERE (owner_pid = %d AND owner_delete = 0) OR (target_pid = %d AND target_delete = 0)", GetPlayerID(), GetPlayerID());
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));

	if (pMsg && pMsg->Get()->uiNumRows != 0)
	{
		std::vector<TExchangeLogItem> logItemVector;
		MYSQL_ROW row;
		while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
		{
			TExchangeLog exchangeLog;
			BYTE col = 0;
			DWORD logID;
			str_to_number(logID, row[col++]);
			strlcpy(exchangeLog.owner, row[col++], sizeof(exchangeLog.owner));
			str_to_number(exchangeLog.ownerPID, row[col++]);
			str_to_number(exchangeLog.ownerGold, row[col++]);
			strlcpy(exchangeLog.ownerIP, row[col++], sizeof(exchangeLog.ownerIP));

			strlcpy(exchangeLog.target, row[col++], sizeof(exchangeLog.target));
			str_to_number(exchangeLog.targetPID, row[col++]);
			str_to_number(exchangeLog.targetGold, row[col++]);
			strlcpy(exchangeLog.targetIP, row[col++], sizeof(exchangeLog.targetIP));
			strlcpy(exchangeLog.date, row[col++], sizeof(exchangeLog.date));
			col++;
			col++;
			exchangeLog.itemsLoaded = false;
			m_mapExchangeLog.emplace(logID, std::make_pair(exchangeLog, logItemVector));
		}
	}

	const WORD logCount = m_mapExchangeLog.size();
	const BYTE subHeader = SUB_EXCHANGELOG_LOAD;
	const bool isNeedClean = true;
	char playerCode[19];
	snprintf(playerCode, sizeof(playerCode), "%s", GetDesc()->GetAccountTable().social_id);

	TPacketGCExchangeLog p;
	p.header = HEADER_GC_EXCHANGE_LOG;
	p.size = sizeof(TPacketGCExchangeLog) + sizeof(BYTE) + sizeof(WORD) + (logCount*(sizeof(DWORD) + sizeof(TExchangeLog)));

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(TPacketGCExchangeLog));
	buf.write(&subHeader, sizeof(BYTE));
	buf.write(&playerCode, sizeof(playerCode));
	buf.write(&isNeedClean, sizeof(bool));
	buf.write(&logCount, sizeof(WORD));
	for (auto it = m_mapExchangeLog.begin(); it != m_mapExchangeLog.end(); ++it)
	{
		buf.write(&it->first, sizeof(DWORD));
		buf.write(&it->second.first, sizeof(TExchangeLog));
	}
	GetDesc()->Packet(buf.read_peek(), buf.size());

	SetProtectTime("ExchangeLogLoaded", 1);
}
void CHARACTER::SendExchangeLogPacket(BYTE subHeader, DWORD id, const TExchangeLog* exchangeLog)
{
	if (!GetDesc())
		return;

	if (SUB_EXCHANGELOG_LOAD_ITEM == subHeader)
		LoadExchangeLogItem(id);
	else if (SUB_EXCHANGELOG_LOAD == subHeader)
	{
		if (exchangeLog)
		{
			if (GetProtectTime("ExchangeLogLoaded") != 1)
				return;
			std::vector<TExchangeLogItem> logItemVector;
			TExchangeLog exchangeLogEx;
			thecore_memcpy(&exchangeLogEx, exchangeLog, sizeof(exchangeLogEx));
			m_mapExchangeLog.emplace(id, std::make_pair(exchangeLogEx, logItemVector));

			const WORD logCount = 1;
			const bool isNeedClean = false;
			char playerCode[19];
			snprintf(playerCode, sizeof(playerCode),"%s", GetDesc()->GetAccountTable().social_id);

			TPacketGCExchangeLog p;
			p.header = HEADER_GC_EXCHANGE_LOG;
			p.size = sizeof(TPacketGCExchangeLog) + sizeof(BYTE) + sizeof(WORD) + (logCount*(sizeof(DWORD) + sizeof(TExchangeLog)));

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(TPacketGCExchangeLog));
			buf.write(&subHeader, sizeof(BYTE));
			buf.write(&playerCode, sizeof(playerCode));
			buf.write(&isNeedClean, sizeof(bool));
			buf.write(&logCount, sizeof(WORD));
			buf.write(&id, sizeof(DWORD));
			buf.write(&exchangeLogEx, sizeof(TExchangeLog));
			GetDesc()->Packet(buf.read_peek(), buf.size());
		}
		else
			LoadExchangeLog();
	}
}
#endif

#ifdef __GEM_SYSTEM__
void CHARACTER::OpenGemSlot()
{
	if (CountSpecifyItem(OPEN_GEM_SLOT_ITEM) < 1)
	{
		ChatPacket(CHAT_TYPE_INFO, "You don't has enought open gem slot item!");
		return;
	}
	const int openSlotFlag = GetQuestFlag("gem.open_slot");
	if (openSlotFlag < 19)
	{
		SetQuestFlag("gem.open_slot", openSlotFlag + 1);
		ChatPacket(CHAT_TYPE_COMMAND, "GemUpdateSlotCount %d", openSlotFlag + 1);
		RemoveSpecifyItem(OPEN_GEM_SLOT_ITEM, 1);
	}
}
void CHARACTER::RefreshGemPlayer()
{
	if (!m_mapGemItems.size())
	{
		LoadGemItems();
		if (!m_mapGemItems.size())
			RefreshGemItems();
	}

	ChatPacket(CHAT_TYPE_COMMAND, "GemClear");
	ChatPacket(CHAT_TYPE_COMMAND, "GemUpdateSlotCount %d", GetQuestFlag("gem.open_slot"));
	ChatPacket(CHAT_TYPE_COMMAND, "GemSetRefreshLeftTime %d", GetQuestFlag("gem.left_time") - time(0));
	std::string cmdString("");
#if __cplusplus < 199711L
	for (itertype(m_mapGemItems) it = m_mapGemItems.begin(); it != m_mapGemItems.end(); ++it)
	{
		const BYTE& slotPos = it->first;
		const TGemItem& itemData = it->second;
#else
	for (const auto& [slotPos, itemData] : m_mapGemItems)
	{
#endif
		cmdString += std::to_string(slotPos).c_str();
		cmdString += "#";
		cmdString += std::to_string(itemData.itemVnum).c_str();
		cmdString += "#";
		cmdString += std::to_string(itemData.itemCount).c_str();
		cmdString += "#";
		cmdString += std::to_string(itemData.itemPrice).c_str();
		cmdString += "#";
		cmdString += std::to_string(itemData.itemBuyed).c_str();
		cmdString += "|";
		if (cmdString.size() + 50 > CHAT_MAX_LEN)
		{
			ChatPacket(CHAT_TYPE_COMMAND, "GemSetItemsWithString %s", cmdString.c_str());
			cmdString.clear();
		}
	}
	if(cmdString.size())
		ChatPacket(CHAT_TYPE_COMMAND, "GemSetItemsWithString %s", cmdString.c_str());
}

void CHARACTER::RefreshGemALL(bool withRealTime)
{
	int leftTime = 0;
	if (withRealTime && GetQuestFlag("gem.left_time") != 0)
	{
		leftTime = time(0) - GetQuestFlag("gem.left_time");
		if (leftTime < 1)
			leftTime = 0;
	}
	SetQuestFlag("gem.left_time", (time(0) + REFRESH_TIME) - leftTime);
	RefreshGemItems();
	RefreshGemPlayer();
}
void CHARACTER::BuyGemItem(BYTE slotIndex)
{
#if __cplusplus < 199711L
	itertype(m_mapGemItems) it = m_mapGemItems.find(slotIndex);
#else
	auto it = m_mapGemItems.find(slotIndex);
#endif
	if (it != m_mapGemItems.end())
	{
		if (slotIndex >= 9)
		{
			if (GetQuestFlag("gem.open_slot") < (slotIndex - 9) + 1)
				return;
		}

		if (it->second.itemBuyed)
			return;
		else if (it->second.itemPrice > GetGem())
			return;
		it->second.itemBuyed = true;
		PointChange(POINT_GEM, -it->second.itemPrice);
		AutoGiveItem(it->second.itemVnum, it->second.itemCount);
		ChatPacket(CHAT_TYPE_COMMAND, "GemSetBuyedSlot %d 1", slotIndex);
		SaveGemItems();
	}
}
void CHARACTER::RefreshGemItems(bool withItem)
{
	m_mapGemItems.clear();

#if __cplusplus < 199711L
	const DWORD gemItems[][3] = {
#else
	constexpr DWORD gemItems[][4] = {
#endif
	//	{item,count,price,lucky}
		{71136,1,100,100},
		{76012,5,1,100},
		{76012,20,5,100},
		{25162,1,2,100},
		{39032,1,1,100},
		{6884,1,250,100},
		{72320,1,10,100},
		{72320,2,20,100},
		{72319,1,20,100},
		{72319,2,40,100},
		{71153,1,100,100},
		{55035,1,5,100},
		{30250,1,25,100},
		{71035,1,25,100},
		{55001,1,150,100},
		{55009,1,55,100},
		{85009,1,250,100},
		{25163,1,3,100},
		{25164,1,5,100},
		{25165,1,7,100},
		{25166,1,10,100},
		{25167,1,12,100},
		{25168,1,12,100},
		{25169,1,15,100},
		{71097,1,50,100},
		{71097,2,100,100},
		{100001,1,5,100},
		{100002,1,10,100},
		{71509,1,150,100},
		{71506,1,150,100},
		{71503,1,150,100},
		{71500,1,150,100},
		{53002,1,150,100},
		{53005,1,150,100},
		{53008,1,150,100},
		{53011,1,150,100},
		{77910,1,10,100},
		{77911,1,35,100},
		{77912,1,70,100},
		{70043,1,20,100},
		{70005,1,20,100},
		{80025,1,10,100},
		{25100,1,1,100},
		{71084,500,10,100},
		{71084,1000,20,100},
		{71084,2000,40,100},
		{71151,1000,5,100},
		{71151,2000,10,100},
		{72346,1,55,100},
		{72346,2,110,100},
		{53996,20,25,100},
		{55036,1,300,100},
		{30402,1,2,100},
		{30403,1,7,100},
		{30404,1,13,100},
		{30405,1,17,100},
		{30406,1,27,100},
		{30407,1,53,100},
		{30408,1,105,100},
		{55401,1,20,100},
		{77927,1,150,100},
		{50348,1,250,100},
		{50349,1,50,100},
	};

	const size_t listLen = _countof(gemItems);
	std::vector<BYTE> m_vecListIndex;
	while (1)
	{
		if (m_vecListIndex.size() >= listLen)
			break;
		if (m_vecListIndex.size() >= 27)
			break;
		for (WORD i = 0; i < listLen; ++i)
		{
			if (number(1, 100) < gemItems[i][3])
				continue;
			if (std::find(m_vecListIndex.begin(), m_vecListIndex.end(), i) != m_vecListIndex.end())
				continue;
			if (m_vecListIndex.size() >= 27)
				break;
#if __cplusplus < 199711L
			m_vecListIndex.push_back(i);
#else
			m_vecListIndex.emplace_back(i);
#endif
		}
	}

#if __cplusplus < 199711L
	for (auto it = m_vecListIndex.begin(); it != m_vecListIndex.end(); ++it)
	{
		const BYTE& listIndex = *it;
#else
	for(const auto& listIndex : m_vecListIndex)
	{
#endif
		TGemItem gemItem;
		gemItem.itemBuyed = false;
		gemItem.itemVnum = gemItems[listIndex][0];
		gemItem.itemCount = gemItems[listIndex][1];
		gemItem.itemPrice = gemItems[listIndex][2];
#if __cplusplus < 199711L
		m_mapGemItems.insert(std::make_pair(m_mapGemItems.size(), gemItem));
#else
		m_mapGemItems.emplace(m_mapGemItems.size(), gemItem);
#endif
	}
	SaveGemItems();
}
void CHARACTER::SaveGemItems()
{
	char filename[45];
	snprintf(filename, sizeof(filename), "locale/germany/gem/%u", GetPlayerID());

	FILE* fp = fopen(filename, "w+");
	if (fp)
	{
		fprintf(fp, "index-vnum-count-price-buyed\n");
#if __cplusplus < 199711L
		for (itertype(m_mapGemItems) it = m_mapGemItems.begin(); it != m_mapGemItems.end(); ++it)
		{
			const BYTE& slotPos = it->first;
			const TGemItem& itemData = it->second;
			fprintf(fp, "%u %u %u %d %u\n", slotPos, itemData.itemVnum, itemData.itemCount, itemData.itemPrice, itemData.itemBuyed);
		}
#else
		for (const auto& [slotPos, itemData] : m_mapGemItems)
			fprintf(fp, "%u %u %u %d %u\n", slotPos, itemData.itemVnum, itemData.itemCount, itemData.itemPrice, itemData.itemBuyed);
#endif
		fclose(fp);
	}

}
void CHARACTER::LoadGemItems()
{
	m_mapGemItems.clear();
	char filename[45];
	snprintf(filename, sizeof(filename), "locale/germany/gem/%u", GetPlayerID());
	FILE* fp;
	if ((fp = fopen(filename, "r")) != NULL)
	{
		char	one_line[256];
		while (fgets(one_line, 256, fp))
		{
			std::vector<std::string> m_vec;
			split_argument(one_line, m_vec);
			if (m_vec.size() != 5)
				continue;
			TGemItem gemItem;
			BYTE slotIndex;
			str_to_number(slotIndex, m_vec[0].c_str());
			str_to_number(gemItem.itemVnum, m_vec[1].c_str());
			str_to_number(gemItem.itemCount, m_vec[2].c_str());
			str_to_number(gemItem.itemPrice, m_vec[3].c_str());
			str_to_number(gemItem.itemBuyed, m_vec[4].c_str());
#if __cplusplus < 199711L
			m_mapGemItems.insert(std::make_pair(slotIndex, gemItem));
#else
			m_mapGemItems.emplace(slotIndex, gemItem);
#endif
		}
		fclose(fp);
	}
}
#endif

#ifdef ENABLE_VOTE4BUFF
long long CHARACTER::GetVoteCoin()
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT votebuff FROM account.account WHERE id = '%d';", GetDesc()->GetAccountTable().id));
	if (pMsg->Get()->uiNumRows == 0)
		return 0;
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	long long coin = 0;
	str_to_number(coin, row[0]);
	return coin;
}
void CHARACTER::SetVoteCoin(long long amount)
{
	std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("UPDATE account.account SET votebuff = '%lld' WHERE id = '%d';", amount, GetDesc()->GetAccountTable().id));
}
#endif

#ifdef RENEWAL_MISSION_BOOKS
void CHARACTER::SetMissionBook(BYTE missionType, BYTE value, DWORD arg, WORD level)
{
#if __cplusplus < 199711L
	for (itertype(m_mapMissionData) it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
#else
	for (auto it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
#endif
	{
#if __cplusplus < 199711L
		const TMissionBookData* missionData = CHARACTER_MANAGER::Instance().GetMissionData(it->first);
#else
		auto missionData = CHARACTER_MANAGER::Instance().GetMissionData(it->first);
#endif
		if (missionData)
		{
			if (missionData->type != missionType)
				continue;
			if (missionData->subtype != 0)
			{
				if (missionData->subtype != arg)
					continue;
			}
			else
			{
				if (!LEVEL_DELTA(level, GetLevel(), missionData->levelRange))
					continue;
			}
			if (it->second.value >= missionData->max || time(0) > it->second.leftTime)
				continue;
			it->second.value += value;
			
			if (it->second.value >= missionData->max)
			{
				it->second.endTime = time(0);
				it->second.value = missionData->max;

				char endTime[34];
				const time_t cur_Time = it->second.endTime;
				const struct tm vKey = *localtime(&cur_Time);
				snprintf(endTime, sizeof(endTime), "%02d:%02d:%02d-%02d/%02d/%02d", vKey.tm_hour, vKey.tm_min, vKey.tm_sec, vKey.tm_mday, vKey.tm_mon, vKey.tm_year + 1900);

				ChatPacket(CHAT_TYPE_COMMAND, "UpdateMissionEndTime %u %s", it->first, endTime);
				
				ChatPacket(CHAT_TYPE_INFO, "Your mission done!");
			}
			if (GetProtectTime("mission_load") == 1)
				ChatPacket(CHAT_TYPE_COMMAND, "UpdateMissionValue %u %lld", it->first, it->second.value);
		}
	}
}
void CHARACTER::RewardMissionBook(WORD missionID)
{
#if __cplusplus < 199711L
	const itertype(m_mapMissionData) it = m_mapMissionData.find(missionID);
#else
	const auto it = m_mapMissionData.find(missionID);
#endif
	if (it != m_mapMissionData.end())
	{
#if __cplusplus < 199711L
		const TMissionBookData* missionData = CHARACTER_MANAGER::Instance().GetMissionData(missionID);
#else
		auto missionData = CHARACTER_MANAGER::Instance().GetMissionData(missionID);
#endif
		if (missionData)
		{
			if(it->second.reward)
			{
				ChatPacket(CHAT_TYPE_INFO, "You already get reward!");
				return;
			}

			if (it->second.value >= missionData->max)
			{
				it->second.reward = true;
				SaveMissionData();
				for (BYTE j = 0; j < 6; ++j)
				{
					if (missionData->rewardItems[j] != 0)
						AutoGiveItem(missionData->rewardItems[j], missionData->rewardCount[j]);
				}
				PointChange(POINT_GOLD, number(missionData->gold[0], missionData->gold[1]));
				PointChange(POINT_EXP, number(missionData->exp[0], missionData->exp[1]));
				ChatPacket(CHAT_TYPE_INFO, "Succesfully get reward!");
				ChatPacket(CHAT_TYPE_COMMAND, "RewardMissionData %u %d", missionID, true);
			}
			else
				ChatPacket(CHAT_TYPE_INFO, "You need first finish mission!");
		}
	}
}
void CHARACTER::DeleteBookMission(WORD missionID)
{
#if __cplusplus < 199711L
	itertype(m_mapMissionData) it = m_mapMissionData.find(missionID);
#else
	auto it = m_mapMissionData.find(missionID);
#endif
	if (it != m_mapMissionData.end())
	{
		m_mapMissionData.erase(it);
		SaveMissionData();
		ChatPacket(CHAT_TYPE_COMMAND, "RemoveMissionData %u", missionID);
		ChatPacket(CHAT_TYPE_INFO, "Mission succesfully removed!");
	}
}
BYTE CHARACTER::MissionCount()
{
	return m_mapMissionData.size();
}
bool CHARACTER::IsMissionHas(WORD missionID)
{
#if __cplusplus < 199711L
	const itertype(m_mapMissionData) it = m_mapMissionData.find(missionID);
#else
	const auto it = m_mapMissionData.find(missionID);
#endif
	if (it != m_mapMissionData.end())
		return true;
	return false;
}
void CHARACTER::GiveNewMission(WORD missionID)
{
#if __cplusplus < 199711L
	const TMissionBookData* missionData = CHARACTER_MANAGER::Instance().GetMissionData(missionID);
#else
	auto missionData = CHARACTER_MANAGER::Instance().GetMissionData(missionID);
#endif
	if (!missionData)
		return;
	TMissionBook misson;
	memset(&misson,0,sizeof(misson));
	misson.leftTime = time(0)+ missionData->maxTime;
#if __cplusplus < 199711L
	m_mapMissionData.insert(std::make_pair(missionID, misson));
#else
	m_mapMissionData.emplace(missionID, misson);
#endif
	SaveMissionData();

	std::string cmdText("");
	ModifySetMissionCMD(missionID, cmdText);
	if (cmdText.length())
		ChatPacket(CHAT_TYPE_COMMAND, "UpdateMissionInfo %s", cmdText.c_str());
}
void CHARACTER::SaveMissionData()
{
	char filename[124];
	snprintf(filename, sizeof(filename), "locale/missionbook/%u", GetPlayerID());
	FILE* fp = fopen(filename, "w+");
	if (fp)
	{
		fprintf(fp, "missionid-value-lefttime-reward\n");
#if __cplusplus < 199711L
		for (itertype(m_mapMissionData) it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
		{
			const BYTE& missionID = it->first;
			const TMissionBook& missionData = it->second;
			fprintf(fp, "%u %lld %d %u %d\n", missionID, missionData.value, missionData.leftTime, missionData.reward, missionData.endTime);
		}
#else
		for (const auto& [missionID, missionData] : m_mapMissionData)
			fprintf(fp, "%u %lld %d %u %d\n", missionID, missionData.value, missionData.leftTime, missionData.reward, missionData.endTime);
#endif
		fclose(fp);
	}
}

void CHARACTER::ModifySetMissionCMD(WORD missionID, std::string& cmdText)
{
#if __cplusplus < 199711L
	const itertype(m_mapMissionData) it = m_mapMissionData.find(missionID);
#else
	const auto it = m_mapMissionData.find(missionID);
#endif
	if (it != m_mapMissionData.end())
	{
#if __cplusplus < 199711L
		const TMissionBookData* missionData = CHARACTER_MANAGER::Instance().GetMissionData(it->first);
#else
		auto missionData = CHARACTER_MANAGER::Instance().GetMissionData(it->first);
#endif
		if (missionData)
		{
			char endTime[34];
			if (it->second.endTime != 0)
			{
				const time_t cur_Time = it->second.endTime;
				const struct tm vKey = *localtime(&cur_Time);
				snprintf(endTime, sizeof(endTime), "%02d:%02d:%02d-%02d/%02d/%02d", vKey.tm_hour, vKey.tm_min, vKey.tm_sec, vKey.tm_mday, vKey.tm_mon, vKey.tm_year + 1900);
			}
			else
				snprintf(endTime, sizeof(endTime), "-");
			cmdText += std::to_string(it->first);
			cmdText += "#";
			cmdText += std::to_string(missionData->missionItemIndex);
			cmdText += "#";
			cmdText += std::to_string(missionData->type);
			cmdText += "#";
			cmdText += std::to_string(missionData->subtype);
			cmdText += "#";
			cmdText += std::to_string(missionData->levelRange);
			cmdText += "#";
			cmdText += std::to_string(it->second.value);
			cmdText += "#";
			cmdText += std::to_string(missionData->max);
			cmdText += "#";
			cmdText += std::to_string(it->second.leftTime-time(0));
			cmdText += "#";
			cmdText += std::to_string(missionData->maxTime);
			cmdText += "#";
			cmdText += endTime;
			cmdText += "#";
			cmdText += std::to_string(missionData->gold[0]);
			cmdText += "#";
			cmdText += std::to_string(missionData->gold[1]);
			cmdText += "#";
			cmdText += std::to_string(missionData->exp[0]);
			cmdText += "#";
			cmdText += std::to_string(missionData->exp[1]);
			cmdText += "#";
			cmdText += std::to_string(it->second.reward);
			cmdText += "#";
			BYTE rewardCount = 0;
			for (BYTE j = 0; j < 6; ++j)
			{
				if ((missionData->rewardItems[j] == 0 && j != 0) || j == 5)
				{
					rewardCount = j + 1;
					break;
				}
			}
			cmdText += std::to_string(rewardCount);
			cmdText += "#";
			for (BYTE j = 0; j < rewardCount; ++j)
			{
				cmdText += std::to_string(missionData->rewardItems[j]);
				cmdText += "#";
				cmdText += std::to_string(missionData->rewardCount[j]);
				cmdText += "#";
			}
		}
		else
		{
			sys_err("Player has %d id mission but not has remove in data!", it->first);
		}
	}
}
void CHARACTER::SendMissionData()
{
	if (GetProtectTime("mission_load") == 1)
		return;
	ChatPacket(CHAT_TYPE_COMMAND, "ClearBookMission");
	std::string cmdText("");
#if __cplusplus < 199711L
	for (itertype(m_mapMissionData) it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
#else
	for (auto it = m_mapMissionData.begin(); it != m_mapMissionData.end(); ++it)
#endif
	{
		ModifySetMissionCMD(it->first, cmdText);
		if (cmdText.length())
		{
			ChatPacket(CHAT_TYPE_COMMAND, "UpdateMissionInfo %s", cmdText.c_str());
			cmdText.clear();
		}
	}
	SetProtectTime("mission_load", 1);
}
void CHARACTER::LoadMissionData()
{
	m_mapMissionData.clear();
	char filename[124];
	snprintf(filename, sizeof(filename), "locale/missionbook/%u", GetPlayerID());
	FILE* fp;
	if ((fp = fopen(filename, "r")) != NULL)
	{
		char	one_line[256];
		while (fgets(one_line, 256, fp))
		{
			std::vector<std::string> m_vec;
			split_argument(one_line, m_vec);
			if (m_vec.size() != 5)
				continue;
			TMissionBook missionItem;
			WORD missionID;
			str_to_number(missionID, m_vec[0].c_str());
			str_to_number(missionItem.value, m_vec[1].c_str());
			str_to_number(missionItem.leftTime, m_vec[2].c_str());
			str_to_number(missionItem.reward, m_vec[3].c_str());
			str_to_number(missionItem.endTime, m_vec[4].c_str());
#if __cplusplus < 199711L
			m_mapMissionData.insert(std::make_pair(missionID, missionItem));
#else
			m_mapMissionData.emplace(missionID, missionItem);
#endif
		}
		fclose(fp);
	}
	
}
#endif


