#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "shop.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "locale_service.h"

CShop::CShop()
	: m_dwVnum(0), m_dwNPCVnum(0)
{
	m_pGrid = M2_NEW CGrid(5, 9);
}

CShop::~CShop()
{
	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	Broadcast(&pack, sizeof(pack));

	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;
		ch->SetShop(NULL);
		++it;
	}

	M2_DELETE(m_pGrid);
}

bool CShop::Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pTable)
{
	/*
	   if (NULL == CMobManager::instance().Get(dwNPCVnum))
	   {
	   sys_err("No such a npc by vnum %d", dwNPCVnum);
	   return false;
	   }
	 */
	sys_log(0, "SHOP #%d (Shopkeeper %d)", dwVnum, dwNPCVnum);
	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;
	BYTE bItemCount;
	for (bItemCount = 0; bItemCount < SHOP_HOST_ITEM_MAX_NUM; ++bItemCount)
		if (0 == (pTable + bItemCount)->vnum)
			break;

	SetShopItems(pTable, bItemCount);
	return true;
}

void CShop::SetShopItems(TShopItemTable * pTable, BYTE bItemCount)
{
	if (bItemCount > SHOP_HOST_ITEM_MAX_NUM)
		return;

	m_pGrid->Clear();

	m_itemVector.resize(SHOP_HOST_ITEM_MAX_NUM);
	memset(&m_itemVector[0], 0, sizeof(SHOP_ITEM) * m_itemVector.size());

	for (int i = 0; i < bItemCount; ++i)
	{
		if (!pTable->vnum)
			continue;
		const TItemTable * item_table = ITEM_MANAGER::instance().GetTable(pTable->vnum);

		if (!item_table)
		{
			sys_err("Shop: no item table by item vnum #%d", pTable->vnum);
			continue;
		}

		int iPos = m_pGrid->FindBlank(1, item_table->bSize);

		if (iPos < 0)
		{
			sys_err("not enough shop window");
			continue;
		}

		if (!m_pGrid->IsEmpty(iPos, 1, item_table->bSize))
		{
			sys_err("not empty position for npc shop");
			continue;
		}

		m_pGrid->Put(iPos, 1, item_table->bSize);

		SHOP_ITEM & item = m_itemVector[iPos];

		item.vnum = pTable->vnum;
		item.count = pTable->count;

#ifdef ENABLE_BUY_ITEMS_WORLDARD
		item.item_vnum_buy = pTable->item_vnum_buy;
		item.item_count_buy = pTable->item_count_buy;
#endif

		if (IS_SET(item_table->dwFlags, ITEM_FLAG_COUNT_PER_1GOLD))
		{
			if (item_table->dwGold == 0)
				item.price = item.count;
			else
				item.price = item.count / item_table->dwGold;
		}
		else
			item.price = item_table->dwGold * item.count;


		char name[36];
		snprintf(name, sizeof(name), "%-20s(#%-5d) (x %d)", item_table->szName, (int) item.vnum, item.count);
		sys_log(0, "SHOP_ITEM: %-36s PRICE %-5d", name, item.price);

		++pTable;
	}
}

