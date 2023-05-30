/*********************************************************************
* title_name		: Combat Zone (Official Webzen 16.4)
* date_created		: 2017.05.21
* filename			: combat_zone.cpp
* author			: VegaS
* version_actual	: Version 0.1.9
*/

#include "stdafx.h"
#include "desc.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "item.h"
#include "char_manager.h"
#include "affect.h"
#include "start_position.h"
#include "p2p.h"
#include "db.h"
#include "skill.h"
#include "dungeon.h"
#include "castle.h"
#include <string>
#include <boost/algorithm/string/replace.hpp>
#include "desc_manager.h"
#include "buffer_manager.h"
#include "dev_log.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include "constants.h"
#include "questmanager.h"
#include "desc_client.h"
#include "sectree_manager.h"
#include "regen.h"
#include <boost/format.hpp>
#include "item_manager.h"
#include "combat_zone.h"
#include "target.h"
#include "party.h"
#include "MountSystem.h"
#ifdef BATTLEPASS_WORLDARD
#include "battlepass.h"
#endif
#define ON_SUCCES_RESTART(ch) ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow"); ch->GetDesc()->SetPhase(PHASE_GAME); ch->SetPosition(POS_STANDING); ch->StartRecoveryEvent();

std::map<std::string, DWORD> m_kMapCombatZoneTargetSign;

DWORD m_pCombatZoneDay[] = {

	DAY_MONDAY,
	DAY_THURSDAY,
	DAY_SATURDAY

	/*DAY_MONDAY,
	DAY_TUESDAY,
	DAY_WEDNESDAY,
	DAY_THURSDAY,
	DAY_FRIDAY,
	DAY_SATURDAY,
	DAY_SUNDAY*/


};

DWORD m_pCombatZoneDateTimeDescript[DAY_MAX_NUM][8] = {
    /**
        @parameters
        @days
        @datetimeHM
		@explanation
    */
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    }
};

DWORD m_pCombatZoneDateTime[DAY_MAX_NUM][8] = {
    /**
        @parameters
        @days
        @datetimeHM
		@explanation
    */
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    },
	{
        22, 59,
        23, 59
    }
	/*
			[DAY_MONDAY] = {
				13, 00,	-- Start hour / Start minute
				15, 00	-- End hour / End minute
			}
	*/
};

SCombatZoneRespawnData objectPos[COMBAT_ZONE_MAX_POS_TELEPORT] =
{
	{ 710100, 26600 },
	{ 724300, 30400 },
	{ 720300, 21500 },
	{ 724300, 31400 },
	{ 719300, 31900 },
	{ 713700, 20000 },
	{ 711200, 20000 },
	{ 711200, 19000 },
	{ 708200, 27700 },
	{ 713500, 29600 }
};

EVENTINFO(TCombatZoneWarpEventInfo)
{
	DynamicCharacterPtr ch;
	DWORD bType, bSeconds;
	TCombatZoneWarpEventInfo() : ch(), bType(0), bSeconds(0) {}
};

EVENTINFO(TCombatZoneLeaveEventInfo)
{
	DynamicCharacterPtr ch;
	DWORD bSeconds;
	TCombatZoneLeaveEventInfo() : ch(), bSeconds(0){}
};

EVENTINFO(TCombatZoneEventInfo)
{
	CCombatZoneManager *pInstanceManager;

	TCombatZoneEventInfo()
	: pInstanceManager(0)
	{
	}
};

EVENTFUNC(combat_zone_warp_event)
{
	TCombatZoneWarpEventInfo* info = dynamic_cast<TCombatZoneWarpEventInfo*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER	ch = info->ch;

	if (!ch || !ch->GetDesc())
		return 0;
	
	if (info->bSeconds > 0) 
	{
		switch (info->bType)
		{
			case COMBAT_ZONE_ACTION_LEAVE:
				ch->ChatPacket(CHAT_TYPE_INFO, "601 %d", info->bSeconds);
				break;
				
			case COMBAT_ZONE_ACTION_PARTICIPATE:
				ch->ChatPacket(CHAT_TYPE_INFO, "602 %d", info->bSeconds);
				break;
		}

		--info->bSeconds;
		return PASSES_PER_SEC(1);
	}
	
	switch (info->bType)
	{
		case COMBAT_ZONE_ACTION_LEAVE:
			ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			break;
				
		case COMBAT_ZONE_ACTION_PARTICIPATE:
		{
			if (ch->GetParty())
			{
				LPPARTY pParty = ch->GetParty();

				if (pParty->GetMemberCount() == 2)
					CPartyManager::instance().DeleteParty(pParty);
				else
					pParty->Quit(ch->GetPlayerID());
			}
			
			DWORD dwIndex = number(0, COMBAT_ZONE_MAX_POS_TELEPORT - 1);
			ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
			ch->WarpSet(objectPos[dwIndex].x, objectPos[dwIndex].y);
		}
		break;
	}

	ch->m_pkCombatZoneWarpEvent = NULL;
	return 0;
}

void WarpSetByTime(LPCHARACTER ch, DWORD bType, DWORD bSeconds)
{
	TCombatZoneWarpEventInfo* info = AllocEventInfo<TCombatZoneWarpEventInfo>();
	info->ch = ch;
	info->bType = bType;
	info->bSeconds = bSeconds;
	ch->m_pkCombatZoneWarpEvent = event_create(combat_zone_warp_event, info, 1);
}

EVENTFUNC(combat_zone_leave_event)
{
	TCombatZoneLeaveEventInfo* info = dynamic_cast<TCombatZoneLeaveEventInfo*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER	ch = info->ch;

	if (!ch || !ch->GetDesc())
		return 0;
	
	// If the player who announced his withdrawal and had the target attached above his head was killed, he will not be allowed to leave the war zone successfully and the withdrawal will be void.
	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "603");
		m_kMapCombatZoneTargetSign.erase(ch->GetName());
		ch->m_pkCombatZoneLeaveEvent = NULL;		
		return 0;
	}
	
	if (info->bSeconds > 0) 
	{
		if (info->bSeconds <= COMBAT_ZONE_LEAVE_WITH_TARGET_COUNTDOWN_WARP_SECONDS)
			ch->ChatPacket(CHAT_TYPE_INFO, "601 %d", info->bSeconds);

		--info->bSeconds;
		return PASSES_PER_SEC(1);
	}

	ch->UpdateCombatZoneRankings(ch->GetName(), ch->GetEmpire(), ch->GetCombatZonePoints()); // Update ranking with the points what he was collected this time.
	ch->SetRealCombatZonePoints(ch->GetRealCombatZonePoints() + ch->GetCombatZonePoints());
#ifdef BATTLEPASS_WORLDARD
	CBattlePass::instance().UpdateMisionBattlePass(ch,COMBAT_ZONE_POINTS_BP,ch->GetCombatZonePoints());					
