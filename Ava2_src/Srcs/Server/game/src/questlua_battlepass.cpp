#include "stdafx.h"
#include "questmanager.h"
#include "battlepass.h"
#include "char.h"


namespace quest
{

	int battlepass_tokenshop(lua_State* L)
	{

		if (!lua_isnumber(L, 1))
		{
			return 1;
		}


		DWORD count = (int)lua_tonumber(L,1);

		if(count <= 0)
		{
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		CBattlePass::instance().UpdateMisionBattlePass(ch,TOKEN_ITEMSHOP_BP,count);

		return 1;
	}

	void RegisterBattlePassFunctionTable()
	{
		luaL_reg battlepass_functions[] =
		{

			{	"token_shop"	,battlepass_tokenshop},
			{	NULL,			NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("BattlePass", battlepass_functions);
	}
}

