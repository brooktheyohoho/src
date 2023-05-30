#include "stdafx.h"
#include "battlepass.h"
#include "char.h"
#include "locale_service.h"
#include "packet.h"
#include "desc_client.h"
#include "item_manager.h"
#include "item.h"
#include "item_manager.h"
#include "locale_service.h"

CBattlePass::CBattlePass()
{
}

CBattlePass::~CBattlePass()
{
	
}

void CBattlePass::LoadBattlePass()
{
	FILE 	*fp;
	char	one_line[256];
	int		value1, value2;
	const char	*delim = " \t\r\n";
	char	*v, *token_string;
	char 	file_name[256+1];

	battlepass_data	*bp_data = NULL;
	battlepass_values_items	bp_value = {0,0};

	snprintf(file_name, sizeof(file_name), "%s/battlepass_config.txt", LocaleService_GetBasePath().c_str());

	if ((fp = fopen(file_name, "r")) == 0)
		return;

	while (fgets(one_line, 256, fp))
	{
		value1 = value2 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);

		if (NULL == token_string)
			continue;

		if ((v = strtok(NULL, delim)))
			str_to_number(value1, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		TOKEN("mission_start")
		{
			bp_data = M2_NEW battlepass_data;
		}

		else TOKEN("type")
		{
			bp_data->type = value1;
		}

		else TOKEN("item")
		{
			bp_value.vnum		= value1;
			bp_value.count		= value2;

			bp_data->item.push_back(bp_value);
		}

		else TOKEN("vnum")
		{
			bp_data->vnum		= value1;
		}

		else TOKEN("count")
		{
			bp_data->count		= value1;
		}

		else TOKEN("mission_end")
		{
			battlepass_datas.push_back(bp_data);
		}
	}
	fclose(fp);
}

void CBattlePass::Open(LPCHARACTER ch)
{

	if(battlepass_datas.size() <= 0){
		return;
	}

	if(ch->GetBattlePassId() <= 0)
	{
		return;
	}

	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_CLEAR);
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_INFO);
	SetUpdateCountMision(ch);
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_INFO_EXTRA);
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_LOADING);
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_OPEN);
}

bool CBattlePass::CheckMisionExist(DWORD type)
{
	for (int i = 0; i < battlepass_datas.size(); ++i)
	{
		if(battlepass_datas[i]->type == type)
		{
			return true;
		}
	}
	return false;
}

DWORD CBattlePass::GetVnumMision(DWORD index)
{
	return battlepass_datas[index]->vnum;
}

DWORD CBattlePass::GetTypeMision(DWORD index)
{
	return battlepass_datas[index]->type;
}

int CBattlePass::GetCountMision(DWORD index)
{
	return battlepass_datas[index]->count;
}


int CBattlePass::GetMisionData(LPCHARACTER ch, DWORD index)
{
	char name_mision[1024];
	snprintf(name_mision, sizeof(name_mision), "battle_pass.mision_index_%d", index);
	int get_count = ch->GetQuestFlag(name_mision);
	return get_count;
}

void CBattlePass::SetMisionData(LPCHARACTER ch , DWORD index, int count)
{
	char name_mision[1024];
	snprintf(name_mision, sizeof(name_mision), "battle_pass.mision_index_%d", index);
	ch->SetQuestFlag(name_mision,GetMisionData(ch,index)+count);

	SetUpdateCountMision(ch);
}


int CBattlePass::GetMisionDataReward(LPCHARACTER ch, DWORD index)
{
	char name_mision[1024];
	snprintf(name_mision, sizeof(name_mision), "battle_pass.reward_index_%d", index);
	int get_count = ch->GetQuestFlag(name_mision);
	return get_count;
}

void CBattlePass::SetMisionDataReward(LPCHARACTER ch , DWORD index, int count)
{
	char name_mision[1024];
	snprintf(name_mision, sizeof(name_mision), "battle_pass.reward_index_%d", index);
	ch->SetQuestFlag(name_mision,GetMisionDataReward(ch,index)+count);

	SetUpdateCountMision(ch);
}




void CBattlePass::UpdateMisionBattlePassExtra(LPCHARACTER ch, DWORD type ,DWORD vnum, int count)
{
	if(battlepass_datas.size() <= 0)
		return;

	if(ch->GetBattlePassId() <= 0)
	{
		return;
	}

	DWORD get_chech = CheckMisionExist(type);

	if(!get_chech)
		return;

	for (int i = 0; i < battlepass_datas.size(); ++i)
	{
		if(GetTypeMision(i) == type)
		{
			int value_count = GetCountMision(i);

			switch (type)
			{
				case KILL_MONSTER_SPECIFIC_BP:
				case DUNGEON_COMPLETE_SPECIFIC_BP:
				case CRAFT_ITEM_SPECIFIC_BP:

				{

					DWORD vnum_get = GetVnumMision(i);
					if(vnum_get <= 0 || vnum <= 0){return;}

					if(vnum_get == vnum)
					{	

						if(GetMisionData(ch,i) < value_count)
						{
							SetMisionData(ch,i,count);

							if(GetMisionData(ch,i) >= value_count)
							{
								SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_MP);
							}

						}
						return;
					}
				}
				break;
			}
		}
	}
	return;
}


