#include "stdafx.h"
#include "guild_ranking.h"
#include "entity.h"
#include "sectree_manager.h"
#include "char.h"
#include "locale_service.h"
#include "regen.h"
#include "log.h"
#include "db.h"
#include "utils.h"
#include "guild.h"
#include "guild_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "start_position.h"
#include "p2p.h"
#include "config.h"

std::vector<CGuild*> guild_list;

CGuildRanking::CGuildRanking()
{
}

CGuildRanking::~CGuildRanking()
{

}

CGuildRankingExtern::CGuildRankingExtern()
{
}

CGuildRankingExtern::~CGuildRankingExtern()
{
	
}

void CGuildRankingExtern::Open(LPCHARACTER ch)
{

	SendDateGuildRanking(ch);
	SendGuildRankingInfoPackets(ch,RANKING_GUILD_SUB_HEADER_OPEN_SEND);
}


struct FGuildCompare : public std::binary_function<CGuild*, CGuild*, bool>
{
	bool operator () (CGuild* g1, CGuild* g2) const
	{
		if (g1->GetLadderPoint() < g2->GetLadderPoint())
			return true;
		if (g1->GetLadderPoint() > g2->GetLadderPoint())
			return false;
		if (g1->GetGuildWarWinCount() < g2->GetGuildWarWinCount())
			return true;
		if (g1->GetGuildWarWinCount() > g2->GetGuildWarWinCount())
			return false;
		if (g1->GetGuildWarLossCount() < g2->GetGuildWarLossCount())
			return true;
		if (g1->GetGuildWarLossCount() > g2->GetGuildWarLossCount())
			return false;
		if (g1->GetLevel() < g2->GetLevel())
			return true;
		if (g1->GetLevel() > g2->GetLevel())
			return false;

		int c = strcmp(g1->GetName(), g2->GetName());
		if (c>0) 
			return true;
		return false;
	}
};


void CGuildRankingExtern::SendDateGuildRanking(LPCHARACTER ch)
{
	SendGuildRankingInfoPackets(ch,RANKING_GUILD_SUB_HEADER_DATES_CLEAR);

	
	guild_list.clear();

	std::map<DWORD, CGuild*> m_mapGuild = CGuildManager::instance().GetGuildInfo();

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second){
			guild_list.push_back(it->second);
		}
	}

	std::sort(guild_list.begin(), guild_list.end(), FGuildCompare());

	if (guild_list.size() != 0){
		SendGuildRankingInfoPackets(ch,RANKING_GUILD_SUB_HEADER_DATES_SEND);
	}

}
int CGuildRankingExtern::GetEmpireGuild(DWORD id)
{
	char szQuery[1024];

	DWORD id_player_receive = 0;

	snprintf(szQuery, sizeof(szQuery), "SELECT empire FROM %splayer_index INNER JOIN player.player ON player.player.account_id = player.player_index.id WHERE player.player.id = %d ",get_table_postfix(),id);

	std::auto_ptr<SQLMsg> pMsg4(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg4->Get()->uiNumRows > 0){
		MYSQL_ROW  row = mysql_fetch_row(pMsg4->Get()->pSQLResult);
		str_to_number(id_player_receive, row[0]);
	}
	
	return id_player_receive;
}

void CGuildRankingExtern::SendGuildRankingInfoPackets(LPCHARACTER ch, BYTE subheader)
{

	TPacketGCGuildRankingReceive pack;
	pack.bSubType = subheader;

	if(subheader == RANKING_GUILD_SUB_HEADER_DATES_SEND)
	{
		int size_list = guild_list.size();

		int n = size_list;
		if (n > MAX_GUILDS){
			n = MAX_GUILDS;
		}

		for (int i = 0; i < n; ++i)
		{
			CGuild * g = guild_list[size_list - i - 1];
			if (!g)
				continue;

			pack.date_ranking.index 		= i;
			memcpy (pack.date_ranking.name, 		g->GetName(), 		sizeof(pack.date_ranking.name));
			pack.date_ranking.reino 		= 	GetEmpireGuild(g->GetMasterPID());
			pack.date_ranking.lv 			= 	g->GetLevel();
			pack.date_ranking.wins 			= 	g->GetGuildWarWinCount();
			pack.date_ranking.defeats 		= 	g->GetGuildWarLossCount();
			pack.date_ranking.draw			= 	g->GetGuildWarDrawCount();

			LPDESC d = ch->GetDesc();

			if (NULL == d)
			{
				sys_err ("User CHARACTER::SendGuildRankingInfoPackets RANKING_GUILD_SUB_HEADER_DATES_SEND (%s)'s DESC is NULL POINT.", ch->GetName());
				return ;
			}

			d->Packet(&pack, sizeof(pack));
		}

	}
	
	else{
		LPDESC d = ch->GetDesc();

		if (NULL == d)
		{
			sys_err ("User CHARACTER::SendGuildRankingInfoPackets (%s)'s DESC is NULL POINT.", ch->GetName());
			return ;
		}

		d->Packet(&pack, sizeof(pack));

	}
	
}