#endif
	ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
	ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
	ch->m_pkCombatZoneLeaveEvent = NULL;
	return 0;
}

void CCombatZoneManager::OnResetRanking()
{
	TPacketGDCombatZoneResetRanking p;
	p.bType = COMBAT_ZONE_SUB_HEADER_NONE;
	db_clientdesc->DBPacketHeader(HEADER_GD_COMBAT_ZONE_RESET_RANKING, 0, sizeof(TPacketGDCombatZoneResetRanking));
	db_clientdesc->Packet(&p, sizeof(p));	
}

EVENTFUNC(combat_zone_event)
{
	if (!event || !event->info)
		return 0;

	TCombatZoneEventInfo* info = dynamic_cast<TCombatZoneEventInfo*>(event->info);

	if (!info)
		return 0;
	
	CCombatZoneManager* pInstanceManager = info->pInstanceManager;

	if (!pInstanceManager)
		return 0;
	
	if(pInstanceManager->GetDaySpecific(pInstanceManager->GetCurrentDay()))
	{
		if (pInstanceManager->AnalyzeTimeZone(COMBAT_ZONE_CAN_START, pInstanceManager->GetCurrentDay()))
		{
			pInstanceManager->SetStatus(STATE_OPENED);
			pInstanceManager->MP();
			return PASSES_PER_SEC(25);
		}
				
		else if (pInstanceManager->AnalyzeTimeZone(COMBAT_ZONE_CAN_FINISH, pInstanceManager->GetCurrentDay()))
		{
			pInstanceManager->SetStatus(STATE_CLOSED);
			return PASSES_PER_SEC(25);
		}
		
		if (pInstanceManager->IsRunning())
			pInstanceManager->Flash();
	}

	
	if (pInstanceManager->GetIsWeekend())
	{
		/*
			The packet to reset is sended just one time, getIsWeekend return the current timer when is Sunday Hour (23) Min (59) Sec (59).
			Weekly ranges are reset each Sunday at midnight (server time) 23:59:59. The top three ranked players will receive a medal that appears above the character, and this will indicate your rank.
			Delete all old players ranks doesn't importantly if happens this:
			
			Now can't find 3 players (this weekend), that means nobody wil win the flags, they need to wait until next weekend for reset again.
			Select from the weekly database the top 3 players that have the most points possible.
			
			If at least 3 players could not be found in the database, the function not will give possibility to give new flag for the players.
			Truncate table weekly after was moved and reset all values to ranking_general, and give the rank top to players.
		*/

		pInstanceManager->OnResetRanking();
		return PASSES_PER_SEC(5);
	}


	
	return PASSES_PER_SEC(1);
}

bool CCombatZoneManager::Initialize()
{
	CheckEventStatus();

	TCombatZoneEventInfo* info = AllocEventInfo<TCombatZoneEventInfo>();
	info->pInstanceManager = this;
	m_pkCombatZoneEvent = event_create(combat_zone_event, info, PASSES_PER_SEC(40));
	return true;
}

void CCombatZoneManager::Destroy()
{
	CheckEventStatus();
}

std::vector<DWORD> parse_array(DWORD arg1 = 0, DWORD arg2 = 0, DWORD arg3 = 0, DWORD arg4 = 0)
{
    std::vector<DWORD> m_vec_infoData;
    m_vec_infoData.push_back(arg1);
    m_vec_infoData.push_back(arg2);
    m_vec_infoData.push_back(arg3);
    m_vec_infoData.push_back(arg4);
    return m_vec_infoData;
}



bool CCombatZoneManager::GetDaySpecific(DWORD day_actual)
{
	for (int a = 0; a <sizeof(m_pCombatZoneDay)/sizeof(*m_pCombatZoneDay); ++a)
	{
		if(day_actual == m_pCombatZoneDay[a]){
			return true;
		}
	}
	return false;
}



void CCombatZoneManager::SendCombatZoneInfoPacket(LPCHARACTER pkTarget, DWORD sub_header, std::vector<DWORD> m_vec_infoData)
{
	if (!pkTarget)
		return;	
	
	DWORD m_pDataArray[COMBAT_ZONE_MAX_ARGS] = {
		m_vec_infoData[0], m_vec_infoData[1], m_vec_infoData[2], m_vec_infoData[3]};

	TPacketGCSendCombatZone pack;
	pack.header = HEADER_GC_SEND_COMBAT_ZONE;
	pack.sub_header = sub_header;
	pack.isRunning = IsRunning();
	thecore_memcpy(&pack.m_pInfoData, m_pDataArray, sizeof(pack.m_pInfoData));

	switch (sub_header)
	{
		case COMBAT_ZONE_SUB_HEADER_OPEN_RANKING:{
			thecore_memcpy(&pack.m_pDataDays, m_pCombatZoneDateTimeDescript, sizeof(pack.m_pDataDays));


			bool check[] =
			{
				false,
				false,
				false,
				false,
				false,
				false,
				false
			};

			DWORD days[] =
			{
				DAY_MONDAY,
				DAY_TUESDAY,
				DAY_WEDNESDAY,
				DAY_THURSDAY,
				DAY_FRIDAY,
				DAY_SATURDAY,
				DAY_SUNDAY
			};

			for (int a = 0; a < sizeof(days)/sizeof(*days); ++a)
			{
				for (int i = 0; i < sizeof(m_pCombatZoneDay)/sizeof(*m_pCombatZoneDay); ++i)
				{
					if(days[a] == m_pCombatZoneDay[i])
					{
						check[a] = true;
					}
				}
			}


			thecore_memcpy(&pack.m_pDataDaysCheck, check, sizeof(pack.m_pDataDaysCheck));

			break;
		}
		default:
			memset(&pack.m_pDataDays, 0, sizeof(pack.m_pDataDays));
			break;
	}

	pkTarget->GetDesc()->Packet(&pack, sizeof(TPacketGCSendCombatZone));
}

struct FCombatZoneSendLeavingTargetSign
{
	LPCHARACTER pkLeaver;
	DWORD stateType;
	FCombatZoneSendLeavingTargetSign(LPCHARACTER ch, DWORD state) : pkLeaver(ch), stateType(state) {}

	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = static_cast<LPCHARACTER>(ent);
			if (ch->IsPC())
			{
				switch (stateType)
				{
					case COMBAT_ZONE_ADD_LEAVING_TARGET:
					{
						TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(ch->GetPlayerID(), TARGET_TYPE_COMBAT_ZONE, pkLeaver->GetVID());
						if (!pInfo)
							CTargetManager::Instance().CreateTarget(ch->GetPlayerID(), COMBAT_ZONE_INDEX_TARGET, pkLeaver->GetName(), TARGET_TYPE_COMBAT_ZONE, pkLeaver->GetVID(), NULL, ch->GetMapIndex(), "1");
					}
					break;

					case COMBAT_ZONE_REMOVE_LEAVING_TARGET:
						CTargetManager::instance().DeleteTarget(ch->GetPlayerID(), COMBAT_ZONE_INDEX_TARGET, pkLeaver->GetName());
						break;
				}
			}
		}
	}
};

