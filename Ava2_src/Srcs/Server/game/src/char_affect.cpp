
#include "stdafx.h"

#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "packet.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "battle.h"
#include "guild.h"
#include "utils.h"
#include "locale_service.h"
#include "lua_incl.h"
#include "arena.h"
#include "horsename_manager.h"
#include "item.h"
#include "DragonSoul.h"
#include "../../common/CommonDefines.h"
#if defined(WJ_COMBAT_ZONE)
	#include "combat_zone.h"
#endif

#if defined(ENABLE_NEW_AUTOPOTION)
constexpr DWORD AUTO_POTION_START_INDEX = AFFECT_AUTO_ATTACK_SPEED_RECOVERY + 20;
constexpr DWORD AUTO_POTION_END_INDEX   = AFFECT_AUTO_WHITE_POTION_RECOVERY + 20;
#define IS_NO_SAVE_AFFECT(type) ((type) == AFFECT_WAR_FLAG || (type) == AFFECT_REVIVE_INVISIBLE || ((type) >= AFFECT_PREMIUM_START && (type) <= AFFECT_PREMIUM_END) || (type) == AFFECT_MOUNT_BONUS || (type >= AUTO_POTION_START_INDEX && type <= AUTO_POTION_END_INDEX)) // @fixme156 added MOUNT_BONUS (if the game core crashes, the bonus would double if present in player.affect)
#define IS_NO_CLEAR_ON_DEATH_AFFECT(type) ((type) == AFFECT_BLOCK_CHAT || ((type) >= 500 && (type) < 600) || (type) == AFFECT_AUTO_ATTACK_SPEED_RECOVERY || (type) == AFFECT_AUTO_MOV_SPEED_RECOVERY || (type) == AFFECT_AUTO_CRITICAL_RECOVERY || (type) == AFFECT_AUTO_PENETRATE_RECOVERY || (type) == AFFECT_AUTO_VIT_RECOVERY || (type) == AFFECT_AUTO_STR_RECOVERY || (type) == AFFECT_AUTO_INT_RECOVERY || (type) == AFFECT_AUTO_DEX_RECOVERY || (type) == AFFECT_AUTO_RED_POTION_RECOVERY|| (type) == AFFECT_AUTO_ORANGE_POTION_RECOVERY|| (type) == AFFECT_AUTO_YELLOW_POTION_RECOVERY|| (type) == AFFECT_AUTO_GREEN_POTION_RECOVERY|| (type) == AFFECT_AUTO_BLUE_POTION_RECOVERY|| (type) == AFFECT_AUTO_WHITE_POTION_RECOVERY || (type >= AUTO_POTION_START_INDEX && type <= AUTO_POTION_END_INDEX))
#else
#define IS_NO_SAVE_AFFECT(type) ((type) == AFFECT_WAR_FLAG || (type) == AFFECT_REVIVE_INVISIBLE || ((type) >= AFFECT_PREMIUM_START && (type) <= AFFECT_PREMIUM_END) || (type) == AFFECT_MOUNT_BONUS) // @fixme156 added MOUNT_BONUS (if the game core crashes, the bonus would double if present in player.affect)
#define IS_NO_CLEAR_ON_DEATH_AFFECT(type) ((type) == AFFECT_BLOCK_CHAT || ((type) >= 500 && (type) < 600))
#endif

#ifdef ENABLE_ADD_REALTIME_AFFECT
bool IsRealTimeAffect(DWORD affectIndex)
{
	switch(affectIndex)
	{
		case AFFECT_AUTO_PICK_UP:
#ifdef ENABLE_VOTE4BUFF
		case AFFECT_VOTE4BUFF:
#endif
			return true;
			break;
	}
	return false;
}
#endif


void SendAffectRemovePacket(LPDESC d, DWORD pid, DWORD type, BYTE point)
{
	TPacketGCAffectRemove ptoc;
	ptoc.bHeader	= HEADER_GC_AFFECT_REMOVE;
	ptoc.dwType		= type;
	ptoc.bApplyOn	= point;
	d->Packet(&ptoc, sizeof(TPacketGCAffectRemove));

	TPacketGDRemoveAffect ptod;
	ptod.dwPID		= pid;
	ptod.dwType		= type;
	ptod.bApplyOn	= point;
	db_clientdesc->DBPacket(HEADER_GD_REMOVE_AFFECT, 0, &ptod, sizeof(ptod));

}

