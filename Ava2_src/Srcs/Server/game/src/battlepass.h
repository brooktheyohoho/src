#include <boost/unordered_map.hpp>
#include "../../common/stl.h"
#include "../../common/tables.h"
#include "../../common/length.h"


class CBattlePass : public singleton<CBattlePass>
{

	public:
		struct battlepass_values_items
		{
			DWORD	vnum;
			int		count;

			bool operator == (const battlepass_values_items& b)
			{
				return (this->count == b.count) && (this->vnum == b.vnum);
			}
		};

		struct battlepass_data
		{

			DWORD 	type;
			DWORD 	vnum;
			int 	count;
			std::vector<battlepass_values_items>	item;

			battlepass_data()
			{
				vnum = 0;
			}

		};


		CBattlePass();
		virtual ~CBattlePass();


		void	LoadBattlePass();
		bool 	CheckMisionExist(DWORD type);
		DWORD 	GetTypeMision(DWORD index);
		int 	GetCountMision(DWORD index);
		int 	GetMisionData(LPCHARACTER ch, DWORD index);
		void 	SetMisionData(LPCHARACTER ch , DWORD index, int count);
		void 	UpdateMisionBattlePass(LPCHARACTER ch, DWORD type, int count);
		void 	SendBattlePassPackets(LPCHARACTER ch, BYTE subheader);
		void 	ResetMisionBattlePass(LPCHARACTER ch);
		void 	Open(LPCHARACTER ch);
		void 	UpdateMisionBattlePassExtra(LPCHARACTER ch, DWORD type ,DWORD vnum, int count);
		DWORD 	GetVnumMision(DWORD index);
		void 	Reward(LPCHARACTER ch, int index);
		int 	GetMisionDataReward(LPCHARACTER ch, DWORD index);
		void 	SetMisionDataReward(LPCHARACTER ch , DWORD index, int count);
		void	SetUpdateCountMision(LPCHARACTER ch);

	public:
		std::vector<battlepass_data*>	battlepass_datas;


};