struct FCombatZoneWarpToHome
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = static_cast<LPCHARACTER>(ent);

			if (ch->IsPC())
			{
				ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			}
		}
	}
};

void CCombatZoneManager::SendLeavingTargetSign(LPCHARACTER ch, DWORD dwType)
{
	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::instance().GetMap(COMBAT_ZONE_MAP_INDEX);
	if (pSectreeMap != NULL)
	{
		FCombatZoneSendLeavingTargetSign f(ch, dwType);
		pSectreeMap->for_each(f);
	}
}

void CCombatZoneManager::RefreshLeavingTargetSign(LPCHARACTER ch)
{
	for (std::map<std::string, DWORD>::const_iterator it = m_kMapCombatZoneTargetSign.begin(); it != m_kMapCombatZoneTargetSign.end(); ++it)
	{
		TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(ch->GetPlayerID(), TARGET_TYPE_COMBAT_ZONE, it->second);
		if (!pInfo)
			CTargetManager::Instance().CreateTarget(ch->GetPlayerID(), COMBAT_ZONE_INDEX_TARGET, it->first.c_str(), TARGET_TYPE_COMBAT_ZONE, it->second, NULL, ch->GetMapIndex(), "1");
	}
}

bool CCombatZoneManager::CanUseAction(LPCHARACTER ch, DWORD bType)
{
	DWORD iTimeElapsed = ch->GetQuestFlag((bType == COMBAT_ZONE_ACTION_PARTICIPATE) ? COMBAT_ZONE_FLAG_WAIT_TIME_JOIN : COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION);
	if (iTimeElapsed && (get_global_time() < iTimeElapsed))
	{	
		DWORD iAmount = (iTimeElapsed - get_global_time());
		DWORD iSec = iAmount % 60;
			iAmount /= 60;
		DWORD iMin = iAmount % 60;
			iAmount /= 60;
		DWORD iHour = iAmount % 24;
		DWORD iDay = iAmount / 24;
		ch->ChatPacket(CHAT_TYPE_INFO, "604 %d %d %d %d", iDay, iHour, iMin, iSec);
		return false;
	}
	
	return true;
}

bool CCombatZoneManager::CanJoin(LPCHARACTER ch)
{
	if (!CCombatZoneManager::instance().IsRunning())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "605");
		return false;
	}

#ifdef ENABLE_MOUNT_SYSTEM
	CMountSystem* MountSystem = ch->GetMountSystem();
	if(MountSystem){
		if(MountSystem->CountSummoned() != 0){
			ch->ChatPacket(CHAT_TYPE_INFO,"No puede pasar a este mapa teniendo una montura/caballo invocado");
			return false;
		}
	}
#endif

	if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO,"No puede pasar a este mapa teniendo una montura/caballo invocado");
		return false;
	}


	if (ch->GetMountVnum())
	{
		ch->ChatPacket(CHAT_TYPE_INFO,"No puede pasar a este mapa teniendo una montura/caballo invocado");
		return false;
	}

	if (ch->GetHorse() != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO,"No puede pasar a este mapa teniendo una montura/caballo invocado");
		return false;
	}
	
	// All 50 or higher level players can enter.
	if (ch->GetLevel() < COMBAT_ZONE_MIN_LEVEL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "606 %d", COMBAT_ZONE_MIN_LEVEL);
		return false;	
	}
	
	if (!CCombatZoneManager::instance().CanUseAction(ch, COMBAT_ZONE_ACTION_PARTICIPATE))
		return false;
	
	return true;
}

void CCombatZoneManager::RequestPotion(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	// Check the last time when you request the potion
	if (!CCombatZoneManager::instance().CanUseAction(ch, COMBAT_ZONE_ACTION_REQUEST_POTION))
		return;
	
	DWORD iMonstersKilled = ch->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED);
	if (iMonstersKilled < COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "607 %d %d", iMonstersKilled, COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT);
		return;
	}
	
	ch->AutoGiveItem(ITEM_COMBAT_ZONE_BATTLE_POTION, ITEM_COMBAT_ZONE_BATTLE_POTION_COUNT);
	ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_REQUEST_POTION);
	ch->SetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED, 0);
	CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_OPEN_RANKING, parse_array(ch->GetRealCombatZonePoints(), (ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) > 0) ? ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) - get_global_time() : 0, ch->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED), COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT));
}

void CCombatZoneManager::Leave(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	DWORD iCombatZonePoints = ch->GetCombatZonePoints(); // Points collected by kills on map
	
	// No Battle Points: You can leave the War Zone immediately by tapping the symbol on the side of the small map.
	if (iCombatZonePoints == 0)
	{
		WarpSetByTime(ch, COMBAT_ZONE_ACTION_LEAVE, COMBAT_ZONE_LEAVE_REGULAR_COUNTDOWN_WARP_SECONDS);
		return;
	}
	
	// Under 5 Battle Points: If a player who announced his withdrawal from the War Zone is killed and dropped below 5 points, he will be able to leave the in 15 seconds. It does not have to be revived.
	if (ch->IsDead() && (iCombatZonePoints < COMBAT_ZONE_REQUIRED_POINTS_TO_LEAVING_WHEN_DEAD && iCombatZonePoints != 0))
	{
		ch->SetRealCombatZonePoints(ch->GetRealCombatZonePoints() + ch->GetCombatZonePoints());
		WarpSetByTime(ch, COMBAT_ZONE_ACTION_LEAVE, COMBAT_ZONE_LEAVE_WHEN_DEAD_UNDER_MIN_POINTS);
		return;
	}
	
	if (iCombatZonePoints < COMBAT_ZONE_REQUIRED_POINTS_TO_LEAVING)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "608 %d", COMBAT_ZONE_REQUIRED_POINTS_TO_LEAVING);
		return;
	}
	
	// You can't announce withdrawal when you are dead, you can do this just if you have more less then 5 points.
	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "609");
		return;
	}
	
	itertype(m_kMapCombatZoneTargetSign) it = m_kMapCombatZoneTargetSign.find(ch->GetName());
	bool isAttachedTargetSign = it != m_kMapCombatZoneTargetSign.end();

	if (isAttachedTargetSign)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "610");
		return;
	}

	/*
		With battle points:
		You can leave the War Zone immediately by tapping the symbol on the side of the small map. As soon as you do that, an arrow above your head will mark you while you wait to leave the War Zone. 
		Any player who will kill you during this time will accumulate 5 points of battle.
		Your job is to survive another 2 minutes in the War Zone and get rid of the intact battle points.

		If you die in the 2 minutes, your retreat from the War Zone will be interrupted and you will lose 50% of the battle points. 
		After that, you will be able to announce your withdrawal again from the War Zone.
	*/
	
	TCombatZoneLeaveEventInfo* info = AllocEventInfo<TCombatZoneLeaveEventInfo>();
	info->ch = ch;
	info->bSeconds = COMBAT_ZONE_TARGET_NEED_TO_STAY_ALIVE;
	ch->m_pkCombatZoneLeaveEvent = event_create(combat_zone_leave_event, info, 1);

	//ch->AddAffect(AFFECT_COMBAT_ZONE_MOVEMENT, 0, 0, 0, INFINITE_AFFECT_DURATION, 0, false); // Player will suffer a reduction in its Movement Speed	
	//ch->SetPoint(POINT_MOV_SPEED, 100);
	ch->ComputePoints();
	ch->UpdatePacket();
	
	ch->ChatPacket(CHAT_TYPE_INFO, "611 %d", (COMBAT_ZONE_TARGET_NEED_TO_STAY_ALIVE / 60));

	CCombatZoneManager::instance().ActTargetSignMap(ch, COMBAT_ZONE_ADD_LEAVING_TARGET);
	CCombatZoneManager::instance().SendLeavingTargetSign(ch, COMBAT_ZONE_ADD_LEAVING_TARGET);
	CCombatZoneManager::instance().Announcement("612");
}

