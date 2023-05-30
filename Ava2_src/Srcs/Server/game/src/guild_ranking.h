#include <boost/unordered_map.hpp>
#include "../../common/stl.h"
#include "../../common/tables.h"

class CGuildRanking
{
	public:
		CGuildRanking ();
		virtual ~CGuildRanking ();
};


class CGuildRankingExtern : public singleton<CGuildRankingExtern>
{
	enum Config{

		MAX_GUILDS = 10
	};

	public:
		CGuildRankingExtern ();
		virtual ~CGuildRankingExtern ();

		void 	Open(LPCHARACTER ch);
		void 	SendDateGuildRanking(LPCHARACTER ch);
		void 	SendGuildRankingInfoPackets(LPCHARACTER ch, BYTE subheader);
		int 	GetEmpireGuild(DWORD id);
		int		GetPositionMeGuild(LPCHARACTER ch);

		void	SetkillsGuildRanking(LPCHARACTER killer,LPCHARACTER victim);
		void 	LoadDatesGuildRankingPlayer();

	private:
		std::vector<SFDateGuildRankingPlayerInfo> date_guild_ranking_player;

};