void SendAffectAddPacket(LPDESC d, CAffect * pkAff)
{
	TPacketGCAffectAdd ptoc;
	ptoc.bHeader		= HEADER_GC_AFFECT_ADD;
	ptoc.elem.dwType		= pkAff->dwType;
	ptoc.elem.bApplyOn		= pkAff->bApplyOn;
	ptoc.elem.lApplyValue	= pkAff->lApplyValue;
	ptoc.elem.dwFlag		= pkAff->dwFlag;
#ifdef ENABLE_ADD_REALTIME_AFFECT
	ptoc.elem.lDuration		= IsRealTimeAffect(ptoc.elem.dwType) ? pkAff->lDuration - time(0): pkAff->lDuration;
#else
	ptoc.elem.lDuration		= pkAff->lDuration;
#endif
	ptoc.elem.lSPCost		= pkAff->lSPCost;
	d->Packet(&ptoc, sizeof(TPacketGCAffectAdd));
}
////////////////////////////////////////////////////////////////////
// Affect
CAffect * CHARACTER::FindAffect(DWORD dwType, BYTE bApply) const
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAffect = *it++;

		if (pkAffect->dwType == dwType && (bApply == APPLY_NONE || bApply == pkAffect->bApplyOn))
			return pkAffect;
	}

	return NULL;
}

EVENTFUNC(affect_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "affect_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->UpdateAffect())
		return 0;
	else
		return passes_per_sec; // 1��
}

bool CHARACTER::UpdateAffect()
{
	// affect_event ���� ó���� ���� �ƴ�����, 1��¥�� �̺�Ʈ���� ó���ϴ� ����
	// �̰� ���̶� ���⼭ ���� ó���� �Ѵ�.
	if (GetPoint(POINT_HP_RECOVERY) > 0)
	{
		if (GetMaxHP() <= GetHP())
		{
			PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
		}
		else
		{
			int iVal = MIN(GetPoint(POINT_HP_RECOVERY), GetMaxHP() * 7 / 100);

			PointChange(POINT_HP, iVal);
			PointChange(POINT_HP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_SP_RECOVERY) > 0)
	{
		if (GetMaxSP() <= GetSP())
			PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));
		else
		{
			int iVal = MIN(GetPoint(POINT_SP_RECOVERY), GetMaxSP() * 7 / 100);

			PointChange(POINT_SP, iVal);
			PointChange(POINT_SP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_HP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_HP, GetPoint(POINT_HP_RECOVER_CONTINUE));
	}

	if (GetPoint(POINT_SP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_SP, GetPoint(POINT_SP_RECOVER_CONTINUE));
	}

	AutoRecoveryItemProcess( AFFECT_AUTO_HP_RECOVERY );
	AutoRecoveryItemProcess( AFFECT_AUTO_SP_RECOVERY );

#if defined(ENABLE_NEW_AUTOPOTION)
	AutoRecoveryItemProcess(AFFECT_AUTO_ATTACK_SPEED_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_MOV_SPEED_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_CRITICAL_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_PENETRATE_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_VIT_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_STR_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_INT_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_DEX_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_RED_POTION_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_ORANGE_POTION_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_YELLOW_POTION_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_GREEN_POTION_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_BLUE_POTION_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_WHITE_POTION_RECOVERY);
#endif

	// ���׹̳� ȸ��
	if (GetMaxStamina() > GetStamina())
	{
		int iSec = (get_dword_time() - GetStopTime()) / 3000;
		if (iSec)
			PointChange(POINT_STAMINA, GetMaxStamina()/1);
	}


	// ProcessAffect�� affect�� ������ true�� �����Ѵ�.
	if (ProcessAffect())
		if (GetPoint(POINT_HP_RECOVERY) == 0 && GetPoint(POINT_SP_RECOVERY) == 0 && GetStamina() == GetMaxStamina())
		{
			m_pkAffectEvent = NULL;
			return false;
		}

	return true;
}