void CCombatZoneManager::Join(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!CCombatZoneManager::instance().CanJoin(ch))
		return;

	WarpSetByTime(ch, COMBAT_ZONE_ACTION_PARTICIPATE, COMBAT_ZONE_JOIN_WARP_SECOND);
}

void CCombatZoneManager::SetStatus(DWORD dwStatus)
{
	switch (dwStatus)
	{
		case STATE_CLOSED:
		{
			CCombatZoneManager::instance().Announcement("613");

			LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::instance().GetMap(COMBAT_ZONE_MAP_INDEX);
			if (pSectreeMap != NULL)
			{
				struct FCombatZoneWarpToHome f;
				pSectreeMap->for_each(f);
			}
		}
		break;
	}

	quest::CQuestManager::instance().RequestSetEventFlag("combat_zone_event", dwStatus);
	quest::CQuestManager::instance().RequestSetEventFlag("combat_zone_event_time", get_global_time()+COMBAT_ZONE_TIME_SET);	
}

bool CCombatZoneManager::IsRunning()
{
	return quest::CQuestManager::instance().GetEventFlag("combat_zone_event");
}

bool CCombatZoneManager::AnalyzeTimeZone(DWORD searchType, DWORD searchDay)
{
	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);
	switch (searchType)
	{
		/*
			SearchDay result the current day from freebsd date_time
			Check if current info h/m/s is equal with settings by day in config from m_pCombatZoneDateTime array.
		*/
		case COMBAT_ZONE_CAN_START:
			return (tm.tm_hour == m_pCombatZoneDateTime[searchDay][0] && tm.tm_min == m_pCombatZoneDateTime[searchDay][1] && tm.tm_sec == 0);

		case COMBAT_ZONE_CAN_FINISH:
			return (tm.tm_hour == m_pCombatZoneDateTime[searchDay][2] && tm.tm_min == m_pCombatZoneDateTime[searchDay][3] && tm.tm_sec == 0);
	}
}

void CCombatZoneManager::CheckEventStatus()
{
	m_kMapCombatZoneTargetSign.clear();
	SetStatus(STATE_CLOSED);
	if (m_pkCombatZoneEvent)
	{
		event_cancel(&m_pkCombatZoneEvent);
		m_pkCombatZoneEvent = NULL;
	}
}

struct FuncFlash
{
	FuncFlash()
	{}

	void operator () (LPDESC d)
	{
		// Operator to send flash on buttonon minimap (all channels) when combat zone start is running for full periodly.
		if (d->GetCharacter() && !CCombatZoneManager::instance().IsCombatZoneMap(d->GetCharacter()->GetMapIndex()))
			CCombatZoneManager::instance().SendCombatZoneInfoPacket(d->GetCharacter(), COMBAT_ZONE_SUB_HEADER_FLASH_ON_MINIMAP, parse_array());	
	}
};

void CCombatZoneManager::Flash()
{
	const DESC_MANAGER::DESC_SET & f = DESC_MANAGER::instance().GetClientSet();
	std::for_each(f.begin(), f.end(), FuncFlash());
}





struct FuncMP
{
	FuncMP()
	{}

	void operator () (LPDESC d)
	{
		if (d->GetCharacter() && !CCombatZoneManager::instance().IsCombatZoneMap(d->GetCharacter()->GetMapIndex())){
			CCombatZoneManager::instance().SendCombatZoneInfoPacket(d->GetCharacter(), COMBAT_ZONE_SUB_HEADER_MP, parse_array());	
		}
	}
};






void CCombatZoneManager::MP()
{
	const DESC_MANAGER::DESC_SET & f = DESC_MANAGER::instance().GetClientSet();
	std::for_each(f.begin(), f.end(), FuncMP());
}


void CCombatZoneManager::MpLogin(LPCHARACTER ch)
{
	if(ch){
		if(!CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()) && CCombatZoneManager::instance().IsRunning())
		{
			CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_MP, parse_array());
		}
	}
}

void CCombatZoneManager::RemoveAffectMob(LPCHARACTER ch, DWORD type)
{
	if(ch)
	{
		if(type == AFFECT_COMBAT_ZONE_MOB)
		{
			if(CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex())){
				CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_BONUS_CLEAR, parse_array());
			}
		}
	}
	
}

void CCombatZoneManager::AffectMobKill(LPCHARACTER ch, int vnum_mob)
{
	int list_bonus[8][2] = {

		{APPLY_ATTBONUS_HUMAN,30},
		{APPLY_ATTBONUS_WARRIOR,50},
		{APPLY_ATTBONUS_ASSASSIN,50},
		{APPLY_ATTBONUS_SURA,50},
		{APPLY_ATTBONUS_SHAMAN,50},
		{APPLY_MAX_HP,25000},
		{APPLY_SKILL_DAMAGE_BONUS,40},
		{APPLY_NORMAL_HIT_DAMAGE_BONUS,40}

	};

	if(ch){
		if(CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex())){
			if (COMBAT_ZONE_VNUM_MOB == vnum_mob)
			{
				if (ch->FindAffect(AFFECT_COMBAT_ZONE_MOB))
				{
					CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_BONUS_CLEAR, parse_array());
					ch->RemoveAffect(AFFECT_COMBAT_ZONE_MOB);
				}

				if(CCombatZoneManager::instance().IsRunning())
				{
					int idx_random = number(0, sizeof(list_bonus)/sizeof(*list_bonus)-1);

					int point_get = list_bonus[idx_random][0];
					int value_get = list_bonus[idx_random][1];

					CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_BONUS, parse_array(point_get,value_get));
					ch->AddAffect(AFFECT_COMBAT_ZONE_MOB, aApplyInfo[point_get].bPointType, value_get, AFF_NONE, COMBAT_ZONE_TIME_BONUS, 0, 0, false);
				}
			}
		}
	}
}

