#pragma once
#ifdef ENABLE_OFFLINESHOP_SYSTEM

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
#include <unordered_map>
#endif

class COfflineShopManager : public singleton<COfflineShopManager>
{
	public:
		COfflineShopManager();
		~COfflineShopManager();
		void			Initialize();

		LPOFFLINESHOP	FindOfflineShopPID(DWORD pid);
		
		void			CompareOffShopEventTime();

		void			OpenMyOfflineShop(LPCHARACTER ch, const char* c_pszSign, TOfflineShopItemTable* pTable, BYTE bItemCount, DWORD shopVnum, BYTE titleType);
		void			CreateOfflineShop(TOfflineShop* offlineshop);
		void			StopShopping(LPCHARACTER ch);
		void			OpenOfflineShop(LPCHARACTER ch);
		void			OpenOfflineShopWithVID(LPCHARACTER ch, DWORD vid);
		bool			HasOfflineShop(LPCHARACTER ch);

		void			Buy(LPCHARACTER ch, DWORD vid, BYTE bPos);
		void			BuyItemReal(TOfflineShopBuy* item);

		void			AddItem(LPCHARACTER ch, BYTE bDisplayPos, TItemPos bPos, long long iPrice);
		void			AddItemReal(OFFLINE_SHOP_ITEM* item);
		
		void			OpenSlot(LPCHARACTER ch, BYTE bPos);
		void			OpenSlotReal(TOfflineShopOpenSlot* ch);

		void			RemoveItem(LPCHARACTER ch, BYTE bPos);
		void			RemoveItemReal(OFFLINE_SHOP_ITEM* item);
		
		void			ShopLogRemove(LPCHARACTER ch);
		void			ShopLogRemoveReal(DWORD ch);

		void			ChangeDecoration(LPCHARACTER ch, TShopDecoration* data);
		void			ChangeDecorationReal(TShopDecoration* ch);

		void			WithdrawMoney(LPCHARACTER ch);
		void			WithdrawMoneyReal(DWORD ch);

		void			DestroyOfflineShop(LPCHARACTER ch);
		void			DestroyOfflineShopReal(DWORD ch);

		void			ChangeTitle(LPCHARACTER ch, const char* title);
		void			ChangeTitleReal(TOfflineShopChangeTitle* p);

		void			CloseOfflineShopForTime(LPOFFLINESHOP offlineshop);
		void			CloseOfflineShopForTimeReal(DWORD offlineshop);

		void			GetBackItem(LPCHARACTER ch);
		void			GetBackItemReal(TOfflineShopBackItem* ch);

		void			ShopAddTime(LPCHARACTER ch);
		void			ShopAddTimeReal(DWORD ch);

		void			RecvPackets(const char * data);
		
		DWORD			FindShopWithName(const char* playerName);

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		void			LoadSearchLanguage();
		bool			CheckItemNames(LPCHARACTER ch, const char* searchInput, std::vector<DWORD>& m_vecItemVnums);
		
		void			ClearItem(DWORD id);
		void			InsertItem(OFFLINE_SHOP_ITEM* p);
		void			SearchItem(LPCHARACTER ch, const char * data);
		void			RemoveInMemory(OFFLINE_SHOP_ITEM* item);
		void			SaveInMemory(OFFLINE_SHOP_ITEM* item);
		bool			getMap(TPacketCGShopSearch* pinfo, std::unordered_map<DWORD, OFFLINE_SHOP_ITEM*>::iterator& it, std::unordered_map<DWORD, OFFLINE_SHOP_ITEM*>::iterator& itend);
#endif


		std::vector<DWORD> 					m_Map_pkShopTimes;

	private:
		LPEVENT								m_pShopTimeEvent;
		
		std::vector<DWORD> 					m_Map_pkOfflineShopCache;
		std::map<DWORD, COfflineShop*> 		m_Map_pkOfflineShopByNPC;
#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMap;
		// sword, two-handed,dagger,bow,claw,fan,bell = 8
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapWeapon;
		// armour,shield,earrings,necklaces,bracelets,shoes,belt,helmets,talisman = 10
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapEquipment;
		// body,hair,weapon,sash = 5
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapCostume;
		// alchemy,beans = 3
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapDragonsoul;
		//std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapConsumables;
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapBook;
		// refinementitems,biologist,quest,energyshards,chests = 5
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapOther;
		// enchant,switcher,advanceditems,stone,rings = 5
		//std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapHelpfulItems[5];
		//std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapOre;
		// namescrolls,skillbook,pets,systempets,egg = 5
		std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapPets;
		//std::unordered_map<DWORD,OFFLINE_SHOP_ITEM*>	m_itemMapMount;
		
		std::map<BYTE, std::vector<std::pair<std::string, DWORD>>>	m_mapItemNames;
#endif
};
#endif