void CHARACTER::StartAffectEvent()
{
	if (m_pkAffectEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkAffectEvent = event_create(affect_event, info, passes_per_sec);
	sys_log(1, "StartAffectEvent %s %p %p", GetName(), this, get_pointer(m_pkAffectEvent));
}

void CHARACTER::ClearAffect(bool bSave)
{
	TAffectFlag afOld = m_afAffectFlag;
	WORD	wMovSpd = GetPoint(POINT_MOV_SPEED);
	WORD	wAttSpd = GetPoint(POINT_ATT_SPEED);

	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it;

		if (bSave)
		{
#ifdef ENABLE_MULTI_FARM_BLOCK
			if(pkAff->dwType == AFFECT_MULTI_FARM_PREMIUM)
			{
				++it;
				continue;
			}
#endif
#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
			if(IsRealTimeAffect(pkAff->dwType))
			{
				++it;
				continue;
			}
#endif
			if ( IS_NO_CLEAR_ON_DEATH_AFFECT(pkAff->dwType) || IS_NO_SAVE_AFFECT(pkAff->dwType) )
			{
				++it;
				continue;
			}

#ifdef ENABLE_NEW_AFFECT_POTION			
			int pkAff_blend[] = {AFFECT_POTION_1, AFFECT_POTION_2, AFFECT_POTION_3, AFFECT_POTION_4, AFFECT_POTION_5, AFFECT_POTION_6, AFFECT_POTION_7, AFFECT_POTION_8, AFFECT_POTION_9, AFFECT_POTION_10, AFFECT_POTION_11};
			
			if ((pkAff->dwType == pkAff_blend[0]) || (pkAff->dwType == pkAff_blend[1]) || (pkAff->dwType == pkAff_blend[2]) ||
				(pkAff->dwType == pkAff_blend[3]) || (pkAff->dwType == pkAff_blend[4]) || (pkAff->dwType == pkAff_blend[5]) ||
				(pkAff->dwType == pkAff_blend[6]) || (pkAff->dwType == pkAff_blend[7]) || (pkAff->dwType == pkAff_blend[8]) ||
				(pkAff->dwType == pkAff_blend[9]) || (pkAff->dwType == pkAff_blend[10]))
			{
				++it;
				continue;
			}
#endif



			
			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
#if defined(WJ_COMBAT_ZONE)
				CCombatZoneManager::instance().RemoveAffectMob(this, pkAff->dwType);
#endif
			}
		}

		ComputeAffect(pkAff, false);

		it = m_list_pkAffect.erase(it);
		CAffect::Release(pkAff);
	}

	if (afOld != m_afAffectFlag ||
			wMovSpd != GetPoint(POINT_MOV_SPEED) ||
			wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	CheckMaximumPoints();

	if (m_list_pkAffect.empty())
		event_cancel(&m_pkAffectEvent);
}