void CBattlePass::UpdateMisionBattlePass(LPCHARACTER ch, DWORD type, int count)
{

	if(battlepass_datas.size() <= 0)
		return;

	if(ch->GetBattlePassId() <= 0)
	{
		return;
	}

	DWORD get_chech = CheckMisionExist(type);

	if(!get_chech)
		return;

	for (int i = 0; i < battlepass_datas.size(); ++i)
	{
		if(GetTypeMision(i) == type)
		{
			int value_count = GetCountMision(i);

			switch (type)
			{
				case KILL_MONSTER_BP:
				case KILL_METIN_BP:
				case KILL_BOSS_BP:
				case CATCH_FISH_BP:
				case FRY_FISH_BP:
				case COMBAT_ZONE_POINTS_BP:
				case DAMAGE_MOBS_BP:
				case DAMAGE_METIN_BP:
				case DAMAGE_BOSS_BP:
				case KILL_PLAYER_BP:
				case KILL_PLAYER_OTHER_EMPIRE_BP:
				case CRAFT_ITEM_BP:
				case ADD_COSTUME_ATTR_BP:
				case CHANGE_COSTUME_ATTR_BP:
				case CHANGE_ATTR_BP:
				case OPEN_CHEST_BP:
				case TOKEN_ITEMSHOP_BP:

				{
					if(GetMisionData(ch,i) < value_count)
					{
						SetMisionData(ch,i,count);
						if(GetMisionData(ch,i) >= value_count)
						{
							SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_MP);
						}
					}


				}
				break;
			}
		}
	}
}

void CBattlePass::Reward(LPCHARACTER ch, int index)
{
	if(battlepass_datas.size() <= 0){
		return;
	}

	if(ch->GetBattlePassId() <= 0)
	{
		return;
	}

	if (index > battlepass_datas.size())
	{
		return;
	}

	for (int i = 0; i < battlepass_datas.size(); ++i)
	{

		if(i == (index-1))
		{
			int value_count = GetCountMision(i);
			if(GetMisionData(ch,i) < value_count)
			{
				return;
			}

			if(GetMisionDataReward(ch,i) == 1)
			{
				return;
			}

			int count_list_misiones = battlepass_datas[i]->item.size();

			SetMisionDataReward(ch,i,1);

			for (int a = 0; a < 4; ++a)
			{
				if(a < count_list_misiones)
				{
					ch->AutoGiveItem(battlepass_datas[i]->item[a].vnum,battlepass_datas[i]->item[a].count);
				}	
			}
		}
	}
}


void CBattlePass::ResetMisionBattlePass(LPCHARACTER ch)
{
	char name_mision[1024];
	const char* type[] = {
		"mision_index",
		"reward_index"
	};

	for (int i = 0; i < 10; ++i)
	{
		for (int itype = 0; itype<sizeof(type)/sizeof(*type); ++itype)
		{
			snprintf(name_mision, sizeof(name_mision), "battle_pass.%s_%d",type[itype],i);
			ch->SetQuestFlag(name_mision,0);
		}
	}
}


void CBattlePass::SetUpdateCountMision(LPCHARACTER ch)
{
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_UPDATE_CLEAR);
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_UPDATE);
	SendBattlePassPackets(ch,BATTLEPASS_SUB_HEADER_UPDATE_LOADING);
}


void CBattlePass::SendBattlePassPackets(LPCHARACTER ch, BYTE subheader)
{

	TPacketGCBattlePass pack;
	pack.subheader = subheader;

	if(subheader == BATTLEPASS_SUB_HEADER_INFO || subheader == BATTLEPASS_SUB_HEADER_UPDATE)
	{
		if(battlepass_datas.size() > 0)
		{
			for (int i = 0; i < battlepass_datas.size(); ++i)
			{

				if(subheader == BATTLEPASS_SUB_HEADER_INFO)
				{
					pack.data.type = battlepass_datas[i]->type;
					pack.data.vnum = battlepass_datas[i]->vnum;

					int count_list_misiones = battlepass_datas[i]->item.size();

					for (int a = 0; a < 4; ++a)
					{
						if(a < count_list_misiones)
						{
							pack.data.data_item[a].vnum 		= battlepass_datas[i]->item[a].vnum;
							pack.data.data_item[a].count 		= battlepass_datas[i]->item[a].count;
						}else{
							pack.data.data_item[a].vnum 		= 0;
							pack.data.data_item[a].count 		= 0;
						}
					}
				}
				else
				{
					pack.data.count = battlepass_datas[i]->count;
					pack.data.count_actual = GetMisionData(ch,i);
					pack.data.reward_status = GetMisionDataReward(ch,i);
				}


				LPDESC d = ch->GetDesc();

				if (NULL == d)
				{
					sys_err ("User CBattlePass::SendBattlePassPackets BATTLEPASS_SUB_HEADER_INFO || BATTLEPASS_SUB_HEADER_UPDATE (%s)'s DESC is NULL POINT.", ch->GetName());
					return ;
				}

				d->Packet(&pack, sizeof(pack));
			}

		}
	}

	else
	{

		if(subheader == BATTLEPASS_SUB_HEADER_INFO_EXTRA)
		{
			pack.time = ch->GetBattlePassEndTime();
		}

		LPDESC d = ch->GetDesc();

		if (NULL == d)
		{
			sys_err ("User CBattlePass::SendBattlePassPackets (%s)'s DESC is NULL POINT.", ch->GetName());
			return ;
		}

		d->Packet(&pack, sizeof(pack));	
	}


}