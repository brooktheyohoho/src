#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "locale_service.h"
#include "../../common/length.h"
#include "exchange.h"
#include "DragonSoul.h"
#include "questmanager.h" // @fixme150
#if defined(WJ_COMBAT_ZONE)	
	#include "combat_zone.h"
#endif
#ifdef ENABLE_EXCHANGE_LOG
#include "quest.h"
#include "questmanager.h"
#endif

void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, long long arg1, TItemPos arg2, DWORD arg3, void * pvData = NULL);

// ��ȯ ��Ŷ
void exchange_packet(LPCHARACTER ch, BYTE sub_header, bool is_me, long long arg1, TItemPos arg2, DWORD arg3, void * pvData)
{
	if (!ch->GetDesc())
		return;

	struct packet_exchange pack_exchg;

	pack_exchg.header 		= HEADER_GC_EXCHANGE;
	pack_exchg.sub_header 	= sub_header;
	pack_exchg.is_me		= is_me;
	pack_exchg.arg1		= arg1;
	pack_exchg.arg2		= arg2;
	pack_exchg.arg3		= arg3;

	if (sub_header == EXCHANGE_SUBHEADER_GC_ITEM_ADD && pvData)
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(((LPITEM) pvData)->GetWindow(), ((LPITEM) pvData)->GetCell());
#endif
#ifdef ELEMENT_SPELL_WORLDARD
		pack_exchg.grade_element = ((LPITEM) pvData)->GetElementGrade();
		pack_exchg.element_type_bonus = ((LPITEM) pvData)->GetElementsType();
		thecore_memcpy(&pack_exchg.attack_element, ((LPITEM) pvData)->GetElementAttacks(), sizeof(pack_exchg.attack_element));
		thecore_memcpy(&pack_exchg.elements_value_bonus, ((LPITEM) pvData)->GetElementsValues(), sizeof(pack_exchg.elements_value_bonus));
#endif
		thecore_memcpy(&pack_exchg.alSockets, ((LPITEM) pvData)->GetSockets(), sizeof(pack_exchg.alSockets));
		thecore_memcpy(&pack_exchg.aAttr, ((LPITEM) pvData)->GetAttributes(), sizeof(pack_exchg.aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
		thecore_memcpy(&pack_exchg.randomAttr, ((LPITEM) pvData)->GetRandomAttributes(), sizeof(pack_exchg.randomAttr));
#endif

#ifdef ENABLE_NEW_NAME_ITEM
		strlcpy(pack_exchg.name,((LPITEM) pvData)->GetNewName(),sizeof(pack_exchg.name));
#endif

	}
	else
	{
#ifdef WJ_ENABLE_TRADABLE_ICON
		pack_exchg.arg4 = TItemPos(RESERVED_WINDOW, 0);
#endif
#ifdef ELEMENT_SPELL_WORLDARD
		pack_exchg.grade_element = 0;
		pack_exchg.element_type_bonus = 0;
		memset(&pack_exchg.attack_element, 0, sizeof(pack_exchg.attack_element));
		memset(&pack_exchg.elements_value_bonus, 0, sizeof(pack_exchg.elements_value_bonus));
#endif	
		memset(&pack_exchg.alSockets, 0, sizeof(pack_exchg.alSockets));
		memset(&pack_exchg.aAttr, 0, sizeof(pack_exchg.aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
		memset(&pack_exchg.randomAttr, 0, sizeof(pack_exchg.randomAttr));
#endif

#ifdef ENABLE_NEW_NAME_ITEM
		strlcpy(pack_exchg.name,"^",sizeof(pack_exchg.name));
#endif
	}

	ch->GetDesc()->Packet(&pack_exchg, sizeof(pack_exchg));
}

// ��ȯ�� ����
bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	if (this == victim)	// �ڱ� �ڽŰ��� ��ȯ�� ���Ѵ�.
		return false;

	if (IsObserverMode())
	{
		ChatPacket(CHAT_TYPE_INFO, "655");
		return false;
	}

	if (victim->IsNPC())
		return false;

#ifdef ENABLE_PREVENT_FLOOD_PACKETS
    const int iPulse = thecore_pulse();
    if (iPulse - GetLastExchangeTime() < PASSES_PER_SEC(3)) // 3 sec
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[1554]Please try again later."));
        return false;
    }

    if (iPulse - victim->GetLastExchangeTime() < PASSES_PER_SEC(3)) // 3 sec
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[1554]Please try again later."));
        return false;
    }
#endif


#if defined(WJ_COMBAT_ZONE)	
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()) || CCombatZoneManager::Instance().IsCombatZoneMap(victim->GetMapIndex()))
		return false;