int CHARACTER::ProcessAffect()
{
	bool	bDiff	= false;
	CAffect	*pkAff	= NULL;

	//
	// �����̾� ó��
	//
	for (int i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int aff_idx = i + AFFECT_PREMIUM_START;

		pkAff = FindAffect(aff_idx);

		if (!pkAff)
			continue;

		int remain = GetPremiumRemainSeconds(i);

		if (remain < 0)
		{
			RemoveAffect(aff_idx);
			bDiff = true;
		}
		else
			pkAff->lDuration = remain + 1;
	}


#ifdef BATTLEPASS_WORLDARD
	pkAff = FindAffect(AFFECT_BATTLE_PASS);
	if (pkAff)
	{
		int remain = GetBattlePassEndTime();
		
		if (remain < 0)
		{
			RemoveAffect(AFFECT_BATTLE_PASS);
			m_dwBattlePassEndTime = 0;
			bDiff = true;
			this->ChatPacket(CHAT_TYPE_INFO,"Ya has finalizado el pase de batalla, debes esperar al siguiente mes.");

		}
		else
			pkAff->lDuration = remain + 1;
	}
#endif
	
	////////// HAIR_AFFECT
	pkAff = FindAffect(AFFECT_HAIR);
	if (pkAff)
	{
		// IF HAIR_LIMIT_TIME() < CURRENT_TIME()
		if ( this->GetQuestFlag("hair.limit_time") < get_global_time())
		{
			// SET HAIR NORMAL
			this->SetPart(PART_HAIR, 0);
			// REMOVE HAIR AFFECT
			RemoveAffect(AFFECT_HAIR);
		}
		else
		{
			// INCREASE AFFECT DURATION
			++(pkAff->lDuration);
		}
	}
	////////// HAIR_AFFECT
	//

	CHorseNameManager::instance().Validate(this);

	TAffectFlag afOld = m_afAffectFlag;
	long lMovSpd = GetPoint(POINT_MOV_SPEED);
	long lAttSpd = GetPoint(POINT_ATT_SPEED);

	itertype(m_list_pkAffect) it;

	it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		pkAff = *it;

		bool bEnd = false;

		if (pkAff->dwType >= GUILD_SKILL_START && pkAff->dwType <= GUILD_SKILL_END)
		{
			if (!GetGuild() || !GetGuild()->UnderAnyWar())
				bEnd = true;
		}

		if (pkAff->lSPCost > 0)
		{
			if (GetSP() < pkAff->lSPCost)
				bEnd = true;
			else
				PointChange(POINT_SP, -pkAff->lSPCost);
		}

		// AFFECT_DURATION_BUG_FIX
		// ���� ȿ�� �����۵� �ð��� ���δ�.
		// �ð��� �ſ� ũ�� ��� ������ ��� ���� ���̶� ������.

#ifdef ENABLE_ADD_REALTIME_AFFECT
		if(IsRealTimeAffect(pkAff->dwType))
		{
			if(time(0) > pkAff->lDuration)
				bEnd = true;
		}
		else
#endif
		{
			if ( --pkAff->lDuration <= 0 )
			{
				bEnd = true;
			}
		}

		// END_AFFECT_DURATION_BUG_FIX

		if (bEnd)
		{
			it = m_list_pkAffect.erase(it);
			ComputeAffect(pkAff, false);
			bDiff = true;
			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
#if defined(WJ_COMBAT_ZONE)
				CCombatZoneManager::instance().RemoveAffectMob(this, pkAff->dwType);
#endif
			}

			CAffect::Release(pkAff);

			continue;
		}

		++it;
	}

	if (bDiff)
	{
		if (afOld != m_afAffectFlag ||
				lMovSpd != GetPoint(POINT_MOV_SPEED) ||
				lAttSpd != GetPoint(POINT_ATT_SPEED))
		{
			UpdatePacket();
		}

		CheckMaximumPoints();
	}

	if (m_list_pkAffect.empty())
		return true;

	return false;
}

void CHARACTER::SaveAffect()
{
	TPacketGDAddAffect p;

	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it++;

		if (IS_NO_SAVE_AFFECT(pkAff->dwType))
			continue;

		sys_log(1, "AFFECT_SAVE: %u %u %d %d", pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration);

		p.dwPID			= GetPlayerID();
		p.elem.dwType		= pkAff->dwType;
		p.elem.bApplyOn		= pkAff->bApplyOn;
		p.elem.lApplyValue	= pkAff->lApplyValue;
		p.elem.dwFlag		= pkAff->dwFlag;
		p.elem.lDuration	= pkAff->lDuration;
		p.elem.lSPCost		= pkAff->lSPCost;
		db_clientdesc->DBPacket(HEADER_GD_ADD_AFFECT, 0, &p, sizeof(p));
	}
}

EVENTINFO(load_affect_login_event_info)
{
	DWORD pid;
	DWORD count;
	char* data;

	load_affect_login_event_info()
	: pid( 0 )
	, count( 0 )
	, data( 0 )
	{
	}
};