void CCombatZoneManager::CalculatePointsByKiller(LPCHARACTER ch, LPCHARACTER pkVictim ,bool isAttachedTargetSign)
{
	ch->SetCombatZonePoints(ch->GetCombatZonePoints() + (isAttachedTargetSign ? (pkVictim->GetCombatZonePoints() / 2) : COMBAT_ZONE_ADD_POINTS_NORMAL_KILLING));
	ch->UpdatePacket();
}

void CCombatZoneManager::ActTargetSignMap(LPCHARACTER ch, DWORD bType)
{
	switch (bType)
	{
		case COMBAT_ZONE_ADD_LEAVING_TARGET:
			m_kMapCombatZoneTargetSign.insert(std::make_pair(ch->GetName(), ch->GetVID()));
			break;
		case COMBAT_ZONE_REMOVE_LEAVING_TARGET:
			m_kMapCombatZoneTargetSign.erase(ch->GetName());
			break;	
	}
}

void CCombatZoneManager::OnDead(LPCHARACTER pkKiller, LPCHARACTER pkVictim)
{
	// Check if exist the killer and victim and if they are players not monsters, npc, stone.
	if (!pkKiller || !pkVictim || !pkKiller->IsPC())
		return;

	/************************************************************************/
	/* Kill monsters for can get potion										*/
	/************************************************************************/	
	
	// if (!pkVictim->IsPC())
	// {
		// DWORD iMonstersKilled = pkKiller->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED);
		// if (iMonstersKilled < COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT)
		// {
			// iMonstersKilled += 1;
			// pkKiller->SetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED, iMonstersKilled);
			// pkKiller->ChatPacket(CHAT_TYPE_NOTICE, "614 %d %d", iMonstersKilled, COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT);
			// return;
		// }
	// }
	
	/************************************************************************/
	/* Kill players on combat zone map										*/
	/************************************************************************/
	

	if(pkVictim->IsCoward()){
		return;
	}

	if (CCombatZoneManager::instance().IsCombatZoneMap(pkKiller->GetMapIndex()))
	{
		// Ugly this but that happens when you try in same time to kills mobs for potion and teleporting to combat zone and if you have a delay 3-5 seconds after warp, victim (mobs) what you wass killing will take like a pc because setMapIndex already was seted to combat zone map.
		if (!IsCombatZoneMap(pkVictim->GetMapIndex()))
			return;
		
		char c_pszTime[128];
		snprintf(c_pszTime, sizeof(c_pszTime), COMBAT_ZONE_FLAG_KILL_LAST_TIME, pkVictim->GetPlayerID());
		int dwKillLastTime = pkKiller->GetQuestFlag(c_pszTime);
		
		// If victim dead was effect of potion battle will be deleted.
		CCombatZoneManager::instance().RemoveAffect(pkVictim);
		
		// If victim dead have attached the target on head
		itertype(m_kMapCombatZoneTargetSign) it = m_kMapCombatZoneTargetSign.find(pkVictim->GetName());
		bool isAttachedTargetSign = it != m_kMapCombatZoneTargetSign.end();
		
		if (isAttachedTargetSign)
			CCombatZoneManager::instance().ActTargetSignMap(pkVictim, COMBAT_ZONE_REMOVE_LEAVING_TARGET);
		
		/*
			* If victim have attached target on head the killer will receive 5 points, it not 1.
			* Victim dead will lost 50% of current points.
			* Kill the same player again within 5 minutes, however, and you won't receive any points.
		*/
		
		if (get_global_time() < dwKillLastTime)
		{
			pkKiller->ChatPacket(CHAT_TYPE_NOTICE, "615 %d", (COMBAT_ZONE_WAIT_TIME_KILL_AGAIN_PLAYER / 60));
		}
		else
		{
			CCombatZoneManager::instance().CalculatePointsByKiller(pkKiller, pkVictim, isAttachedTargetSign);
			pkKiller->ChatPacket(CHAT_TYPE_NOTICE, "616 %d %d", (isAttachedTargetSign ? (pkVictim->GetCombatZonePoints() / 2) : COMBAT_ZONE_ADD_POINTS_NORMAL_KILLING), pkKiller->GetCombatZonePoints());	
			pkKiller->SetQuestFlag(c_pszTime, get_global_time() + COMBAT_ZONE_WAIT_TIME_KILL_AGAIN_PLAYER);	
		}

		pkVictim->SetCombatZoneDeaths(pkVictim->GetCombatZoneDeaths() + COMBAT_ZONE_ADD_DEATHS_POINTS);

#ifdef BATTLEPASS_WORLDARD
		CBattlePass::instance().UpdateMisionBattlePass(pkKiller,COMBAT_ZONE_KILL_BP,1);					
#endif

		if (isAttachedTargetSign)
		{
			if((pkVictim->GetCombatZonePoints() / 2) <= 0){
				pkVictim->SetCombatZonePoints(0);
			}else{
				pkVictim->SetCombatZonePoints(pkVictim->GetCombatZonePoints() / 2);
			}
			
		}else{
		
			DWORD points_delete = pkVictim->GetCombatZonePoints();

			if (points_delete > 0){

				if (points_delete < COMBAT_ZONE_DIVIDE_NUM_POINTS){
					pkVictim->SetCombatZonePoints(0);
				}
				else{
					pkVictim->SetCombatZonePoints(points_delete-COMBAT_ZONE_DIVIDE_NUM_POINTS);
				}
			}
		}


		
		pkVictim->UpdatePacket();

		if (pkVictim->GetCombatZonePoints())
			pkVictim->ChatPacket(CHAT_TYPE_NOTICE, "617 %d", pkVictim->GetCombatZonePoints());
	}
}