#endif


	//Fix Revive Time
	if (victim->IsDead() || IsDead())
		return false;
	
	//PREVENT_TRADE_WINDOW
	if ( IsOpenSafebox() || GetShop() || IsCubeOpen()
#if defined(__BL_67_ATTR__)
		|| Is67AttrOpen()
#endif
		)
	{
		ChatPacket( CHAT_TYPE_INFO, "656" );
		return false;
	}

	if ( victim->IsOpenSafebox() || victim->GetShop() || victim->IsCubeOpen()
#if defined(__BL_67_ATTR__)
		|| victim->Is67AttrOpen()
#endif
		)
	{
		ChatPacket( CHAT_TYPE_INFO, "657" );
		return false;
	}
	//END_PREVENT_TRADE_WINDOW
	int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	// �Ÿ� üũ
	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		exchange_packet(this, EXCHANGE_SUBHEADER_GC_ALREADY, 0, 0, NPOS, 0);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		ChatPacket(CHAT_TYPE_INFO, "658");
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	//
	SetExchangeTime();
	victim->SetExchangeTime();

	exchange_packet(victim, EXCHANGE_SUBHEADER_GC_START, 0, GetVID(), NPOS, 0);
	exchange_packet(this, EXCHANGE_SUBHEADER_GC_START, 0, victim->GetVID(), NPOS, 0);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

#ifdef ENABLE_EXCHANGE_LOG
	logIndex = -1;
#endif
	m_lGold = 0;

	m_pOwner = pOwner;
	pOwner->SetExchange(this);

	m_pGrid = M2_NEW CGrid(4,3);
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos)
{
	assert(m_pOwner != NULL && GetCompany());

	if (!item_pos.IsValidItemPosition())
		return false;

	// ���� ��ȯ�� �� ����
	if (item_pos.IsEquipPosition())
		return false;

	LPITEM item;

	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "659");
		return false;
	}

	if (true == item->isLocked())
	{
		return false;
	}

	// �̹� ��ȯâ�� �߰��� �������ΰ�?
	if (item->IsExchanging())
	{
		sys_log(0, "EXCHANGE under exchanging");
		return false;
	}

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		sys_log(0, "EXCHANGE not empty item_pos %d %d %d", display_pos, 1, item->GetSize());
		return false;
	}
#ifdef __SOULBINDING_SYSTEM__
	if (item->IsBind() || item->IsUntilBind())
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "You can't exchange this item because is binded!");
		return false;
	}
#endif
	Accept(false);
	GetCompany()->Accept(false);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i]		= item;
		m_aItemPos[i]		= item_pos;
		m_abItemDisplayPos[i]	= display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		exchange_packet(m_pOwner,
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				true,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		exchange_packet(GetCompany()->GetOwner(),
				EXCHANGE_SUBHEADER_GC_ITEM_ADD,
				false,
				item->GetVnum(),
				TItemPos(RESERVED_WINDOW, display_pos),
				item->GetCount(),
				item);

		sys_log(0, "EXCHANGE AddItem success %s pos(%d, %d) %d", item->GetName(), item_pos.window_type, item_pos.cell, display_pos);

		return true;
	}

	// �߰��� ������ ����
	return false;
}