EVENTFUNC(load_affect_login_event)
{
	load_affect_login_event_info* info = dynamic_cast<load_affect_login_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "load_affect_login_event_info> <Factor> Null pointer" );
		return 0;
	}

	DWORD dwPID = info->pid;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

	if (!ch)
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}

	LPDESC d = ch->GetDesc();

	if (!d)
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}

	if (d->IsPhase(PHASE_HANDSHAKE) ||
			d->IsPhase(PHASE_LOGIN) ||
			d->IsPhase(PHASE_SELECT) ||
			d->IsPhase(PHASE_DEAD) ||
			d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		sys_log(1, "Affect Load by Event");
		ch->LoadAffect(info->count, (TPacketAffectElement*)info->data);
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
	else
	{
		sys_err("input_db.cpp:quest_login_event INVALID PHASE pid %d", ch->GetPlayerID());
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
}

void CHARACTER::LoadAffect(DWORD dwCount, TPacketAffectElement * pElements)
{
	m_bIsLoadedAffect = false;

	if (!GetDesc()->IsPhase(PHASE_GAME))
	{
		if (test_server)
			sys_log(0, "LOAD_AFFECT: Creating Event", GetName(), dwCount);

		load_affect_login_event_info* info = AllocEventInfo<load_affect_login_event_info>();

		info->pid = GetPlayerID();
		info->count = dwCount;
		info->data = M2_NEW char[sizeof(TPacketAffectElement) * dwCount];
		thecore_memcpy(info->data, pElements, sizeof(TPacketAffectElement) * dwCount);

		event_create(load_affect_login_event, info, PASSES_PER_SEC(1));

		return;
	}

	ClearAffect(true);

	if (test_server)
		sys_log(0, "LOAD_AFFECT: %s count %d", GetName(), dwCount);

	TAffectFlag afOld = m_afAffectFlag;

	long lMovSpd = GetPoint(POINT_MOV_SPEED);
	long lAttSpd = GetPoint(POINT_ATT_SPEED);

	for (DWORD i = 0; i < dwCount; ++i, ++pElements)
	{
		// �������� �ε������ʴ´�.
		if (pElements->dwType == SKILL_MUYEONG)
			continue;
#ifdef ENABLE_CONQUEROR_LEVEL
		if (pElements->dwType == SKILL_CHEONUN)
			continue;
#endif

		if (AFFECT_AUTO_HP_RECOVERY == pElements->dwType || AFFECT_AUTO_SP_RECOVERY == pElements->dwType)
		{
			LPITEM item = FindItemByID( pElements->dwFlag );

			if (NULL == item)
				continue;

			item->Lock(true);
		}

		if (pElements->bApplyOn >= POINT_MAX_NUM)
		{
			sys_err("invalid affect data %s ApplyOn %u ApplyValue %d",
					GetName(), pElements->bApplyOn, pElements->lApplyValue);
			continue;
		}

		if (test_server)
		{
			sys_log(0, "Load Affect : Affect %s %d %d", GetName(), pElements->dwType, pElements->bApplyOn );
		}

		CAffect* pkAff = CAffect::Acquire();
		m_list_pkAffect.push_back(pkAff);

		pkAff->dwType		= pElements->dwType;
		pkAff->bApplyOn		= pElements->bApplyOn;
		pkAff->lApplyValue	= pElements->lApplyValue;
		pkAff->dwFlag		= pElements->dwFlag;
		pkAff->lDuration	= pElements->lDuration;
		pkAff->lSPCost		= pElements->lSPCost;

		SendAffectAddPacket(GetDesc(), pkAff);

		ComputeAffect(pkAff, true);


	}

	if ( CArenaManager::instance().IsArenaMap(GetMapIndex()) == true )
	{
		RemoveGoodAffect();
	}

	if (afOld != m_afAffectFlag || lMovSpd != GetPoint(POINT_MOV_SPEED) || lAttSpd != GetPoint(POINT_ATT_SPEED))
	{
		UpdatePacket();
	}

	StartAffectEvent();

	m_bIsLoadedAffect = true;

	// ��ȥ�� ���� �ε� �� �ʱ�ȭ
	DragonSoul_Initialize();

	// @fixme118 (regain affect hp/mp)
	if (!IsDead())
	{
		PointChange(POINT_HP, GetMaxHP() - GetHP());
		PointChange(POINT_SP, GetMaxSP() - GetSP());
	}
}

bool CHARACTER::AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube )
{
	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT && lDuration > 1)
	{
		ChatPacket(CHAT_TYPE_INFO, "96");
	}
	// END_OF_CHAT_BLOCK

	if (lDuration == 0)
	{
		sys_err("Character::AddAffect lDuration == 0 type %d", lDuration, dwType);
		lDuration = 1;
	}