int CShop::Buy(LPCHARACTER ch, BYTE pos)
{
	if (pos >= m_itemVector.size())
	{
		sys_log(0, "Shop::Buy : invalid position %d : %s", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	sys_log(0, "Shop::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);
	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;

	SHOP_ITEM& r_item = m_itemVector[pos];

#ifdef ENABLE_BUY_ITEMS_WORLDARD
	if (r_item.price < 0 && r_item.item_vnum_buy == 0)
#else
	if (r_item.price < 0)	
#endif
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

#ifdef FULL_YANG
	long long dwPrice = r_item.price;
#else
	DWORD dwPrice = r_item.price;
#endif

#ifdef ENABLE_BUY_ITEMS_WORLDARD
	DWORD 	item_vnum_buy  = r_item.item_vnum_buy;
	DWORD 	item_count_buy = r_item.item_count_buy;
#endif


#ifdef ENABLE_BUY_ITEMS_WORLDARD
	if (item_vnum_buy == 0)
	{
		if (ch->GetGold() < (long long) dwPrice)
		{
			sys_log(1, "Shop::Buy : Not enough money : %s has %lld, price %lld", ch->GetName(), ch->GetGold(), dwPrice);

			return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
		}
	}
	else
	{
		if (ch->CountSpecifyItemBuy(item_vnum_buy) < (int) item_count_buy){
			ch->ChatPacket(CHAT_TYPE_INFO, "Shop::Buy : Not enough count : %s has %d, price %d", ch->GetName(), ch->CountSpecifyItemBuy(item_vnum_buy), item_count_buy);
			sys_log(1, "Shop::Buy : Not enough count : %s has %d, price %d", ch->GetName(), ch->CountSpecifyItemBuy(item_vnum_buy), item_count_buy);
			return SHOP_SUBHEADER_GC_NOT_ENOUGH_COUNT;
		}
	}
#else
	if (ch->GetGold() < dwPrice)
	{
		sys_log(1, "Shop::Buy : Not enough money : %s has %d, price %d", ch->GetName(), ch->GetGold(), dwPrice);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}
#endif

#ifdef ENABLE_ZODIAC_MISSION
	if((r_item.vnum >= 33001 && r_item.vnum <= 33030) || (r_item.vnum == 33034))
	{
		char text[256];
		snprintf(text,sizeof(text),"Zodiac_Item_%d",r_item.vnum);
		if(ch->GetProtectTime(text) <= 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nesne tukendi."));
			return SHOP_SUBHEADER_GC_NOT_ENOUGH_LUCKY;
		}
		else
		{
			ch->SetProtectTime(text,ch->GetProtectTime(text)-50);
		}
	}
#endif

	LPITEM item = ch->AutoGiveItem(r_item.vnum, r_item.count, -1, true);
	if (!item)
		return SHOP_SUBHEADER_GC_SOLD_OUT;


#ifdef ENABLE_BUY_ITEMS_WORLDARD
	if (item_vnum_buy == 0)
		ch->PointChange(POINT_GOLD, -dwPrice, false);
	else
		ch->RemoveSpecifyItem(item_vnum_buy, item_count_buy);
#else
	ch->PointChange(POINT_GOLD, -dwPrice, false);
#endif

	ITEM_MANAGER::instance().FlushDelayedSave(item);
	LogManager::instance().ItemLog(ch, item, "BUY", item->GetName());

	if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
		LogManager::instance().GoldBarLog(ch->GetPlayerID(), item->GetID(), PERSONAL_SHOP_BUY, "");

	if (item)
	{
#ifdef ENABLE_BUY_ITEMS_WORLDARD
		if (r_item.item_vnum_buy == 0)
			sys_log(0, "SHOP: BUY: name %s %s(x %d):%u price %u", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), dwPrice);
		else
			sys_log(0, "SHOP: BUY: name %s %s(x %d):%u count %u", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), item_count_buy);
#else
		sys_log(0, "SHOP: BUY: name %s %s(x %d):%u price %u", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), dwPrice);
#endif
	}

	ch->Save();
	return (SHOP_SUBHEADER_GC_OK);
}

bool CShop::AddGuest(LPCHARACTER ch, DWORD owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.insert(GuestMapType::value_type(ch, bOtherEmpire));

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_START;

	TPacketGCShopStart pack2;

	memset(&pack2, 0, sizeof(pack2));
	pack2.owner_vid = owner_vid;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];
		pack2.items[i].vnum = item.vnum;
		pack2.items[i].price = item.price;
		pack2.items[i].count = item.count;
#ifdef ENABLE_BUY_ITEMS_WORLDARD
		pack2.items[i].item_vnum_buy = item.item_vnum_buy;
		pack2.items[i].item_count_buy = item.item_count_buy;
#endif
	}
	pack.size = sizeof(pack) + sizeof(pack2);

	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCShopStart));
	return true;
}

void CShop::RemoveGuest(LPCHARACTER ch)
{
	if (ch->GetShop() != this)
		return;

	m_map_guest.erase(ch);
	ch->SetShop(NULL);

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	ch->GetDesc()->Packet(&pack, sizeof(pack));
}

void CShop::Broadcast(const void * data, int bytes)
{
	sys_log(1, "Shop::Broadcast %p %d", data, bytes);
	GuestMapType::iterator it= m_map_guest.begin();
	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;
		if (ch && ch->GetDesc())
			ch->GetDesc()->Packet(data, bytes);
		++it;
	}
}

void CShop::BroadcastUpdateItem(BYTE pos)
{
	TPacketGCShop pack;
	TPacketGCShopUpdateItem pack2;
	memset(&pack2, 0, sizeof(TPacketGCShopUpdateItem));

	TEMP_BUFFER	buf;
	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_UPDATE_ITEM;
	pack.size		= sizeof(pack) + sizeof(pack2);
	pack2.pos		= pos;
	pack2.item.vnum	= m_itemVector[pos].vnum;
	pack2.item.price	= m_itemVector[pos].price;
	pack2.item.count	= m_itemVector[pos].count;
	buf.write(&pack, sizeof(pack));
	buf.write(&pack2, sizeof(pack2));
	Broadcast(buf.read_peek(), buf.size());
}