bool CExchange::RemoveItem(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	exchange_packet(GetOwner(),	EXCHANGE_SUBHEADER_GC_ITEM_DEL, true, pos, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ITEM_DEL, false, pos, PosOfInventory, 0);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos]	    = NULL;
	m_aItemPos[pos]	    = NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

bool CExchange::AddGold(long long gold)
{
	if (gold <= 0)
		return false;

	if (GetOwner()->GetGold() < gold)
	{
		// ������ �ִ� ���� ����.
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_LESS_GOLD, 0, 0, NPOS, 0);
		return false;
	}

	if (m_lGold > 0)
		return false;

	Accept(false);
	GetCompany()->Accept(false);

	m_lGold = gold;

	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, true, m_lGold, NPOS, 0);
	exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_GOLD_ADD, false, m_lGold, NPOS, 0);
	return true;
}

// ���� ����� �ִ���, ��ȯ�Ϸ��� �������� ������ �ִ��� Ȯ�� �Ѵ�.
bool CExchange::Check(int * piItemCount)
{
	if (GetOwner()->GetGold() < m_lGold)
		return false;

	int item_count = 0;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

#ifdef ENABLE_SPECIAL_STORAGE
bool CExchange::CheckSpecialStorageSpace(BYTE bStorageType)
{
	static CGrid s_grid1(SPECIAL_INVENTORY_PAGE_COLUMN, SPECIAL_INVENTORY_PAGE_ROW);
	static CGrid s_grid2(SPECIAL_INVENTORY_PAGE_COLUMN, SPECIAL_INVENTORY_PAGE_ROW);
	static CGrid s_grid3(SPECIAL_INVENTORY_PAGE_COLUMN, SPECIAL_INVENTORY_PAGE_ROW);
	static CGrid s_grid4(SPECIAL_INVENTORY_PAGE_COLUMN, SPECIAL_INVENTORY_PAGE_ROW);
	//static CGrid s_grid5(SPECIAL_INVENTORY_PAGE_COLUMN, SPECIAL_INVENTORY_PAGE_ROW);
	s_grid1.Clear();
	s_grid2.Clear();
	s_grid3.Clear();
	s_grid4.Clear();
	//s_grid5.Clear();

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;

	int i;

	for (i = 0; i < SPECIAL_INVENTORY_PAGE_SIZE*1; ++i)
	{
		if(bStorageType == 0)
			item = victim->GetUpgradeInventoryItem(i);
		else if(bStorageType == 1)
			item = victim->GetBookInventoryItem(i);
		else if(bStorageType == 2)
			item = victim->GetStoneInventoryItem(i);
		else if(bStorageType == 3)
			item = victim->GetChangeInventoryItem(i);
		else if(bStorageType == 4)
			item = victim->GetCostumeInventoryItem(i);
		if(!item)
			continue;

		s_grid1.Put(i, 1, item->GetSize());
	}

	for (i = SPECIAL_INVENTORY_PAGE_SIZE*1; i < SPECIAL_INVENTORY_PAGE_SIZE*2; ++i)
	{
		if(bStorageType == 0)
			item = victim->GetUpgradeInventoryItem(i);
		else if(bStorageType == 1)
			item = victim->GetBookInventoryItem(i);
		else if(bStorageType == 2)
			item = victim->GetStoneInventoryItem(i);
		else if(bStorageType == 3)
			item = victim->GetChangeInventoryItem(i);
		else if(bStorageType == 4)
			item = victim->GetCostumeInventoryItem(i);
		if(!item)
			continue;

		s_grid2.Put(i - SPECIAL_INVENTORY_PAGE_SIZE*1, 1, item->GetSize());
	}

	for (i = SPECIAL_INVENTORY_PAGE_SIZE*2; i < SPECIAL_INVENTORY_PAGE_SIZE*3; ++i)
	{
		if (i >= 90+(5*victim->GetUnlockSlotsW(bStorageType+1)))
		{
			s_grid3.Put(i - SPECIAL_INVENTORY_PAGE_SIZE*2, 1, 1);
			continue;
		}

		if(bStorageType == 0)
			item = victim->GetUpgradeInventoryItem(i);
		else if(bStorageType == 1)
			item = victim->GetBookInventoryItem(i);
		else if(bStorageType == 2)
			item = victim->GetStoneInventoryItem(i);
		else if(bStorageType == 3)
			item = victim->GetChangeInventoryItem(i);
		else if(bStorageType == 4)
			item = victim->GetCostumeInventoryItem(i);
		if(!item)
			continue;

		s_grid3.Put(i - SPECIAL_INVENTORY_PAGE_SIZE*2, 1, item->GetSize());
	}

	for (i = SPECIAL_INVENTORY_PAGE_SIZE*3; i < SPECIAL_INVENTORY_PAGE_SIZE*4; ++i)
	{
		if (i >= 90+(5*victim->GetUnlockSlotsW(bStorageType+1)))
		{
			s_grid4.Put(i - SPECIAL_INVENTORY_PAGE_SIZE*3, 1, 1);
			continue;
		}

		if(bStorageType == 0)
			item = victim->GetUpgradeInventoryItem(i);
		else if(bStorageType == 1)
			item = victim->GetBookInventoryItem(i);
		else if(bStorageType == 2)
			item = victim->GetStoneInventoryItem(i);
		else if(bStorageType == 3)
			item = victim->GetChangeInventoryItem(i);
		else if(bStorageType == 4)
			item = victim->GetCostumeInventoryItem(i);
		if(!item)
			continue;

		s_grid4.Put(i - SPECIAL_INVENTORY_PAGE_SIZE*3, 1, item->GetSize());
	}
	
	/*
	for (i = SPECIAL_INVENTORY_PAGE_SIZE*4; i < SPECIAL_INVENTORY_PAGE_SIZE*5; ++i)
	{
		if(bStorageType == 0)
			item = victim->GetUpgradeInventoryItem(i);
		else if(bStorageType == 1)
			item = victim->GetBookInventoryItem(i);
		else if(bStorageType == 2)
			item = victim->GetStoneInventoryItem(i);
		else if(bStorageType == 3)
			item = victim->GetChangeInventoryItem(i);
		else if(bStorageType == 4)
			item = victim->GetCostumeInventoryItem(i);
		if(!item)
			continue;
		s_grid5.Put(i - SPECIAL_INVENTORY_PAGE_SIZE*4, 1, item->GetSize());
	}
	*/


	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;
#ifdef UNLOCK_INVENTORY_WORLDARD
		int slots;
		if(item->IsUpgradeItem())
			slots = victim->GetEmptyUpgradeInventory(item);
		else if(item->IsBook())
			slots = victim->GetEmptyBookInventory(item);
		else if(item->IsStone())
			slots = victim->GetEmptyStoneInventory(item);
		else if(item->IsChange())
			slots = victim->GetEmptyChangeInventory(item);
		else if(item->IsCostume())
			slots = victim->GetEmptyCostumeInventory(item);
		if (slots < 0)
			return false;
#endif

		if(bStorageType < 0 || bStorageType > 4)
			continue;
		if((bStorageType == 0 && (!item->IsUpgradeItem())) || (bStorageType == 1 && (!item->IsBook())) || (bStorageType == 2 && (!item->IsStone())) || (bStorageType == 3 && (!item->IsChange())) || (bStorageType == 4 && (!item->IsCostume())))
			continue;
		int iPos;
		if ((iPos = s_grid1.FindBlank(1, item->GetSize())) >= 0)
			s_grid1.Put(iPos, 1, item->GetSize());
		else if((iPos = s_grid2.FindBlank(1, item->GetSize())) >= 0)
			s_grid2.Put(iPos, 1, item->GetSize());
		else if((iPos = s_grid3.FindBlank(1, item->GetSize())) >= 0)
			s_grid3.Put(iPos, 1, item->GetSize());
		else if((iPos = s_grid4.FindBlank(1, item->GetSize())) >= 0)
			s_grid4.Put(iPos, 1, item->GetSize());
		//else if((iPos = s_grid5.FindBlank(1, item->GetSize())) >= 0)
		//	s_grid5.Put(iPos, 1, item->GetSize());
		else{
			return false;
		}
	}

	return true;
}
#endif