#if defined(ENABLE_IGNORE_LOWER_BUFFS)
	switch (dwType)
	{
		case SKILL_HOSIN:
		case SKILL_REFLECT:
		case SKILL_GICHEON:
		case SKILL_JEONGEOP:
		case SKILL_KWAESOK:
		case SKILL_JEUNGRYEOK:
#if defined(ENABLE_WOLFMAN_CHARACTER)
		case SKILL_CHEONGRANG:
#endif
		{
			const CAffect * pkAffect = FindAffect(dwType);
			if (!pkAffect)
				break;
				
			if (lApplyValue < pkAffect->lApplyValue)
			{
				ChatPacket(CHAT_TYPE_INFO, "<AddAffect> has blocked receiving skill (%s) because power is (%ld%%) more small then current one (%ld%%).", CSkillManager::instance().Get(dwType)->szName, lApplyValue, pkAffect->lApplyValue);
				return false;
			}
		}
		break;
		default:
			break;
	}
#endif
	CAffect * pkAff = NULL;

	if (IsCube)
		pkAff = FindAffect(dwType,bApplyOn);
	else
		pkAff = FindAffect(dwType);

	if (dwFlag == AFF_STUN)
	{
		if (m_posDest.x != GetX() || m_posDest.y != GetY())
		{
			m_posDest.x = m_posStart.x = GetX();
			m_posDest.y = m_posStart.y = GetY();
			battle_end(this);

			SyncPacket();
		}
	}

	// �̹� �ִ� ȿ���� ���� ���� ó��
	if (pkAff && bOverride)
	{
		ComputeAffect(pkAff, false); // �ϴ� ȿ���� �����ϰ�

		if (GetDesc())
			SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
	}
	else
	{
		//
		// �� ���带 �߰�
		//
		// NOTE: ���� ���� type ���ε� ���� ����Ʈ�� ���� �� �ִ�.
		//
		pkAff = CAffect::Acquire();
		m_list_pkAffect.push_back(pkAff);

	}

	sys_log(1, "AddAffect %s type %d apply %d %d flag %u duration %d", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);
	sys_log(0, "AddAffect %s type %d apply %d %d flag %u duration %d", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);

	pkAff->dwType	= dwType;
	pkAff->bApplyOn	= bApplyOn;
	pkAff->lApplyValue	= lApplyValue;
	pkAff->dwFlag	= dwFlag;
#ifdef ENABLE_ADD_REALTIME_AFFECT
	pkAff->lDuration	= IsRealTimeAffect(dwType) ? lDuration + time(0) : lDuration;
#else
	pkAff->lDuration	= lDuration;
#endif
	pkAff->lSPCost	= lSPCost;

	WORD wMovSpd = GetPoint(POINT_MOV_SPEED);
	WORD wAttSpd = GetPoint(POINT_ATT_SPEED);

	ComputeAffect(pkAff, true);

	if (pkAff->dwFlag || wMovSpd != GetPoint(POINT_MOV_SPEED) || wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	StartAffectEvent();

	if (IsPC())
	{
		SendAffectAddPacket(GetDesc(), pkAff);

		if (IS_NO_SAVE_AFFECT(pkAff->dwType))
			return true;

		TPacketGDAddAffect p;
		p.dwPID			= GetPlayerID();
		p.elem.dwType		= pkAff->dwType;
		p.elem.bApplyOn		= pkAff->bApplyOn;
		p.elem.lApplyValue	= pkAff->lApplyValue;
		p.elem.dwFlag		= pkAff->dwFlag;
		p.elem.lDuration	= pkAff->lDuration;
		p.elem.lSPCost		= pkAff->lSPCost;
		db_clientdesc->DBPacket(HEADER_GD_ADD_AFFECT, 0, &p, sizeof(p));
	}

	return true;
}

void CHARACTER::RefreshAffect()
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it++;
		ComputeAffect(pkAff, true);
	}
}