void CCombatZoneManager::RemoveAffect(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	const DWORD m_pkAffectCombatZone[4] = {
		AFFECT_COMBAT_ZONE_MOVEMENT, 
		AFFECT_COMBAT_ZONE_POTION, 
		AFFECT_COMBAT_ZONE_DEFENSE,
		AFFECT_COMBAT_ZONE_MOB,
	};
	
	for (int i=0; i<_countof(m_pkAffectCombatZone); i++)
	{
		const CAffect* pAffect = ch->FindAffect(m_pkAffectCombatZone[i]);

		if (pAffect)
			ch->RemoveAffect(const_cast<CAffect*>(pAffect));

	}
	
	CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_BONUS_CLEAR, parse_array());

	if (ch->FindAffect(AFFECT_MOUNT))
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);
		ch->MountVnum(0);
	}
		
	if (ch->IsPolymorphed())
	{
		ch->SetPolymorph(0);
		ch->RemoveAffect(AFFECT_POLYMORPH);
	}
			
	if (ch->IsHorseRiding())
		ch->StopRiding();
}

void CCombatZoneManager::OnLogout(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
	{
		CCombatZoneManager::instance().RemoveAffect(ch);

		// If he logout from map the points collected and deaths collected will be deleted and will teleport on map1 and give the skills back by cache.
		CCombatZoneManager::instance().ActTargetSignMap(ch, COMBAT_ZONE_REMOVE_LEAVING_TARGET);
#if defined(COMBAT_ZONE_SET_SKILL_PERFECT)
		CCombatZoneManager::instance().SetSkill(ch, COMBAT_ZONE_GET_SKILL_BACK_BY_CACHE);
#endif
		ch->SetCombatZoneDeaths(0);	 // Set deaths points to 0
		ch->SetCombatZonePoints(0);	 // Set points collected to 0
	}
}

void CCombatZoneManager::OnLogin(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	if (CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
	{
		CCombatZoneManager::instance().RemoveAffect(ch);

		// If try to login on combat zone on another channel with other methods then join button, warp them on map1.
		// If a player maybe he login on map when combat zone isn't active, that can happens very rare like if he login after some days and still remaning in map, because action to logout doesn't was with succesfully.
		if (!CCombatZoneManager::instance().IsRunning())
		{
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			return;
		}
		
		CCombatZoneManager::instance().ActTargetSignMap(ch, COMBAT_ZONE_REMOVE_LEAVING_TARGET);
		CCombatZoneManager::instance().RefreshLeavingTargetSign(ch);
#if defined(COMBAT_ZONE_SET_SKILL_PERFECT)
		CCombatZoneManager::instance().AppendSkillCache(ch); // All players' skills are set to 'Perfect Master',
#endif		

		DWORD ADDED_DEFENSE = (DEF_ADDED_BONUS - (DEF_MULTIPLIER * (ch->GetLevel() - COMBAT_ZONE_MIN_LEVEL)) - (ch->GetLevel() - COMBAT_ZONE_MIN_LEVEL));
		ch->AddAffect(AFFECT_COMBAT_ZONE_DEFENSE, POINT_DEF_GRADE, (ADDED_DEFENSE < 0 ? 0 : ADDED_DEFENSE), AFF_NONE, INFINITE_AFFECT_DURATION, 0, false, false);
		ch->UpdatePacket();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "COMBAT_ZONE 0");
		ch->ChatPacket(CHAT_TYPE_COMMAND, "COMBAT_ZONE_ESTADISTICAS OPEN");
		CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_TIME, parse_array(quest::CQuestManager::instance().GetEventFlag("combat_zone_event_time") - get_global_time() ));

		ch->StartNoticeMapCombatZone();

	}
	else{

		CCombatZoneManager::instance().MpLogin(ch);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "COMBAT_ZONE %d", quest::CQuestManager::instance().GetEventFlag("combat_zone_event"));
		ch->ChatPacket(CHAT_TYPE_COMMAND, "COMBAT_ZONE_ESTADISTICAS CLOSE");
		CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_TIME, parse_array());
	}
}

const DWORD * CCombatZoneManager::GetSkillList(LPCHARACTER ch)
{
	static const DWORD matrixArraySkill[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][SKILL_COUNT_INDEX] =
	{
		{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} }, // WARRIOR
		{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} }, // ASSASSIN
		{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} }, // SURA
		{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} }, // SHAMAN
#if defined(ENABLE_WOLFMAN_CHARACTER) || defined(WOLFMAN_CHARACTER)
		{ { 170,171,172,173,174,175 }, {	170,171,172,173,174,175 } } // WOLFMAN
#endif
	};
	return matrixArraySkill[ch->GetJob()][ch->GetSkillGroup() - 1];
}

void CCombatZoneManager::AppendSkillCache(LPCHARACTER ch)
{
	// He don't have the skills selected so we dont will store nothing.
	if (ch->GetSkillGroup() == 0)
		return;

	const DWORD * matrixArraySkill = CCombatZoneManager::instance().GetSkillList(ch);
	
	TPacketGDCombatZoneSkillsCache p;
	p.dwPID	= ch->GetPlayerID();
	p.dwSkillLevel1	= ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_1]);	
	p.dwSkillLevel2	= ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_2]);	
	p.dwSkillLevel3	= ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_3]);	
	p.dwSkillLevel4	= ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_4]);	
	p.dwSkillLevel5	= ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_5]);	
	p.dwSkillLevel6	= ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_6]);		
	db_clientdesc->DBPacketHeader(HEADER_GD_COMBAT_ZONE_SKILLS_CACHE, 0, sizeof(TPacketGDCombatZoneSkillsCache));
	db_clientdesc->Packet(&p, sizeof(p));

	// Set perfect skills after cached all old skills
	CCombatZoneManager::instance().SetSkill(ch, COMBAT_ZONE_SET_SKILL_MAX_LEVEL);
}

void CCombatZoneManager::SetSkill(LPCHARACTER ch, DWORD state)
{
	if (ch->GetSkillGroup() == 0)
		return;

	const DWORD * matrixArraySkill = CCombatZoneManager::instance().GetSkillList(ch);
	switch (state)
	{
		case COMBAT_ZONE_SET_SKILL_MAX_LEVEL:
		{
			for (int i = 0; i < SKILL_COUNT_INDEX; ++i)
				ch->SetSkillLevel(matrixArraySkill[i], SKILL_MAX_LEVEL);	
		}
		break;

		case COMBAT_ZONE_GET_SKILL_BACK_BY_CACHE:
		{
			std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT skillLevel1, skillLevel2, skillLevel3, skillLevel4, skillLevel5, skillLevel6 FROM player.combat_zone_skills_cache WHERE pid = '%d'", ch->GetPlayerID()));
			if (!pMsg->Get()->uiNumRows)
				return;
			
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			for (int i = 0; i < SKILL_COUNT_INDEX; ++i)
				ch->SetSkillLevel(matrixArraySkill[i], atoi(row[i]));
		}
		break;
	}

	ch->ComputePoints();
	ch->SkillLevelPacket();
}

