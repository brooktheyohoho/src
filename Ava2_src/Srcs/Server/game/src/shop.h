#pragma once

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

class CShop
{
	public:
		typedef struct shop_item
		{
			DWORD	vnum;
#ifdef FULL_YANG
			long long price;
#else
			DWORD price;
#endif
			DWORD	count;
#ifdef ENABLE_BUY_ITEMS_WORLDARD
			DWORD 	item_vnum_buy;
			DWORD 	item_count_buy;
#endif
			shop_item()
			{
				vnum = 0;
				price = 0;
				count = 0;
#ifdef ENABLE_BUY_ITEMS_WORLDARD
				item_vnum_buy = 0;
				item_count_buy = 0;
#endif
			}
		} SHOP_ITEM;

		CShop();
		virtual ~CShop();

		bool	Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pItemTable);
		void	SetShopItems(TShopItemTable * pItemTable, BYTE bItemCount);

		virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
		void	RemoveGuest(LPCHARACTER ch);

		virtual int	Buy(LPCHARACTER ch, BYTE pos);

		void	BroadcastUpdateItem(BYTE pos);

		DWORD	GetVnum() { return m_dwVnum; }
		DWORD	GetNPCVnum() { return m_dwNPCVnum; }

	protected:
		void	Broadcast(const void * data, int bytes);

	protected:
		DWORD				m_dwVnum;
		DWORD				m_dwNPCVnum;
		CGrid *				m_pGrid;
		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;
};