void CHARACTER::ComputeAffect(CAffect * pkAff, bool bAdd)
{

	if (bAdd && pkAff->dwType >= GUILD_SKILL_START && pkAff->dwType <= GUILD_SKILL_END)
	{
		if (!GetGuild())
			return;

		if (!GetGuild()->UnderAnyWar())
			return;
	}

	if (pkAff->dwFlag)
	{
		if (!bAdd)
			m_afAffectFlag.Reset(pkAff->dwFlag);
		else
			m_afAffectFlag.Set(pkAff->dwFlag);
	}

	if (bAdd)
		PointChange(pkAff->bApplyOn, pkAff->lApplyValue);
	else
		PointChange(pkAff->bApplyOn, -pkAff->lApplyValue);
		
	if (pkAff->dwType == SKILL_MUYEONG)
	{
		if (bAdd)
			StartMuyeongEvent();
		else
			StopMuyeongEvent();
	}

#ifdef ENABLE_CONQUEROR_LEVEL
    	if (pkAff->dwType == SKILL_CHEONUN)
    	{
        	if (bAdd)
            		StartCheonunEvent(pkAff->lApplyValue);
        	else
            		StopCheonunEvent();
    	}
#endif

}

bool CHARACTER::RemoveAffect(CAffect * pkAff)
{
	if (!pkAff)
		return false;

#if defined(ENABLE_NEW_AUTOPOTION)
	switch (pkAff->dwType)
	{
		case AFFECT_AUTO_ATTACK_SPEED_RECOVERY:
		case AFFECT_AUTO_MOV_SPEED_RECOVERY:
		case AFFECT_AUTO_CRITICAL_RECOVERY:
		case AFFECT_AUTO_PENETRATE_RECOVERY:
		case AFFECT_AUTO_VIT_RECOVERY:
		case AFFECT_AUTO_STR_RECOVERY:
		case AFFECT_AUTO_INT_RECOVERY:
		case AFFECT_AUTO_DEX_RECOVERY:
		case AFFECT_AUTO_RED_POTION_RECOVERY:
		case AFFECT_AUTO_ORANGE_POTION_RECOVERY:
		case AFFECT_AUTO_YELLOW_POTION_RECOVERY:
		case AFFECT_AUTO_GREEN_POTION_RECOVERY:
		case AFFECT_AUTO_BLUE_POTION_RECOVERY:
		case AFFECT_AUTO_WHITE_POTION_RECOVERY:
			RemoveAffect(pkAff->dwType + 20);
			break;

		default:
			break;
	}
#endif

	// AFFECT_BUF_FIX
	m_list_pkAffect.remove(pkAff);
	// END_OF_AFFECT_BUF_FIX

	ComputeAffect(pkAff, false);

	if (AFFECT_REVIVE_INVISIBLE != pkAff->dwType)
#if defined(ENABLE_NEW_AUTOPOTION)
	{
		if (pkAff->dwType < AFFECT_AUTO_ATTACK_SPEED_RECOVERY || pkAff->dwType > AFFECT_AUTO_WHITE_POTION_RECOVERY)
			ComputePoints();
	}
#else
		ComputePoints();
#endif
	else  // @fixme110
		UpdatePacket();

	CheckMaximumPoints();

	if (test_server)
		sys_log(0, "AFFECT_REMOVE: %s (flag %u apply: %u)", GetName(), pkAff->dwFlag, pkAff->bApplyOn);

	if (IsPC())
	{
		SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn);
	}

	CAffect::Release(pkAff);
	return true;
}

bool CHARACTER::RemoveAffect(DWORD dwType)
{
	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT)
	{
		ChatPacket(CHAT_TYPE_INFO, "97");
	}
	// END_OF_CHAT_BLOCK

	bool flag = false;

	CAffect * pkAff;

	while ((pkAff = FindAffect(dwType)))
	{
		RemoveAffect(pkAff);
		flag = true;
	}

	return flag;
}

bool CHARACTER::IsAffectFlag(DWORD dwAff) const
{
	return m_afAffectFlag.IsSet(dwAff);
}