void CCombatZoneManager::OnRestart(LPCHARACTER ch, int subcmd)
{
	if (!ch)
		return;
	
	// The waiting time for restart is' 10 seconds', which rises by 5 seconds whenever the character is killed within the map, up to a maximum of 30 seconds; 
	// When he collect the 30 seconds to wait, next death will be reseted the deaths to 0 for can start again calculation seconds by deaths;
	if (ch->GetCombatZoneDeaths() == COMBAT_ZONE_MAX_DEATHS_TO_INCREASE_TIMER_RESTART - 1)
		ch->SetCombatZoneDeaths(0);

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);
	int iSecondsRequestToWait = 170 - (ch->GetCombatZoneDeaths() * COMBAT_ZONE_INCREASE_SECONDS_RESTART);

	switch (subcmd)
	{
		case SCMD_RESTART_HERE:
		{
			if (iTimeToDead > iSecondsRequestToWait)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "618 %d", (iTimeToDead - iSecondsRequestToWait));
				return;
			}

			ON_SUCCES_RESTART(ch);
			ch->RestartAtSamePos();
			ch->PointChange(POINT_HP, (ch->GetMaxHP() / 2) - ch->GetHP()); // 50%
			ch->ReviveInvisible(3);
		}
		break;

		case SCMD_RESTART_TOWN:
		{
			if (iTimeToDead > iSecondsRequestToWait)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "619 %d", (iTimeToDead - iSecondsRequestToWait));
				return;
			}

			// Coordinates random for each restart on town
			DWORD dwIndex = GetRandomPos();

			ON_SUCCES_RESTART(ch);
			ch->PointChange(POINT_HP, (ch->GetMaxHP() - ch->GetHP())); // 100%
			ch->Show(COMBAT_ZONE_MAP_INDEX, objectPos[dwIndex].x, objectPos[dwIndex].y, 0);
			ch->Stop();
			ch->UpdatePacket();
		}
		break;
			
		case SCMD_RESTART_COMBAT_ZONE:
		{
			if (!CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
				return;
			
			if (!ch->CountSpecifyItem(ITEM_COMBAT_ZONE_REINCARNATION))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "620 %s", ITEM_MANAGER::instance().GetTable(ITEM_COMBAT_ZONE_REINCARNATION)->szLocaleName);
				return;
			}
			
			ON_SUCCES_RESTART(ch);
			ch->RemoveSpecifyItem(ITEM_COMBAT_ZONE_REINCARNATION, 1);
			ch->RestartAtSamePos();
			ch->PointChange(POINT_HP, (ch->GetMaxHP() - ch->GetHP())); // 100%
			ch->ReviveInvisible(3);
		}
		break;
	}
}

void CCombatZoneManager::ShowCurrentTimeZone(LPCHARACTER ch)
{
	if (!ch)
		return;

	time_t currentTime;
	struct tm *localTime;
	time(&currentTime);
	localTime = localtime(&currentTime);
	
	ch->ChatPacket(CHAT_TYPE_INFO, "621 %s", asctime(localTime));
}

/************************************************************************/
/* MEMBER ACTION														*/
/************************************************************************/
void CCombatZoneManager::RequestAction(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGCombatZoneRequestAction * p = (TPacketCGCombatZoneRequestAction *)c_pData;
	
	if (!ch)
		return;
	
	switch (p->action)
	{
		case COMBAT_ZONE_ACTION_OPEN_RANKING:
#if defined(COMBAT_ZONE_SHOW_SERVER_TIME_ZONE_ON_CHAT)
			ShowCurrentTimeZone(ch);
#endif
			CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_OPEN_RANKING, parse_array(ch->GetRealCombatZonePoints(), (ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) > 0) ? ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) - get_global_time() : 0, ch->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED), COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT));
			CCombatZoneManager::instance().RequestRanking(ch, COMBAT_ZONE_TYPE_RANKING_WEEKLY);
			return;
			
		case COMBAT_ZONE_ACTION_CHANGE_PAGE_RANKING:
		{
			switch (p->value)
			{
				case COMBAT_ZONE_TYPE_RANKING_WEEKLY: 
					CCombatZoneManager::instance().RequestRanking(ch, COMBAT_ZONE_TYPE_RANKING_WEEKLY);
					return;

				case COMBAT_ZONE_TYPE_RANKING_ALL: 
					CCombatZoneManager::instance().RequestRanking(ch, COMBAT_ZONE_TYPE_RANKING_ALL); 
					return;

				default:
					return;
			}
		}
			
		case COMBAT_ZONE_ACTION_PARTICIPATE:
			CCombatZoneManager::instance().Join(ch);
			return;
		
		case COMBAT_ZONE_ACTION_LEAVE:
			CCombatZoneManager::instance().Leave(ch);
			return;
			
		case COMBAT_ZONE_ACTION_REQUEST_POTION:
			CCombatZoneManager::instance().RequestPotion(ch);
			return;

		default:
			return;
	}
}

void CCombatZoneManager::Announcement(const char * format, ...)
{
	if (!format)
		return;

	char szBuf[CHAT_MAX_LEN + 1];
	va_list args;

	// Initializes ap to retrieve the additional arguments after parameter
	// Write formatted data from variable argument list to sized buffer
	va_start(args, format);
	vsnprintf(szBuf, sizeof(szBuf), format, args);
	va_end(args);

	SendNoticeMap(szBuf, COMBAT_ZONE_MAP_INDEX, true);
}

bool CCombatZoneManager::IsCombatZoneMap(int iMapIndex)
{
	if (iMapIndex == COMBAT_ZONE_MAP_INDEX)
		return true;
		
	return false;
}