bool CExchange::CheckSpace()
{
	static CGrid s_grid1(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 1
	static CGrid s_grid2(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 2
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	static CGrid s_grid3(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 3
	static CGrid s_grid4(INVENTORY_PAGE_COLUMN, INVENTORY_PAGE_ROW); // inven page 4
#endif

	s_grid1.Clear();
	s_grid2.Clear();
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	s_grid3.Clear();
	s_grid4.Clear();
#endif

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;
	int i;
	for (i = 0; i < INVENTORY_PAGE_SIZE*1; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;
		s_grid1.Put(i, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE*1; i < INVENTORY_PAGE_SIZE*2; ++i)
	{
		if (!(item = victim->GetInventoryItem(i)))
			continue;
		s_grid2.Put(i - INVENTORY_PAGE_SIZE*1, 1, item->GetSize());
	}
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
	for (i = INVENTORY_PAGE_SIZE*2; i < INVENTORY_PAGE_SIZE*3; ++i)
	{
		if (i >= 90+(5*victim->GetUnlockSlotsW()))
		{
			s_grid3.Put(i - INVENTORY_PAGE_SIZE*2, 1, 1);
			continue;
		}
		if (!(item = victim->GetInventoryItem(i)))
			continue;
		s_grid3.Put(i - INVENTORY_PAGE_SIZE*2, 1, item->GetSize());
	}
	for (i = INVENTORY_PAGE_SIZE*3; i < INVENTORY_PAGE_SIZE*4; ++i)
	{
		if (i >= 90+(5*victim->GetUnlockSlotsW()))
		{
			s_grid4.Put(i - INVENTORY_PAGE_SIZE*3, 1, 1);
			continue;
		}
		if (!(item = victim->GetInventoryItem(i)))
			continue;
		s_grid4.Put(i - INVENTORY_PAGE_SIZE*3, 1, item->GetSize());
	}
#endif

	// ��... ���� ������ ������... ��ȥ�� �κ��� ��� �κ� ���� ���� ���� �� �߸��̴� �Ф�
	static std::vector <WORD> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);

	// �ϴ� ��ȥ���� ��ȯ���� ���� ���ɼ��� ũ�Ƿ�, ��ȥ�� �κ� ����� ��ȥ���� ���� �� �ϵ��� �Ѵ�.
	bool bDSInitialized = false;

	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;
		#ifdef UNLOCK_INVENTORY_WORLDARD
		int slots;
		if (item->IsDragonSoul())
			slots = victim->GetEmptyDragonSoulInventory(item);
		else
			slots = victim->GetEmptyInventory(item->GetSize());

		if (slots < 0)
			return false;
		#endif
		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}
			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			WORD wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
			{
				WORD wPos = wBasePos + i;
				if (0 == s_vDSGrid[wPos])
				{
					bool bEmpty = true;
					for (int j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] =  wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
				return false;
		}
		else
		{
			int iPos;
			if ((iPos = s_grid1.FindBlank(1, item->GetSize())) >= 0)
				s_grid1.Put(iPos, 1, item->GetSize());
			else if ((iPos = s_grid2.FindBlank(1, item->GetSize())) >= 0)
				s_grid2.Put(iPos, 1, item->GetSize());
#ifdef ENABLE_EXTEND_INVEN_SYSTEM
			else if ((iPos = s_grid3.FindBlank(1, item->GetSize())) >= 0)
				s_grid3.Put(iPos, 1, item->GetSize());
			else if ((iPos = s_grid4.FindBlank(1, item->GetSize())) >= 0)
				s_grid4.Put(iPos, 1, item->GetSize());
#endif
			else
				return false;
		}
	}

	return true;
}

// ��ȯ �� (�����۰� �� ���� ������ �ű��)
bool CExchange::Done()
{
	int		empty_pos, i;
	LPITEM	item;

	LPCHARACTER	victim = GetCompany()->GetOwner();
#ifdef ENABLE_EXCHANGE_LOG
	char szQuery[QUERY_MAX_LEN];
#endif
	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		if (item->IsDragonSoul())
			empty_pos = victim->GetEmptyDragonSoulInventory(item);
#ifdef ENABLE_SPECIAL_STORAGE
		else if(item->IsUpgradeItem())
			empty_pos = victim->GetEmptyUpgradeInventory(item);
		else if(item->IsBook())
			empty_pos = victim->GetEmptyBookInventory(item);
		else if(item->IsStone())
			empty_pos = victim->GetEmptyStoneInventory(item);
		else if(item->IsChange())
			empty_pos = victim->GetEmptyChangeInventory(item);
		else if(item->IsCostume())
			empty_pos = victim->GetEmptyCostumeInventory(item);
#endif
		else
			empty_pos = victim->GetEmptyInventory(item->GetSize());

		if (empty_pos < 0)
		{
			sys_err("Exchange::Done : Cannot find blank position in inventory %s <-> %s item %s",
					m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);

#ifdef ENABLE_EXCHANGE_LOG
		int iLen = snprintf(szQuery, sizeof(szQuery), "INSERT INTO log.exchange_log_items(id, pid, item_id, pos, vnum, count");
		for (BYTE j=0;j< ITEM_SOCKET_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", socket%u",j);
		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", attrtype%u, attrvalue%u", j, j);
		iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ") VALUES(%d, %u, %u, %d, %d, %d", logIndex, GetOwner()->GetPlayerID(), item->GetID(), m_abItemDisplayPos[i], item->GetVnum(), item->GetCount());
		for (BYTE j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", %ld", item->GetSocket(j));
		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
			iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", %u, %d", item->GetAttributeType(j), item->GetAttributeValue(j));
		iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ")");
		std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));