void CHARACTER::RemoveGoodAffect()
{
	RemoveAffect(AFFECT_MOV_SPEED);
	RemoveAffect(AFFECT_ATT_SPEED);
	RemoveAffect(AFFECT_STR);
	RemoveAffect(AFFECT_DEX);
	RemoveAffect(AFFECT_INT);
	RemoveAffect(AFFECT_CON);
	RemoveAffect(AFFECT_CHINA_FIREWORK);

	RemoveAffect(SKILL_JEONGWI);
	RemoveAffect(SKILL_GEOMKYUNG);
	RemoveAffect(SKILL_CHUNKEON);
	RemoveAffect(SKILL_EUNHYUNG);
	RemoveAffect(SKILL_GYEONGGONG);
	RemoveAffect(SKILL_GWIGEOM);
	RemoveAffect(SKILL_TERROR);
	RemoveAffect(SKILL_JUMAGAP);
	RemoveAffect(SKILL_MANASHILED);
	RemoveAffect(SKILL_HOSIN);
	RemoveAffect(SKILL_REFLECT);
	RemoveAffect(SKILL_KWAESOK);
	RemoveAffect(SKILL_JEUNGRYEOK);
	RemoveAffect(SKILL_GICHEON);
#ifdef ENABLE_WOLFMAN_CHARACTER
	// ������(WOLFMEN) ���� �߰�
	RemoveAffect(SKILL_JEOKRANG);
	RemoveAffect(SKILL_CHEONGRANG);
#endif
#ifdef ENABLE_CONQUEROR_LEVEL
	RemoveAffect(SKILL_CHEONUN);
#endif

}

bool CHARACTER::IsGoodAffect(BYTE bAffectType) const
{
	switch (bAffectType)
	{
		case (AFFECT_MOV_SPEED):
		case (AFFECT_ATT_SPEED):
		case (AFFECT_STR):
		case (AFFECT_DEX):
		case (AFFECT_INT):
		case (AFFECT_CON):
		case (AFFECT_CHINA_FIREWORK):

		case (SKILL_JEONGWI):
		case (SKILL_GEOMKYUNG):
		case (SKILL_CHUNKEON):
		case (SKILL_EUNHYUNG):
		case (SKILL_GYEONGGONG):
		case (SKILL_GWIGEOM):
		case (SKILL_TERROR):
		case (SKILL_JUMAGAP):
		case (SKILL_MANASHILED):
		case (SKILL_HOSIN):
		case (SKILL_REFLECT):
		case (SKILL_KWAESOK):
		case (SKILL_JEUNGRYEOK):
		case (SKILL_GICHEON):
#ifdef ENABLE_WOLFMAN_CHARACTER
		// ������(WOLFMEN) ���� �߰�
		case (SKILL_JEOKRANG):
		case (SKILL_CHEONGRANG):
#endif
#ifdef ENABLE_CONQUEROR_LEVEL
		case (SKILL_CHEONUN):
#endif

			return true;
	}
	return false;
}

void CHARACTER::RemoveBadAffect()
{
	sys_log(0, "RemoveBadAffect %s", GetName());
	// ��
	RemovePoison();
#ifdef ENABLE_WOLFMAN_CHARACTER
	RemoveBleeding();
#endif
	RemoveFire();

	// ����           : Value%�� ������ 5�ʰ� �Ӹ� ���� ���� ���ư���. (������ 1/2 Ȯ���� Ǯ��)               AFF_STUN
	RemoveAffect(AFFECT_STUN);

	// ���ο�         : Value%�� ������ ����/�̼� ��� ��������. ���õ��� ���� �޶��� ����� ��� �� ��쿡   AFF_SLOW
	RemoveAffect(AFFECT_SLOW);

	// ���Ӹ���
	RemoveAffect(SKILL_TUSOK);

	// ����
	//RemoveAffect(SKILL_CURSE);

	// �Ĺ���
	//RemoveAffect(SKILL_PABUP);

	// ����           : Value%�� ������ ������Ų��. 2��                                                       AFF_FAINT
	//RemoveAffect(AFFECT_FAINT);

	// �ٸ�����       : Value%�� ������ �̵��ӵ��� ����Ʈ����. 5�ʰ� -40                                      AFF_WEB
	//RemoveAffect(AFFECT_WEB);

	// ����         : Value%�� ������ 10�ʰ� ������. (������ Ǯ��)                                        AFF_SLEEP
	//RemoveAffect(AFFECT_SLEEP);

	// ����           : Value%�� ������ ����/��� ��� ����Ʈ����. ���õ��� ���� �޶��� ����� ��� �� ��쿡 AFF_CURSE
	//RemoveAffect(AFFECT_CURSE);

	// ����           : Value%�� ������ 4�ʰ� �����Ų��.                                                     AFF_PARA
	//RemoveAffect(AFFECT_PARALYZE);

	// �ε��ں�       : ���� ���
	//RemoveAffect(SKILL_BUDONG);
}