/************************************************************************/
/* GUI RANKING IN-GAME													*/
/************************************************************************/
void CCombatZoneManager::RequestRanking(LPCHARACTER ch, DWORD bType)
{
	char szQuery[QUERY_MAX_LEN + 1];
	switch (bType)
	{
		case COMBAT_ZONE_TYPE_RANKING_WEEKLY:
			snprintf(szQuery, sizeof(szQuery), "player.combat_zone_ranking_weekly");		
			break;
		case COMBAT_ZONE_TYPE_RANKING_ALL:
			snprintf(szQuery, sizeof(szQuery), "player.combat_zone_ranking_general");
			break;
	}

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT * FROM %s ORDER BY memberPoints DESC LIMIT %d", szQuery, COMBAT_ZONE_MAX_ROWS_RANKING));
	if (pMsg->uiSQLErrno != 0)
		return;
	
	MYSQL_ROW row;
	DWORD i = 0;
	
	TPacketGCCombatZoneRankingData p;
	p.header = HEADER_GC_RANKING_COMBAT_ZONE;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		p.rankingData[i] = TPacketGCCombatZoneRanking();;
		p.rankingData[i].rank = i;
		strncpy(p.rankingData[i].name, row[0], sizeof(p.rankingData[i].name));
		str_to_number(p.rankingData[i].empire, row[1]);		
		str_to_number(p.rankingData[i].points, row[2]);	
		i++;
	}

	if (pMsg->Get()->uiNumRows < COMBAT_ZONE_MAX_ROWS_RANKING)
	{
		while (i < COMBAT_ZONE_MAX_ROWS_RANKING)
		{
			p.rankingData[i] = TPacketGCCombatZoneRanking();
			p.rankingData[i].rank = COMBAT_ZONE_EMPTY_VALUE_ROW;
			strncpy(p.rankingData[i].name, "", sizeof(p.rankingData[i].name));
			p.rankingData[i].empire = COMBAT_ZONE_EMPTY_VALUE_ROW;
			p.rankingData[i].points = COMBAT_ZONE_EMPTY_VALUE_ROW;
			i++;
		}
	}
	
	// Special slot rank for can see by self.
	std::auto_ptr<SQLMsg> pFindSQL(DBManager::instance().DirectQuery(
		"SELECT memberName, memberEmpire, memberPoints, "
		"FIND_IN_SET (memberPoints, (SELECT GROUP_CONCAT(memberPoints ORDER BY memberPoints DESC) "
			"FROM %s)) AS rank "
		"FROM %s WHERE memberName = '%s'", szQuery, szQuery, ch->GetName()
	));
	
	DWORD dwSelfSlot = COMBAT_ZONE_MAX_ROWS_RANKING;
	if (pFindSQL->Get()->uiNumRows > 0)
	{
		MYSQL_ROW rows = mysql_fetch_row(pFindSQL->Get()->pSQLResult);
		
		p.rankingData[dwSelfSlot] = TPacketGCCombatZoneRanking();;
		str_to_number(p.rankingData[dwSelfSlot].rank, rows[3]);	
		strncpy(p.rankingData[dwSelfSlot].name, rows[0], sizeof(p.rankingData[dwSelfSlot].name));
		str_to_number(p.rankingData[dwSelfSlot].empire, rows[1]);		
		str_to_number(p.rankingData[dwSelfSlot].points, rows[2]);	
	}
	else
	{
		p.rankingData[dwSelfSlot] = TPacketGCCombatZoneRanking();
		p.rankingData[dwSelfSlot].rank = COMBAT_ZONE_EMPTY_VALUE_ROW;
		strncpy(p.rankingData[dwSelfSlot].name, "", sizeof(p.rankingData[dwSelfSlot].name));
		p.rankingData[dwSelfSlot].empire = COMBAT_ZONE_EMPTY_VALUE_ROW;
		p.rankingData[dwSelfSlot].points = COMBAT_ZONE_EMPTY_VALUE_ROW;
	}

	ch->GetDesc()->Packet(&p, sizeof(p));
}

std::string GetRowMysqlString(DWORD state)
{
	switch(state) 
	{
		case COMBAT_ZONE_GET_POINTS:
			return "combat_zone_points"; 
		case COMBAT_ZONE_GET_RANK:
			return "combat_zone_rank";
	}
}

DWORD CCombatZoneManager::GetValueByType(LPCHARACTER ch, int state)
{
	if (ch->GetDesc() != NULL)
	{
		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT %s FROM player.player WHERE id = %d", GetRowMysqlString(state).c_str(), ch->GetPlayerID()));
		if (pMsg->Get()->uiNumRows == 0)
			return 0;

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		return atoi(row[0]);
	}
}

bool CCombatZoneManager::GetIsWeekend()
{
	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);
	
	int dwArray[4] = { 0, 23, 59, 58 };
	
	return (tm.tm_wday == dwArray[PAD_DAY] && tm.tm_hour == dwArray[PAD_HOUR] && tm.tm_min == dwArray[PAD_MIN] && (tm.tm_sec == dwArray[PAD_SEC] || tm.tm_sec == 59));
}

bool CCombatZoneManager::CanUseItem(LPCHARACTER ch, LPITEM item)
{
	if (!ch || !item)
		return false;

	switch (item->GetVnum())
	{
		case ITEM_COMBAT_ZONE_BATTLE_POTION:
		{
			if (!CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "622");
				return false;
			}
			
			if (ch->FindAffect(AFFECT_COMBAT_ZONE_POTION))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "623");
				return false;
			}
			
			// Affect have attached bonus until you are not dead, when you dead lost the bonus hp.
			ch->AddAffect(AFFECT_COMBAT_ZONE_POTION, POINT_MAX_HP, BATTLE_POTION_MAX_HP, AFF_NONE, INFINITE_AFFECT_DURATION, 0, 0, true);
			ch->AddAffect(AFFECT_COMBAT_ZONE_POTION, POINT_ATT_GRADE_BONUS, BATTLE_POTION_MAX_ATT, AFF_NONE, INFINITE_AFFECT_DURATION, 0, 0, false);
			ch->RemoveSpecifyItem(item->GetVnum(), 1);
#if defined(COMBAT_ZONE_SHOW_EFFECT_POTION)
			ch->EffectPacket(SE_COMBAT_ZONE_POTION);
#endif
		}
		break;

		case ITEM_COMBAT_ZONE_FIELD_BOX_1:
		case ITEM_COMBAT_ZONE_FIELD_BOX_2:
		case ITEM_COMBAT_ZONE_WOODEN_CHEST:
		{
			std::vector <DWORD> dwVnums;
			std::vector <DWORD> dwCounts;
			std::vector <LPITEM> item_gets(NULL);
			int count = 0;
			
			if (ch->GiveItemFromSpecialItemGroup(item->GetVnum(), dwVnums, dwCounts, item_gets, count))
			{
				item->SetSocket(0, item->GetSocket(0) + 1);

				if (item->GetSocket(0) >= COMBAT_ZONE_CHEST_MAX_OPENED)
				{
					ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (COMBAT_ZONE_ITEM_CHEST)");
					return false;
				}
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "624");
				return false;
			}
		}
		break;
	}
	
	return true;
}

DWORD CCombatZoneManager::GetFirstDayHour()
{
	time_t now = time(NULL);
	tm * nowT = localtime(&now);
	nowT->tm_hour = 0;
	nowT->tm_min = 0;
	nowT->tm_sec = 0;
	time_t dwTimeStampHour = mktime(nowT);
    return dwTimeStampHour;
}

DWORD CCombatZoneManager::GetCurrentDay()
{
	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);
	
	switch (tm.tm_wday)
	{
		case 0:
			return DAY_SUNDAY;
		case 1:
			return DAY_MONDAY;
		case 2:
			return DAY_TUESDAY;
		case 3:
			return DAY_WEDNESDAY;
		case 4:
			return DAY_THURSDAY;
		case 5:
			return DAY_FRIDAY;
		case 6:
			return DAY_SATURDAY;
	}
}