#endif

		if (item->GetVnum() == 90008 || item->GetVnum() == 90009) // VCARD
		{
			VCardUse(m_pOwner, victim, item);
			continue;
		}

		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);

		item->RemoveFromCharacter();
		if (item->IsDragonSoul())
			item->AddToCharacter(victim, TItemPos(DRAGON_SOUL_INVENTORY, empty_pos));
#ifdef ENABLE_SPECIAL_STORAGE
		else if(item->IsUpgradeItem())
			item->AddToCharacter(victim, TItemPos(UPGRADE_INVENTORY, empty_pos));
		else if(item->IsBook())
			item->AddToCharacter(victim, TItemPos(BOOK_INVENTORY, empty_pos));
		else if(item->IsStone())
			item->AddToCharacter(victim, TItemPos(STONE_INVENTORY, empty_pos));
		else if(item->IsChange())
			item->AddToCharacter(victim, TItemPos(CHANGE_INVENTORY, empty_pos));
		else if(item->IsCostume())
			item->AddToCharacter(victim, TItemPos(COSTUME_INVENTORY, empty_pos));
#endif
		else
			item->AddToCharacter(victim, TItemPos(INVENTORY, empty_pos));
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		item->SetExchanging(false);
		{
			char exchange_buf[51];

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), GetOwner()->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(victim, item, "EXCHANGE_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), victim->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(GetOwner(), item, "EXCHANGE_GIVE", exchange_buf);

			if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
			{
				LogManager::instance().GoldBarLog(victim->GetPlayerID(), item->GetID(), EXCHANGE_TAKE, "");
				LogManager::instance().GoldBarLog(GetOwner()->GetPlayerID(), item->GetID(), EXCHANGE_GIVE, "");
			}
#ifdef EXCHANGE_LOG
			bool firstPlayer = true;
			if (GetCompany()){
				firstPlayer = false;
			}
			DWORD tradeID = LogManager::instance().ExchangeLog(1, GetOwner()->GetPlayerID(), victim->GetPlayerID(), GetOwner()->GetX(), GetOwner()->GetY(), m_lGold, GetCompany()->m_lGold);
			LogManager::instance().ExchangeItemLog(tradeID, item, firstPlayer ? "A" : "B", empty_pos);
#endif
		}

		m_apItems[i] = NULL;
	}

	if (m_lGold)
	{
		GetOwner()->PointChange(POINT_GOLD, -m_lGold, true);
		victim->PointChange(POINT_GOLD, m_lGold, true);

		if (m_lGold > 1000)
		{
			char exchange_buf[51];
			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", GetOwner()->GetPlayerID(), GetOwner()->GetName());
			LogManager::instance().CharLog(victim, m_lGold, "EXCHANGE_GOLD_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s", victim->GetPlayerID(), victim->GetName());
			LogManager::instance().CharLog(GetOwner(), m_lGold, "EXCHANGE_GOLD_GIVE", exchange_buf);
		}
	}
	
