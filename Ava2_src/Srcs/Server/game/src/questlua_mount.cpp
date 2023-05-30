#include "stdafx.h"
  
#include "questlua.h"
#include "questmanager.h"
#include "horsename_manager.h"
#include "char.h"
#include "affect.h"
#include "config.h"
#include "utils.h"
#include "db.h"

#include "MountSystem.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

extern int(*check_name) (const char * str);

namespace quest
{

#ifdef ENABLE_MOUNT_SYSTEM
	int mount_summon(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CMountSystem* MountSystem = ch->GetMountSystem();
		
#ifdef MOUNT_SYSTEM_BONUS
		LPITEM pItem = CQuestManager::instance().GetCurrentItem();
		if (!pItem)
		{
			lua_pushnumber (L, 0);
			return 1;
		}
#endif

		if (!ch || !MountSystem)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		if (0 == MountSystem)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		DWORD mobVnum = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0;
		
#ifdef MOUNT_SYSTEM_BONUS
		CMountActor* MState = MountSystem->Summon(mobVnum, ch->GetName(), pItem);
#else
		CMountActor* MState = MountSystem->Summon(mobVnum, ch->GetName());
#endif

		if (MState != NULL)
			lua_pushnumber(L, 1);
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int mount_unsummon(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CMountSystem* MountSystem = ch->GetMountSystem();

		if (0 == MountSystem)
			return 0;

		DWORD mobVnum = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0;

		MountSystem->Unsummon(mobVnum);
		return 1;
	}
	
	int mount_count_summoned(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CMountSystem* MountSystem = ch->GetMountSystem();

		lua_Number count = 0;

		if (0 != MountSystem)
			count = (lua_Number)MountSystem->CountSummoned();

		lua_pushnumber(L, count);

		return 1;
	}
	
	int mount_bonus(lua_State* L)
	{
		BYTE applyOn = static_cast<BYTE>(lua_tonumber(L, 1));
		long value = static_cast<long>(lua_tonumber(L, 2));
		long duration = static_cast<long>(lua_tonumber(L, 3));

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if( NULL != ch )
		{
			ch->RemoveAffect(AFFECT_MOUNT_BONUS);
			ch->AddAffect(AFFECT_MOUNT_BONUS, aApplyInfo[applyOn].bPointType, value, AFF_NONE, duration, 0, false);
		}

		return 0;
	}

	int mount_ride(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if (!ch)
			return 0;

		if (ch->IsDead() || ch->IsStun())
			return 0;
		
		if (ch->GetQuestFlag("MountSystem.mounted") > 0)
		{
			return 0;
		}

		if(ch->GetMountingVnumM() != 0)
		{
			CMountSystem* MountSystem = ch->GetMountSystem();
			if(MountSystem != NULL)
			{
				if (ch->IsHorseRiding())
					ch->StopRiding(); 
				ch->SetQuestFlag("MountSystem.mounted", ch->GetMountingVnumM());
				MountSystem->Mount(ch->GetMountingVnumM());
				return 1;
			}
		}
		return 0;
	}
	
	
	int find_mount_affect(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if (!ch)
			return 0;

		if (ch->IsDead())
			return 0;
		
		if (ch->FindAffect(AFFECT_MOUNT_BONUS))
			lua_pushnumber(L, 1);
		else
			lua_pushnumber(L, 0);
		
		return 1;
	}
	
	int add_mount_affect(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		BYTE applyOn = (BYTE) lua_tonumber(L, 1);
		long value = (long) lua_tonumber(L, 2);
		long duration = (long) lua_tonumber(L, 3);

		if (!ch)
			return 0;

		if (ch->IsDead())
			return 0;
		
		if (ch->FindAffect(AFFECT_MOUNT_BONUS)){
			ch->RemoveAffect(AFFECT_MOUNT_BONUS);
		}
		ch->AddAffect(AFFECT_MOUNT_BONUS, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);
		
		
		return 1;
	}
	
	int del_mount_affect(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!ch)
			return 0;

		if (ch->IsDead())
			return 0;
		
		if (ch->FindAffect(AFFECT_MOUNT_BONUS))
			ch->RemoveAffect(AFFECT_MOUNT_BONUS);
		
		return 1;
	}
	
	int mount_is_mine(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER mch = q.GetCurrentCharacterPtr();
		LPCHARACTER tch = q.GetCurrentNPCCharacterPtr();
		CMountSystem* MountSystem = mch->GetMountSystem();
		CMountActor* MountActor = MountSystem->GetByVID(tch->GetVID());

		lua_pushboolean(L, tch && MountActor && MountActor->GetOwner() == mch);
		return 1;
	}
	
	int mount_resummon(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CMountSystem* MountSystem = ch->GetMountSystem();
		
		DWORD mobVnum = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0;
		DWORD itemVnum = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : 0;
		
#ifdef MOUNT_SYSTEM_BONUS
		LPITEM pItem = ch->FindSpecifyItem(itemVnum);

		if (!pItem)
		{
			lua_pushnumber (L, 0);
			return 1;
		}
#endif

		if (!ch || !MountSystem)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		if (0 == MountSystem)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

#ifdef MOUNT_SYSTEM_BONUS
		CMountActor* MState = MountSystem->Summon(mobVnum, ch->GetName(), pItem);
#else
		CMountActor* MState = MountSystem->Summon(mobVnum, ch->GetName());
#endif

		if (MState != NULL)
			lua_pushnumber(L, 1);
		else
			lua_pushnumber(L, 0);

		return 1;
	}
	
	int mount_is_summon(lua_State* L)
	{

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if (!ch)
			lua_pushboolean(L, 0);
			return 1;

		if (ch->IsDead() || ch->IsStun())
			lua_pushboolean(L, 0);
			return 1;
		
		if (ch->GetQuestFlag("MountSystem.mounted") != 0)
		{
			lua_pushboolean(L, 1);
		}else{
			lua_pushboolean(L, 0);
		}

		return 1;

	}

	void RegisterMountFunctionTable()
	{
		luaL_reg mountF[] =
		{
			{ "summon",			mount_summon },
			{ "resummon",			mount_resummon },
			{ "unsummon",			mount_unsummon },
			{ "count_summoned",	mount_count_summoned },
			{ "mount_bonus",	mount_bonus },
			{ "mount_ride",	mount_ride },
			{ "find_mount_affect",	find_mount_affect },
			{ "mount_add_affect",	add_mount_affect },
			{ "mount_del_affect",	del_mount_affect },
			{ "is_mine",	mount_is_mine },
			{ "is_summon" , mount_is_summon},
			{ NULL,				NULL }
		};

		CQuestManager::instance().AddLuaFunctionTable("Mount", mountF);
	}
#endif

}