#ifdef ENABLE_PREVENT_FLOOD_PACKETS
    GetOwner()->SetLastExchangeTime();
    victim->SetLastExchangeTime();
#endif	

	m_pGrid->Clear();
	return true;
}

// ��ȯ�� ����
bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	// �� �� ���� �����Ƿ� ��ȯ ����
	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int	iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		//PREVENT_PORTAL_AFTER_EXCHANGE
		GetOwner()->SetExchangeTime();
		victim->SetExchangeTime();
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		// @fixme150 BEGIN
		if (quest::CQuestManager::instance().GetPCForce(GetOwner()->GetPlayerID())->IsRunning() == true)
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "660");
			victim->ChatPacket(CHAT_TYPE_INFO, "661");
			goto EXCHANGE_END;
		}
		else if (quest::CQuestManager::instance().GetPCForce(victim->GetPlayerID())->IsRunning() == true)
		{
			victim->ChatPacket(CHAT_TYPE_INFO, "660");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "661");
			goto EXCHANGE_END;
		}
		// @fixme150 END

		// exchange_check ������ ��ȯ�� �����۵��� ���ڸ��� �ֳ� Ȯ���ϰ�,
		// ��ũ�� ����� �ֳ� Ȯ���Ѵ�, �ι�° ���ڷ� ��ȯ�� ������ ����
		// �� �����Ѵ�.
		if (!Check(&iItemCount))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "630");
			victim->ChatPacket(CHAT_TYPE_INFO, "662");
			goto EXCHANGE_END;
		}

		// ���� ���� ������ ������ ������ ����ǰ�� ���� �ڸ��� �ֳ� Ȯ���Ѵ�.
		if (!CheckSpace())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "663");
			victim->ChatPacket(CHAT_TYPE_INFO, "638");
			goto EXCHANGE_END;
		}

		// ���浵 ����������..
		if (!GetCompany()->Check(&iItemCount))
		{
			victim->ChatPacket(CHAT_TYPE_INFO, "630");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "662");
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->ChatPacket(CHAT_TYPE_INFO, "663");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "638");
			goto EXCHANGE_END;
		}

#ifdef ENABLE_SPECIAL_STORAGE
		for(int i = 0; i < 5; i++)
		{
			if (!CheckSpecialStorageSpace(i))
			{
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, "663");
				victim->ChatPacket(CHAT_TYPE_INFO, "638");
				goto EXCHANGE_END;
			}
			
			if (!GetCompany()->CheckSpecialStorageSpace(i))
			{
				victim->ChatPacket(CHAT_TYPE_INFO, "663");
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, "638");
				goto EXCHANGE_END;
			}
		}
#endif
		
		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

		{
#ifdef ENABLE_EXCHANGE_LOG
			TExchangeLog p;
			strlcpy(p.owner, victim->GetName(), sizeof(p.owner));
			p.ownerPID = victim->GetPlayerID();
			p.ownerGold = GetCompany()->m_lGold;
			strlcpy(p.ownerIP, victim->GetDesc()->GetHostName(), sizeof(p.ownerIP));

			strlcpy(p.target, GetOwner()->GetName(), sizeof(p.target));
			p.targetPID = GetOwner()->GetPlayerID();
			p.targetGold = m_lGold;
			strlcpy(p.targetIP, GetOwner()->GetDesc()->GetHostName(), sizeof(p.targetIP));
			p.itemsLoaded = false;

			const time_t curr_time = time(0);
			const tm* curr_tm = localtime(&curr_time);
			strftime(p.date, 50, "%T - %d/%m/%y", curr_tm);

			//const bool isGameMasterExchange = (GetOwner()->IsGm() || victim->IsGm()) ? true: false;
			const bool isGameMasterExchange = false;//SHOW GAMEMASTER EXCHANGE IN GAME

			const int newLogID = quest::CQuestManager::instance().GetEventFlag("ex_log_index")+1;
			char szQuery[QUERY_MAX_LEN];
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO log.exchange_log(id, owner, owner_pid, owner_gold, owner_ip, target, target_pid, target_gold, target_ip, date, owner_delete, target_delete) VALUES(%d, '%s', %u, %lld, '%s', '%s', %u, %lld, '%s', '%s', %d, %d)", newLogID, p.owner, p.ownerPID, p.ownerGold, p.ownerIP, p.target, p.targetPID, p.targetGold, p.targetIP, p.date, isGameMasterExchange, isGameMasterExchange);
			delete DBManager::instance().DirectQuery(szQuery);
			quest::CQuestManager::instance().RequestSetEventFlag("ex_log_index", newLogID);
			logIndex = newLogID;
			GetCompany()->logIndex = newLogID;
			if(!isGameMasterExchange)
			{
				if (GetOwner()->GetProtectTime("ExchangeLogLoaded") == 1)
					GetOwner()->SendExchangeLogPacket(SUB_EXCHANGELOG_LOAD, newLogID, &p);
				if (victim->GetProtectTime("ExchangeLogLoaded") == 1)
					victim->SendExchangeLogPacket(SUB_EXCHANGELOG_LOAD, newLogID, &p);
			}
#endif
			if (Done())
			{
				if (m_lGold) // ���� ���� ���� ����
					GetOwner()->Save();

				if (GetCompany()->Done())
				{
					if (GetCompany()->m_lGold) // ���� ���� ���� ����
						victim->Save();

					// INTERNATIONAL_VERSION
					GetOwner()->ChatPacket(CHAT_TYPE_INFO, "664 %s", victim->GetName());
					victim->ChatPacket(CHAT_TYPE_INFO, "664 %s", GetOwner()->GetName());
					// END_OF_INTERNATIONAL_VERSION
				}
			}
		}

EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		// �ƴϸ� accept�� ���� ��Ŷ�� ������.
		exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, true, m_bAccept, NPOS, 0);
		exchange_packet(GetCompany()->GetOwner(), EXCHANGE_SUBHEADER_GC_ACCEPT, false, m_bAccept, NPOS, 0);
		return true;
	}
}

// ��ȯ ���
void CExchange::Cancel()
{
	exchange_packet(GetOwner(), EXCHANGE_SUBHEADER_GC_END, 0, 0, NPOS, 0);
	GetOwner()->SetExchange(NULL);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE(this);
}

