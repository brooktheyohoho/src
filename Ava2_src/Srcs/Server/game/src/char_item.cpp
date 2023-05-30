#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "BattleArena.h"
#include "arena.h"
#include "dev_log.h"
#include "pcbang.h"
#include "threeway_war.h"
#include "itemblock.h"
#ifdef ENABLE_SWITCHBOT
#include "new_switchbot.h"
#endif
#include "safebox.h"
#include "shop.h"

#ifdef ENABLE_NEWSTUFF
#include "pvp.h"
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
#include "PetSystem.h"
#endif

#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/CommonDefines.h"
//auction_temp
#ifdef __AUCTION__
#include "auction_manager.h"
#endif
const int ITEM_BROKEN_METIN_VNUM = 28960;
#define ENABLE_EFFECT_EXTRAPOT
#define ENABLE_BOOKS_STACKFIX
#if defined(WJ_COMBAT_ZONE)	
	#include "combat_zone.h"
#endif
#ifdef __NEWPET_SYSTEM__
#include "New_PetSystem.h"
#endif

#include "block_change_costume.h"
#ifdef __NEW_EVENTS__
	#include "new_events.h"
#endif

#ifdef ENABLE_MULTI_REFINE_WORLDARD
#include "multi_refine.h"
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	#include "tournament.h"
#endif
#include "MountSystem.h"
#ifdef BATTLEPASS_WORLDARD
#include "battlepass.h"
#endif
// CHANGE_ITEM_ATTRIBUTES
// const DWORD CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
// const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};


//±??¯º?, ±??¯±â¾ïº?, °á?¥¹??ö
static bool IS_SUMMON_ITEM(int vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}

static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int map_index)
{
	// ¸ù?°´ø?ü
	if (IS_MONKEY_DUNGEON(map_index))
		return false;
	// ¼º
	if (IS_CASTLE_MAP(map_index))
		return false;

	switch (map_index)
	{
		case 66 : // »ç±??¸¿ö
		case 71 : // °?¹? ´ø?ü 2?þ
		case 72 : // ?µ?? µ¿±¼
		case 73 : // ?µ?? µ¿±¼ 2?þ
		case 193 : // °?¹? ´ø?ü 2-1?þ
#if 0
		case 184 : // ?µ?? µ¿±¼(½?¼ö)
		case 185 : // ?µ?? µ¿±¼ 2?þ(½?¼ö)
		case 186 : // ?µ?? µ¿±¼(?µ?¶)
		case 187 : // ?µ?? µ¿±¼ 2?þ(?µ?¶)
		case 188 : // ?µ?? µ¿±¼(?ø³ë)
		case 189 : // ?µ?? µ¿±¼ 2?þ(?ø³ë)
#endif
//		case 206 : // ¾?±?µ¿±¼
		case 216 : // ¾?±?µ¿±¼
		case 217 : // °?¹? ´ø?ü 3?þ
		case 208 : // ?µ?? µ¿±¼ (¿ë¹æ)

		case 113 : // OX Event ¸?
			return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;

	// ¸ðµç private ¸??¸·? ¿ö?? º?°¡´?
	if (map_index > 10000) return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int nMapIndex)
{
	if (!g_bEnableBootaryCheck) return true;

	switch (nMapIndex)
	{
		case 1 :
		case 3 :
		case 21 :
		case 23 :
		case 41 :
		case 43 :
			return true;
	}

	return false;
}

// item socket ?? ??·??ä?¸??°ú °°?º?ö ?¼?© -- by mhh
static bool FN_check_item_socket(LPITEM item)
{
#ifdef ENABLE_SORT_INVEN
	if (item->GetVnum() >= 71124 && item->GetVnum() <= 71177)
		return true;

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_AFFECT)
		return true;

#endif
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}

// item socket º¹»ç -- by mhh
static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{
	// ³²?? ±??ö
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}
	// ¿©??±??ö
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

#ifdef ELEMENT_SPELL_WORLDARD
	if (IsOpenElementsSpell())
		return false;
#endif

#if defined(__BL_67_ATTR__)
	if (Is67AttrOpen())
		return false;
#endif

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

	if (IsWarping())
		return false;
	#ifdef __SASH_SYSTEM__
	if ((m_bSashCombination) || (m_bSashAbsorption))
		return false;
	#endif
	return true;
}

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}

#ifdef ENABLE_SPECIAL_STORAGE
LPITEM CHARACTER::GetUpgradeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(UPGRADE_INVENTORY, wCell));
}
LPITEM CHARACTER::GetBookInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(BOOK_INVENTORY, wCell));
}
LPITEM CHARACTER::GetStoneInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(STONE_INVENTORY, wCell));
}
LPITEM CHARACTER::GetChangeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(CHANGE_INVENTORY, wCell));
}
LPITEM CHARACTER::GetCostumeInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(COSTUME_INVENTORY, wCell));
}
#endif

#ifdef CHANGE_EQUIP_WORLDARD 
LPITEM CHARACTER::GetChangeEquipItem(WORD wCell) const
{
	return GetItem(TItemPos(CHANGE_EQUIP,wCell));
}
#endif

LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return NULL;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pDSItems[wCell];
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSU item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pSSUItems[wCell];
	case BOOK_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSB item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pSSBItems[wCell];
	case STONE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSS item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pSSSItems[wCell];
	case CHANGE_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSC item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pSSCItems[wCell];
	case COSTUME_INVENTORY:
		if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid SSC item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pCostumeItems[wCell];
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid switchbot item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pSwitchbotItems[wCell];
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		if(wCell >= CHANGE_EQUIP_SLOT_COUNT)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid change_equip item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pChangeEquipItem[wCell];
#endif

	default:
		return NULL;
	}
	return NULL;
}

void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem, bool bWereMine)

{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}
	// ±âº» ??º¥?ä¸®
	switch(window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pItems[wCell];

			if (pOld)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
							continue;

						m_pointsInstant.bItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 ·? ??´? °??º ºó°÷?» ?¼?©?? ¶§ °°?º
						// ¾??????º ¿¹¿??³¸®??±â ?§??
						m_pointsInstant.bItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pItems[wCell] = pItem;
		}
		break;
	// ¿ë?¥¼® ??º¥?ä¸®
	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pDSItems[wCell];

			if (pOld)
			{
				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
							continue;

						m_pointsInstant.wDSItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
					return;
				}

				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 ·? ??´? °??º ºó°÷?» ?¼?©?? ¶§ °°?º
						// ¾??????º ¿¹¿??³¸®??±â ?§??
						m_pointsInstant.wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pDSItems[wCell] = pItem;
		}
		break;
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pSSUItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pSSUItems[p] && m_pointsInstant.pSSUItems[p] != pOld)
							continue;

						m_pointsInstant.wSSUItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wSSUItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSU item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.wSSUItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wSSUItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pSSUItems[wCell] = pItem;
		}
		break;
	case BOOK_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pSSBItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pSSBItems[p] && m_pointsInstant.pSSBItems[p] != pOld)
							continue;

						m_pointsInstant.wSSBItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wSSBItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.wSSBItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wSSBItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pSSBItems[wCell] = pItem;
		}
		break;
	case STONE_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pSSSItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pSSSItems[p] && m_pointsInstant.pSSSItems[p] != pOld)
							continue;

						m_pointsInstant.wSSSItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wSSSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSB item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.wSSSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wSSSItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pSSSItems[wCell] = pItem;
		}
		break;

	case CHANGE_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pSSCItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pSSCItems[p] && m_pointsInstant.pSSCItems[p] != pOld)
							continue;

						m_pointsInstant.wSSCItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wSSCItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSC item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.wSSCItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wSSCItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pSSCItems[wCell] = pItem;
		}
		break;
	
	case COSTUME_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pCostumeItems[wCell];

			if (pOld)
			{
				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pCostumeItems[p] && m_pointsInstant.pCostumeItems[p] != pOld)
							continue;

						m_pointsInstant.wCostumeItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wCostumeItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid SSC item cell %d", wCell);
					return;
				}

				if (wCell < SPECIAL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							continue;

						m_pointsInstant.wCostumeItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wCostumeItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pCostumeItems[wCell] = pItem;
		}
		break;
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
	{
		LPITEM pOld = m_pointsInstant.pSwitchbotItems[wCell];
		if (pItem && pOld)
		{
			return;
		}

		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CHARACTER::SetItem: invalid switchbot item cell %d", wCell);
			return;
		}

		if (pItem)
		{
			CSwitchbotManager::Instance().RegisterItem(GetPlayerID(), pItem->GetID(), wCell);
		}
		else
		{
			CSwitchbotManager::Instance().UnregisterItem(GetPlayerID(), wCell);
		}

		m_pointsInstant.pSwitchbotItems[wCell] = pItem;
	}
	break;
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		{
			if (wCell >= CHANGE_EQUIP_SLOT_COUNT)
			{
				sys_err("CHARACTER::SetItem: invalid CHANGE_EQUIP item cell %d", wCell);
				return;
			}
			LPITEM pOld = m_pointsInstant.pChangeEquipItem[wCell];

			if (pOld && pItem)
			{
				return;
			}

			m_pointsInstant.pChangeEquipItem[wCell] = pItem;

		}
		break;	
#endif


	default:
		sys_err ("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		// ?®?å ¾?????: ¼­¹ö¿¡¼­ ¾????? ??·¡±? ?¤º¸¸¦ º¸³½´?
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;

			pack.count = pItem->GetCount();
#ifdef __SOULBINDING_SYSTEM__
			pack.bind = pItem->GetBind();
#endif
			pack.vnum = pItem->GetVnum();
#ifdef ENABLE_NEW_NAME_ITEM
			strlcpy(pack.name,pItem->GetNewName(),sizeof(pack.name));
#endif
			pack.flags = pItem->GetFlag();
			pack.anti_flags	= pItem->GetAntiFlag();
#ifdef ENABLE_NEWITEM_AFFECT
			pack.highlight = bWereMine || (Cell.window_type == DRAGON_SOUL_INVENTORY);
#else
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);
#endif
#ifdef ELEMENT_SPELL_WORLDARD
			pack.grade_element = pItem->GetElementGrade();
			thecore_memcpy(pack.attack_element, pItem->GetElementAttacks(), sizeof(pack.attack_element));
			pack.element_type_bonus = pItem->GetElementsType();
			thecore_memcpy(pack.elements_value_bonus, pItem->GetElementsValues(), sizeof(pack.elements_value_bonus));
#endif
			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
			thecore_memcpy(pack.randomAttr, pItem->GetRandomAttributes(), sizeof(pack.randomAttr));
#endif

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			pack.count = 0;
#ifdef __SOULBINDING_SYSTEM__
			pack.bind = 0;
#endif
			pack.vnum = 0;

#ifdef ELEMENT_SPELL_WORLDARD
			pack.grade_element = 0;
			memset(pack.attack_element, 0, sizeof(pack.attack_element));
			pack.element_type_bonus = 0;
			memset(pack.elements_value_bonus, 0, sizeof(pack.elements_value_bonus));

#endif

			memset(pack.alSockets, 0, sizeof(pack.alSockets));
			memset(pack.aAttr, 0, sizeof(pack.aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
			memset(pack.randomAttr, 0, sizeof(pack.randomAttr));
#endif


			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;	
#ifdef ENABLE_SPECIAL_STORAGE
		case UPGRADE_INVENTORY:
			pItem->SetWindow(UPGRADE_INVENTORY);
			break;
		case BOOK_INVENTORY:
			pItem->SetWindow(BOOK_INVENTORY);
			break;
		case STONE_INVENTORY:
			pItem->SetWindow(STONE_INVENTORY);
			break;
		case CHANGE_INVENTORY:
			pItem->SetWindow(CHANGE_INVENTORY);
			break;
		case COSTUME_INVENTORY:
			pItem->SetWindow(COSTUME_INVENTORY);
			break;
#endif
#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			pItem->SetWindow(SWITCHBOT);
			break;
#endif	

#ifdef CHANGE_EQUIP_WORLDARD
		case CHANGE_EQUIP:
			pItem->SetWindow(CHANGE_EQUIP);
			break;
#endif
		}
	}
}

LPITEM CHARACTER::GetWear(BYTE bCell) const
{
	// > WEAR_MAX_NUM : ¿ë?¥¼® ½½·?µé.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(BYTE bCell, LPITEM item)
{
	// > WEAR_MAX_NUM : ¿ë?¥¼® ½½·?µé.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item);


	if (!item && bCell == WEAR_WEAPON)
	{
		// ±?°? »ç¿ë ½? ¹þ´? °???¶ó¸é ?¿°ú¸¦ ¾ø¾?¾? ??´?.
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(UPGRADE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(BOOK_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(STONE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(CHANGE_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
	for (i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(COSTUME_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif
#ifdef ENABLE_SWITCHBOT
	for (i = 0; i < SWITCHBOT_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(SWITCHBOT, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	for (i = 0; i < CHANGE_EQUIP_SLOT_COUNT; ++i)
	{
		if ((item = GetItem(TItemPos(CHANGE_EQUIP, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}

#endif
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	switch (Cell.window_type)
	{
	case INVENTORY:
		{
			WORD bCell = Cell.cell;

			// bItemCell?º 0?? false???» ³ª?¸³»±â ?§?? + 1 ??¼­ ?³¸®??´?.
			// µû¶ó¼­ iExceptionCell¿¡ 1?» ´õ?? ºñ±³??´?.
			++iExceptionCell;

			if (Cell.IsBeltInventoryPosition())
			{
				LPITEM beltItem = GetWear(WEAR_BELT);

				if (NULL == beltItem)
					return false;

				if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
					return false;

				if (m_pointsInstant.bItemGrid[bCell])
				{
					if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
						return true;

					return false;
				}

				if (bSize == 1)
					return true;

			}
			#ifdef UNLOCK_INVENTORY_WORLDARD
			else if (bCell >= 90+(5*GetUnlockSlotsW()))
				return false;
			
			else if (bSize == 2){
				if (bCell+5 >= 90+(5*GetUnlockSlotsW()))
					return false;
			}
			else if (bSize == 3){
				if (bCell+10 >= 90+(5*GetUnlockSlotsW()))
					return false;
			}
			#else
			else if (bCell >= INVENTORY_MAX_NUM)
				return false;
			#endif

			if (m_pointsInstant.bItemGrid[bCell])
			{
				if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

					do
					{
						BYTE p = bCell + (5 * j);

						#ifdef UNLOCK_INVENTORY_WORLDARD
						if (p >= 90+(5*GetUnlockSlotsW()))
							return false;
						#else
						if (p >= INVENTORY_MAX_NUM)
							return false;
						#endif

						if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
							return false;

						if (m_pointsInstant.bItemGrid[p])
							if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// ?©±â°¡ 1??¸é ???­?» ?÷?ö??´? °???¹?·? ±?³? ¸®??
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= INVENTORY_MAX_NUM)
						return false;

					if (p / (INVENTORY_MAX_NUM / INVENTORY_PAGE_COUNT) != bPage)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	case DRAGON_SOUL_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			// bItemCell?º 0?? false???» ³ª?¸³»±â ?§?? + 1 ??¼­ ?³¸®??´?.
			// µû¶ó¼­ iExceptionCell¿¡ 1?» ´õ?? ºñ±³??´?.
			iExceptionCell++;

			if (m_pointsInstant.wDSItemGrid[wCell])
			{
				if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.wDSItemGrid[p])
							if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// ?©±â°¡ 1??¸é ???­?» ?÷?ö??´? °???¹?·? ±?³? ¸®??
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
		{
			WORD wCell = Cell.cell;
			//if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
			//	return false;

			#ifdef UNLOCK_INVENTORY_WORLDARD
			if (wCell >= 90+(5*GetUnlockSlotsW(1)))
				return false;
			
			else if (bSize == 2){
				if (wCell+5 >= 90+(5*GetUnlockSlotsW(1)))
					return false;
			}
			else if (bSize == 3){
				if (wCell+10 >= 90+(5*GetUnlockSlotsW(1)))
					return false;
			}
			#else
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;
			#endif

			iExceptionCell++;

			if (m_pointsInstant.wSSUItemGrid[wCell])
			{
				if (m_pointsInstant.wSSUItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						#ifdef UNLOCK_INVENTORY_WORLDARD
						if (p >= 90+(5*GetUnlockSlotsW(1)))
							return false;
						#else
						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;
						#endif

						//if (p >= SPECIAL_INVENTORY_MAX_NUM)
						//	return false;

						if (m_pointsInstant.wSSUItemGrid[p])
							if (m_pointsInstant.wSSUItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p]) // old bItemGrid
						if (m_pointsInstant.wSSUItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	case BOOK_INVENTORY:
		{
			WORD wCell = Cell.cell;
			#ifdef UNLOCK_INVENTORY_WORLDARD
			if (wCell >= 90+(5*GetUnlockSlotsW(2)))
				return false;
			
			else if (bSize == 2){
				if (wCell+5 >= 90+(5*GetUnlockSlotsW(2)))
					return false;
			}
			else if (bSize == 3){
				if (wCell+10 >= 90+(5*GetUnlockSlotsW(2)))
					return false;
			}
			#else
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;
			#endif

			iExceptionCell++;

			if (m_pointsInstant.wSSBItemGrid[wCell])
			{
				if (m_pointsInstant.wSSBItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						#ifdef UNLOCK_INVENTORY_WORLDARD
						if (p >= 90+(5*GetUnlockSlotsW(2)))
							return false;
						#else
						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;
						#endif

						if (m_pointsInstant.wSSBItemGrid[p])
							if (m_pointsInstant.wSSBItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p]) // old bItemGrid
						if (m_pointsInstant.wSSBItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	case STONE_INVENTORY:
		{
			WORD wCell = Cell.cell;
			#ifdef UNLOCK_INVENTORY_WORLDARD
			if (wCell >= 90+(5*GetUnlockSlotsW(3)))
				return false;
			
			else if (bSize == 2){
				if (wCell+5 >= 90+(5*GetUnlockSlotsW(3)))
					return false;
			}
			else if (bSize == 3){
				if (wCell+10 >= 90+(5*GetUnlockSlotsW(3)))
					return false;
			}
			#else
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;
			#endif

			iExceptionCell++;

			if (m_pointsInstant.wSSSItemGrid[wCell])
			{
				if (m_pointsInstant.wSSSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						#ifdef UNLOCK_INVENTORY_WORLDARD
						if (p >= 90+(5*GetUnlockSlotsW(3)))
							return false;
						#else
						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;
						#endif

						if (m_pointsInstant.wSSSItemGrid[p])
							if (m_pointsInstant.wSSSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p]) // old bItemGrid
						if (m_pointsInstant.wSSSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	case CHANGE_INVENTORY:
		{
			WORD wCell = Cell.cell;
			#ifdef UNLOCK_INVENTORY_WORLDARD
			if (wCell >= 90+(5*GetUnlockSlotsW(4)))
				return false;
			
			else if (bSize == 2){
				if (wCell+5 >= 90+(5*GetUnlockSlotsW(4)))
					return false;
			}
			else if (bSize == 3){
				if (wCell+10 >= 90+(5*GetUnlockSlotsW(4)))
					return false;
			}
			#else
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;
			#endif

			iExceptionCell++;

			if (m_pointsInstant.wSSCItemGrid[wCell])
			{
				if (m_pointsInstant.wSSCItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						#ifdef UNLOCK_INVENTORY_WORLDARD
						if (p >= 90+(5*GetUnlockSlotsW(4)))
							return false;
						#else
						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;
						#endif

						if (m_pointsInstant.wSSCItemGrid[p])
							if (m_pointsInstant.wSSCItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p]) // old bItemGrid
						if (m_pointsInstant.wSSCItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

	case COSTUME_INVENTORY:
		{
			WORD wCell = Cell.cell;
			#ifdef UNLOCK_INVENTORY_WORLDARD
			if (wCell >= 90+(5*GetUnlockSlotsW(5)))
			{
				sys_err("STEP 1");
				return false;
			}
			else if (bSize == 2){
				if (wCell+5 >= 90+(5*GetUnlockSlotsW(5)))
				{
					sys_err("STEP 2");
					return false;
				}
			}
			else if (bSize == 3){
				if (wCell+10 >= 90+(5*GetUnlockSlotsW(5)))
				{
					sys_err("STEP 3");
					return false;
				}
			}
			#else
			if (wCell >= SPECIAL_INVENTORY_MAX_NUM)
				return false;
			#endif

			iExceptionCell++;

			if (m_pointsInstant.wCostumeItemGrid[wCell])
			{
				if (m_pointsInstant.wCostumeItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						int p = wCell + (5 * j);

						#ifdef UNLOCK_INVENTORY_WORLDARD
						if (p >= 90+(5*GetUnlockSlotsW(5)))
							return false;
						#else
						if (p >= SPECIAL_INVENTORY_MAX_NUM)
							return false;
						#endif

						if (m_pointsInstant.wCostumeItemGrid[p])
							if (m_pointsInstant.wCostumeItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					int p = wCell + (5 * j);

					if (p >= SPECIAL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.wCostumeItemGrid[p]) // old bItemGrid
						if (m_pointsInstant.wCostumeItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		{
		WORD wCell = Cell.cell;
		if (wCell >= SWITCHBOT_SLOT_COUNT)
		{
			return false;
		}

		if (m_pointsInstant.pSwitchbotItems[wCell])
		{
			return false;
		}

		return true;
		}
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		{
		WORD wCell = Cell.cell;
		if (wCell >= CHANGE_EQUIP_SLOT_COUNT)
		{
			return false;
		}

		if (m_pointsInstant.pChangeEquipItem[wCell])
		{
			return false;
		}

		return true;
		}
#endif
	}
	return false;
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	// NOTE: ?ö?ç ?? ??¼ö´? ¾????? ?ö±?, ?¹µæ µî?? ???§¸¦ ?? ¶§ ??º¥?ä¸®?? ºó ?­?» ?£±â ?§?? »ç¿ëµ?°í ??´?µ¥,
	//		º§?® ??º¥?ä¸®´? ?¯¼ö ??º¥?ä¸®??¹?·? °?»ç???ö ¾?µµ·? ??´?. (±âº» ??º¥?ä¸®: INVENTORY_MAX_NUM ±î?ö¸¸ °?»ç)
#ifdef UNLOCK_INVENTORY_WORLDARD
	for ( int i = 0; i < 90+(5*GetUnlockSlotsW()); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
#else
	for ( int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
#endif
}

int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

#ifdef ENABLE_SPECIAL_STORAGE
int CHARACTER::GetEmptyUpgradeInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsUpgradeItem())
		return -1;
	
	BYTE bSize = pItem->GetSize();


#ifdef UNLOCK_INVENTORY_WORLDARD
	for ( int i = 0; i < 90+(5*GetUnlockSlotsW(1)); ++i)
		if (IsEmptyItemGrid(TItemPos (UPGRADE_INVENTORY, i), bSize))
			return i;
	return -1;
#else
	for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (UPGRADE_INVENTORY, i), bSize))
			return i;
	return -1;
#endif

}
int CHARACTER::GetEmptyBookInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsBook())
		return -1;
	
	BYTE bSize = pItem->GetSize();

#ifdef UNLOCK_INVENTORY_WORLDARD
	for ( int i = 0; i < 90+(5*GetUnlockSlotsW(2)); ++i)
		if (IsEmptyItemGrid(TItemPos (BOOK_INVENTORY, i), bSize))
			return i;
	return -1;
#else
	for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (BOOK_INVENTORY, i), bSize))
			return i;
	return -1;
#endif

}
int CHARACTER::GetEmptyStoneInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsStone())
		return -1;
	
	BYTE bSize = pItem->GetSize();

#ifdef UNLOCK_INVENTORY_WORLDARD
	for ( int i = 0; i < 90+(5*GetUnlockSlotsW(3)); ++i)
		if (IsEmptyItemGrid(TItemPos (STONE_INVENTORY, i), bSize))
			return i;
	return -1;
#else
	for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (STONE_INVENTORY, i), bSize))
			return i;
	return -1;
#endif

}
int CHARACTER::GetEmptyChangeInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsChange())
		return -1;
	
	BYTE bSize = pItem->GetSize();

#ifdef UNLOCK_INVENTORY_WORLDARD
	for ( int i = 0; i < 90+(5*GetUnlockSlotsW(4)); ++i)
		if (IsEmptyItemGrid(TItemPos (CHANGE_INVENTORY, i), bSize))
			return i;
	return -1;
#else
	for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (CHANGE_INVENTORY, i), bSize))
			return i;
	return -1;
#endif

}

int CHARACTER::GetEmptyCostumeInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsCostume() || !pItem->IsCostumeSkin())
		return -1;
	
	BYTE bSize = pItem->GetSize();

#ifdef UNLOCK_INVENTORY_WORLDARD
	for ( int i = 0; i < 90+(5*GetUnlockSlotsW(5)); ++i)
		if (IsEmptyItemGrid(TItemPos (COSTUME_INVENTORY, i), bSize))
			return i;
	return -1;
#else
	for ( int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (COSTUME_INVENTORY, i), bSize))
			return i;
	return -1;
#endif

}

#endif

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;
#ifdef UNLOCK_INVENTORY_WORLDARD
	int slot = 90+(5*GetUnlockSlotsW());
	for (int i = 0; i < 90+(5*GetUnlockSlotsW()); ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();
#ifdef UNLOCK_INVENTORY_WORLDARD
	return (slot - count);
#else
	return (INVENTORY_MAX_NUM - count);
#endif
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		// ¿©±â¼­ ±ú?ø¼®?? ??µ¿?û?¸·? ?»¼? µ?
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		// ¼??? ¼³?¤
		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}

	}

	// ¸??÷ ¾????? ¼³?¤
	pkOldItem->CopyAttributeTo(pkNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), 1, way);
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), success, way);
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if ( ch != NULL )
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

#ifdef ENABLE_MULTI_REFINE_WORLDARD
bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly, BYTE index)
#else
bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
#endif
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	//°³·® ½?°£?¦?? : upgrade_refine_scroll.quest ¿¡¼­ °³·®?? 5º???³»¿¡ ??¹? °³·®?»
	//?ø????¼ö ¾ø?½
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	int prt_multi_refine = CMultiRefineManager::instance().GetMultiRefineSize(item->GetVnum());

	int refine_set = item->GetRefineSet();
	DWORD result_vnum = item->GetRefinedVnum();

	if (prt_multi_refine != 0)
	{
		std::map<int, TMultiRefineTable>  m_map_MultiRefineRecipe = CMultiRefineManager::instance().GetMultiRefineRecipe();

		for (itertype(m_map_MultiRefineRecipe) it = m_map_MultiRefineRecipe.begin(); it != m_map_MultiRefineRecipe.end(); ++it)
		{
			
			const TMultiRefineTable* prt_new = &it->second;

			if (prt_new->id == index)
			{
				refine_set = prt_new->refine_set;
				result_vnum = prt_new->refine_vnum;
			}
		}
	}
	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(refine_set);
#else
	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());
#endif

	if (!prt)
		return false;

	if (item->GetCount() > 1)
	{
		int iEmptyPos;
		if (item->IsUpgradeItem()){
			iEmptyPos = item->IsDragonSoul() ? GetEmptyDragonSoulInventory(item) : GetEmptyUpgradeInventory(item);
		}
		else if (item->IsBook()){
			iEmptyPos = item->IsDragonSoul() ? GetEmptyDragonSoulInventory(item) : GetEmptyBookInventory(item);
		}
		else if (item->IsStone()){
			iEmptyPos = item->IsDragonSoul() ? GetEmptyDragonSoulInventory(item) : GetEmptyStoneInventory(item);
		}
		else{
			iEmptyPos = item->IsDragonSoul() ? GetEmptyDragonSoulInventory(item) : GetEmptyInventory(item->GetSize());
		}

		if (iEmptyPos < 0)
		{
			ChatPacket(CHAT_TYPE_INFO, "You do not have space in your inventory");
			return false;
		}

	}

#ifndef ENABLE_MULTI_REFINE_WORLDARD
	DWORD result_vnum = item->GetRefinedVnum();
#endif

	// REFINE_COST
	int cost = ComputeRefineFee(prt->cost);

	int RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");

	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, "115");
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, "116");
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(result_vnum);
#else
	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());
#endif

	if (!pProto)
	{

#ifdef ENABLE_MULTI_REFINE_WORLDARD
		sys_err("DoRefine NOT GET ITEM PROTO %d", result_vnum);
#else
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
#endif
		ChatPacket(CHAT_TYPE_INFO, "117");
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, "118");
		return false;
	}

	if (!bMoneyOnly && !RefineChance)
	{
		for (int i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				ChatPacket(CHAT_TYPE_INFO, "119");
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int prob = number(1, 100);

#ifdef ENABLE_FEATURES_REFINE_SYSTEM	
	if (IsRefineThroughGuild() || bMoneyOnly)
	{
		prob -= 10;
	}
	
	int success_prob = prt->prob;
	success_prob += CRefineManager::instance().Result(this);
#else
	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;	
#endif

#ifdef ENABLE_BATTLE_PASS
	CHARACTER_MANAGER::Instance().DoMission(this, MISSION_CRAFT_ITEM, 1, item->GetVnum());
#endif

	// END_OF_REFINE_COST

#ifdef ENABLE_FEATURES_REFINE_SYSTEM	
	if (prob <= success_prob)
#else
	if (prob <= prt->prob)	
#endif
	{
		// ¼º°ø! ¸ðµç ¾??????? »ç¶ó?ö°í, °°?º ¼?¼º?? ´?¸¥ ¾????? ?¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);

			if (item->GetCount() == 1)
			{
				BYTE bCell = item->GetCell();



				ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");


				if (pkNewItem->IsUpgradeItem()){
					pkNewItem->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, bCell));
				}
				else if (pkNewItem->IsBook()){
					pkNewItem->AddToCharacter(this, TItemPos(BOOK_INVENTORY, bCell));
				}
				else if (pkNewItem->IsStone()){
					pkNewItem->AddToCharacter(this, TItemPos(STONE_INVENTORY, bCell));
				}
				else{
					pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
				}


			}else{

				item->SetCount(item->GetCount() - 1);

				if (pkNewItem->IsUpgradeItem()){
					int iEmptyPos = pkNewItem->IsDragonSoul() ? GetEmptyDragonSoulInventory(pkNewItem) : GetEmptyUpgradeInventory(pkNewItem);
					pkNewItem->AddToCharacter(this, TItemPos(pkNewItem->IsDragonSoul() ? DRAGON_SOUL_INVENTORY : UPGRADE_INVENTORY, iEmptyPos));
				}
				else if (pkNewItem->IsBook()){
					int iEmptyPos = pkNewItem->IsDragonSoul() ? GetEmptyDragonSoulInventory(pkNewItem) : GetEmptyBookInventory(pkNewItem);
					pkNewItem->AddToCharacter(this, TItemPos(pkNewItem->IsDragonSoul() ? DRAGON_SOUL_INVENTORY : BOOK_INVENTORY, iEmptyPos));
				}
				else if (pkNewItem->IsStone()){
					int iEmptyPos = pkNewItem->IsDragonSoul() ? GetEmptyDragonSoulInventory(pkNewItem) : GetEmptyStoneInventory(pkNewItem);
					pkNewItem->AddToCharacter(this, TItemPos(pkNewItem->IsDragonSoul() ? DRAGON_SOUL_INVENTORY : STONE_INVENTORY, iEmptyPos));
				}
				else{
					int iEmptyPos = pkNewItem->IsDragonSoul() ? GetEmptyDragonSoulInventory(pkNewItem) : GetEmptyInventory(pkNewItem->GetSize());
					pkNewItem->AddToCharacter(this, TItemPos(pkNewItem->IsDragonSoul() ? DRAGON_SOUL_INVENTORY : INVENTORY, iEmptyPos));
				}
			}
			
			// END_OF_DETAIL_REFINE_LOG

			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			sys_log(0, "Refine Success %d", cost);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -cost);
			sys_log(0, "PayPee %d", cost);
			PayRefineFee(cost);
			#ifdef ENABLE_FEATURES_REFINE_SYSTEM
	CRefineManager::instance().Reset(this);
#endif
			sys_log(0, "PayPee End %d", cost);
		}
		else
		{
			// DETAIL_REFINE_LOG
			// ¾????? »ý¼º¿¡ ½??? -> °³·® ½???·? °£??
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		// ½???! ¸ðµç ¾??????? »ç¶ó?ü.
		DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		item->AttrLog();
		if (item->GetCount() == 1)
		{
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
		}else{
			item->SetCount(item->GetCount() - 1);
		}

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
#ifdef ENABLE_FEATURES_REFINE_SYSTEM
	CRefineManager::instance().Reset(this);
#endif
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN   = 1, // ??±¹¿¡¼­¸¸ »ç¿ë
	YONGSIN_SCROLL = 2,
	MUSIN_SCROLL   = 3,
	YAGONG_SCROLL  = 4,
	MEMO_SCROLL	   = 5,
	BDRAGON_SCROLL	= 6,
};

#ifdef ENABLE_MULTI_REFINE_WORLDARD
bool CHARACTER::DoRefineWithScroll(LPITEM item, BYTE index)
#else
bool CHARACTER::DoRefineWithScroll(LPITEM item)
#endif
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}
	ClearRefineMode();
	//°³·® ½?°£?¦?? : upgrade_refine_scroll.quest ¿¡¼­ °³·®?? 5º???³»¿¡ ??¹? °³·®?»
	//?ø????¼ö ¾ø?½
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}
#ifdef ENABLE_MULTI_REFINE_WORLDARD
	int refine_set = item->GetRefineSet();
	DWORD result_vnum = item->GetRefinedVnum();
	/*
	int prt_multi_refine = CMultiRefineManager::instance().GetMultiRefineSize(item->GetVnum());
	if (prt_multi_refine != 0)
	{
		std::map<int, TMultiRefineTable>  m_map_MultiRefineRecipe = CMultiRefineManager::instance().GetMultiRefineRecipe();

		for (itertype(m_map_MultiRefineRecipe) it = m_map_MultiRefineRecipe.begin(); it != m_map_MultiRefineRecipe.end(); ++it)
		{
			
			const TMultiRefineTable* prt_new = &it->second;

			if (prt_new->id == index)
			{
				refine_set = prt_new->refine_set;
				result_vnum = prt_new->refine_vnum;
			}
		}
	}
	*/
	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(refine_set);
#else
	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());
#endif
	if (!prt)
		return false;
	LPITEM pkItemScroll;

	// °³·®¼­ ?¼?©
	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

#ifndef ENABLE_MULTI_REFINE_WORLDARD
	DWORD result_vnum = item->GetRefinedVnum();
#endif
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, "116");
		return false;
	}

	// MUSIN_SCROLL
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			ChatPacket(CHAT_TYPE_INFO, "120");
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pkItemScroll->GetValue(1))
		{
			ChatPacket(CHAT_TYPE_INFO, "121");
			return false;
		}
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM_METIN || item->GetRefineLevel() != 4)
		{
			ChatPacket(CHAT_TYPE_INFO, "122");
			return false;
		}
	}

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, "117");
		return false;
	}

	if (GetGold() < prt->cost)
	{
		ChatPacket(CHAT_TYPE_INFO, "118");
		return false;
	}

	for (int i = 0; i < prt->material_count; ++i)
	{
		if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
			}
			ChatPacket(CHAT_TYPE_INFO, "119");
			return false;
		}
	}

	for (int i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);

	int prob = number(1, 100);
	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN ||
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL ||
		pkItemScroll->GetValue(0) == YAGONG_SCROLL) // ?ö?¶, ¿ë½??? ??º¹¼­, ¾?°ø?? ºñ?ü¼­  ?³¸®
	{
		const char hyuniron_prob[9] = { 100, 75, 65, 55, 45, 40, 35, 25, 20 };
		const char yagong_prob[9] = { 100, 100, 90, 80, 70, 60, 50, 30, 20 };

		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			success_prob = hyuniron_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			success_prob = yagong_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) {} // @fixme121
		else
		{
			sys_err("REFINE : Unknown refine scroll item. Value0: %d", pkItemScroll->GetValue(0));
		}

		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) // ?ö?¶?º ¾??????? º?¼­?®¾? ??´?.
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL) // ¹«½??? ??º¹¼­´? 100% ¼º°ø (+4±î?ö¸¸)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}
	
	// fix pergaminos
	if (pkItemScroll->GetVnum() == 39022 || pkItemScroll->GetVnum() == 71032 ||pkItemScroll->GetVnum() == 76009)
		success_prob = prt->prob+10;

	if (test_server)
	{
		ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
	}

#ifdef ENABLE_BATTLE_PASS
	CHARACTER_MANAGER::Instance().DoMission(this, MISSION_CRAFT_ITEM, 1, item->GetVnum());
	CHARACTER_MANAGER::Instance().DoMission(this, MISSION_REFINE_ITEM, 1, pkItemScroll->GetVnum());
#endif


	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);
#ifdef ENABLE_FEATURES_REFINE_SYSTEM	
	success_prob += CRefineManager::instance().Result(this);
#endif
	if (prob <= success_prob)
	{
		// ¼º°ø! ¸ðµç ¾??????? »ç¶ó?ö°í, °°?º ¼?¼º?? ´?¸¥ ¾????? ?¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bWindow = item->GetWindow();
			BYTE bCell = item->GetCell();


			NotifyRefineSuccess(this, item, szRefineType);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pkNewItem->AddToCharacter(this, TItemPos(bWindow, bCell));
			//if (pkNewItem->IsUpgradeItem()){
			//	pkNewItem->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, bCell));
			//}
			//else if (pkNewItem->IsBook()){
			//	pkNewItem->AddToCharacter(this, TItemPos(BOOK_INVENTORY, bCell));
			//}
			//else if (pkNewItem->IsStone()){
			//	pkNewItem->AddToCharacter(this, TItemPos(STONE_INVENTORY, bCell));
			//}
			//else{
			//	pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			//}

			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
#ifdef ENABLE_FEATURES_REFINE_SYSTEM
			CRefineManager::instance().Reset(this);
#endif
		}
		else
		{
			// ¾????? »ý¼º¿¡ ½??? -> °³·® ½???·? °£??
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		// ½???! ¸ðµç ¾??????? »ç¶ó?ö°í, °°?º ¼?¼º?? ³·?º µî±??? ¾????? ?¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE FAIL", pkNewItem->GetName());

			BYTE bWindow = item->GetWindow();
			BYTE bCell = item->GetCell();

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			NotifyRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

			pkNewItem->AddToCharacter(this, TItemPos(bWindow, bCell));
			
			//if (pkNewItem->IsUpgradeItem()){
			//	pkNewItem->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, bCell));
			//}
			//else if (pkNewItem->IsBook()){
			//	pkNewItem->AddToCharacter(this, TItemPos(BOOK_INVENTORY, bCell));
			//}
			//else if (pkNewItem->IsStone()){
			//	pkNewItem->AddToCharacter(this, TItemPos(STONE_INVENTORY, bCell));
			//}
			//else{
			//	pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell));
			//}

			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			pkNewItem->AttrLog();

			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
#ifdef ENABLE_FEATURES_REFINE_SYSTEM
	CRefineManager::instance().Reset(this);
#endif
		}
		else
		{
			// ¾????? »ý¼º¿¡ ½??? -> °³·® ½???·? °£??
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType); // °³·®½? ¾????? »ç¶ó?ö?ö ¾??½

		PayRefineFee(prt->cost);
#ifdef ENABLE_FEATURES_REFINE_SYSTEM
	CRefineManager::instance().Reset(this);
#endif
	}

	return true;
}

#ifdef ELEMENT_SPELL_WORLDARD

bool CHARACTER::ElementsSpellItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;


	BYTE bCell = pkTarget->GetCell();


	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;


	TPacketGCElementsSpell p;

	p.header = HEADER_GC_ELEMENTS_SPELL;
	p.pos = bCell;
	

	if(pkItem->GetSubType() == USE_ELEMENT_DOWNGRADE)
	{
		p.subheader = ELEMENT_SPELL_SUB_HEADER_OPEN;
		p.cost = GOLD_DOWNGRADE_BONUS_ELEMENTS_SPELL;
		p.func = false;
		p.grade_add = 0;
	}
	else if(pkItem->GetSubType() == USE_ELEMENT_UPGRADE)
	{
		p.subheader = ELEMENT_SPELL_SUB_HEADER_OPEN;
		p.cost = GOLD_ADD_BONUS_ELEMENTS_SPELL;
		p.func = true;
		if (pkTarget->GetElementGrade() == 0){
			p.grade_add = pkItem->GetValue(0);
		}else{
			p.grade_add = 0;
		}

	}
	else if(pkItem->GetSubType() == USE_ELEMENT_CHANGE)
	{
		p.subheader = ELEMENT_SPELL_SUB_HEADER_OPEN_CHANGE;
		p.cost = GOLD_CHANGE_BONUS_ELEMENTS_SPELL;
	}

	GetDesc()->Packet(&p, sizeof(TPacketGCElementsSpell));

	SetOpenElementsSpell(true,pkItem->GetCell());
	return true;

}


void CHARACTER::ElementsSpellItemFunc(int pos, BYTE type_select)
{

	if (GetExchange() || IsOpenSafebox() || GetShop() || IsCubeOpen())
	{
		SetOpenElementsSpell(false);
		return;
	}

	if (m_iElementsAdditionalCell < 0 || pos < 0){
		return;
	}

	LPITEM itemWeapon = GetInventoryItem(pos);
	LPITEM itemElements = GetInventoryItem(m_iElementsAdditionalCell);

	if (!itemWeapon || !itemElements)
	{
		return;
	}

	if (itemElements->GetSubType() != USE_ELEMENT_UPGRADE && itemElements->GetSubType() != USE_ELEMENT_DOWNGRADE && itemElements->GetSubType() != USE_ELEMENT_CHANGE){
		return;
	}


	if (itemWeapon->GetType() != ITEM_WEAPON){
		return;
	}

	if (itemElements->GetSubType() == USE_ELEMENT_UPGRADE)
	{

		if (GetGold() < GOLD_ADD_BONUS_ELEMENTS_SPELL)
		{
			ChatPacket(CHAT_TYPE_INFO, "Sorry, you don't have enough Yang.");
			return;
		}

		int percent = number(1,100);

		if (percent <= PERCENT_ADD_BONUS_ELEMENTS_SPELL){
			if(itemWeapon->GetElementGrade() == 0)
			{
				DWORD attack_element = number(ATTACK_RANGE_BONUS_ELEMENTS_SPELL_MIN,ATTACK_RANGE_BONUS_ELEMENTS_SPELL_MAX);
				short sValue = number(VALUES_RANGE_BONUS_ELEMENTS_SPELL_MIN,VALUES_RANGE_BONUS_ELEMENTS_SPELL_MAX);
				itemWeapon->SetElementNew(1,attack_element,itemElements->GetValue(0),sValue);
				ChatPacket(CHAT_TYPE_COMMAND, "ElementsSpellSuceeded");
			}

			else if(itemWeapon->GetElementGrade() > 0)
			{
				if(itemWeapon->GetElementsType() != itemElements->GetValue(0)){
					return;
				}

				DWORD attack_element = itemWeapon->GetElementAttack(itemWeapon->GetElementGrade()-1)+ATTACK_RANGE_BONUS_ELEMENTS_SPELL_MIN;
				short sValue = itemWeapon->GetElementsValue(itemWeapon->GetElementGrade()-1)+VALUES_RANGE_BONUS_ELEMENTS_SPELL_MIN;
				itemWeapon->SetElementNew(itemWeapon->GetElementGrade()+1,number(attack_element,attack_element+(ATTACK_RANGE_BONUS_ELEMENTS_SPELL_MAX-ATTACK_RANGE_BONUS_ELEMENTS_SPELL_MIN)),itemWeapon->GetElementsType(),number(sValue,sValue+(VALUES_RANGE_BONUS_ELEMENTS_SPELL_MAX-VALUES_RANGE_BONUS_ELEMENTS_SPELL_MIN)));
				ChatPacket(CHAT_TYPE_COMMAND, "ElementsSpellSuceeded");
			}
		}else{
			ChatPacket(CHAT_TYPE_COMMAND, "ElementsSpellFailed");
		}

		PointChange(POINT_GOLD, -GOLD_ADD_BONUS_ELEMENTS_SPELL);
	}
	else if(itemElements->GetSubType() == USE_ELEMENT_DOWNGRADE)
	{

		if (GetGold() < GOLD_DOWNGRADE_BONUS_ELEMENTS_SPELL)
		{
			ChatPacket(CHAT_TYPE_INFO, "Sorry, you don't have enough Yang.");
			return;
		}

		if((itemWeapon->GetElementGrade())-1 <= 0){
			itemWeapon->DeleteAllElement(0);
		}else{
			itemWeapon->DeleteAllElement((itemWeapon->GetElementGrade())-1);
		}

		ChatPacket(CHAT_TYPE_COMMAND, "ElementsSpellDownGradeSuceeded");

		PointChange(POINT_GOLD, -GOLD_DOWNGRADE_BONUS_ELEMENTS_SPELL);
	}
	else if(itemElements->GetSubType() == USE_ELEMENT_CHANGE)
	{
		if (GetGold() < GOLD_CHANGE_BONUS_ELEMENTS_SPELL)
		{
			ChatPacket(CHAT_TYPE_INFO, "Sorry, you don't have enough Yang.");
			return;
		}

		if(itemWeapon->GetElementGrade() <= 0)
		{
			return;
		}

		if (type_select == itemWeapon->GetElementsType())
		{
			return;
		}

		itemWeapon->ChangeElement(type_select);
		ChatPacket(CHAT_TYPE_COMMAND, "ElementsSpellChangeSuceeded %d %d",itemWeapon->GetVnum(),type_select);
		PointChange(POINT_GOLD, -GOLD_CHANGE_BONUS_ELEMENTS_SPELL);
	}

	itemElements->SetCount(itemElements->GetCount()-1);
}

void CHARACTER::SetOpenElementsSpell(bool b, int iAdditionalCell)
{
	m_OpenElementsSpell = b;
	m_iElementsAdditionalCell = iAdditionalCell;
}

#endif

#ifdef ENABLE_SPECIAL_STORAGE

bool CHARACTER::RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell, BYTE bSpecialStorage)
#else
bool CHARACTER::RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell)

#endif
{
	if (bCell > INVENTORY_MAX_NUM)
		return false;
	LPITEM item;

	if (bSpecialStorage == UPGRADE_INVENTORY){
		item = GetUpgradeInventoryItem(bCell);
	}

	else if (bSpecialStorage == BOOK_INVENTORY){
		item = GetBookInventoryItem(bCell);
	}

	else if (bSpecialStorage == STONE_INVENTORY){
		item = GetStoneInventoryItem(bCell);
	}

	else if (bSpecialStorage == INVENTORY){
		item = GetInventoryItem(bCell);
	}

	if (!item)
		return false;

	// REFINE_COST
	if (bType == REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("deviltower_zone.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, "123");
		return false;
	}
	// END_OF_REFINE_COST

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	SendMultiRefinePackets(MULTIREFINE_CLEAR_INFO);
#endif

	TPacketGCRefineInformation p;
	
#ifdef ENABLE_MULTI_REFINE_WORLDARD

	if(item->GetRefinedVnum() != 0)
	{

		p.header = HEADER_GC_REFINE_INFORMATION;
		p.pos = bCell;
		p.src_vnum = item->GetVnum();
		p.result_vnum = item->GetRefinedVnum();
		p.type = bType;
		p.special_storage = bSpecialStorage;
		p.index = item->GetVnum()+9999;
		p.bSubType = MULTIREFINE_ADD_INFO;

		if (p.result_vnum == 0)
		{
			sys_err("RefineInformation p.result_vnum == 0");
			ChatPacket(CHAT_TYPE_INFO, "117");
			return false;
		}
	}

#else

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;
	p.special_storage = bSpecialStorage;
	if (p.result_vnum == 0)
	{
		sys_err("RefineInformation p.result_vnum == 0");
		ChatPacket(CHAT_TYPE_INFO, "117");
		return false;
	}

#endif

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, "124");
			return false;
		}
		else
		{

			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, "125");
				ChatPacket(CHAT_TYPE_INFO, "126");
				return false;
			}
		}
	}

	CRefineManager & rm = CRefineManager::instance();

	//const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());

	const TRefineTable* prt = NULL;

	if(item->GetRefinedVnum() != 0)
	{
		prt = rm.GetRefineRecipe(item->GetRefineSet());
	}

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	int prt_multi_refine = CMultiRefineManager::instance().GetMultiRefineSize(item->GetVnum());

	if (!prt && prt_multi_refine <= 0)
	{
		sys_err("RefineInformation NOT GET REFINE SET refineSet: %d itemVnum: %d", item->GetRefineSet(), item->GetVnum());
		ChatPacket(CHAT_TYPE_INFO, "117");
		return false;
	}

#else
	if (!prt)
	{
		sys_err("RefineInformation NOT GET REFINE SET refineSet: %d itemVnum: %d", item->GetRefineSet(), item->GetVnum());
		ChatPacket(CHAT_TYPE_INFO, "117");
		return false;
	}
#endif

#ifdef ENABLE_MULTI_REFINE_WORLDARD

	if(item->GetRefinedVnum() != 0)
	{

		if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
		{
			// ??º»?º ?¦¿?
			if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
			{
				ChatPacket(CHAT_TYPE_INFO, "115");
				return false;
			}
			p.cost = 0;
		}
		else{
			p.cost = ComputeRefineFee(prt->cost);
		}

		p.prob = prt->prob;
		if (bType == REFINE_TYPE_MONEY_ONLY)
		{
			p.material_count = 0;
			memset(p.materials, 0, sizeof(p.materials));
		}
		else
		{
			p.material_count = prt->material_count;
			thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));
		}

		GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	}

#else
	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		// ??º»?º ?¦¿?
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, "115");
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);

	//END_MAIN_QUEST_LV7
	p.prob = prt->prob;
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset(p.materials, 0, sizeof(p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));
	}
	// END_OF_REFINE_COST

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));
#endif

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	if (prt_multi_refine != 0)
	{
		SendMultiRefinePackets(MULTIREFINE_ADD_INFO,item,bCell,bType,bSpecialStorage);
	}
	
	SendMultiRefinePackets(MULTIREFINE_LOAD_INFO);
	SendMultiRefinePackets(MULTIREFINE_OPEN);
#endif

	SetRefineMode(iAdditionalCell);
	return true;
}

#ifdef ENABLE_MULTI_REFINE_WORLDARD
void CHARACTER::SendMultiRefinePackets(BYTE bSubType, LPITEM item, BYTE bCell, BYTE bType, BYTE bSpecialStorage)
{
	TPacketGCRefineInformation p;
	p.header = HEADER_GC_REFINE_INFORMATION;
	p.bSubType = bSubType;

	if (bSubType == MULTIREFINE_ADD_INFO)
	{
		
		std::map<int, TMultiRefineTable>  m_map_MultiRefineRecipe = CMultiRefineManager::instance().GetMultiRefineRecipe();

		for (itertype(m_map_MultiRefineRecipe) it = m_map_MultiRefineRecipe.begin(); it != m_map_MultiRefineRecipe.end(); ++it)
		{
			
			const TMultiRefineTable* prt = &it->second;

			if (prt->vnum == item->GetVnum())
			{

				CRefineManager & rm = CRefineManager::instance();
				const TRefineTable* prt_old = rm.GetRefineRecipe(prt->refine_set);

				if (!prt_old)
				{
					return;
				}

				p.header = HEADER_GC_REFINE_INFORMATION;
				p.pos = bCell;
				p.src_vnum = item->GetVnum();
				p.result_vnum = prt->refine_vnum;
				p.type = bType;
				p.index = prt->id;
				p.special_storage = bSpecialStorage;

				if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
				{
					// ??º»?º ?¦¿?
					if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
					{
						ChatPacket(CHAT_TYPE_INFO, "115");
						return;
					}
					p.cost = 0;
				}
				else{
					p.cost = ComputeRefineFee(prt_old->cost);
				}

				p.prob = prt_old->prob;

				if (bType == REFINE_TYPE_MONEY_ONLY)
				{
					p.material_count = 0;
					memset(p.materials, 0, sizeof(p.materials));
				}
				else
				{
					p.material_count = prt_old->material_count;
					thecore_memcpy(&p.materials, prt_old->materials, sizeof(prt_old->materials));
				}

				GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));
			}
		}
	}else{
		GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));
	}
}
#endif

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;
	
	if(pkTarget->GetCount() != 1)
	{
		ChatPacket(CHAT_TYPE_INFO,"1077");
		return false;
	}

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// XXX ¼º´?, ¼??? °³·®¼­´? »ç¶ó?³½?´?´?...
		// XXX ¼º´?°³·®¼­´? ??º¹?? ¼­°¡ µ?¾ú´?!
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL){
			if (pkTarget->IsUpgradeItem()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell(),UPGRADE_INVENTORY);
			}
			else if (pkTarget->IsBook()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell(),BOOK_INVENTORY);
			}
			//else if (pkTarget->IsStone()){
			//	RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell(),STONE_INVENTORY);
			//}
			else{
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell(),INVENTORY);
			}
		}
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN){
			//if (pkTarget->IsUpgradeItem()){
			//	RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell(), UPGRADE_INVENTORY);
			//}
			if (pkTarget->IsBook()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell(),BOOK_INVENTORY);
			}
			//else if (pkTarget->IsStone()){
			//	RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell(),STONE_INVENTORY);
			//}
			else{
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell(),INVENTORY);
			}

		}
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			if (pkTarget->IsUpgradeItem()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell(),UPGRADE_INVENTORY);
			}
			else if (pkTarget->IsBook()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell(),BOOK_INVENTORY);
			}
			//else if (pkTarget->IsStone()){
			//	RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell(),STONE_INVENTORY);
			//}
			else{
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell(),INVENTORY);
			}
		}
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			if (pkTarget->IsUpgradeItem()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell(),UPGRADE_INVENTORY);
			}
			else if (pkTarget->IsBook()){
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell(),BOOK_INVENTORY);
			}
			//else if (pkTarget->IsStone()){
			//	RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell(),STONE_INVENTORY);
			//}
			else{
				RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell(),INVENTORY);
			}
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT", pkTarget->GetName());

		bool bHasMetinStone = false;

		const BYTE maxSocketforMetin = 3;
		
		for (int i = 0; i < maxSocketforMetin; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < maxSocketforMetin; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::instance().GetTable(pkTarget->GetSocket(i));
					//pkTarget->SetSocket(i, pTable->alValues[2]);
					// ±ú?øµ¹·? ´ë?¼????´?
					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, "127");
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "kill_campfire_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, "128");
		return false;
	}

	int pos;

	if (item->GetCount() == 1)	// ¾??????? ??³ª¶ó¸é ±?³? ¼???.
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1) // ±?·¸?ö ¾?´?¸é ´?¸¥ ??º¥?ä¸® ½½·??» ?£´?´?.
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, "129");
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
		// ¾?·æ±ºµµ ??¶§
		case 301:
		case 302:
		case 303:
		case 304:
			if( GetLevel() < 90 )
			{
				ChatPacket(CHAT_TYPE_INFO, "130");
				return;
			}
			else
				break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, "131");
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		sys_log(1, "Recall: %s %d %d -> %d %d", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

#ifdef ENABLE_NEW_AFFECT_POTION
bool CHARACTER::SetAffectPotion(LPITEM item)
{
	int blend_get_affect[] = {AFFECT_POTION_1, AFFECT_POTION_2, AFFECT_POTION_3, AFFECT_POTION_4, AFFECT_POTION_5, AFFECT_POTION_6, AFFECT_POTION_7, AFFECT_POTION_8, AFFECT_POTION_9, AFFECT_POTION_10, AFFECT_POTION_11};
	
	int blend_null[] = {APPLY_NONE, AFF_NONE, 0, false};
	
	int blend_list[] = {50821, 50822, 50823, 50824, 50825, 50826, 51003, 51004, 51005, 51006, 51007};
	
	const char* blend_succes = {"<Affect Potion> Set icon ingame for item: |cFFc9ff00|H|h[%s]"};
	
	int	blend_time	= item->GetSocket(2);
	
	switch (item->GetVnum())
	{
		case 50821:
			AddAffect(blend_get_affect[0], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;
		case 50822:
			AddAffect(blend_get_affect[1], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);
			break;
		case 50823:
			AddAffect(blend_get_affect[2], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;
		case 50824:
			AddAffect(blend_get_affect[3], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
		case 50825:
			AddAffect(blend_get_affect[4], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
		case 50826:
			AddAffect(blend_get_affect[5], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;

		case 51003:
			AddAffect(blend_get_affect[6], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
		case 51004:
			AddAffect(blend_get_affect[7], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
		case 51005:
			AddAffect(blend_get_affect[8], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
		case 51006:
			AddAffect(blend_get_affect[9], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
		case 51007:
			AddAffect(blend_get_affect[10], blend_null[0], blend_null[2], blend_null[1], blend_time, blend_null[2], blend_null[3], blend_null[3]);	
			break;	
	}	
		//ChatPacket(CHAT_TYPE_INFO, blend_succes, item->GetName());	
	return true;
}
#endif

int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT	= 30;
	static const int WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int curLife		= ch->GetHP();
		const int needPercent	= WARP_NEED_LIFE_PERCENT;
		const int needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "132");
			return -1;
		}

		consumeLife = needLife;


		// CheckMinLifeForWarp: µ¶¿¡ ????¼­ ???¸¸é ¾?µ?¹?·? »ý¸í·? ??¼?·®´? ³²°???´?
		const int minPercent	= WARP_MIN_LIFE_PERCENT;
		const int minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, "133");
		return -1;
	}

	return needSP;
}

// #define ENABLE_FIREWORK_STUN
#define ENABLE_ADDSTONE_FAILURE
bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	//int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, "130");
					return false;
				}
				break;

			case LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case LIMIT_TIMER_BASED_ON_WEAR:
				//iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		sys_log(0, "USE_ITEM %s, Inven %d, Cell %d, ItemType %d, SubType %d", item->GetName(), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

	if ( CArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, "134");
		return false;
	}
#ifdef ENABLE_NEWSTUFF
	else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && IsLimitedPotionOnPVP(item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, "134");
		return false;
	}
#endif

	// @fixme402 (IsLoadedAffect to block affect hacking)
	if (!IsLoadedAffect())
	{
		ChatPacket(CHAT_TYPE_INFO, "Affects are not loaded yet!");
		return false;
	}

	// @fixme141 BEGIN
	if (TItemPos(item->GetWindow(), item->GetCell()).IsBeltInventoryPosition())
	{
		LPITEM beltItem = GetWear(WEAR_BELT);

		if (NULL == beltItem)
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you have no equipped belt.");
			return false;
		}

		if (false == CBeltInventoryHelper::IsAvailableCell(item->GetCell() - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
		{
			ChatPacket(CHAT_TYPE_INFO, "<Belt> You can't use this item if you don't upgrade your belt.");
			return false;
		}
	}
	// @fixme141 END

	// ¾????? ???? »ç¿ë ????º???´? »ç¿ë???ö ¾?¾?µµ ½?°£?? ?÷°¨µ?´? ¹æ½? ?³¸®.
	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		// ?? ¹ø??¶óµµ »ç¿ë?? ¾????????ö ¿©º?´? Socket1?» º¸°í ??´???´?. (Socket1¿¡ »ç¿ë?½¼ö ±â·?)
		if (0 == item->GetSocket(1))
		{
			// »ç¿ë°¡´?½?°£?º Default °ª?¸·? Limit Value °ª?» »ç¿ë??µ?, Socket0¿¡ °ª?? ???¸¸é ±? °ª?» »ç¿ë??µµ·? ??´?. (´??§´? ??)
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}

		if (false == item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);
	}
#ifdef __NEWPET_SYSTEM__

	if (item->GetVnum() == 55001) {

		LPITEM item2;

		if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
			return false;

		if (item2->IsExchanging())
			return false;

		if (item2->GetVnum() > 55704 || item2->GetVnum() < 55701)
			return false;

		
		char szQuery1[1024];
		snprintf(szQuery1, sizeof(szQuery1), "SELECT duration FROM new_petsystem WHERE id = %lu LIMIT 1", item2->GetID());
		std::auto_ptr<SQLMsg> pmsg2(DBManager::instance().DirectQuery(szQuery1));
		if (pmsg2->Get()->uiNumRows > 0) {
			MYSQL_ROW row = mysql_fetch_row(pmsg2->Get()->pSQLResult);
			if (atoi(row[0]) > 0) {
				if (GetNewPetSystem()->IsActivePet()) {
					ChatPacket(CHAT_TYPE_INFO, "Devi mandare via il tuo pet prima di proseguire.");
					return false;
				}

				DBManager::instance().DirectQuery("UPDATE new_petsystem SET duration =(tduration) WHERE id = %d", item2->GetID());
				ChatPacket(CHAT_TYPE_INFO, "La durata del tuo pet e stata ripristinata.");
			}
			else {
				DBManager::instance().DirectQuery("UPDATE new_petsystem SET duration =(tduration/2) WHERE id = %d", item2->GetID());
				ChatPacket(CHAT_TYPE_INFO, "La durata del tuo pet e stata ripristinata.");
			}
			item->SetCount(item->GetCount() - 1);
			return true;
		}
		else
			return false;
	}

	if (item->GetVnum() >= 55701 && item->GetVnum() <= 55704) {
		LPITEM item2;

		if (item2 = GetItem(DestCell)) {
			if (item2->GetVnum() == 55002) {
				if(item2->GetAttributeValue(0) > 0){
					ChatPacket(CHAT_TYPE_INFO, "Il trasportino contiene gia un Pet.");
				}
				else{
					if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
						return false;

					if (item2->IsExchanging())
						return false;

					if (GetNewPetSystem()->IsActivePet()) {
						ChatPacket(CHAT_TYPE_INFO, "Devi mandare via il tuo Pet prima di proseguire.");
						return false;
					}


					for (int b = 0; b < 3; b++) {
						item2->SetForceAttribute(b, 1, item->GetAttributeValue(b));
					}

					item2->SetForceAttribute(3, 1, item->GetAttributeValue(3));
					item2->SetForceAttribute(4, 1, item->GetAttributeValue(4));
					DWORD vnum1 = item->GetVnum()-55700;
					item2->SetSocket(0, vnum1);
					item2->SetSocket(1, item->GetSocket(1));
					DBManager::instance().DirectQuery("UPDATE new_petsystem SET id =%d WHERE id = %d", item2->GetID(), item->GetID());
					ITEM_MANAGER::instance().RemoveItem(item);
					return true;
				}
			}
		}
	}


	if (item->GetVnum() == 55002 && item->GetAttributeValue(0) > 0) {

		int pos = GetEmptyInventory(item->GetSize());
		if(pos == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, "Non hai abbastanza spazio.");
			return false;
		}

		if (item->IsExchanging())
			return false;
		DWORD vnum2 = 55700+item->GetSocket(0);
		LPITEM item2 = AutoGiveItem(vnum2, 1);
		for (int b = 0; b < 3; b++) {
			item2->SetForceAttribute(b, 1, item->GetAttributeValue(b));
		}
		item2->SetForceAttribute(3, 1, item->GetAttributeValue(3));
		item2->SetForceAttribute(4, 1, item->GetAttributeValue(4));
		item2->SetSocket(1,item->GetSocket(1));
		DBManager::instance().DirectQuery("UPDATE new_petsystem SET id =%d WHERE id = %d", item2->GetID(), item->GetID());
		ITEM_MANAGER::instance().RemoveItem(item);
		return true;

	}
#endif

	switch (item->GetType())
	{


		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM_QUEST:
		{
			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, "134");
					return false;
				}
			}
#ifdef RENEWAL_MISSION_BOOKS
			if (item->GetVnum() >= 50307 && item->GetVnum() <= 50310)
			{
				CHARACTER_MANAGER::Instance().GiveNewMission(item, this);
				return true;
			}
#endif
#ifdef ENABLE_RARITY
			DWORD itemVnum = item->GetVnum();
			if (itemVnum >= 6880 && itemVnum <= 6884)
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!item2->IsRarityItem())
						return false;

					if (itemVnum == 6884)//change rarity
					{
						item2->GiveRandomRarity();
						ChatPacket(CHAT_TYPE_INFO, "1071");
						item->SetCount(item->GetCount() - 1);
					}
					else
					{
						int increasePercent = 5;
						if (itemVnum == 6881)
							increasePercent = 25;
						else if (itemVnum == 6882)
							increasePercent = 50;
						else if (itemVnum == 6883)
							increasePercent = 100;
						if (item2->IncreaseRarity(increasePercent))
						{
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, "1072");
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, "1073");
							return false;
						}

					}
				}
				return true;
			}
#endif

			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
		}
		break;

		case ITEM_CAMPFIRE:
			{
				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy));

				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, "135");
					return false;
				}

				if (tree->IsAttr((long)(GetX()+fx), (long)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, "136");
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy), 0, false, number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();

				info->ch = campfire;

				campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);
			}
			break;

		case ITEM_UNIQUE:
			{
				switch (item->GetSubType())
				{
					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_STR:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_RESIST_MAGIC:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
							if (item->GetSubType() == USE_SPECIAL)
							{
								sys_log(0, "ITEM_UNIQUE: USE_SPECIAL %u", item->GetVnum());
#ifdef ENABLE_MULTI_FARM_BLOCK
								if (item->GetVnum() >= 55610 && item->GetVnum() <= 55615)
								{
									if (FindAffect(AFFECT_MULTI_FARM_PREMIUM))
									{
										ChatPacket(CHAT_TYPE_INFO, "1074");
										return false;
									}
									else
									{
										AddAffect(AFFECT_MULTI_FARM_PREMIUM, POINT_NONE, item->GetValue(1), AFF_NONE, item->GetValue(0), 0, false);
										item->SetCount(item->GetCount() - 1);
										CHARACTER_MANAGER::Instance().CheckMultiFarmAccount(GetDesc()->GetHostName(), GetPlayerID(), GetName(), GetRewardStatus());
										ChatPacket(CHAT_TYPE_INFO, "1075");
										ChatPacket(CHAT_TYPE_INFO, "1076");
									}
								}
#endif
#ifdef ENABLE_OFFLINESHOP_SYSTEM
								if (item->GetVnum() == 71221)
								{
									if(FindAffect(AFFECT_DECORATION))
									{
										ChatPacket(CHAT_TYPE_INFO, "137");
										return false;
									}
									else
									{
										AddAffect(AFFECT_DECORATION, POINT_NONE, 0, AFF_NONE, 60*60*24*30*item->GetValue(0), 0, true); // 1 Gun
										ChatPacket(CHAT_TYPE_INFO, "138 %d",item->GetValue(0));
										item->SetCount(item->GetCount()-1);
									}
								}
#endif
							}
							else
							{
								if (!item->IsEquipped())
									EquipItem(item);
								else
									UnequipItem(item);
							}
						}
						break;
				}
			}
			break;
#ifdef __ITEM_SHINING__
		case ITEM_SHINING:
#endif
		case ITEM_NEW_RINGS:
		case ITEM_COSTUME:
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:		// ½?±? ¹??ö ¾?????
		case ITEM_BELT:		// ½?±? º§?® ¾?????
			// MINING
		case ITEM_PICK:
			// END_OF_MINING
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
			// ?ø¿ë???ö ¾??º ¿ë?¥¼®?º »ç¿ë?? ¼ö ¾ø´?.
			// ?¤»ó?û?? ?¬¶ó¶ó¸é, ¿ë?¥¼®¿¡ °ü??¿© item use ???¶?» º¸³¾ ¼ö ¾ø´?.
			// ¿ë?¥¼® ?ø¿ë?º item move ???¶?¸·? ??´?.
			// ?ø¿ë?? ¿ë?¥¼®?º ???â??´?.
		case ITEM_DS:
			{
				if (!item->IsEquipped())
					return false;
				return DSManager::instance().PullOut(this, NPOS, item);
			break;
			}
		case ITEM_SPECIAL_DS:
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
#ifdef ENABLE_NEW_PET_SYSTEM
		case ITEM_PET:
		{
			switch (item->GetSubType())
			{
			case PET_LEVELABLE:
			{
				time_t now = get_global_time();
				if (GetProtectTime("newpet.ride") > now)
				{
					ChatPacket(CHAT_TYPE_INFO, "139 %d", GetProtectTime("newpet.ride") - now);
					return false;
				}
				SetProtectTime("newpet.ride", now + 1);

				if (time(0) > item->GetSocket(POINT_PET_DURATION))
				{
					ChatPacket(CHAT_TYPE_INFO, "140 I%d", 55001);
					if (item->IsEquipped())
						UnequipItem(item);
					return false;
				}

				if (!item->IsEquipped())
					EquipItem(item);
				else
					UnequipItem(item);
			}
			break;

			case PET_PROTEIN:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;
					long oldTime = item2->GetSocket(POINT_PET_DURATION) - time(0);
					if (oldTime < 0)
						oldTime = 0;
					if (oldTime >= ((60 * 60 * 24 * 20) * 6) - 100)
					{
						ChatPacket(CHAT_TYPE_INFO, "141");
						return false;
					}
					else
					{
						long newTime = MIN((oldTime + 60 * 60 * 24 * 7), (60 * 60 * 24 * 20) * 6);
						item2->SetSocket(POINT_PET_DURATION, time(0) + newTime);
						item->SetCount(item->GetCount() - 1);
						ChatPacket(CHAT_TYPE_INFO, "142 %d", 7);
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							if (petActor->PointChange(POINT_PET_DURATION, 60 * 60 * 24 * 7))
							{
								ChatPacket(CHAT_TYPE_INFO, "142 %d", 7);
								item->SetCount(item->GetCount() - 1);
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, "141");
								return false;
							}
						}
						else
							ChatPacket(CHAT_TYPE_INFO, "143");
					}
				}
			}
			break;

			case PET_DEW:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;

					BYTE bonusType = item->GetValue(0);
					BYTE bonusStep = item->GetValue(1);

					DWORD petLevel = item2->GetSocket(POINT_PET_LEVEL);
					long bonusLevel = item2->GetSocket(POINT_PET_BONUS_1 + bonusType);

					if (bonusStep == 1)
					{
						if (bonusLevel >= 0 && bonusLevel <= 4)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							//item2->UpdatePacket();
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, "144");
							return false;
						}
					}
					else if (bonusStep == 2)
					{
						if (petLevel < 40)
						{
							ChatPacket(CHAT_TYPE_INFO, "145 %d", 40);
							return false;
						}

						if (bonusLevel >= 5 && bonusLevel <= 9)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							//item2->UpdatePacket();
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, "144");
							return false;
						}

					}
					else if (bonusStep == 3)
					{
						if (petLevel < 75)
						{
							ChatPacket(CHAT_TYPE_INFO, "145 %d", 75);
							return false;
						}

						if (bonusLevel >= 10 && bonusLevel <= 14)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							//item2->UpdatePacket();
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, "144");
							return false;
						}
					}
					else if (bonusStep == 4)
					{
						if (petLevel < 100)
						{
							ChatPacket(CHAT_TYPE_INFO, "145 %d", 100);
							return false;
						}

						if (bonusLevel >= 15 && bonusLevel <= 19)
						{
							item2->SetSocket(POINT_PET_BONUS_1 + bonusType, bonusLevel + 1);
							//item2->UpdatePacket();
							item->SetCount(item->GetCount() - 1);
							return true;
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, "144");
							return false;
						}
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							if (petActor->IncreaseBonus(item->GetValue(0), item->GetValue(1)))
								item->SetCount(item->GetCount() - 1);
						}
						else
							ChatPacket(CHAT_TYPE_INFO, "143");
					}
				}
			}
			break;

			case PET_SKILL:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;

					BYTE skillIndex = item->GetValue(0);

					BYTE bySlotIndex = 99;
					for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
					{
						if (item2->GetAttributeType(j) == skillIndex) {
							bySlotIndex = j;
							break;
						}
					}
					if (bySlotIndex == 99)
					{
						BYTE emptyIndex = 99;
						for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
						{
							if (item2->GetAttributeType(j) == 0) {
								emptyIndex = j;
								break;
							}
						}
						if (emptyIndex == 99)
						{
							ChatPacket(CHAT_TYPE_INFO, "146");
							return false;
						}

						item2->SetForceAttribute(emptyIndex, skillIndex, 0);
						ChatPacket(CHAT_TYPE_INFO, "147 I%d", 55009+skillIndex);
						item->SetCount(item->GetCount() - 1);
						return true;
					}
					else
					{
						BYTE type = item2->GetAttributeType(bySlotIndex);
						long value = item2->GetAttributeValue(bySlotIndex);
						if (value > 19)
						{
							ChatPacket(CHAT_TYPE_INFO, "148 I%d", 55009+skillIndex);
							return false;
						}
						item2->SetForceAttribute(bySlotIndex, type, value + 1);
						ChatPacket(CHAT_TYPE_INFO, "149 I%d", 55009+skillIndex);
						item->SetCount(item->GetCount() - 1);
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							if (petActor->IncreaseSkill(item->GetValue(0)))
								item->SetCount(item->GetCount() - 1);
						}
						else
							ChatPacket(CHAT_TYPE_INFO, "143");
					}
				}
			}
			break;

			case PET_SKILL_DEL_BOOK:
			{
				WORD index = DestCell.cell;
				if (index >= 0 && index <= 15)
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor) {
							LPITEM petItem = petActor->GetSummonItem();
							if (!petItem)
								return false;
							BYTE type = petItem->GetAttributeType(index);
							if (type >= 1 && type <= 25)
							{
								petActor->PointChange(POINT_PET_SKILL_INDEX_1 + index, 0);
								petActor->PointChange(POINT_PET_SKILL_LEVEL_1 + index, 0);
								ChatPacket(CHAT_TYPE_INFO, "150 I%d", 55009+type);
								item->SetCount(item->GetCount() - 1);
							}
						}
						else
							ChatPacket(CHAT_TYPE_INFO, "143");
					}
				}
			}
			break;
			case PET_EXPFOOD_PER:
			{
				CPetSystem* pet = GetPetSystem();
				if (pet)
				{
					LPPET petActor = pet->GetNewPet();
					if (petActor) {
						DWORD petExp = petActor->GetNextExp();
						float realExp = (float(petActor->GetNextExp()) / 100.0) * 50.0;
						if (petActor->PointChange(POINT_PET_EXP, int(realExp)))
						{
							ChatPacket(CHAT_TYPE_INFO, "151");
							item->SetCount(item->GetCount() - 1);
						}
						else
							ChatPacket(CHAT_TYPE_INFO, "152");
					}
					else
						ChatPacket(CHAT_TYPE_INFO, "143");
				}
			}
			break;
			case PET_SKILL_ALL_DEL_BOOK:
			{
				LPITEM item2 = GetItem(DestCell);
				if (item2 != NULL)
				{
					if (!(item2->GetType() == ITEM_PET && item2->GetSubType() == PET_LEVELABLE))
						return false;
					bool isHaveSkill = false;
					for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
					{
						BYTE type = item2->GetAttributeType(j);
						if (type != 0 && type != 99)
						{
							isHaveSkill = true;
							break;
						}
					}
					if (!isHaveSkill)
					{
						ChatPacket(CHAT_TYPE_INFO, "153");
						return false;
					}
					else
					{
						for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
						{
							BYTE type = item2->GetAttributeType(j);
							if (type != 0 && type != 99)
								item2->SetForceAttribute(j, 0, 0);
						}
						ChatPacket(CHAT_TYPE_INFO, "154");
						item->SetCount(item->GetCount() - 1);
					}
				}
				else
				{
					CPetSystem* pet = GetPetSystem();
					if (pet)
					{
						LPPET petActor = pet->GetNewPet();
						if (petActor)
						{
							LPITEM petItem = petActor->GetSummonItem();
							if (!petItem)
								return false;
							bool isHaveSkill = false;
							for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
							{
								BYTE type = petItem->GetAttributeType(j);
								if (type != 0 && type != 99)
								{
									isHaveSkill = true;
									break;
								}
							}
							if (!isHaveSkill)
							{
								ChatPacket(CHAT_TYPE_INFO, "153");
								return false;
							}
							else
							{
								for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
								{
									BYTE type = petItem->GetAttributeType(j);
									if (type != 0 && type != 99)
										petItem->SetForceAttribute(j, 0, 0);
								}
								ChatPacket(CHAT_TYPE_COMMAND, "UpdatePet %d", POINT_PET_SKILL_INDEX_1);
								ChatPacket(CHAT_TYPE_INFO, "154");
								item->SetCount(item->GetCount() - 1);
							}
						}
						else
							ChatPacket(CHAT_TYPE_INFO, "143");
					}
				}
			}
			break;
			}
		}
		break;
#endif
		case ITEM_FISH:
			{
				if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, "134");
					return false;
				}
#ifdef ENABLE_NEWSTUFF
				else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
				{
					ChatPacket(CHAT_TYPE_INFO, "134");
					return false;
				}
#endif

				if (item->GetSubType() == FISH_ALIVE)
					fishing::UseFish(this, item);
			}
			break;

		case ITEM_TREASURE_BOX:
			{
				return false;
				ChatPacket(CHAT_TYPE_TALKING, "155");
			}
			break;

		case ITEM_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() != ITEM_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, "156");
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					ChatPacket(CHAT_TYPE_TALKING, "157");
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets(0);
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
					{
						item->SetCount(item->GetCount()-1);
						item2->SetCount(item2->GetCount()-1);
						
						for (int i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
#ifdef __CHAT_SETTINGS__
									ChatPacket(CHAT_TYPE_MONEY_INFO, "158 %d", dwCounts[i]);
#else
									ChatPacket(CHAT_TYPE_INFO, "158 %d", dwCounts[i]);
#endif
									break;
								case CSpecialItemGroup::EXP:
									ChatPacket(CHAT_TYPE_INFO, "159");
#ifdef __CHAT_SETTINGS__
									ChatPacket(CHAT_TYPE_EXP_INFO, "160 %d", dwCounts[i]);
#else
									ChatPacket(CHAT_TYPE_INFO, "160 %d", dwCounts[i]);
#endif
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, "161");
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, "162");
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, "163");
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, "164");
									break;
#ifdef ENABLE_WOLFMAN_CHARACTER
								case CSpecialItemGroup::BLEEDING:
									ChatPacket(CHAT_TYPE_INFO, "164");
									break;
#endif
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, "161");
									break;
								default:
									if (item_gets[i])
									{
#ifdef __CHAT_SETTINGS__
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_ITEM_INFO, "165 %s %d", item_gets[i]->GetName(), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_ITEM_INFO, "166 %s", item_gets[i]->GetName());
#else
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_INFO, "165 %s %d", item_gets[i]->GetName(), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_INFO, "166 %s", item_gets[i]->GetName());
#endif
									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, "167");
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, "167");
					return false;
				}
			}
			break;

		case ITEM_GIFTBOX:
			{
#ifdef ENABLE_NEWSTUFF
				if (0 != g_BoxUseTimeLimitValue)
				{
					if (get_dword_time() < m_dwLastBoxUseTime+g_BoxUseTimeLimitValue)
					{
						ChatPacket(CHAT_TYPE_INFO, "168");
						return false;
					}
				}

				m_dwLastBoxUseTime = get_dword_time();
#endif
				DWORD dwBoxVnum = item->GetVnum();
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				std::vector <LPITEM> item_gets(0);
				int count = 0;

				if( dwBoxVnum > 51500 && dwBoxVnum < 52000 )	// ¿ë?¥¿ø¼®µé
				{
					if( !(this->DragonSoul_IsQualified()) )
					{
						ChatPacket(CHAT_TYPE_INFO,"169");
						return false;
					}
				}

#if defined(__BL_67_ATTR__)
				switch (dwBoxVnum)
				{
				case POWERSHARD_CHEST:
					// The Powershard Chest can be purchased from Seon-Hae in exchange for 10 Skill Books.(From Wiki)
					// It can contain Powershards of any kind or a Skill Book.(From Wiki)
					// You can edit here for skill books(From black)
					if (number(1, 100) <= 30)
						AutoGiveItem(CItemVnumHelper::Get67MaterialVnum(number(0, gPlayerMaxLevel)));
					else
						ChatPacket(CHAT_TYPE_TALKING, "170");
					item->SetCount(item->GetCount() - 1);
					return true;
				case ELEGANT_POWERSHARD_CHEST:
					if (number(1, 100) <= 60)
						AutoGiveItem(CItemVnumHelper::Get67MaterialVnum(number(0, gPlayerMaxLevel)));
					else
						ChatPacket(CHAT_TYPE_TALKING, "170");
					item->SetCount(item->GetCount() - 1);
					return true;
				case LUCENT_POWERSHARD_CHEST:
					for (BYTE _i = 0; _i < 5; _i++)
						AutoGiveItem(CItemVnumHelper::Get67MaterialVnum(number(0, gPlayerMaxLevel)));
					item->SetCount(item->GetCount() - 1);
					return true;
				default:
					break;
				}
#endif

#ifdef ENABLE_MULTI_CHEST
				OpenChest(item, 1);
				return true;
#else
	
				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
				{

					item->SetCount(item->GetCount()-1);

#ifdef BATTLEPASS_WORLDARD
					CBattlePass::instance().UpdateMisionBattlePass(this, OPEN_CHEST_BP, 1);
#endif

					for (int i = 0; i < count; i++){
						switch (dwVnums[i])
						{
						case CSpecialItemGroup::GOLD:
#ifdef __CHAT_SETTINGS__
							ChatPacket(CHAT_TYPE_MONEY_INFO, "158 %d", dwCounts[i]);
#else
							ChatPacket(CHAT_TYPE_INFO, "158 %d", dwCounts[i]);
#endif
							break;
						case CSpecialItemGroup::EXP:
							ChatPacket(CHAT_TYPE_INFO, "159");
#ifdef __CHAT_SETTINGS__
							ChatPacket(CHAT_TYPE_EXP_INFO, "160 %d", dwCounts[i]);
#else
							ChatPacket(CHAT_TYPE_INFO, "160 %d", dwCounts[i]);
#endif
							break;
						case CSpecialItemGroup::MOB:
							ChatPacket(CHAT_TYPE_INFO, "161");
							break;
						case CSpecialItemGroup::SLOW:
							ChatPacket(CHAT_TYPE_INFO, "162");
							break;
						case CSpecialItemGroup::DRAIN_HP:
							ChatPacket(CHAT_TYPE_INFO, "163");
							break;
						case CSpecialItemGroup::POISON:
							ChatPacket(CHAT_TYPE_INFO, "164");
							break;
#ifdef ENABLE_WOLFMAN_CHARACTER
						case CSpecialItemGroup::BLEEDING:
							ChatPacket(CHAT_TYPE_INFO, "164");
							break;
#endif
						case CSpecialItemGroup::MOB_GROUP:
							ChatPacket(CHAT_TYPE_INFO, "161");
							break;
						default:
							if (item_gets[i])
							{
#ifdef __CHAT_SETTINGS__
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_ITEM_INFO, "165 %s %d", item_gets[i]->GetName(), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_ITEM_INFO, "166 %s", item_gets[i]->GetName());
#else
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_INFO, "165 %s %d", item_gets[i]->GetName(), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_INFO, "166 %s", item_gets[i]->GetName());
#endif
							}
						}
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, "171");
					return false;
				}
#endif
			}
			break;

		case ITEM_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return false;
				}

				DWORD dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, "172");
				}
				else
					ChatPacket(CHAT_TYPE_INFO, "173");
			}
			break;

		case ITEM_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, "174");
					return false;
				}

				DWORD dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{
					// »õ·?¿î ¼ö·?¼­´? value 0 ¿¡ ½º?³ ¹ø?£°¡ ???¸¹?·? ±?°??» »ç¿ë.
					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::instance().RemoveItem(item);

					return false;
				}

				if (true == LearnSkillByBook(dwVnum))
				{
#ifdef ENABLE_BOOKS_STACKFIX
					item->SetCount(item->GetCount() - 1);
#else
					ITEM_MANAGER::instance().RemoveItem(item);
#endif

					int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					if (distribution_test_server)
						iReadDelay /= 3;

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM_USE:
			{


				if(!CanUseItem(GetMapIndex(),item->GetVnum()))
				{
					ChatPacket(CHAT_TYPE_INFO, "You can't use this");
					return false;
				}

				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						sys_log (0, "ADD addtional effect : vnum(%d) subtype(%d)", item->GetOriginalVnum(), item->GetSubType());


					int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int apply_value = item->GetValue(2);
					int apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, "175");
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_STR:
									case APPLY_DEX:
									case APPLY_CON:
									case APPLY_INT:
									case APPLY_CAST_SPEED:
									case APPLY_RESIST_MAGIC:
									case APPLY_ATT_GRADE_BONUS:
									case APPLY_DEF_GRADE_BONUS:
										AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							item->SetCount(item->GetCount() - 1);
							break;

					case USE_AFFECT :
						{
							if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, "175");
							}
							else
							{
								// PC_BANG_ITEM_ADD
								if (item->IsPCBangItem() == true)
								{
									// PC¹æ???ö ?¼?©??¼­ ?³¸®
									if (CPCBangManager::instance().IsPCBangIP(GetDesc()->GetHostName()) == false)
									{
										// PC¹æ?? ¾?´?!
										ChatPacket(CHAT_TYPE_INFO, "176");
										return false;
									}
								}
								// END_PC_BANG_ITEM_ADD

								AddAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_POTION_NODELAY:
						{

							if(!CanUseItem(GetMapIndex(),item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "You can't use this");
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, "177");
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, "178");
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, "177");
										return false;
										break;
								}
							}
#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "134");
								return false;
							}
#endif

							bool used = false;

							if (item->GetValue(0) != 0) // HP ?ý´ë°ª ?¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP ?ý´ë°ª ?¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % ?¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % ?¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, "179");
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}


				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{

					if(!CanUseItem(GetMapIndex(),item->GetVnum()))
					{
						ChatPacket(CHAT_TYPE_INFO, "You can't use this");
						return false;
					}

					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						ChatPacket(CHAT_TYPE_INFO, "134");
						return false;
					}
#ifdef ENABLE_NEWSTUFF
					else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
					{
						ChatPacket(CHAT_TYPE_INFO, "134");
						return false;
					}
#endif
				}

				if (test_server)
				{
					 sys_log (0, "USE_ITEM %s Type %d SubType %d vnum %d", item->GetName(), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
					case USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// ¿ì¼± ¿ë?¥¼®¿¡ °ü??¼­¸¸ ??µµ·? ??´?.
							if (pDestItem->IsDragonSoul())
							{
								int ret;
								char buf[128];
								if (item->GetVnum() == DRAGON_HEART_VNUM)
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
								}
								else
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								}
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%ld}", ret, item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, "180 %d", ret);
									item->SetCount(item->GetCount() - 1);
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%ld}", item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, "181");
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// ¿ì¼± ¿ë?¥¼®¿¡ °ü??¼­¸¸ ??µµ·? ??´?.
							if (pDestItem->IsDragonSoul())
							{
								int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, "180 %d", ret);
									sprintf(buf, "Increase %ds by item{VN:%d VAL%d:%ld}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, "181");
									sprintf(buf, "No change by item{VN:%d VAL%d:%ld}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_SPECIAL:

						switch (item->GetVnum())
						{
#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
							case 70002:
							{
								if(FindAffect(AFFECT_AUTO_PICK_UP))
								{
									ChatPacket(CHAT_TYPE_INFO, "1099");
									return false;
								}
								const int affectime = 60 * 60 * 24 * 30; // 30 DAY
								AddAffect(AFFECT_AUTO_PICK_UP, POINT_NONE, 0, AFF_NONE, affectime, 0, false, true);
								item->SetCount(item->GetCount()-1);
								ChatPacket(CHAT_TYPE_INFO, "1100 %d", 30);
								return true;
							}
							break;
#endif
#ifdef ENABLE_SOULBIND_SYSTEM
							case 50263:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;
									
									if (item2->IsEquipped() || item2->IsExchanging())
										return false;
									
									if (item2->IsSealed()){
										ChatPacket(CHAT_TYPE_INFO, "182");
										return false;
									}
									
									if (item2->GetType() != ITEM_WEAPON && item2->GetType() != ITEM_ARMOR && item2->GetType() != ITEM_COSTUME)
										return false;

									item2->SetSealBind();
									ChatPacket(CHAT_TYPE_INFO, "183");
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "SET_SEALBIND_SUCCESS", buf);
									}
									item->SetCount(item->GetCount() - 1);
								}
								break;
							
							case 50264:
								{
									LPITEM item2;
									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;
									
									if(item2->isLocked() || item2->IsEquipped() || item2->GetSealBindTime() >= 0)
										return false;
									
									long duration = 72 * 60 * 60;
									item2->SetSealBind(time(0) + duration);
									item2->StartUnSealBindTimerExpireEvent();
									ChatPacket(CHAT_TYPE_INFO, "184");
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "REMOVE_SEALBIND_TIME_BEGIN", buf);
									}
									item->SetCount(item->GetCount() - 1);
								}
								break;
#endif
#ifdef ENABLE_ZODIAC_MISSION
						case 33031: // 12 Bead 
								{
									int bCount = GetQuestFlag("Bead.Count");
									SetQuestFlag("Bead.Count",GetQuestFlag("Bead.Count")+12);
									bCount = GetQuestFlag("Bead.Count"); // Update
									ChatPacket(CHAT_TYPE_COMMAND, "SetBeadCount %d",bCount);
									item->SetCount(item->GetCount()-1);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d Adet Animasfer eklendi. "),12);
								}
								break;
							case 33032: // 24 Bead 
								{
									int bCount = GetQuestFlag("Bead.Count");
									SetQuestFlag("Bead.Count",GetQuestFlag("Bead.Count")+24);
									bCount = GetQuestFlag("Bead.Count"); // Update
									ChatPacket(CHAT_TYPE_COMMAND, "SetBeadCount %d",bCount);
									item->SetCount(item->GetCount()-1);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d Adet Animasfer eklendi. "),24);
								}
								break;
							case 33033: // 36 Bead 
								{
									int bCount = GetQuestFlag("Bead.Count");
									SetQuestFlag("Bead.Count",GetQuestFlag("Bead.Count")+36);
									bCount = GetQuestFlag("Bead.Count"); // Update
									ChatPacket(CHAT_TYPE_COMMAND, "SetBeadCount %d",bCount);
									item->SetCount(item->GetCount()-1);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d Adet Animasfer eklendi. "),36);
								}
								break;

							case 33034: // 48 Bead 
								{
									int bCount = GetQuestFlag("Bead.Count");
									SetQuestFlag("Bead.Count",GetQuestFlag("Bead.Count")+12);
									bCount = GetQuestFlag("Bead.Count"); // Update
									ChatPacket(CHAT_TYPE_COMMAND, "SetBeadCount %d",bCount);
									item->SetCount(item->GetCount()-1);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d Adet Animasfer eklendi. "),12);
								}
								break;

							case 33035: // 60 Bead 
								{
									int bCount = GetQuestFlag("Bead.Count");
									SetQuestFlag("Bead.Count",GetQuestFlag("Bead.Count")+60);
									bCount = GetQuestFlag("Bead.Count"); // Update
									ChatPacket(CHAT_TYPE_COMMAND, "SetBeadCount %d",bCount);
									item->SetCount(item->GetCount()-1);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d Adet Animasfer eklendi. "),60);
								}
								break;
#endif
							//?©¸®½º¸¶½º ¶õ??
							case ITEM_NOG_POCKET:
								{
									/*
									¶õ??´?·??¡ : item_proto value ??¹?
										??µ¿¼?µµ  value 1
										°ø°?·?	  value 2
										°æ?è?¡    value 3
										?ö¼?½?°£  value 0 (´??§ ??)

									*/
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, "175");
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;

							//¶ó¸¶´?¿ë »ç??
							case ITEM_RAMADAN_CANDY:
								{
									/*
									»ç??´?·??¡ : item_proto value ??¹?
										??µ¿¼?µµ  value 1
										°ø°?·?	  value 2
										°æ?è?¡    value 3
										?ö¼?½?°£  value 0 (´??§ ??)

									*/
									// @fixme147 BEGIN
									if (FindAffect(AFFECT_RAMADAN_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, "175");
										return false;
									}
									// @fixme147 END
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case ITEM_MARRIAGE_RING:
								{
									marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
									if (pMarriage)
									{
										if (pMarriage->ch1 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, "134");
												break;
											}
#if defined(WJ_COMBAT_ZONE)	
											if (CCombatZoneManager::Instance().IsCombatZoneMap(pMarriage->ch1->GetMapIndex()))
											{
												ChatPacket(CHAT_TYPE_INFO, "185");
												break;
											}
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
											if (CTournamentPvP::instance().IsTournamentMap(pMarriage->ch1, TOURNAMENT_BLOCK_RING_MARRIAGE)){
												return false;
											}
#endif
										}

										if (pMarriage->ch2 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, "134");
												break;
											}
#if defined(WJ_COMBAT_ZONE)	
											if (CCombatZoneManager::Instance().IsCombatZoneMap(pMarriage->ch2->GetMapIndex()))
											{
												ChatPacket(CHAT_TYPE_INFO, "185");
												break;
											}
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
											if (CTournamentPvP::instance().IsTournamentMap(pMarriage->ch2, TOURNAMENT_BLOCK_RING_MARRIAGE)){
												return false;
											}
#endif
										}

										int consumeSP = CalculateConsumeSP(this);

										if (consumeSP < 0)
											return false;

										PointChange(POINT_SP, -consumeSP, false);

										WarpToPID(pMarriage->GetOther(GetPlayerID()));
									}
									else
										ChatPacket(CHAT_TYPE_INFO, "186");
								}
								break;

								//±â?¸ ¿ë±â?? ¸??ä
							case UNIQUE_ITEM_CAPE_OF_COURAGE:
								//¶ó¸¶´? º¸»ó¿ë ¿ë±â?? ¸??ä
							case 70057:
							case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
								AggregateMonster();
								// item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							case 30093:
							case 30094:
							case 30095:
							case 30096:
								// º¹??¸?´?
								{
									const int MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										DWORD count;
										int prob;
										DWORD vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									LuckyBagInfo * bi = NULL;
									bi = b1;

									int pct = number(1, 1000);

									int i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										// ½º?³¼ö·?¼­´? ?¯¼ö??°? ??´?.
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										PointChange(POINT_GOLD, 1000, true);
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;

#ifdef BATTLEPASS_WORLDARD
							case 93101:
							case 93102:
							case 93103:
							case 93104:
							case 93105:
							case 93106:
							case 93107:
							case 93108:
							case 93109:
							case 93110:
							case 93111:
							case 93112:
								{
									int iSeconds = GetSecondsTillNextMonth();
			
									if(iSeconds < 0)
									{
										ChatPacket(CHAT_TYPE_INFO, "Error BattlePass 01");
										return false;
									}

									if(FindAffect(AFFECT_BATTLE_PASS))
									{
										ChatPacket(CHAT_TYPE_INFO, "Usted ya tiene activado el battlepass");
										return false;
									}
									else
									{

										time_t iTime;
										time(&iTime);
										struct tm endTime = *localtime(&iTime);
										int iCurrentMonth = endTime.tm_mon;

										if (iCurrentMonth+1 != item->GetValue(0)){
											ChatPacket(CHAT_TYPE_INFO,"Ese battlepass dejo de funcionar");
											return false;
										}

										m_dwBattlePassEndTime = get_global_time() + iSeconds;
										CBattlePass::instance().ResetMisionBattlePass(this);
										AddAffect(AFFECT_BATTLE_PASS, POINT_BATTLE_PASS_ID, item->GetValue(0), 0, iSeconds, 0, true);
										ITEM_MANAGER::instance().RemoveItem(item);
										ChatPacket(CHAT_TYPE_INFO,"Acabas de activar el Pase de Batalla.");

									}

								}	
								break;

#endif

							case 50004: // ??º¥?®¿ë °¨?ö±â
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{
										// ?³?½ »ç¿ë½?
										int iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::instance().GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "187");
											return false;
										}
									}

									int dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{
										// ¹?°?!
										ChatPacket(CHAT_TYPE_INFO, "188");

										// »ç¿ë?½¼ö¿¡ µû¶ó ??´? ¾??????» ´?¸£°? ??´?.
										struct TEventStoneInfo
										{
											DWORD dwVnum;
											int count;
											int prob;
										};
										const int EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50001,  1,  0 },
											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50001,  1,  0 },
											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50001,  1, 10 },
											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int prob = number(1, 100);

											for (int i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													if (info[i].dwVnum == 50001)
													{
														DWORD * pdw = M2_NEW DWORD[2];

														pdw[0] = info[i].dwVnum;
														pdw[1] = info[i].count;

														// ???·¼­´? ¼????» ¼³?¤??´?
														DBManager::instance().ReturnQuery(QID_LOTTO, GetPlayerID(), pdw,
																"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())",
																get_table_postfix(), GetPlayerID());
													}
													else
														AutoGiveItem(info[i].dwVnum, info[i].count);

													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (DWORD)GetVID());

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ¹®??±î?ö º¸³»±â

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id	= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());

										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;

									// ¸¹?? »ç¿ë???¸¸é »ç¶ó?ø´?.
									const int STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, "189");
										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(DWORD)GetVID(), dist, (int)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ¹®??±î?ö º¸³»±â

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id		= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());
									}

								}
								break;

							case 27989: // ¿µ¼®°¨?ö±â
							case 76006: // ¼±¹°¿ë ¿µ¼®°¨?ö±â
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

									if (pMap != NULL)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											DWORD max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												DWORD dist = (DWORD)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												stone++;
											}

											if (pTarget != NULL)
											{
												int val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
														(int)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "190");
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "190");
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
											ITEM_MANAGER::instance().RemoveItem(item);
										}
									}
									break;
								}
								break;

							case 27996: // µ¶º´
								item->SetCount(item->GetCount() - 1);
								AttackedByPoison(NULL); // @warme008
								break;

							case 27987: // ?¶°³
								// 50  µ¹?¶°¢ 47990
								// 30  ²?
								// 10  ¹é?ø?? 47992
								// 7   ?»?ø?? 47993
								// 3   ???ø?? 47994
								{
									item->SetCount(item->GetCount() - 1);

									int r = number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, "191");
										AutoGiveItem(27990);
									}
									else
									{
										const int prob_table_gb2312[] =
										{
											95, 97, 99
										};

										const int * prob_table = prob_table_gb2312;

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, "192");
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, "193");
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, "194");
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "195");
											AutoGiveItem(27994);
										}
									}
								}
								break;

							case 71013: // ???¦¿ë?ø??
								CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100: // ?ø??
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50301: // ?ë¼?·? ¼ö·?¼­
							case 50302:
							case 50303:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, "196");
										return false;
									}

									int lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, "197");
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, "198");
										return false;
									}

									if (LearnSkillByBook(SKILL_LEADERSHIP))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50304: // ¿¬°è±â ¼ö·?¼­
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, "174");
										return false;

									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, "199");
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, "200");
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, "201");
										return false;
									}

									int iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;
							case 50311: // ¾ð¾î ¼ö·?¼­
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, "174");
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, "202");
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 : // ??º» ¸» ¼??¯ ½º?³ ¼ö·?¼­
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, "174");
										return false;

									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, "203");
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316: // º¯½? ¼ö·?¼­
							case 50323: case 50324: // ?õ?÷ ¼ö·?¼­
							case 50325: case 50326: // ?¶?ë ¼ö·?¼­
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, "196");
										return false;
									}

									int iSkillLevelLowLimit = item->GetValue(0);
									int iSkillLevelHighLimit = item->GetValue(1);
									int iPct = MINMAX(0, item->GetValue(2), 100);
									int iLevelLimit = item->GetValue(3);
									DWORD dwSkillVnum = 0;

									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, "204");
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, "203");
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, "197");
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, "205");
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, "174");
										return false;

									}
									DWORD dwSkillVnum = SKILL_CREATE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, "203");
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server)
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, "174");
										return false;

									}
									DWORD dwSkillVnum = SKILL_MINING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, "203");
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
#ifdef ENABLE_BOOKS_STACKFIX
										item->SetCount(item->GetCount() - 1);
#else
										ITEM_MANAGER::instance().RemoveItem(item);
#endif

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, "174");
										return false;

									}
									DWORD dwSkillVnum = SKILL_HORSE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, "206");
										return false;
									}

									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{
											// ??¾?¼ú¼­ »ç¿ë??¿¡´? ½?°£ ?¦?? ¹«½?
											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, "207");
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, "208");
										return false;
									}

									if (number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, "209");
										ChatPacket(CHAT_TYPE_INFO, "210");
										PointChange(POINT_HORSE_SKILL, 1);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "211");
									}
#ifdef ENABLE_BOOKS_STACKFIX
									item->SetCount(item->GetCount() - 1);
#else
									ITEM_MANAGER::instance().RemoveItem(item);
#endif
								}
								break;

							case 70102: // ¼±µ?
							case 70103: // ¼±µ?
								{
									if (GetAlignment() >= 0)
										return false;

									int delta = MIN(-GetAlignment(), item->GetValue(0));

									sys_log(0, "%s ALIGNMENT ITEM %d", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, "212");
										ChatPacket(CHAT_TYPE_INFO, "213 %d", delta/10);
									}
								}
								break;

							case 71107: // ?µµµº¹¼þ¾?
								{
									int val = item->GetValue(0);
									int interval = item->GetValue(1);
									quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());
									int last_use_time = pPC->GetFlag("mythical_peach.last_use_time");

									if (get_global_time() - last_use_time < interval * 60 * 60)
									{
										if (test_server == false)
										{
											ChatPacket(CHAT_TYPE_INFO, "214");
											return false;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "215");
										}
									}

									if (GetAlignment() == 200000)
									{
										ChatPacket(CHAT_TYPE_INFO, "216");
										return false;
									}

									if (200000 - GetAlignment() < val * 10)
									{
										val = (200000 - GetAlignment()) / 10;
									}

									int old_alignment = GetAlignment() / 10;

									UpdateAlignment(val*10);

									item->SetCount(item->GetCount()-1);
									pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

									ChatPacket(CHAT_TYPE_TALKING, "212");
									ChatPacket(CHAT_TYPE_INFO, "213 %d", val);

									char buf[256 + 1];
									snprintf(buf, sizeof(buf), "%d %d", old_alignment, GetAlignment() / 10);
									LogManager::instance().CharLog(this, val, "MYTHICAL_PEACH", buf);
								}
								break;

							case 71109: // ?»¼®¼­
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
										return false;

									if (item2->GetSocketCount() == 0)
										return false;
				
									switch( item2->GetType() )
									{
										case ITEM_WEAPON:
											break;
										case ITEM_ARMOR:
											switch (item2->GetSubType())
											{
											case ARMOR_EAR:
											case ARMOR_WRIST:
											case ARMOR_NECK:
#ifdef ENABLE_GLOVE_SYSTEM
											case ARMOR_GLOVE:
#endif
#ifdef ITEM_TALISMAN_EQUIPMENT
											case ARMOR_PENDANT:
#endif
												ChatPacket(CHAT_TYPE_INFO, "217");
												return false;
											}
											break;

										default:
											return false;
									}

									std::stack<long> socket;

									const BYTE maxSocketforMetin = 3;

									for (int i = 0; i < maxSocketforMetin; ++i)
										socket.push(item2->GetSocket(i));

									int idx = maxSocketforMetin - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, "217");
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != NULL)
									{
										item2->SetSocket(idx, 1);

										char buf[256+1];
										snprintf(buf, sizeof(buf), "%s(%u) %s(%u)",
												item2->GetName(), item2->GetID(), pItemReward->GetName(), pItemReward->GetID());
										LogManager::instance().ItemLog(this, item, "USE_DETACHMENT_ONE", buf);

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:   // ?»»ö?¦
							case 70202:   // ¿°»ö¾?(?ò»ö)
							case 70203:   // ¿°»ö¾?(±?»ö)
							case 70204:   // ¿°»ö¾?(»¡°£»ö)
							case 70205:   // ¿°»ö¾?(°¥»ö)
							case 70206:   // ¿°»ö¾?(°??º»ö)
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, "218");
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										quest::CQuestManager& q = quest::CQuestManager::instance();
										quest::PC* pPC = q.GetPC(GetPlayerID());

										if (pPC)
										{
											int last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "219 %d", last_dye_level+3);
											}
										}
									}
								}
								break;

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
#ifdef ENABLE_MULTI_CHEST
									OpenChest(item, 1);
									return true;
#else
									
									DWORD dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
#ifdef __CHAT_SETTINGS__
												ChatPacket(CHAT_TYPE_MONEY_INFO, "158 %d", dwCounts[i]);
#else
												ChatPacket(CHAT_TYPE_INFO, "158 %d", dwCounts[i]);
#endif
										}

										item->SetCount(item->GetCount() - 1);
									}
#endif
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
#ifdef ENABLE_MULTI_CHEST
									OpenChest(item, 1);
									return true;
#else
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;
									if (((item->GetVnum() == ITEM_VALENTINE_ROSE) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_VALENTINE_CHOCOLATE) && (SEX_FEMALE==GET_SEX(this))))
									{
										// ¼ºº°?? ¸??ö¾?¾? ¾µ ¼ö ¾ø´?.
										ChatPacket(CHAT_TYPE_INFO, "220");
										return false;
									}
									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
#endif
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
#ifdef ENABLE_MULTI_CHEST
									OpenChest(item, 1);
									return true;
#else
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;
									if (((item->GetVnum() == ITEM_WHITEDAY_CANDY) && (SEX_MALE==GET_SEX(this))) ||
										((item->GetVnum() == ITEM_WHITEDAY_ROSE) && (SEX_FEMALE==GET_SEX(this))))
									{
										// ¼ºº°?? ¸??ö¾?¾? ¾µ ¼ö ¾ø´?.
										ChatPacket(CHAT_TYPE_INFO, "220");
										return false;
									}
									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
#endif
								}
								break;

							case 50011: // ¿ù±¤º¸??
								{
#ifdef ENABLE_MULTI_CHEST
									OpenChest(item, 1);
									return true;
#else
									DWORD dwBoxVnum = 50011;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);
											LogManager::instance().ItemLog(this, item, "MOONLIGHT_GET", buf);

											//ITEM_MANAGER::instance().RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
#ifdef __CHAT_SETTINGS__
												ChatPacket(CHAT_TYPE_MONEY_INFO, "158 %d", dwCounts[i]);
#else
												ChatPacket(CHAT_TYPE_INFO, "158 %d", dwCounts[i]);
#endif
												break;

											case CSpecialItemGroup::EXP:
												ChatPacket(CHAT_TYPE_INFO, "159");
#ifdef __CHAT_SETTINGS__
												ChatPacket(CHAT_TYPE_EXP_INFO, "160 %d", dwCounts[i]);
#else
												ChatPacket(CHAT_TYPE_INFO, "160 %d", dwCounts[i]);
#endif
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, "161");
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, "162");
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, "163");
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, "164");
												break;
#ifdef ENABLE_WOLFMAN_CHARACTER
											case CSpecialItemGroup::BLEEDING:
												ChatPacket(CHAT_TYPE_INFO, "164");
												break;
#endif
											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, "161");
												break;

											default:
												if (item_gets[i])
												{
#ifdef __CHAT_SETTINGS__
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_ITEM_INFO, "165 %s %d", item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_ITEM_INFO, "166 %s", item_gets[i]->GetName());
#else
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, "165 %s %d", item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, "166 %s", item_gets[i]->GetName());
#endif
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, "171");
										return false;
									}
#endif
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									//PointChange(POINT_GOLD, -iCost);
									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50107:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, "134");
										return false;
									}
#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, "134");
										return false;
									}
#endif

									EffectPacket(SE_CHINA_FIREWORK);
#ifdef ENABLE_FIREWORK_STUN
									// ½º?? °ø°??» ¿?·???´?
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, "134");
										return false;
									}
#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, "134");
										return false;
									}
#endif

									EffectPacket(SE_SPIN_TOP);
#ifdef ENABLE_FIREWORK_STUN
									// ½º?? °ø°??» ¿?·???´?
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
#endif
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case 90008: // VCARD
							case 90009: // VCARD
								VCardUse(this, this, item);
								break;

							case ITEM_ELK_VNUM: // µ·²?·¯¹?
								{
									int iGold = item->GetSocket(0);
									ITEM_MANAGER::instance().RemoveItem(item);
									ChatPacket(CHAT_TYPE_INFO, "158 %d", iGold);
									PointChange(POINT_GOLD, iGold);
								}
								break;

								//±º???? ?õ?¥
							case 70021:
								{
									int HealPrice = quest::CQuestManager::instance().GetEventFlag("MonarchHealGold");
									if (HealPrice == 0)
										HealPrice = 2000000;

									if (CMonarch::instance().HealMyEmpire(this, HealPrice))
									{
										char szNotice[256];
										snprintf(szNotice, sizeof(szNotice), "221 %s", EMPIRE_NAME(GetEmpire()));
										SendNoticeMap(szNotice, GetMapIndex(), false);

										ChatPacket(CHAT_TYPE_INFO, "222");
									}
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 : // º¯½? ???¼º? ??½?
								{
									if (m_pkChrTarget != NULL)
									{
										if (m_pkChrTarget->IsPolymorphed())
										{
											m_pkChrTarget->SetPolymorph(0);
											m_pkChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;
#ifdef CHANGE_DS_ATRIBUTES
							case 79001 :
								{
									LPITEM item2;

									if (!DSManager::instance().IsValidCellForThisItem(item2, DestCell) || !item2->IsDragonSoul())
										return false;
									
									if (ITEM_DS != item2->GetType())
									{
										ChatPacket(CHAT_TYPE_INFO, "223");
										return false;
									}
									
									if (item2->IsExchanging()) // @fixme114
										return false;

									item->SetCount(item->GetCount() - 1);
									// if (DSManager::PutAttributes(item2)){
										// ChatPacket(CHAT_TYPE_INFO, "224");
									// }
									// else{
										// ChatPacket(CHAT_TYPE_INFO, "225");
									// }
								}
								break;
#endif
#if !defined(__BL_67_ATTR__)
							case 71051 : // 진?��?
								{
									// ?�?? ?��??? 베?��?진?��? ?�용�?지
									if (LC_IsEurope() || LC_IsSingapore() || LC_IsVietnam())
										return false;

									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, "226");
										return false;
									}

									if (item2->AddRareAttribute() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, "227");

										int iAddedIdx = item2->GetRareAttrCount() + 4;
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										LogManager::instance().ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_RARE_ATTR",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "228");
									}
								}
								break;
#endif
#if !defined(__BL_67_ATTR__)
							case 71052 : // 진?�경
								{
									// ?�?? ?��??? 베?��?진?��? ?�용�?지
									if (LC_IsEurope() || LC_IsSingapore() || LC_IsVietnam())
										return false;

									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, "229");
										return false;
									}

									if (item2->ChangeRareAttribute() == true)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_RARE_ATTR", buf);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "230");
									}
								}
								break;
#endif

							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:

#if defined(ENABLE_NEW_AUTOPOTION)
							case ITEM_AUTO_ATTACK_SPEED_RECOVERY:
							case ITEM_AUTO_MOV_SPEED_RECOVERY:
							case ITEM_AUTO_CRITICAL_RECOVERY:
							case ITEM_AUTO_PENETRATE_RECOVERY:
							case ITEM_AUTO_VIT_RECOVERY:
							case ITEM_AUTO_STR_RECOVERY:
							case ITEM_AUTO_INT_RECOVERY:
							case ITEM_AUTO_DEX_RECOVERY:

							case ITEM_AUTO_RED_POTION_RECOVERY:
							case ITEM_AUTO_ORANGE_POTION_RECOVERY:
							case ITEM_AUTO_YELLOW_POTION_RECOVERY:
							case ITEM_AUTO_GREEN_POTION_RECOVERY:
							case ITEM_AUTO_BLUE_POTION_RECOVERY:
							case ITEM_AUTO_WHITE_POTION_RECOVERY:
#endif

							// ¹«½?¹«½????ö¸¸ ???ü¿¡ ??´ø °? °í?¡±â´? ¹«¼·°í...
							// ±?·¡¼­ ±?³? ??µå ??µù. ¼±¹° »ó??¿ë ??µ¿¹°¾? ¾?????µé.
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, "177");
										return false;
									}
#ifdef ENABLE_NEWSTUFF
									else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
									{
										ChatPacket(CHAT_TYPE_INFO, "134");
										return false;
									}
#endif

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
											type = AFFECT_AUTO_SP_RECOVERY;
											break;

#if defined(ENABLE_NEW_AUTOPOTION)
										case ITEM_AUTO_ATTACK_SPEED_RECOVERY:
											type = AFFECT_AUTO_ATTACK_SPEED_RECOVERY;
											break;

										case ITEM_AUTO_MOV_SPEED_RECOVERY:
											type = AFFECT_AUTO_MOV_SPEED_RECOVERY;
											break;

										case ITEM_AUTO_CRITICAL_RECOVERY:
											type = AFFECT_AUTO_CRITICAL_RECOVERY;
											break;

										case ITEM_AUTO_PENETRATE_RECOVERY:
											type = AFFECT_AUTO_PENETRATE_RECOVERY;
											break;

										case ITEM_AUTO_VIT_RECOVERY:
											type = AFFECT_AUTO_VIT_RECOVERY;
											break;

										case ITEM_AUTO_STR_RECOVERY:
											type = AFFECT_AUTO_STR_RECOVERY;
											break;

										case ITEM_AUTO_INT_RECOVERY:
											type = AFFECT_AUTO_INT_RECOVERY;
											break;

										case ITEM_AUTO_DEX_RECOVERY:
											type = AFFECT_AUTO_DEX_RECOVERY;
											break;

										case ITEM_AUTO_RED_POTION_RECOVERY:
											type = AFFECT_AUTO_RED_POTION_RECOVERY;
											break;

										case ITEM_AUTO_ORANGE_POTION_RECOVERY:
											type = AFFECT_AUTO_ORANGE_POTION_RECOVERY;
											break;

										case ITEM_AUTO_YELLOW_POTION_RECOVERY:
											type = AFFECT_AUTO_YELLOW_POTION_RECOVERY;
											break;

										case ITEM_AUTO_GREEN_POTION_RECOVERY:
											type = AFFECT_AUTO_GREEN_POTION_RECOVERY;
											break;

										case ITEM_AUTO_BLUE_POTION_RECOVERY:
											type = AFFECT_AUTO_BLUE_POTION_RECOVERY;
											break;

										case ITEM_AUTO_WHITE_POTION_RECOVERY:
											type = AFFECT_AUTO_WHITE_POTION_RECOVERY;
											break;
#endif

									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int pos = GetEmptyInventory(item->GetSize());

										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, "129");
											break;
										}

										item->SetCount( item->GetCount() - 1 );

										LPITEM item2 = ITEM_MANAGER::instance().CreateItem( item->GetVnum(), 1 );
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect( type );

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (true == isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess( type );
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect( pAffect );

											item->Lock(false);
											item->SetSocket(0, false);
										}
										else
										{
											LPITEM old = FindItemByID( pAffect->dwFlag );

											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect( pAffect );

											EPointTypes bonus = POINT_NONE;

											if (true == isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess( type );
										}
									}
								}
								break;
						}
						break;

					case USE_CLEAR:
						{
							switch (item->GetVnum())
							{
#ifdef ENABLE_WOLFMAN_CHARACTER
								case 27124: // Bandage
									RemoveBleeding();
									break;
#endif
								case 27874: // Grilled Perch
								default:
									RemoveBadAffect();
									break;
							}
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								quest::CQuestManager& q = quest::CQuestManager::instance();
								quest::PC* pPC = q.GetPC(GetPlayerID());

								if (pPC != NULL)
								{
									int last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, "214");
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_POTION_NODELAY:
						{

							if(!CanUseItem(GetMapIndex(),item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "You can't use this");
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, "177");
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, "178");
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, "177");
										return false;
								}
							}
#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "134");
								return false;
							}
#endif

							bool used = false;

							if (item->GetValue(0) != 0) // HP ?ý´ë°ª ?¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP ?ý´ë°ª ?¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % ?¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % ?¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, "179");
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case USE_POTION:
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, "177");
								return false;
							}

							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, "178");
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, "177");
									return false;
							}
						}
#ifdef ENABLE_NEWSTUFF
						else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
						{
							ChatPacket(CHAT_TYPE_INFO, "134");
							return false;
						}
#endif

						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_DXUP_PURPLE);
#endif
									break;

								case APPLY_ATT_SPEED:
									AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
#ifdef ENABLE_EFFECT_EXTRAPOT
									EffectPacket(SE_SPEEDUP_GREEN);
#endif
									break;

								case APPLY_STR:
									AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_TALISMAN:
						{
							const int TOWN_PORTAL	= 1;
							const int MEMORY_PORTAL = 2;


							// gm_guild_build, oxevent ¸?¿¡¼­ ±??¯º? ±??¯±â¾ïº? ¸¦ »ç¿ë¸ø??°? ¸·?½
							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, "231");
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, "134");
								return false;
							}
#ifdef ENABLE_NEWSTUFF
							else if (g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(item->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO, "134");
								return false;
							}
#endif

							if (m_pkWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, "232");
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL) // ±??¯º?
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, "233");

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL) // ±??¯±â¾ïº?
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, "234 %s%s",
												item->GetName(),
												"");
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case USE_TUNING:
					case USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;
								
#ifdef __SASH_SYSTEM__
							if (item->GetValue(0) == SASH_CLEAN_ATTR_VALUE0)
							{
								if (!CleanSashAttr(item, item2))
									return false;
								
								return true;
							}
#endif
							//if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343) // ¿µ¼®+3 ?
							//{
							//	ChatPacket(CHAT_TYPE_INFO, "235");
							//	return false;
							//}
							//if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)  // ¿µ¼®+4
							//{
							//	if (item->GetVnum() == 71056) // ?»·æ??¼û°á
							//	{
							//		RefineItem(item, item2);
							//	}
							//	else
							//	{
							//		ChatPacket(CHAT_TYPE_INFO, "236");
							//	}
							//}
							//else
							//{
							//	RefineItem(item, item2);
							//}
							RefineItem(item, item2);
						}
						break;

					case USE_CHANGE_COSTUME_ATTR:
					case USE_RESET_COSTUME_ATTR:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							if (ITEM_COSTUME != item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, "223");
								return false;
							}
#ifdef ENABLE_SOULBIND_SYSTEM
							if(item2->IsSealed()){
								ChatPacket(CHAT_TYPE_INFO, "237");
								return false;
							}
#endif
							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							if (item2->GetAttributeSetIndex() == -1)
							{
								ChatPacket(CHAT_TYPE_INFO, "223");
								return false;
							}

							if(CheckVnumCostumeBlock(item2->GetVnum()))
							{
								ChatPacket(CHAT_TYPE_INFO,"A este objeto no se le puede a?dir o cambiar el bonus.");
								return false;
							}


#ifdef ADD_BONUS_INTO_COSTUMES


							item2->SetAttrRings(false);

							if (item->GetVnum() == 70063){
								DWORD bonus_count = 3;
								if (item2->GetVnum() >= 73001 and item2->GetVnum() <= 73012 or item2->GetVnum() >= 73251 and item2->GetVnum() <= 73262 or item2->GetVnum() >= 73501 and item2->GetVnum() <= 73512 or item2->GetVnum() >= 73751 and item2->GetVnum() <= 73762)
									return false;
								// if (item2->GetSubType() == COSTUME_HAIR)
									// bonus_count = 2;
								if (item2->GetAttributeCount() < bonus_count)
								{

									if (item2->GetSubType() == COSTUME_HAIR)
									{
										item2->AddAttrCostumeHair();	
									}
									else{
										item2->AddAttrCostume();
									}
										
									ChatPacket(CHAT_TYPE_INFO, "238"); // se ha agregado con exito

#ifdef BATTLEPASS_WORLDARD
									CBattlePass::instance().UpdateMisionBattlePass(this,ADD_COSTUME_ATTR_BP,1);					
#endif	

									item->SetCount(item->GetCount() - 1);
								}
								else{
									ChatPacket(CHAT_TYPE_INFO, "239");
								}
							}
							else if (item->GetVnum() == 70064){
								if (item2->GetAttributeCount() == 0)
								{
									ChatPacket(CHAT_TYPE_INFO, "240");
									return false;
								}
								
								

								//
								if (item2->GetSubType() == COSTUME_HAIR)
								{
									
									item2->ChangeAttrCostumeHair();
								}else{
									item2->ChangeAttrCostume();
								}
								
#ifdef BATTLEPASS_WORLDARD
								CBattlePass::instance().UpdateMisionBattlePass(this,CHANGE_COSTUME_ATTR_BP,1);					
#endif	

								item->SetCount(item->GetCount() - 1);
								ChatPacket(CHAT_TYPE_INFO, "241");
							}
							break;
#else
							switch (item->GetSubType())
							{
								case USE_CHANGE_COSTUME_ATTR:
									if (item2->GetSubType() == COSTUME_HAIR)
									{
										
										item2->ChangeAttrCostumeHair();
									}else{
										item2->ChangeAttrCostume();
									}
#ifdef BATTLEPASS_WORLDARD
									CBattlePass::instance().UpdateMisionBattlePass(this,CHANGE_COSTUME_ATTR_BP,1);					
#endif
									//item2->ChangeAttribute();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_COSTUME_ATTR", buf);
									}
									break;
								case USE_RESET_COSTUME_ATTR:
									item2->ClearAttribute();
									item2->AlterToMagicItem();
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "RESET_COSTUME_ATTR", buf);
									}
									break;
							}
							ChatPacket(CHAT_TYPE_INFO, "241");

							item->SetCount(item->GetCount() - 1);
							break;
#endif
						}
#ifdef ELEMENT_SPELL_WORLDARD
					case USE_ELEMENT_DOWNGRADE:
					case USE_ELEMENT_UPGRADE:
					case USE_ELEMENT_CHANGE:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging() || item2->IsEquipped())
								return false;

							ElementsSpellItem(item, item2);
						}
						break;
#endif
						//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
					case USE_PUT_INTO_BELT_SOCKET:
					case USE_PUT_INTO_RING_SOCKET:
					case USE_PUT_INTO_ACCESSORY_SOCKET:
					case USE_ADD_ACCESSORY_SOCKET:
					case USE_CLEAN_SOCKET:
					case USE_CHANGE_ATTRIBUTE:

					case USE_CHANGE_ATTRIBUTE2 :
					case USE_ADD_ATTRIBUTE:
					case USE_ADD_ATTRIBUTE2:
						{
#ifdef ENABLE_LOCK_ATTR
							if (item->GetVnum() == 50348 || item->GetVnum() == 50349)
							{
								LPITEM item3;
								if (!IsValidItemPosition(DestCell) || !(item3 = GetItem(DestCell)))
								{
									quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
									return true;
								}
							}
#endif
#ifdef ENABLE_COPY_ATTR_ITEM
							if (item->GetVnum() == 77927)
							{
								LPITEM item2;
								if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								{
									for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
										item->SetForceAttribute(j, 0, 0);
									return false;
								}
								if (item2->IsExchanging() || item2->IsEquipped())
									return false;
								if (item2->CanCopyAttrItem())
								{
									bool isHasAlreadyAttr = false;
									for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
									{
										if (item->GetAttribute(j).bType > 0)
										{
											isHasAlreadyAttr = true;
											break;
										}
									}
									if (isHasAlreadyAttr)
									{
										if (item->GetSocket(1) == ITEM_WEAPON)
										{
											if (!(item->GetSocket(1) == item2->GetType() && item->GetSocket(3) == item2->GetProto()->sAddonType))
											{
												ChatPacket(CHAT_TYPE_INFO, "You can't add attr for this item!");
												return false;
											}
										}
										else
										{
											if (!(item->GetSocket(1) == item2->GetType() && item->GetSocket(2) == item2->GetSubType() && item->GetSocket(3) == item2->GetProto()->sAddonType))
											{
												ChatPacket(CHAT_TYPE_INFO, "You can't add attr for this item!");
												return false;
											}
										}
										
										item2->SetAttributes(item->GetAttributes());
										item2->UpdatePacket();
										item->SetCount(item->GetCount() - 1);
										ChatPacket(CHAT_TYPE_INFO, "Succesfully added attr.");
									}
									else
									{
										isHasAlreadyAttr = false;
										for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
										{
											if (item2->GetAttribute(j).bType > 0)
											{
												isHasAlreadyAttr = true;
												break;
											}
										}
										if (!isHasAlreadyAttr)
										{
											ChatPacket(CHAT_TYPE_INFO, "This item not has any attribute. Please try with has attr item.");
											return false;

										}
										item->SetSocket(1, item2->GetType());
										item->SetSocket(2, item2->GetSubType());
										item->SetSocket(3, item2->GetProto()->sAddonType);
										item->SetAttributes(item2->GetAttributes());
										item->UpdatePacket();
										for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
											item2->SetForceAttribute(j, 0, 0);
										ChatPacket(CHAT_TYPE_INFO, "Succesfully saving attr in item");
									}
								}
								else
									ChatPacket(CHAT_TYPE_INFO, "You can't use on this!");
								
								return true;
							}
#endif

							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}
#ifdef __SOULBINDING_SYSTEM__
							if (item2->IsBind() && item->GetSubType() != USE_PUT_INTO_BELT_SOCKET && item->GetSubType() != USE_PUT_INTO_RING_SOCKET && item->GetSubType() != USE_PUT_INTO_ACCESSORY_SOCKET && item->GetSubType() != USE_ADD_ACCESSORY_SOCKET)
							{
								ChatPacket(CHAT_TYPE_INFO, "242");
								return false;
							}
							
							if (item2->IsUntilBind() && item->GetSubType() != USE_PUT_INTO_BELT_SOCKET && item->GetSubType() != USE_PUT_INTO_RING_SOCKET && item->GetSubType() != USE_PUT_INTO_ACCESSORY_SOCKET && item->GetSubType() != USE_ADD_ACCESSORY_SOCKET)
							{
								ChatPacket(CHAT_TYPE_INFO, "243");
								return false;
							}
#endif
							// [NOTE] ??½º?¬ ¾?????¿¡´? ¾????? ???? »ý¼º½? ·£´ý ¼?¼º?» º?¿©??µ?, ?ç°æ?ç°¡ µîµî?º ¸·¾?´?¶ó´? ¿ä?»?? ??¾ú?½.
							// ¿ø·¡ ANTI_CHANGE_ATTRIBUTE °°?º ¾????? Flag¸¦ ??°¡??¿© ±â?¹ ·¹º§¿¡¼­ ?¯¿¬??°? ???®·? ?? ¼ö ??µµ·? ?? ¿¹?¤??¾ú?¸³ª
							// ±?µý°? ??¿ä¾ø?¸´? ´??¡°í »¡¸® ??´?·¡¼­ ±?³? ¿©±â¼­ ¸·?½... -_-
#ifndef ENABLE_SPECIAL_COSTUME_ATTR
#if defined(__BL_67_ATTR__)
							if (ITEM_COSTUME == item2->GetType() && item->GetSubType() != USE_CHANGE_ATTRIBUTE2)
#else
							if (ITEM_COSTUME == item2->GetType())
#endif
							{
								ChatPacket(CHAT_TYPE_INFO, "223");
								return false;
							}
#endif
							if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
								return false;

							item2->SetAttrRings(false);
							
							switch (item->GetSubType())
							{
								case USE_CLEAN_SOCKET:
									{
										//const BYTE maxSocketforMetin = ITEM_SOCKET_MAX_NUM;
										const BYTE maxSocketforMetin = 3;
										bool isHasBrokeStone = false;
										std::vector<long> metinSlot;
										for (BYTE i = 0; i < maxSocketforMetin; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												isHasBrokeStone = true;
											else
												metinSlot.push_back(item2->GetSocket(i));
										}
										if (!isHasBrokeStone)
										{
											ChatPacket(CHAT_TYPE_INFO, "244");
											return false;
										}
										for (BYTE i = 0; i < maxSocketforMetin; ++i)
										{
											if(metinSlot.size() >= i+1)
												item2->SetSocket(i, metinSlot[i]);
											else
												item2->SetSocket(i, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CLEAN_SOCKET", buf);
										}

										item->SetCount(item->GetCount() - 1);

									}
									break;

								case USE_CHANGE_ATTRIBUTE :
								//case USE_CHANGE_ATTRIBUTE2 : // @fixme123

#ifdef ENABLE_LOCK_ATTR
									if (item->GetVnum() == 50348 || item->GetVnum() == 50349)
									{
										if (item->GetVnum() == 50348)// lock algorithm
										{
											const int lockItemIndex = item->GetSocket(0);
											if (lockItemIndex == 0 || lockItemIndex > MAX_NORM_ATTR_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]Uncorrect lock attr item index. Please select your want index!");
												return false;
											}
											const BYTE attrCount = item2->GetAttributeCount();
											if (attrCount != 5)
											{
												ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]Item need 5 attr count!");
												return false;
											}
											BYTE attrIndex = 0;
											if (item2->GetAttrLockIndex(0) == 0)
												attrIndex = 0;
											else if (item2->GetAttrLockIndex(1) == 0)
												attrIndex = 1;
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]This item already have max lock attr index!");
												return false;
											}
											if(item2->GetAttrLockIndex(0) == lockItemIndex)
											{
												ChatPacket(CHAT_TYPE_INFO, "You can't add same index!");
												return false;
											}
											if (item2->IsAddonItem() && (lockItemIndex == 1 || lockItemIndex == 2) && (item2->GetAttrLockIndex(0) == 1 || item2->GetAttrLockIndex(0) == 2))
											{
												ChatPacket(CHAT_TYPE_INFO, "Addon item already blocked 1 and 2 index attr!");
												return false;
											}
											item2->SetForceAttribute(LOCK_ATTR_DATA_INDEX+attrIndex, 0, lockItemIndex);
											item->SetCount(item->GetCount() - 1);
											ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]Lock attr index successfully!");
										}
										else//remove lock
										{
											const int removeIndex = item->GetSocket(0);
											if(removeIndex == 0)
											{
												ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]You need select your want remove index!");
												return false;
											}
											BYTE attrIndex = 0;
											if(item2->GetAttrLockIndex(0) == removeIndex)
												attrIndex = 0;
											else if(item2->GetAttrLockIndex(1) == removeIndex)
												attrIndex = 1;
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]This item don't have lock attr index!");
												return false;
											}
											item2->SetForceAttribute(LOCK_ATTR_DATA_INDEX+attrIndex, 0, 0);
											item->SetCount(item->GetCount() - 1);
											ChatPacket(CHAT_TYPE_INFO, "[Attr Lock]Removing Lock attr index successfully!");
										}
										return true;
									}
#endif

									if ((item2->GetType() == ITEM_DS && item->GetVnum() != 71097) || (item2->GetType() != ITEM_DS && item->GetVnum() == 71097))
									{
										ChatPacket(CHAT_TYPE_INFO, "1055");
										return false;
									}

#ifdef ENABLE_SPECIAL_COSTUME_ATTR
									{
										const DWORD bonusItemVnum = item->GetVnum();
										bool isPendant = (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_PENDANT);
										if ((isPendant == true && bonusItemVnum != 71302) || (isPendant == false && bonusItemVnum == 71302))
										{
											ChatPacket(CHAT_TYPE_INFO, "1056");
											return false;
										}
										if (bonusItemVnum == 71302)
										{
											item2->ChangeAttribute();
											item->SetCount(item->GetCount() - 1);
											ChatPacket(CHAT_TYPE_INFO, "1067");
											return true;
										}

										bool itsCostumeAttr = (bonusItemVnum >= 53998 && bonusItemVnum <= 54500);
										if ((itsCostumeAttr == true && !item2->CanBonusCostume()) || (itsCostumeAttr == false && item2->CanBonusCostume()))
										{
											ChatPacket(CHAT_TYPE_INFO, "1056");
											return false;
										}

										if (bonusItemVnum == 53998)
										{
											bool isChange = false;
											for (int j = MAX_NORM_ATTR_NUM - 1; j >= 0; --j)
											{
												if (item2->GetAttributeType(j) != APPLY_NONE)
												{
													isChange = true;
													item->SetCount(item->GetCount() - 1);
													item2->SetForceAttribute(j, 0, 0);
													break;
												}
											}
											if (isChange)
												ChatPacket(CHAT_TYPE_INFO, "1058");
											else
												ChatPacket(CHAT_TYPE_INFO, "1060");
											return true;
										}
										else if (bonusItemVnum == 53999)
										{
											bool isChange = false;
											for (int j = 0; j < MAX_NORM_ATTR_NUM; ++j)
											{
												if (item2->GetAttributeType(j) != APPLY_NONE)
												{
													isChange = true;
													item2->SetForceAttribute(j, 0, 0);
												}
											}
											if (isChange)
											{
												item->SetCount(item->GetCount() - 1);
												ChatPacket(CHAT_TYPE_INFO, "1059");
											}
											else
												ChatPacket(CHAT_TYPE_INFO, "1060");
											return true;
										}
										else if (bonusItemVnum >= 53998 && bonusItemVnum <= 54500)
										{
											const BYTE applyType = item->GetValue(0);
											const long applyValue = item->GetValue(1);
											if (applyType == APPLY_NONE)
											{
												ChatPacket(CHAT_TYPE_INFO, "1061");
												return false;
											}
											const BYTE subTypeFlag = item2->GetBonusFlag();
											if (item->GetValue(2) != 0 && subTypeFlag != 0)
											{
												if (!IS_SET(item->GetValue(2), 1 << subTypeFlag))
												{
													ChatPacket(CHAT_TYPE_INFO, "1056");
													return false;
												}
											}
											const BYTE maxAttrCount = item2->GetBonusMaxAttr();
											if (item2->GetAttributeCount() >= maxAttrCount)
											{
												ChatPacket(CHAT_TYPE_INFO, "1062");
												return false;
											}
											else if(item2->HasAttr(applyType))
											{
												ChatPacket(CHAT_TYPE_INFO, "1063");
												return false;
											}
											item2->AddAttribute(applyType, applyValue);
											ChatPacket(CHAT_TYPE_INFO, "1057");
											item->SetCount(item->GetCount() - 1);
											return true;
										}
									}
#endif

									if (item2->GetAttributeSetIndex() == -1 && item2->GetType() != ITEM_DS)
									{
										ChatPacket(CHAT_TYPE_INFO, "223");
										return false;
									}

									if (item2->GetAttributeCount() == 0 && item2->GetType() != ITEM_DS)
									{
										ChatPacket(CHAT_TYPE_INFO, "240");
										return false;
									}

									

									if ((GM_PLAYER == GetGMLevel()) && (false == test_server) && (g_dwItemBonusChangeTime > 0))
									{
										//
										// Event Flag ¸¦ ?ë?? ???ü¿¡ ¾????? ¼?¼º º¯°æ?» ?? ½?°£?¸·? º??? ?æº??? ½?°£?? ?ê·¶´??ö °?»ç??°í
										// ½?°£?? ?æº??÷ ?ê·¶´?¸é ?ö?ç ¼?¼ºº¯°æ¿¡ ´ë?? ½?°£?» ¼³?¤?? ??´?.
										//

										// DWORD dwChangeItemAttrCycle = quest::CQuestManager::instance().GetEventFlag(msc_szChangeItemAttrCycleFlag);
										// if (dwChangeItemAttrCycle < msc_dwDefaultChangeItemAttrCycle)
											// dwChangeItemAttrCycle = msc_dwDefaultChangeItemAttrCycle;
										DWORD dwChangeItemAttrCycle = g_dwItemBonusChangeTime;

										quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());

										if (pPC)
										{
											DWORD dwNowSec = get_global_time();

											DWORD dwLastChangeItemAttrSec = pPC->GetFlag(msc_szLastChangeItemAttrFlag);

											if (dwLastChangeItemAttrSec + dwChangeItemAttrCycle > dwNowSec)
											{
												ChatPacket(CHAT_TYPE_INFO, "245 %d %d",
														dwChangeItemAttrCycle, dwChangeItemAttrCycle - (dwNowSec - dwLastChangeItemAttrSec));
												return false;
											}

											pPC->SetFlag(msc_szLastChangeItemAttrFlag, dwNowSec);
										}
									}

									if (item->GetSubType() == USE_CHANGE_ATTRIBUTE2)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 0, 30, 40, 3
										};
										if(item2->GetType() == ITEM_NEW_RINGS)
										{
											item2->SetAttrRings(true);
										}else{
											item2->SetAttrRings(false);
										}
										item2->ChangeAttribute(aiChangeProb);

									}
									else if (item->GetVnum() == 76014)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 10, 50, 39, 1
										};
										if(item2->GetType() == ITEM_NEW_RINGS)
										{
											item2->SetAttrRings(true);
										}else{
											item2->SetAttrRings(false);
										}
										item2->ChangeAttribute(aiChangeProb);
									}

									else
									{
										// ¿¬?ç°æ ?¯¼ö?³¸®
										// ?ý´ë·? ¿¬?ç°¡ ??°¡ ¾?µ?°?¶ó ??¿© ??µå ??µù??.
										if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
										{
#ifdef ITEM_TALISMAN_EQUIPMENT
											if ((item2->GetType() == ITEM_WEAPON)
												|| ((item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY) || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_PENDANT)))
#else
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
#endif
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, "246");
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "247");
												break;
											}
										}
										
									}


									if (item2->GetType() == ITEM_DS && item->GetVnum() == 71097)
									{
										//ChatPacket(CHAT_TYPE_INFO, "prueba funcion exitosa");
										if(DSManager::instance().ChangeAttributes(this,item2)){
											item->SetCount(item->GetCount() - 1);
										}
										break;
									}

									else{
										if(item2->GetType() == ITEM_NEW_RINGS)
										{
											item2->SetAttrRings(true);
										}else{
											item2->SetAttrRings(false);
										}
										item2->ChangeAttribute();
#ifdef BATTLEPASS_WORLDARD
										CBattlePass::instance().UpdateMisionBattlePass(this,CHANGE_ATTR_BP,1);					
#endif	
										ChatPacket(CHAT_TYPE_INFO, "241");
										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
										}
										item->SetCount(item->GetCount() - 1);
										break;
									}
										

									
									break;

								case USE_ADD_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, "223");
										return false;
									}
#ifdef ENABLE_SOULBIND_SYSTEM
									if(item2->IsSealed()){
										ChatPacket(CHAT_TYPE_INFO, "237");
										return false;
									}
#endif
									if (item2->GetAttributeCount() < 5)
									{
#ifdef ENABLE_SPECIAL_COSTUME_ATTR
										{
											const DWORD bonusItemVnum = item->GetVnum();
											bool isPendant = (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_PENDANT);
											if ((isPendant == true && bonusItemVnum != 71301) || (isPendant == false && bonusItemVnum == 71301))
											{
												ChatPacket(CHAT_TYPE_INFO, "1056");
												return false;
											}
											if (bonusItemVnum == 71301)
											{
												item2->AddAttribute();
												item->SetCount(item->GetCount() - 1);
												ChatPacket(CHAT_TYPE_INFO, "1057");
												return true;
											}
										}
#endif
										
										// ¿¬?ç°¡ ?¯¼ö?³¸®
										// ?ý´ë·? ¿¬?ç°¡ ??°¡ ¾?µ?°?¶ó ??¿© ??µå ??µù??.
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
#ifdef ITEM_TALISMAN_EQUIPMENT
											if ((item2->GetType() == ITEM_WEAPON)
												|| ((item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY) || (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_PENDANT) || (item2->GetType() == ITEM_COSTUME && item2->GetSubType() == COSTUME_SASH)))
#else
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
#endif
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, "246");
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "247");
												break;
											}
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if(item2->GetType() == ITEM_NEW_RINGS)
										{
											item2->SetAttrRings(true);
										}else{
											item2->SetAttrRings(false);
										}

										item2->AddAttribute();
										ChatPacket(CHAT_TYPE_INFO, "248");

										int iAddedIdx = item2->GetAttributeCount() - 1;
										LogManager::instance().ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_ATTRIBUTE_SUCCESS",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum());


										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "239");
									}
									break;

#if defined(__BL_67_ATTR__)
								case USE_CHANGE_ATTRIBUTE2:
									if (item2->GetAttributeSetIndex() == -1 || item2->GetRareAttrCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, "229");
										return false;
									}

									if (item2->IsEquipped())
										return false;

									if ((item->GetVnum() == SMALL_ORISON && number(1, 100) >= 10) == false 
										&& item2->ChangeRareAttribute() == true)
										ChatPacket(CHAT_TYPE_INFO, "227");
									else
										ChatPacket(CHAT_TYPE_INFO, "228");

									item->SetCount(item->GetCount() - 1);
									break;
#endif

								case USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
#ifdef ENABLE_ADDSTONE_FAILURE
												if (number(1, 100) <= 50)
#else
												if (1)
#endif
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, "238");
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, "249");
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_FAIL", buf);
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "250");
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, "251");
										}
									}
									break;

								case USE_PUT_INTO_BELT_SOCKET:
								case USE_PUT_INTO_ACCESSORY_SOCKET:
									if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
									{

										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
										{
											if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
											{
												item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
												ChatPacket(CHAT_TYPE_INFO, "252");
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, "253");
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_FAIL", buf);
											}

											item->SetCount(item->GetCount() - 1);
										}
										else
										{
											if (item2->GetAccessorySocketMaxGrade() == 0)
												ChatPacket(CHAT_TYPE_INFO, "254");
											else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, "255");
												ChatPacket(CHAT_TYPE_INFO, "256");
											}
											else
												ChatPacket(CHAT_TYPE_INFO, "257");
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, "258");
									}
									break;
							}
							if (item2->IsEquipped())
							{
								BuffOnAttr_AddBuffsFromItem(item2);
							}
						}
						break;
						//  END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case USE_BAIT:
						{

							if (m_pkFishingEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, "259");
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, "260 %s", item->GetName());
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, "261 %s", item->GetName());
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;
#ifdef __SOULBINDING_SYSTEM__
					case USE_BIND:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;
							
							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}
							
							if (item2->IsExchanging() || item2->IsEquipped())
								return false;
							
							if (item2->IsBind())
							{
								ChatPacket(CHAT_TYPE_INFO, "262");
								return false;
							}
							
							if (item2->IsUntilBind())
							{
								ChatPacket(CHAT_TYPE_INFO, "263");
								return false;
							}
							
							if (IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_BIND))
							{
								ChatPacket(CHAT_TYPE_INFO, "264");
								return false;
							}
							
							long time_proc = item->GetValue(3);
							item2->Bind(time_proc);
							ChatPacket(CHAT_TYPE_INFO, "265");
							item->SetCount(item->GetCount() - 1);
						}
						break;
					case USE_UNBIND:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;
							
							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}
							
							if (item2->IsExchanging() || item2->IsEquipped())
								return false;
							
							if (item2->IsUntilBind())
							{
								ChatPacket(CHAT_TYPE_INFO, "263");
								return false;
							}
							
							if (!item2->IsBind())
							{
								ChatPacket(CHAT_TYPE_INFO, "266");
								return false;
							}
							
							long time_proc = item->GetValue(3);
							item2->Bind(time_proc);
							item2->StartUnBindingExpireEvent();
							ChatPacket(CHAT_TYPE_INFO, "267");
							item->SetCount(item->GetCount() - 1);
						}
						break;
#endif
					case USE_MOVE:
					case USE_TREASURE_BOX:
					case USE_MONEYBAG:
						break;

					case USE_AFFECT :
						{
							if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, "175");
							}
							else
							{
								// PC_BANG_ITEM_ADD
								if (item->IsPCBangItem() == true)
								{
									// PC¹æ???ö ?¼?©??¼­ ?³¸®
									if (CPCBangManager::instance().IsPCBangIP(GetDesc()->GetHostName()) == false)
									{
										// PC¹æ?? ¾?´?!
										ChatPacket(CHAT_TYPE_INFO, "176");
										return false;
									}
								}
								// END_PC_BANG_ITEM_ADD

								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_CREATE_STONE:
						AutoGiveItem(number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					// ¹°¾? ?¦?¶ ½º?³¿ë ·¹½??? ?³¸®
					case USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							DWORD dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							DWORD dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, "268");
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, "268");
									return false;
								}
							}

							if (pSource1 != NULL)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, "269 %s", pSource1->GetName());
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != NULL)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, "269 %s", pSource2->GetName());
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, "270");
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, "271");
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() || item2->IsEquipped()) // @fixme114
					return false;

				if (item2->GetType() == ITEM_PICK) return false;
				if (item2->GetType() == ITEM_ROD) return false;

				int i;

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					DWORD dwVnum;

					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

					if (!p)
						continue;

					if (item->GetValue(5) == p->alValues[5])
					{
						ChatPacket(CHAT_TYPE_INFO, "272");
						return false;
					}
				}

				if (item2->GetType() == ITEM_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, "273");
						return false;
					}
				}
				else if (item2->GetType() == ITEM_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, "274");
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, "275");
					return false;
				}

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
#ifdef ENABLE_BATTLE_PASS
						CHARACTER_MANAGER::Instance().DoMission(this, MISSION_SPRITE_STONE, 1, item->GetVnum());
#endif
#ifdef ENABLE_ADDSTONE_FAILURE
						if (number(1, 100) <= 30)
#else
						if (1)
#endif
						{
							ChatPacket(CHAT_TYPE_INFO, "276");
							item2->SetSocket(i, item->GetVnum());
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, "277");
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						LogManager::instance().ItemLog(this, item2, "SOCKET", item->GetName());

						if(item->GetCount() == 1){
							ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (METIN)");
						}else{
							item->SetCount(item->GetCount() - 1);
						}
						break;
					}

				if (i == ITEM_SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, "275");
			}
			break;

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
		case ITEM_LOTTERY:
			break;

		case ITEM_TOTEM:
			{
				if (!item->IsEquipped())
					EquipItem(item);
			}
			break;

		case ITEM_BLEND:
			// »õ·?¿î ¾???µé
			sys_log(0,"ITEM_BLEND!!");
			if (Blend_Item_find(item->GetVnum()))
			{
				int		affect_type		= AFFECT_BLEND;
				int		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int		apply_value		= item->GetSocket(1);
				int		apply_duration	= item->GetSocket(2);

				if (FindAffect(affect_type, apply_type))
				{
					ChatPacket(CHAT_TYPE_INFO, "175");
				}
				else
				{
					//if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					//{
					//	ChatPacket(CHAT_TYPE_INFO, "175");
					//}
					//else
					//{
						// ChatPacket(CHAT_TYPE_INFO, "<test1 Affect Potion> Set icon ingame for item: [%s]", item->GetName());
						#ifdef ENABLE_NEW_AFFECT_POTION
							SetAffectPotion(item);
						#endif		
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						
						item->SetCount(item->GetCount() - 1);
					//}
				}
			}
			break;
		case ITEM_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (NULL == pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;

		case ITEM_NONE:
			sys_err("Item type NONE %s", item->GetName());
			break;

		default:
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
	}

	return true;
}

int g_nPortalLimitTime = 10;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	//WORD wDestCell = DestCell.cell;
	//BYTE bDestInven = DestCell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
			return false;

	sys_log(0, "%s: USE_ITEM %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition())
	{
		CSwitchbot* pkSwitchbot = CSwitchbotManager::Instance().FindSwitchbot(GetPlayerID());
		if (pkSwitchbot && pkSwitchbot->IsActive(Cell.cell))
		{
			return false;
		}

		int iEmptyCell = GetEmptyInventory(item->GetSize());

		if (iEmptyCell == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, "278");
			return false;
		}

		MoveItem(Cell, TItemPos(INVENTORY, iEmptyCell), item->GetCount());
		return true;
	}
#endif



	
	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, "279");
		return false;
	}

	if (IsStun())
		return false;

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, "280");
		return false;
	}
#if defined(WJ_COMBAT_ZONE)
	if (!CCombatZoneManager::instance().CanUseItem(this, item))
		return false;
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsLimitedItem(this, item->GetVnum()))
		return false;
	
	if (!CTournamentPvP::instance().CanUseItem(this, item))
		return false;
#endif

	#ifdef __KINGDOMS_WAR__
	if ((GetMapIndex() == KingdomsWar::MAP_INDEX) && (KingdomsWar::IS_UNACCPETABLE_ITEM(item->GetVnum()) == 1) && (GetGMLevel() == GM_PLAYER))
	{
		ChatPacket(CHAT_TYPE_INFO, KingdomsWar::MSG[4]);
		return false;
	}
	#endif


	if(!CanUseItem(GetMapIndex(),item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, "You can't use this");
		return false;
	}

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, "281");
			return false;
		}

		// °æ?¥¹??ö »ç¿ë?ö »ó´ë¹æ?? SUMMONABLE_ZONE¿¡ ??´?°¡´? WarpToPC()¿¡¼­ ?¼?©

		//»ï°?¸® °ü·? ¸?¿¡¼­´? ±??¯º?¸¦ ¸·¾?¹ö¸°´?.
		if (CThreeWayWar::instance().IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, "282");
			return false;
		}
		int iPulse = thecore_pulse();

		//?¢°í ¿¬?? ?¼?©
		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, "283 %d", g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false;
		}

		//°?·¡°ü·? ?¢ ?¼?©
		if (GetExchange() || GetShop() || IsOpenSafebox() || IsCubeOpen()
#if defined(__BL_67_ATTR__)
			|| Is67AttrOpen()
#endif
			)
		{
			ChatPacket(CHAT_TYPE_INFO, "284");
			return false;
		}

		//PREVENT_REFINE_HACK
		//°³·®?? ½?°£?¼?©
		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, "285 %d", g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK


		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, "286 %d", g_nPortalLimitTime);
				return false;
			}

		}
		//END_PREVENT_ITEM_COPY


		//±??¯º? °?¸®?¼?©
		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;
			//±??¯±â¾ïº?
			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}
			//±??¯º?
			else if (item->GetVnum() == 22000)
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, "287");
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist);
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE
		//±³?¯ ?? ½?°£?¼?©
		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, "288 %d", g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	//º¸µû¸® ºñ´? »ç¿ë½? °?·¡?¢ ?¦?? ?¼?©
	if ((item->GetVnum() == 50200) || (item->GetVnum() == 71049))
	{
		if (GetExchange() || GetShop()|| IsOpenSafebox() || IsCubeOpen()
#if defined(__BL_67_ATTR__)
			|| Is67AttrOpen()
#endif
			)
		{
			ChatPacket(CHAT_TYPE_INFO, "289");
			return false;
		}

	}
	//END_PREVENT_TRADE_WINDOW

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, "290");
		return false;
	}
	// @fixme150 END
#ifdef __SOULBINDING_SYSTEM__
	if (item->IsBind() || item->IsUntilBind())
	{
		ChatPacket(CHAT_TYPE_INFO, "291");
		return false;
	}
#endif
#ifdef __ANTI_EXP__
	if (item->GetVnum() == 72501)
	{
		int so = item->GetSocket(0);
		if (so == 0 && GetQuestFlag("anti_exp.block_exp") == 0){
			item->Lock(true);
			item->SetSocket(0, 1);
			SetQuestFlag("anti_exp.block_exp", 1);
		}
		else{
			item->Lock(false);
			item->SetSocket(0, 0);
			SetQuestFlag("anti_exp.block_exp", 0);
		}
	}
#endif

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG)) // »ç¿ë ·?±?¸¦ ³²±â´? ¾????? ?³¸®
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 32 + 1];
		int len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, DestCell);

		if (NULL == ITEM_MANAGER::instance().FindByVID(vid)) // UseItemEx¿¡¼­ ¾??????? »è?¦ µ?¾ú´?. »è?¦ ·?±?¸¦ ³²±è
		{
			LogManager::instance().ItemLog(this, vid, vnum, "REMOVE", hint);
		}
		else if (oldCount != item->GetCount())
		{
			snprintf(hint + len, sizeof(hint) - len, " %u", oldCount - 1);
			LogManager::instance().ItemLog(this, vid, vnum, "USE_ITEM", hint);
		}
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

bool CHARACTER::DropItem(TItemPos Cell, DWORD bCount)
{
	LPITEM item = NULL;

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, "292");
		return false;
	}
#ifdef ENABLE_NEWSTUFF
	if (0 != g_ItemDropTimeLimitValue)
	{
		if (get_dword_time() < m_dwLastItemDropTime+g_ItemDropTimeLimitValue)
		{
			ChatPacket(CHAT_TYPE_INFO, "168");
			return false;
		}
	}

	m_dwLastItemDropTime = get_dword_time();
#endif
	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;
#ifdef ENABLE_SOULBIND_SYSTEM 
	if (item->IsSealed()){
		ChatPacket(CHAT_TYPE_INFO, "293");
		return false;
	}
#endif
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		ChatPacket(CHAT_TYPE_INFO, "294");
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);	// Quickslot ¿¡¼­ ?ö¿ò

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				sys_log(0, "[DROP_ITEM] drop item count == 0");
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		// copy item socket -- by mhh
		FN_copy_item_socket(pkItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %u", pkItemToDrop->GetID(), pkItemToDrop->GetCount());
		LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		ChatPacket(CHAT_TYPE_INFO, "295");
#ifdef ENABLE_NEWSTUFF
		pkItemToDrop->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPITEM]);
#else
		pkItemToDrop->StartDestroyEvent();
#endif

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);

		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %u %u", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "DROP", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}

bool CHARACTER::DropGold(int gold)
{
	if (gold <= 0 || gold > GetGold())
		return false;

	if (!CanHandleItem())
		return false;

	if (0 != g_GoldDropTimeLimitValue)
	{
		if (get_dword_time() < m_dwLastGoldDropTime+g_GoldDropTimeLimitValue)
		{
			ChatPacket(CHAT_TYPE_INFO, "168");
			return false;
		}
	}

	m_dwLastGoldDropTime = get_dword_time();

	LPITEM item = ITEM_MANAGER::instance().CreateItem(1, gold);

	if (item)
	{
		PIXEL_POSITION pos = GetXYZ();

		if (item->AddToGround(GetMapIndex(), pos))
		{
			//Motion(MOTION_PICKUP);
			PointChange(POINT_GOLD, -gold, true);

			if (gold > 1000) // ?µ¿ø ??»ó¸¸ ±â·???´?.
				LogManager::instance().CharLog(this, gold, "DROP_GOLD", "");

#ifdef ENABLE_NEWSTUFF
			item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_DROPGOLD]);
#else
			item->StartDestroyEvent();
#endif
			ChatPacket(CHAT_TYPE_INFO, "296 %d", 150/60);
		}

		Save();
		return true;
	}

	return false;
}

bool CHARACTER::IsValidItemChangeEquip(BYTE cell, LPITEM item)
{
	int iWearCell = item->FindEquipCell(this);


	if(cell < 0)
	{
		return false;
	}

	if(cell > CHANGE_EQUIP_SLOT_COUNT)
	{
		return false;
	}

	if(GetChangeEquipItem(cell)){
		return false;
	}

	if(iWearCell == WEAR_ARROW){
		return false;
	}


	BYTE page_index_ce = 1;

	for (int i = 1; i < CHANGE_EQUIP_PAGE_EXTRA; ++i)
	{
		if(cell >= CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*i && cell < (CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(i+1))
		{
			page_index_ce = i;
			cell = cell - ((CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*i);
		}
	}

	if (item->GetType() == ITEM_NEW_RINGS)
	{
		if(cell < WEAR_RINGS_1 || cell > WEAR_RINGS_2){
			return false;
		}

		if(iWearCell < WEAR_RINGS_1 || iWearCell > WEAR_RINGS_2){
			return false;
		}

		DWORD index_old = CHANGE_EQUIP_SLOT_COUNT-(CHANGE_EQUIP_SLOT_COUNT/page_index_ce);


		if(page_index_ce > 1){
			index_old = CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA*page_index_ce;
		}


		for (int i = index_old; i < (CHANGE_EQUIP_SLOT_COUNT/CHANGE_EQUIP_PAGE_EXTRA)*(page_index_ce+1); ++i)
		{
			LPITEM item_check = GetChangeEquipItem(i);
			if(item_check)
			{
				if(item_check->GetType() == ITEM_NEW_RINGS)
				{
					if(item_check)
					{
						if(item_check->GetAccessorySocketVnumRings(item_check) == item->GetAccessorySocketVnumRings(item))
						{
							return false;
						}
					}
				}
			}
		}

	}else{
		if(iWearCell != cell)
		{
			return false;
		}
	}

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
	}

	if (false == FN_check_item_sex(this, item))
	{
		return false;
	}
	
	return true;
}

bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, DWORD count)
{
	LPITEM item = NULL;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, "292");
		return false;
	}

	// ±â?¹???? ¿ä?»?¸·? º§?® ??º¥?ä¸®¿¡´? ?¯?¤ ?¸???? ¾?????¸¸ ³??» ¼ö ??´?.
	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		ChatPacket(CHAT_TYPE_INFO, "297");
		return false;
	}

#ifdef ENABLE_SWITCHBOT
	if (Cell.IsSwitchbotPosition() && CSwitchbotManager::Instance().IsActive(GetPlayerID(), Cell.cell))
	{
		ChatPacket(CHAT_TYPE_INFO, "298");
		return false;
	}

	if (DestCell.IsSwitchbotPosition() && !SwitchbotHelper::IsValidItem(item))
	{
		ChatPacket(CHAT_TYPE_INFO, "299");
		return false;
	}
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	
	if (DestCell.IsDefaultInventoryPosition() && Cell.IsEquipPosition())
	{
		if (!CanUnequipNow(item))
			return false;

		if (item->IsCostumeMount() && GetHorse())
			HorseSummon(false);
	}
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	if(DestCell.IsChangeEquipPosition())
	{
		if(!IsValidItemChangeEquip(DestCell.cell,item)){
			ChatPacket(CHAT_TYPE_INFO,"No puedes mover el item a ese slot");
			return false;
		}
	}
#endif

	// ??¹? ?ø¿ë???? ¾??????» ´?¸¥ °÷?¸·? ¿?±â´? °æ¿ì, '?å?¥ ???¦' °¡´??? ?ö ?®????°í ¿?±è
	if (Cell.IsEquipPosition())
	{
		if (!CanUnequipNow(item))
			return false;

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		int iWearCell = item->FindEquipCell(this);
		if (iWearCell == WEAR_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, "300");
				return false;
			}

			if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
				return UnequipItem(item);
		}
#endif
	}

	if (DestCell.IsEquipPosition())
	{
		if (GetItem(DestCell))	// ?åºñ?? °æ¿ì ?? °÷¸¸ °?»ç??µµ µ?´?.
		{
			ChatPacket(CHAT_TYPE_INFO, "301");

			return false;
		}
		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}
				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}
		// ¿ë?¥¼®?? ¾?´? ¾??????º ¿ë?¥¼® ??º¥¿¡ µé¾î°¥ ¼ö ¾ø´?.
		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;
#ifdef ENABLE_SPECIAL_STORAGE
		if (item->IsUpgradeItem())
		{
			if (DestCell.window_type != UPGRADE_INVENTORY && DestCell.window_type != INVENTORY && DestCell.window_type != SAFEBOX)
				return false;

			// if (!DSManager::instance().IsValidCellForThisItem2(item, DestCell))
				// return false;
		}
		else if (UPGRADE_INVENTORY == DestCell.window_type)
			return false;
		
		if (item->IsBook())
		{
			if (DestCell.window_type != BOOK_INVENTORY && DestCell.window_type != INVENTORY && DestCell.window_type != SAFEBOX)
				return false;

			//if (!DSManager::instance().IsValidCellForThisItem2(item, DestCell))
			//	return false;
		}
		else if (BOOK_INVENTORY == DestCell.window_type)
			return false;

		if (item->IsStone())
		{
			if (DestCell.window_type != STONE_INVENTORY && DestCell.window_type != INVENTORY && DestCell.window_type != SAFEBOX)
				return false;

			// if (!DSManager::instance().IsValidCellForThisItem2(item, DestCell)){
				// ChatPacket(CHAT_TYPE_INFO, "302");
				// return false;
			// }
		}
		else if (STONE_INVENTORY == DestCell.window_type){
			return false;
		}

		if (item->IsChange())
		{
			if (DestCell.window_type != CHANGE_INVENTORY && DestCell.window_type != INVENTORY && DestCell.window_type != SAFEBOX)
				return false;

			// if (!DSManager::instance().IsValidCellForThisItem2(item, DestCell)){
				// ChatPacket(CHAT_TYPE_INFO, "302");
				// return false;
			// }
		}
		else if (CHANGE_INVENTORY == DestCell.window_type){
			return false;
		}

		if (item->IsCostume() || item->IsCostumeSkin())
		{
			if (DestCell.window_type != COSTUME_INVENTORY && DestCell.window_type != INVENTORY && DestCell.window_type != SAFEBOX && DestCell.window_type != CHANGE_EQUIP)
				return false;

			// if (!DSManager::instance().IsValidCellForThisItem2(item, DestCell)){
				// ChatPacket(CHAT_TYPE_INFO, "302");
				// return false;
			// }
		}
		else if (COSTUME_INVENTORY == DestCell.window_type){
			return false;
		}

#endif
		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum()) // ???¥ ¼ö ??´? ¾??????? °æ¿ì
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			sys_log(0, "%s: ITEM_STACK %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			count = MIN(g_bItemCountLimit - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);

#ifdef ENABLE_MINI_GAME_CATCH_KING
			if (item2->GetVnum() == CATCH_KING_DROP_ITEM && item2->GetCount() >= 25)
			{
				const LPITEM& KingDeck = AutoGiveItem(CATCH_KING_PLAY_ITEM);
				if (KingDeck != nullptr)
					item2->SetCount(item2->GetCount() - 25);
			}
#endif

			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
			return false;

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			sys_log(0, "%s: ITEM_MOVE %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->RemoveFromCharacter();
#ifdef ENABLE_NEWITEM_AFFECT
			SetItem(DestCell, item, false);
#else
			SetItem(DestCell, item);
#endif

			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{

#ifdef ENABLE_PREVENT_FLOOD_PACKETS
            const int iPulse = thecore_pulse();
            if (iPulse - GetLastItemSplitTime() < PASSES_PER_SEC(3)) // 3 sec
            {
                ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[1554]Please try again later."));
                return false;
            }
#endif


			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell,
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			item2->AddToCharacter(this, DestCell);
#ifdef ENABLE_PREVENT_FLOOD_PACKETS
            SetLastItemSplitTime();
#endif
			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
		}
	}

#ifdef ENABLE_MOUNT_SKIN
	if (DestCell.IsDefaultInventoryPosition() && Cell.IsEquipPosition())
	{
		if (item->IsCostumeMountSkin() && GetHorse())
		{
			HorseSummon(false);
			HorseSummon(true);
		}
	}
#endif
	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;
		FFindOwnership(LPITEM item): item(item), owner(NULL){}
		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;
		FCountNearMember(LPCHARACTER center ): total(0), x(center->GetX()), y(center->GetY()){}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney)
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney)
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);

					if (iMoney > 1000) // ?µ¿ø ??»ó¸¸ ±â·???´?.
					{
						LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(ch, iMoney, "GET_GOLD", ""));
					}
				}
		}
	};
}

void CHARACTER::GiveGold(long long iAmount)
{
	if (iAmount <= 0)
		return;




	sys_log(0, "GIVE_GOLD: %s %lld", GetName(), iAmount);

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		// ???¼°¡ ??´? °æ¿ì ³ª´©¾î °¡?ø´?.
		long long dwTotal = iAmount;
		long long dwMyAmount = dwTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);

		if (dwMyAmount > 1000) // ?µ¿ø ??»ó¸¸ ±â·???´?.
		{
			LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(this, dwMyAmount, "GET_GOLD", ""));
		}
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);

		if (iAmount > 1000) // ?µ¿ø ??»ó¸¸ ±â·???´?.
		{
			LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().CharLog(this, iAmount, "GET_GOLD", ""));
		}
	}
}

bool CHARACTER::CanGetGroundItem(LPITEM item, LPCHARACTER partyOwner)
{
	if (item->IsOwnership(this))
		return true;
	else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
	{
		NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);
		GetParty()->ForEachOnlineMember(funcFindOwnership);
		LPCHARACTER owner = funcFindOwnership.owner;
		if(!owner)
			return false;
		if (item->IsDragonSoul())
		{
			if (owner->GetEmptyDragonSoulInventory(item) == -1)
			{
				if (GetEmptyDragonSoulInventory(item) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
		{
			if (owner->GetEmptyUpgradeInventory(item) == -1)
			{
				if (GetEmptyUpgradeInventory(item) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
		else if (item->IsBook())
		{
			if (owner->GetEmptyBookInventory(item) == -1)
			{
				if (GetEmptyBookInventory(item) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
		else if (item->IsStone())
		{
			if (owner->GetEmptyStoneInventory(item) == -1)
			{
				if (GetEmptyStoneInventory(item) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
		else if (item->IsChange())
		{
			if (owner->GetEmptyChangeInventory(item) == -1)
			{
				if (GetEmptyChangeInventory(item) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
		else if (item->IsCostume())
		{
			if (owner->GetEmptyCostumeInventory(item) == -1)
			{
				if (GetEmptyCostumeInventory(item) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
#endif
		else
		{
			if (owner->GetEmptyInventory(item->GetSize()) == -1)
			{
				if (GetEmptyInventory(item->GetSize()) != -1)
					return true;
			}
			else
			{
				partyOwner = owner;
				return true;
			}
		}
	}
	return false;
}
bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		// @fixme150 BEGIN
		if (item->GetType() == ITEM_QUEST)
		{
			if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
			{
				ChatPacket(CHAT_TYPE_INFO, "303");
				return false;
			}
		}
		// @fixme150 END

		if (item->IsOwnership(this))
		{
			// ¸¸¾? ???¸·? ??´? ¾??????? ¿¤?©¶ó¸é
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				M2_DESTROY_ITEM(item);

				Save();
			}
			// ?ò¹ü?? ¾???????¶ó¸é
			else
			{
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;


							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;


							item2->SetCount(item2->GetCount() + bCount2);

#ifdef ENABLE_MINI_GAME_CATCH_KING
							if (item2->GetVnum() == CATCH_KING_DROP_ITEM && item2->GetCount() >= 25)
							{
								const LPITEM& KingDeck = AutoGiveItem(CATCH_KING_PLAY_ITEM);
								if (KingDeck != nullptr)
									item2->SetCount(item2->GetCount() - 25);
							}
#endif

							if (bCount == 0)
							{
#ifdef __CHAT_SETTINGS__
								ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
								ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}
#ifdef ENABLE_SPECIAL_STORAGE
				if (item->IsUpgradeItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetUpgradeInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{

							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;
						
							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
#ifdef __CHAT_SETTINGS__
								ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
								ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}
				else if (item->IsBook() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetBookInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{

							int mithras;

							for (mithras = 0; mithras < ITEM_SOCKET_MAX_NUM; ++mithras)
								if (item2->GetSocket(mithras) != item->GetSocket(mithras))
									break;

							if (mithras != ITEM_SOCKET_MAX_NUM)
								continue;

							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

#ifdef ENABLE_MINI_GAME_CATCH_KING
							if (item2->GetVnum() == CATCH_KING_DROP_ITEM && item2->GetCount() >= 25)
							{
								const LPITEM& KingDeck = AutoGiveItem(CATCH_KING_PLAY_ITEM);
								if (KingDeck != nullptr)
									item2->SetCount(item2->GetCount() - 25);
							}
#endif

							if (bCount == 0)
							{
#ifdef __CHAT_SETTINGS__
								ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
								ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}
				else if (item->IsStone() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetStoneInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{

							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
#ifdef __CHAT_SETTINGS__
								ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
								ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}
				else if (item->IsChange() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetChangeInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{

							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
#ifdef __CHAT_SETTINGS__
								ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
								ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}
				else if (item->IsCostume() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					DWORD bCount = item->GetCount();

					for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetCostumeInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{

							DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
#ifdef __CHAT_SETTINGS__
								ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
								ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}

#endif
				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						sys_log(0, "No empty ds inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
				{
					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						sys_log(0, "No empty ssu inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
				else if (item->IsBook())
				{
					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						sys_log(0, "No empty ssu inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
				else if (item->IsStone())
				{
					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						sys_log(0, "No empty ssu inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}

				else if (item->IsChange())
				{
					if ((iEmptyCell = GetEmptyChangeInventory(item)) == -1)
					{
						sys_log(0, "No empty ssc inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
				
				else if (item->IsCostume())
				{
					if ((iEmptyCell = GetEmptyCostumeInventory(item)) == -1)
					{
						sys_log(0, "No empty ssc inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}

#endif
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}

				item->RemoveFromGround();

				if (item->IsDragonSoul())
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#ifdef ENABLE_SPECIAL_STORAGE
				else if (item->IsUpgradeItem())
					item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
				else if (item->IsBook())
					item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, iEmptyCell));
				else if (item->IsStone())
					item->AddToCharacter(this, TItemPos(STONE_INVENTORY, iEmptyCell));
				else if (item->IsChange())
					item->AddToCharacter(this, TItemPos(CHANGE_INVENTORY, iEmptyCell));
				else if (item->IsCostume())
					item->AddToCharacter(this, TItemPos(COSTUME_INVENTORY, iEmptyCell));
#endif
				else
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

				char szHint[32+1];
				snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
				LogManager::instance().ItemLog(this, item, "GET", szHint);
#ifdef __CHAT_SETTINGS__
				ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item->GetName());
#else
				ChatPacket(CHAT_TYPE_INFO, "305 %s", item->GetName());
#endif
				if (item->GetType() == ITEM_QUEST || item->GetType() == ITEM_MATERIAL)
					quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
			}



			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
		{
			// ´?¸¥ ???¼¿ø ¼??¯±? ¾??????» ???¸·?°í ??´?¸é
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
#ifndef FIX_PICK_ITEMS
			// @fixme115
			if (!owner)
				return false;
#endif
			int iEmptyCell;

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}
#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyUpgradeInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyUpgradeInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}
			else if (item->IsBook())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyBookInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyBookInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}
			else if (item->IsStone())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyStoneInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyStoneInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}
			else if (item->IsChange())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyChangeInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyChangeInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}
			else if (item->IsCostume())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyCostumeInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyCostumeInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}
#endif
			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, "306");
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#ifdef ENABLE_SPECIAL_STORAGE
			else if (item->IsUpgradeItem())
				item->AddToCharacter(owner, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
			else if (item->IsBook())
				item->AddToCharacter(owner, TItemPos(BOOK_INVENTORY, iEmptyCell));
			else if (item->IsStone())
				item->AddToCharacter(owner, TItemPos(STONE_INVENTORY, iEmptyCell));
			else if (item->IsChange())
				item->AddToCharacter(owner, TItemPos(CHANGE_INVENTORY, iEmptyCell));
			else if (item->IsCostume())
				item->AddToCharacter(owner, TItemPos(COSTUME_INVENTORY, iEmptyCell));
#endif
			else
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));

			
			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(owner, item, "GET", szHint);

			if (owner == this){
#ifdef __CHAT_SETTINGS__
				ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item->GetName());
#else
				ChatPacket(CHAT_TYPE_INFO, "305 %s", item->GetName());
#endif
			}
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, "307 %s %s", GetName(), item->GetName());
				ChatPacket(CHAT_TYPE_INFO, "308 %s %s", owner->GetName(), item->GetName());
			}

			if (item->GetType() == ITEM_QUEST || item->GetType() == ITEM_MATERIAL)
				quest::CQuestManager::instance().PickupItem (owner->GetPlayerID(), item);

			return true;
		}


	}

	return false;
}

bool CHARACTER::SwapItem(BYTE bWindow, BYTE bCell, BYTE bDestWindow, BYTE bDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(bWindow, bCell), destCell(bDestWindow, bDestCell);

	// ¿?¹?¸¥ Cell ???ö °?»ç
	// ¿ë?¥¼®?º Swap?? ¼ö ¾ø?¸¹?·?, ¿©±â¼­ °?¸².
	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	// °°?º CELL ???ö °?»ç
	if (bCell == bDestCell)
		return false;

	// µ? ´? ?åºñ?¢ ?§?¡¸é Swap ?? ¼ö ¾ø´?.
	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	// item2°¡ ?åºñ?¢¿¡ ??´? °??? µ?µµ·?.
	if (srcCell.IsEquipPosition())
	{
		item1 = GetItem(destCell);
		item2 = GetItem(srcCell);
	}
	else
	{
		item1 = GetItem(srcCell);
		item2 = GetItem(destCell);
	}

	if (!item1 || !item2)
		return false;

	if (item1 == item2)
	{
	    sys_log(0, "[WARNING][WARNING][HACK USER!] : %s %d %d", m_stName.c_str(), bCell, bDestCell);
	    return false;
	}

	if (!IsEmptyItemGrid(TItemPos (item1->GetWindow(), item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	// ¹?²? ¾??????? ?åºñ?¢¿¡ ???¸¸é
	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		BYTE bInvenCell = item1->GetCell();

		// ?ø¿ë???? ¾??????» ¹þ?» ¼ö ??°í, ?ø¿ë ¿¹?¤ ¾??????? ?ø¿ë °¡´??? »ó??¿©¾?¸¸ ?ø??
		if (item2->IsDragonSoul() || item2->GetType() == ITEM_BELT) // @fixme117
		{
			if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
				return false;
		}

		if (bEquipCell != item1->FindEquipCell(this)) // °°?º ?§?¡??¶§¸¸ ?ã¿ë
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
			item2->AddToCharacter(this, TItemPos(bWindow, bInvenCell));
		else
			sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
	}
	else
	{
		BYTE bCell1 = item1->GetCell();
		BYTE bCell2 = item2->GetCell();

		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

		item1->AddToCharacter(this, TItemPos(bDestWindow, bCell2));
		item2->AddToCharacter(this, TItemPos(bWindow, bCell1));
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	int iWearCell = item->FindEquipCell(this);
	if (iWearCell == WEAR_WEAPON)
	{
		LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
		if (costumeWeapon && !UnequipItem(costumeWeapon))
		{
			ChatPacket(CHAT_TYPE_INFO, "300");
			return false;
		}
	}
#endif

	if (false == CanUnequipNow(item))
		return false;

	int pos;
	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();

	if (item->IsDragonSoul())
	{
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
	}
	else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));


#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if(item->IsCostumeMount())
	{
		if (GetHorse())
			HorseSummon(false);
	}
#endif

#ifdef ENABLE_MOUNT_SKIN
	if(item->IsCostumeMountSkin())
	{
		if (GetHorse())
			HorseSummon(false);
		HorseSummon(true);
	}
#endif

#ifdef ENABLE_MOUNT_SYSTEM
	if(item->IsRideItem())
	{
		CMountSystem* MountSystem = GetMountSystem();
		if(MountSystem){
			if(MountSystem->CountSummoned() != 0){
				MountSystem->Unsummon(item->GetValue(2));
				ChatPacket(CHAT_TYPE_INFO,"Skin Montura Guardada.");
			}
		}
	}
#endif


	CheckMaximumPoints();

	return true;
}

//
// @version	05/07/05 Bang2ni - Skill »ç¿ë?? 1.5 ?? ??³»¿¡ ?åºñ ?ø¿ë ±??ö
//
bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{
	if (item->IsExchanging())
		return false;
	if (false == item->IsEquipable())
		return false;
	if (false == CanEquipNow(item))
		return false;
	int iWearCell = item->FindEquipCell(this, iCandidateCell);

	if (iWearCell < 0)
		return false;
	// ¹«¾ð°¡¸¦ ?º »ó??¿¡¼­ ??½?µµ ??±â ±??ö
	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, "309");
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, "310");
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, "280");
		return false;
	}

	//½?±? ?»°? »ç¿ë½? ±â?¸ ¸» »ç¿ë¿©º? ?¼?©
	if(item->IsRideItem() && IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, "311");// you can modify here...
		return false;
	}

	// ?­»ì ??¿?¿¡´? ¸¶?ö¸· °ø°? ½?°£ ¶?´? ½º?³ »ç¿ë 1.5 ??¿¡ ?åºñ ±³?¼°¡ °¡´?
	DWORD dwCurTime = get_dword_time();

	if (iWearCell != WEAR_ARROW
		&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		ChatPacket(CHAT_TYPE_INFO, "312");
		return false;
	}

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (iWearCell == WEAR_WEAPON)
	{
		if (item->GetType() == ITEM_WEAPON)
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && costumeWeapon->GetValue(3) != item->GetSubType() && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, "300");
				return false;
			}
		}
		else //fishrod/pickaxe
		{
			LPITEM costumeWeapon = GetWear(WEAR_COSTUME_WEAPON);
			if (costumeWeapon && !UnequipItem(costumeWeapon))
			{
				ChatPacket(CHAT_TYPE_INFO, "300");
				return false;
			}
		}
	}
	else if (iWearCell == WEAR_COSTUME_WEAPON)
	{
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);
			if (!pkWeapon || pkWeapon->GetType() != ITEM_WEAPON || item->GetValue(3) != pkWeapon->GetSubType())
			{
				ChatPacket(CHAT_TYPE_INFO, "313");
				return false;
			}
		}
	}
#endif


	if (item->GetType() == ITEM_NEW_RINGS){

		LPITEM rings1 = GetWear(WEAR_RINGS_1);
		LPITEM rings2 = GetWear(WEAR_RINGS_2);

		if(rings1)
		{

			if(rings1->GetAccessorySocketVnumRings(rings1) == item->GetAccessorySocketVnumRings(item))
			{
				ChatPacket(CHAT_TYPE_INFO,"No puede equipar el mismo anillo.");
				return false;
			}
		}

		if(rings2)
		{
			if(rings2->GetAccessorySocketVnumRings(rings2) == item->GetAccessorySocketVnumRings(item))
			{
				ChatPacket(CHAT_TYPE_INFO,"No puede equipar el mismo anillo.");
				return false;
			}
		}

	}

	// ¿ë?¥¼® ?¯¼ö ?³¸®
	if (item->IsDragonSoul())
	{
		// °°?º ?¸???? ¿ë?¥¼®?? ??¹? µé¾î°¡ ??´?¸é ?ø¿ë?? ¼ö ¾ø´?.
		// ¿ë?¥¼®?º swap?» ?ö¿ø??¸é ¾?µ?.
		if(GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, "??¹? °°?º ?¾·ù?? ¿ë?¥¼®?» ?ø¿ë??°í ??½?´?´?.");
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	// ¿ë?¥¼®?? ¾?´?.
	else
	{
		// ?ø¿ë?? °÷¿¡ ¾??????? ??´?¸é,
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			// ?? ¾??????º ??¹ø ¹??÷¸é º¯°æ º?°¡. swap ¿ª½? ¿??ü º?°¡
			if (item->GetWearFlag() == WEARABLE_ABILITY)
				return false;

			if (false == SwapItem(item->GetWindow(), item->GetCell(), INVENTORY, INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			BYTE bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

	if (true == item->IsEquipped())
	{
		// ¾????? ???? »ç¿ë ????º???´? »ç¿ë???ö ¾?¾?µµ ½?°£?? ?÷°¨µ?´? ¹æ½? ?³¸®.
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			// ?? ¹ø??¶óµµ »ç¿ë?? ¾????????ö ¿©º?´? Socket1?» º¸°í ??´???´?. (Socket1¿¡ »ç¿ë?½¼ö ±â·?)
			if (0 == item->GetSocket(1))
			{
				// »ç¿ë°¡´?½?°£?º Default °ª?¸·? Limit Value °ª?» »ç¿ë??µ?, Socket0¿¡ °ª?? ???¸¸é ±? °ª?» »ç¿ë??µµ·? ??´?. (´??§´? ??)
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[(unsigned char)(item->GetProto()->cLimitRealTimeFirstUseIndex)].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const DWORD& dwVnum = item->GetVnum();

		// ¶ó¸¶´? ??º¥?® ??½?´??? ¹??ö(71135) ?ø¿ë½? ???å?® ¹?µ¿
		if (true == CItemVnumHelper::IsRamadanMoonRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_RAMADAN_RING);
		}
		// ??·??© »ç??(71136) ?ø¿ë½? ???å?® ¹?µ¿
		else if (true == CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}
		// ??º¹?? ¹??ö(71143) ?ø¿ë½? ???å?® ¹?µ¿
		else if (true == CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}
		// »ç¶û?? ??´ø?®(71145) ?ø¿ë½? ???å?® ¹?µ¿
		else if (true == CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
		// ITEM_UNIQUE?? °æ¿ì, SpecialItemGroup¿¡ ?¤??µ?¾î ??°í, (item->GetSIGVnum() != NULL)
		//
		else if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}
		#ifdef __SASH_SYSTEM__
		else if ((item->GetType() == ITEM_COSTUME) && (item->GetSubType() == COSTUME_SASH))
			this->EffectPacket(SE_EFFECT_SASH_EQUIP);
		#endif
		if (
			(ITEM_UNIQUE == item->GetType() && UNIQUE_SPECIAL_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE))
			|| (ITEM_UNIQUE == item->GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE))

		)
		{
			quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
		}

#if defined(WJ_COMBAT_ZONE)
#ifdef ENABLE_MOUNT_SYSTEM
		if(item->IsRideItem() && !CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		{
			CMountSystem* MountSystem = GetMountSystem();
			if(MountSystem){
				if(MountSystem->CountSummoned() <= 0){
					CMountActor* MState = MountSystem->Summon(item->GetValue(2), GetName(), item);
					if(MState){
						ChatPacket(CHAT_TYPE_INFO,"Skin Montura Invocada.");
					}
				}
			}
		}
#endif
#endif

	}
#ifdef __ITEM_SHINING__
	if (item->GetType() == ITEM_SHINING)
		this->UpdatePacket();
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsCostumeMount()
#ifdef ENABLE_MOUNT_SKIN
		|| item->IsCostumeMountSkin()
#endif
	)
	{
		if (GetHorse())
			HorseSummon(false);
		HorseSummon(true);
	}
#endif
	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (size_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if(0 == bOldValue)
	{
		CBuffOnAttributes* pBuff = NULL;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
				{
#ifdef ENABLE_GLOVE_SYSTEM
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD, WEAR_GLOVE };
#else
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD};
#endif
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			case POINT_COSTUME_ATTR_BONUS:
				{
					static BYTE abSlot[] = {
						WEAR_COSTUME_BODY,
						WEAR_COSTUME_HAIR,
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
						WEAR_COSTUME_WEAPON,
#endif
						WEAR_COSTUME_NEW_SASH,
					};
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));

		}
		else
			pBuff = it->second;
		if (pBuff != NULL)
			pBuff->On(bNewValue);
	}
	else
	{
		assert (m_map_buff_on_attrs.end() != it);
		it->second->ChangeBuffValue(bNewValue);
	}
}



LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);
	}

#ifdef ENABLE_SPECIAL_STORAGE
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(4))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (GetChangeInventoryItem(i) && GetChangeInventoryItem(i)->GetVnum() == vnum)
			return GetChangeInventoryItem(i);
	}

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(5))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (GetCostumeInventoryItem(i) && GetCostumeInventoryItem(i)->GetVnum() == vnum)
			return GetCostumeInventoryItem(i);
	}
	
#endif

	return NULL;
}


LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i=BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}


#ifdef ENABLE_BUY_ITEMS_WORLDARD
int CHARACTER::CountSpecifyItemBuy(DWORD vnum) const
{
	int	count = 0;
	LPITEM item;

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			// °³?? »ó?¡¿¡ µî·?µ? ¹°°???¸é ³?¾î°£´?.
			count += item->GetCount();
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(1))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetUpgradeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(2))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetBookInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(3))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetStoneInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(4))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetChangeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(5))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetCostumeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#endif
	return count;
}
#endif

int CHARACTER::CountSpecifyItem(DWORD vnum) const
{
	int	count = 0;
	LPITEM item;

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}

#ifdef ENABLE_SPECIAL_STORAGE
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(1))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetUpgradeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(2))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetBookInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(3))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetStoneInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(4))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetChangeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW(5))); ++i)
#else
	for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		item = GetCostumeInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			count += item->GetCount();
		}
	}

#endif
	return count;
}

void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count)
{
	if (0 == count)
		return;

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (vnum >= 80003 && vnum <= 80007)
			LogManager::instance().GoldBarLog(GetPlayerID(), GetInventoryItem(i)->GetID(), QUEST, "RemoveSpecifyItem");

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW(1))); ++i)
#else
	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetUpgradeInventoryItem(i))
			continue;

		if (GetUpgradeInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (count >= GetUpgradeInventoryItem(i)->GetCount())
		{
			count -= GetUpgradeInventoryItem(i)->GetCount();
			GetUpgradeInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetUpgradeInventoryItem(i)->SetCount(GetUpgradeInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW(2))); ++i)
#else
	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetBookInventoryItem(i))
			continue;

		if (GetBookInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (count >= GetBookInventoryItem(i)->GetCount())
		{
			count -= GetBookInventoryItem(i)->GetCount();
			GetBookInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetBookInventoryItem(i)->SetCount(GetBookInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW(3))); ++i)
#else
	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetStoneInventoryItem(i))
			continue;

		if (GetStoneInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (count >= GetStoneInventoryItem(i)->GetCount())
		{
			count -= GetStoneInventoryItem(i)->GetCount();
			GetStoneInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetStoneInventoryItem(i)->SetCount(GetStoneInventoryItem(i)->GetCount() - count);
			return;
		}
	}

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW(4))); ++i)
#else
	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetChangeInventoryItem(i))
			continue;

		if (GetChangeInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (count >= GetChangeInventoryItem(i)->GetCount())
		{
			count -= GetChangeInventoryItem(i)->GetCount();
			GetChangeInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetChangeInventoryItem(i)->SetCount(GetChangeInventoryItem(i)->GetCount() - count);
			return;
		}
	}
#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW(5))); ++i)
#else
	for (UINT i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetCostumeInventoryItem(i))
			continue;

		if (GetCostumeInventoryItem(i)->GetVnum() != vnum)
			continue;

		if (count >= GetCostumeInventoryItem(i)->GetCount())
		{
			count -= GetCostumeInventoryItem(i)->GetCount();
			GetCostumeInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetCostumeInventoryItem(i)->SetCount(GetCostumeInventoryItem(i)->GetCount() - count);
			return;
		}
	}

#endif
	// ¿¹¿??³¸®°¡ ¾???´?.
	if (count)
		sys_log(0, "CHARACTER::RemoveSpecifyItem cannot remove enough item vnum %u, still remain %d", vnum, count);
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (int i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

#ifdef UNLOCK_INVENTORY_WORLDARD
	for (UINT i = 0; i < (90 + (5*GetUnlockSlotsW())); ++i)
#else
	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
#endif
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return;
	}
	if (item->GetOwner() != NULL && item->GetOwner() != this)
	{
		sys_err ("item %d 's owner exists!",item->GetID());
		return;
	}

	DWORD bCount = item->GetCount();

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i=0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);
			if (inv_item == NULL)
				continue;
			if (inv_item->GetType() == ITEM_BLEND && inv_item->GetVnum() == item->GetVnum())
			{
				if (inv_item->GetSocket(0) == item->GetSocket(0) && inv_item->GetSocket(1) == item->GetSocket(1) && inv_item->GetSocket(2) == item->GetSocket(2) && inv_item->GetCount() < g_bItemCountLimit)
				{
					inv_item->SetCount(inv_item->GetCount() + item->GetCount());
					M2_DESTROY_ITEM(item);
					return;
				}
			}
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetUpgradeInventoryItem(i);
			if (!item2)
				continue;
			if (item2->GetVnum() == item->GetVnum())
			{
				DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;
				item2->SetCount(item2->GetCount() + bCount2);
				if (bCount == 0)
				{
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem(GetPlayerID(), item2);
					M2_DESTROY_ITEM(item);
					return;
				}
			}
		}
		
	}
	else if (item->IsBook() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetBookInventoryItem(i);
			if (!item2)
				continue;
			if (item2->GetVnum() == item->GetVnum())
			{
				int mithras;
				for (mithras = 0; mithras < ITEM_SOCKET_MAX_NUM; ++mithras)
					if (item2->GetSocket(mithras) != item->GetSocket(mithras))
						break;
				if (mithras != ITEM_SOCKET_MAX_NUM)
					continue;
				const DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;
				item2->SetCount(item2->GetCount() + bCount2);
				if (bCount == 0)
				{
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					M2_DESTROY_ITEM(item);
					return;
				}
			}
		}
	}
	else if (item->IsStone() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetStoneInventoryItem(i);
			if (!item2)
				continue;
			if (item2->GetVnum() == item->GetVnum())
			{
				const DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;
				item2->SetCount(item2->GetCount() + bCount2);
				if (bCount == 0)
				{
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					M2_DESTROY_ITEM(item);
					return;
				}
			}
		}
	}
	else if (item->IsChange() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetChangeInventoryItem(i);
			if (!item2)
				continue;
			if (item2->GetVnum() == item->GetVnum())
			{
				const DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;
				item2->SetCount(item2->GetCount() + bCount2);
				if (bCount == 0)
				{
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					M2_DESTROY_ITEM(item);
					return;
				}
			}
		}
	}
	else if (item->IsCostume() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetCostumeInventoryItem(i);
			if (!item2)
				continue;
			if (item2->GetVnum() == item->GetVnum())
			{
				const DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;
				item2->SetCount(item2->GetCount() + bCount2);
				if (bCount == 0)
				{
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					M2_DESTROY_ITEM(item);
					return;
				}
			}
		}
	}
	else
	{
		TItemTable * p = ITEM_MANAGER::instance().GetTable(item->GetVnum());
		if (!p)
		{
			M2_DESTROY_ITEM(item);
			return;
		}
		if (p->dwFlags & ITEM_FLAG_STACKABLE)
		{
			for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
			{
				LPITEM invItem = GetInventoryItem(i);
				if (!invItem)
					continue;
#ifdef ENABLE_SORT_INVEN
				if (invItem->GetOriginalVnum() == item->GetVnum() && FN_check_item_socket(invItem))
#else
				if (invItem->GetVnum() == item->GetVnum() && FN_check_item_socket(invItem))
#endif
				{
					if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
					{
						if (bCount < p->alValues[1])
							bCount = p->alValues[1];
					}
					const DWORD bCount2 = MIN(g_bItemCountLimit - invItem->GetCount(), bCount);
					bCount -= bCount2;
					invItem->SetCount(invItem->GetCount() + bCount2);
					if (bCount == 0)
					{
						if (invItem->GetType() == ITEM_QUEST || invItem->GetType() == ITEM_MATERIAL)
							quest::CQuestManager::instance().PickupItem (GetPlayerID(), invItem);
						M2_DESTROY_ITEM(item);
						return;
					}
				}
			}
		}
	}

	item->SetCount(bCount);
#endif

	int cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
	{
		cell = GetEmptyUpgradeInventory(item);
	}
	else if (item->IsBook())
	{
		cell = GetEmptyBookInventory(item);
	}
	else if (item->IsStone())
	{
		cell = GetEmptyStoneInventory(item);
	}
	else if (item->IsChange())
	{
		cell = GetEmptyChangeInventory(item);
	}
	else if (item->IsCostume())
	{
		cell = GetEmptyCostumeInventory(item);
	}
#endif
	else
	{
		cell = GetEmptyInventory (item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, cell));
		else if (item->IsBook())
			item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, cell));
		else if (item->IsStone())
			item->AddToCharacter(this, TItemPos(STONE_INVENTORY, cell));
		else if (item->IsChange())
			item->AddToCharacter(this, TItemPos(CHANGE_INVENTORY, cell));
		else if (item->IsCostume())
			item->AddToCharacter(this, TItemPos(COSTUME_INVENTORY, cell));
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));


		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}

		if (item->GetType() == ITEM_QUEST || item->GetType() == ITEM_MATERIAL)
			quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
	}
	else
	{
		item->AddToGround (GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif

		if (longOwnerShip)
			item->SetOwnership (this, 300);
		else
			item->SetOwnership (this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}
}

LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum, DWORD bCount, int iRarePct, bool bMsg)
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND)
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

#ifdef ENABLE_SORT_INVEN
			if (item->GetOriginalVnum() == dwItemVnum && FN_check_item_socket(item))
#else
			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
#endif
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				DWORD bCount2 = MIN(g_bItemCountLimit - item->GetCount(), bCount);
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg){
#ifdef __CHAT_SETTINGS__
						ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item->GetName());
#else
						ChatPacket(CHAT_TYPE_INFO, "305 %s", item->GetName());
#endif
					}
					if (item->GetType() == ITEM_QUEST || item->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
					return item;
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i=0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
							inv_item->GetSocket(1) == item->GetSocket(1) &&
							inv_item->GetSocket(2) == item->GetSocket(2) &&
							inv_item->GetCount() < g_bItemCountLimit)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						return inv_item;
					}
				}
			}
		}
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		DWORD bCount = item->GetCount();

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetUpgradeInventoryItem(i);

			if (!item2)
				continue;

			if (item2->GetVnum() == item->GetVnum())
			{

				DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;
			
				item2->SetCount(item2->GetCount() + bCount2);

				if (bCount == 0)
				{
#ifdef __CHAT_SETTINGS__
					ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
					ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
					M2_DESTROY_ITEM(item);
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					return item2;
				}
			}
		}

		item->SetCount(bCount);
	}
	else if (item->IsBook() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		DWORD bCount = item->GetCount();

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetBookInventoryItem(i);

			if (!item2)
				continue;

			if (item2->GetVnum() == item->GetVnum())
			{

				int mithras;

				for (mithras = 0; mithras < ITEM_SOCKET_MAX_NUM; ++mithras)
					if (item2->GetSocket(mithras) != item->GetSocket(mithras))
						break;

				if (mithras != ITEM_SOCKET_MAX_NUM)
					continue;

				DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;

				item2->SetCount(item2->GetCount() + bCount2);

				if (bCount == 0)
				{
#ifdef __CHAT_SETTINGS__
					ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
					ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
					M2_DESTROY_ITEM(item);
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					return item2;
				}
			}
		}

		item->SetCount(bCount);
	}
	else if (item->IsStone() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		DWORD bCount = item->GetCount();

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetStoneInventoryItem(i);

			if (!item2)
				continue;

			if (item2->GetVnum() == item->GetVnum())
			{

				DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;

				item2->SetCount(item2->GetCount() + bCount2);

				if (bCount == 0)
				{
#ifdef __CHAT_SETTINGS__
					ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
					ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
					M2_DESTROY_ITEM(item);
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					return item2;
				}
			}
		}
		item->SetCount(bCount);
	}
	else if (item->IsChange() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		DWORD bCount = item->GetCount();

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetChangeInventoryItem(i);

			if (!item2)
				continue;

			if (item2->GetVnum() == item->GetVnum())
			{

				DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;

				item2->SetCount(item2->GetCount() + bCount2);

				if (bCount == 0)
				{
#ifdef __CHAT_SETTINGS__
					ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
					ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
					M2_DESTROY_ITEM(item);
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					return item2;
				}
			}
		}

		item->SetCount(bCount);
	}
	else if (item->IsCostume() && item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		DWORD bCount = item->GetCount();

		for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item2 = GetCostumeInventoryItem(i);
			if (!item2)
				continue;
			if (item2->GetVnum() == item->GetVnum())
			{

				DWORD bCount2 = MIN(g_bItemCountLimit - item2->GetCount(), bCount);
				bCount -= bCount2;

				item2->SetCount(item2->GetCount() + bCount2);

				if (bCount == 0)
				{
#ifdef __CHAT_SETTINGS__
				ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item2->GetName());
#else
				ChatPacket(CHAT_TYPE_INFO, "305 %s", item2->GetName());
#endif
					M2_DESTROY_ITEM(item);
					if (item2->GetType() == ITEM_QUEST || item2->GetType() == ITEM_MATERIAL)
						quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
					return item2;
				}
			}
		}
		item->SetCount(bCount);
	}

#endif

	int iEmptyCell;
	if (item->IsDragonSoul())
	{
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	}
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
	{
		iEmptyCell = GetEmptyUpgradeInventory(item);
	}
	else if (item->IsBook())
	{
		iEmptyCell = GetEmptyBookInventory(item);
	}
	else if (item->IsStone())
	{
		iEmptyCell = GetEmptyStoneInventory(item);
	}
	else if (item->IsChange())
	{
		iEmptyCell = GetEmptyChangeInventory(item);
	}
	else if (item->IsCostume())
	{
		iEmptyCell = GetEmptyCostumeInventory(item);
	}
#endif
	else
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg){
#ifdef __CHAT_SETTINGS__
			ChatPacket(CHAT_TYPE_ITEM_INFO, "304 %s", item->GetName());
#else
			ChatPacket(CHAT_TYPE_INFO, "305 %s", item->GetName());
#endif
		}

		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, iEmptyCell));
		else if (item->IsBook())
			item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, iEmptyCell));
		else if (item->IsStone())
			item->AddToCharacter(this, TItemPos(STONE_INVENTORY, iEmptyCell));
		else if (item->IsChange())
			item->AddToCharacter(this, TItemPos(CHANGE_INVENTORY, iEmptyCell));
		else if (item->IsCostume())
			item->AddToCharacter(this, TItemPos(COSTUME_INVENTORY, iEmptyCell));
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
#ifdef ENABLE_NEWSTUFF
		item->StartDestroyEvent(g_aiItemDestroyTime[ITEM_DESTROY_TIME_AUTOGIVE]);
#else
		item->StartDestroyEvent();
#endif
		// ¾??¼ µå¶ø flag°¡ °?·???´? ¾??????? °æ¿ì,
		// ??º¥¿¡ ºó °ø°£?? ¾ø¾î¼­ ¾î?¿ ¼ö ¾ø?? ¶³¾î?®¸®°? µ?¸é,
		// ownership?» ¾??????? »ç¶ó?ú ¶§±î?ö(300??) ?¯?ö??´?.
		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}

	sys_log(0,
		"7: %d %d", dwItemVnum, bCount);
	return item;
}

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	// @fixme150 BEGIN
	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, "314");
		return false;
	}
	// @fixme150 END

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
#ifdef ENABLE_SOULBIND_SYSTEM
	if(item->IsSealed()){
		from->ChatPacket(CHAT_TYPE_INFO, "315");
		return false;
	}
#endif

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	int prt_multi_refine = CMultiRefineManager::instance().GetMultiRefineSize(item->GetVnum());
#endif

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH &&
					(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		//case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
#ifdef ENABLE_MULTI_REFINE_WORLDARD
			if (item->GetType() == ITEM_WEAPON && 
					(item->GetRefinedVnum() || prt_multi_refine != 0))
#else
			if (item->GetType() == ITEM_WEAPON && 
					item->GetRefinedVnum())
#endif
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		//case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
#ifdef ENABLE_MULTI_REFINE_WORLDARD
			if (item->GetType() == ITEM_ARMOR && 
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					(item->GetRefinedVnum() || prt_multi_refine != 0))
#else
			if (item->GetType() == ITEM_ARMOR && 
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
#endif
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		//case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
#ifdef ENABLE_MULTI_REFINE_WORLDARD
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					(item->GetRefinedVnum() || prt_multi_refine != 0))
#else
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
#endif
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
#ifdef ENABLE_MULTI_REFINE_WORLDARD
			if (item->GetRefinedVnum() || prt_multi_refine != 0)
#else
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
#endif
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:

#ifdef ENABLE_MULTI_REFINE_WORLDARD
			if (prt_multi_refine != 0 || item->GetRefineSet() >= 500)
#else
			if (item->GetRefineSet() >= 500)
#endif
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
#ifdef ENABLE_MULTI_REFINE_WORLDARD
			if (item->GetRefinedVnum() || prt_multi_refine != 0)
#else
			if (item->GetRefinedVnum())
#endif
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:
			// ??±? ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, "316");
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, "317");
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:
			// ??±? ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, "316");
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, "317");
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:
			// °í±? ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, "316");
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, "317");
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	int prt_multi_refine = CMultiRefineManager::instance().GetMultiRefineSize(item->GetVnum());
#endif
	
	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

		/*
			// DEVILTOWER_NPC
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			#ifdef ENABLE_MULTI_REFINE_WORLDARD
				if ((item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500) || (prt_multi_refine != 0))

			#else
				if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)	
			#endif
			{
				from->SetRefineNPC(this);

				if (item->IsUpgradeItem()){
					from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY, -1,UPGRADE_INVENTORY);
				}
				else if (item->IsBook()){
					from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY, -1,BOOK_INVENTORY);
				}
				else if (item->IsStone()){
					from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY,-1,STONE_INVENTORY);
				}
				else{
					from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY, -1,INVENTORY);
				}

			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, "117");
			}
			break;
			// END_OF_DEVILTOWER_NPC
		*/
		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
			#ifdef ENABLE_MULTI_REFINE_WORLDARD
				if (item->GetRefinedVnum() || prt_multi_refine != 0)

			#else
				if (item->GetRefinedVnum())
			#endif

			{
				from->SetRefineNPC(this);

				//if (item->IsUpgradeItem() || ){
				//	from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL, -1,UPGRADE_INVENTORY);
				//}
				//else if (item->IsBook()){
				//	from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL, -1,BOOK_INVENTORY);
				//}
				if (item->IsStone() && item->GetWindow() == STONE_INVENTORY){
					from->ChatPacket(CHAT_TYPE_INFO, "2508");
					return;
				}
				else{
					from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL, -1,INVENTORY);
				}


			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, "117");
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				from->ReviveHorse();
				item->SetCount(item->GetCount()-1);
				from->ChatPacket(CHAT_TYPE_INFO, "318");
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
					item->GetVnum() == ITEM_HORSE_FOOD_2 ||
					item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				from->FeedHorse();
				from->ChatPacket(CHAT_TYPE_INFO, "319");
				item->SetCount(item->GetCount()-1);
				EffectPacket(SE_HPUP_RED);
			}
			break;

		default:
			sys_log(0, "TakeItem %s %d %s", from->GetName(), GetRaceNum(), item->GetName());
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	// ¾ð¾î¹??ö?? °æ¿ì ¾ð¾î¹??ö(°?º») ???öµµ ?¼?©??´?.
	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC( NULL );
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD> &dwItemVnums,
											std::vector<DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess = true;
	int pos_check = 0;

	

	for (int i = 0; i < n; i++)
	{

		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		if(dwVnum)
		{
			LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(dwVnum, dwCount);
			if (pkNewItem)
			{
				int pos = -1;

#ifdef ENABLE_SPECIAL_STORAGE
				if (pkNewItem->IsUpgradeItem())
				{
					pos = GetEmptyUpgradeInventory(pkNewItem);
				}
				else if (pkNewItem->IsBook())
				{
					pos = GetEmptyBookInventory(pkNewItem);
				}
				else if (pkNewItem->IsStone())
				{
					pos = GetEmptyStoneInventory(pkNewItem);
				}
				else if (pkNewItem->IsChange())
				{
					pos = GetEmptyChangeInventory(pkNewItem);
				}
				else if (pkNewItem->IsCostume())
				{
					pos = GetEmptyCostumeInventory(pkNewItem);
				}
				else{
					pos = GetEmptyInventory(pkNewItem->GetSize());
				}
#endif
				if(pos != -1)
				{
					pos_check += 1;
				}
			}
		}
	}

	/*
	if (CountEmptyInventory() < pos_check || pos_check <= 0)
	{
		ChatPacket(CHAT_TYPE_INFO, "No tiene suficiente espacio en su inventario");
		return false;
	}
*/

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;

							
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				PointChange(POINT_GOLD, dwCount);
				LogManager::instance().CharLog(this, dwCount, "TREASURE_GOLD", "");

				bSuccess = true;
				break;
			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);
					LogManager::instance().CharLog(this, dwCount, "TREASURE_EXP", "");

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					sys_log(0, "CSpecialItemGroup::MOB %d", dwCount);
					int x = GetX() + number(-500, 500);
					int y = GetY() + number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
						ch->SetAggressive();
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::SLOW:
				{
					sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP()*dwCount/100;
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(NULL);
					bSuccess = true;
				}
				break;
#ifdef ENABLE_WOLFMAN_CHARACTER
			case CSpecialItemGroup::BLEEDING:
				{
					AttackedByBleeding(NULL);
					bSuccess = true;
				}
				break;
#endif
			case CSpecialItemGroup::MOB_GROUP:
				{
					int sx = GetX() - number(300, 500);
					int sy = GetY() - number(300, 500);
					int ex = GetX() + number(300, 500);
					int ey = GetY() + number(300, 500);
					CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

					bSuccess = true;
				}
				break;
			default:
				{

					item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

					if (item_get)
					{
						bSuccess = true;
					}
				}
				break;
		}

		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;

		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		// ·¹º§ ?¦??¿¡ °?¸²
		ChatPacket(CHAT_TYPE_INFO, "320");
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000; // 73001 - 72000 = 1001 º??? ?ì¾î ¹ø?£ ½???
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			break; // NOTE: ?? ?ì¾î??µå´? ¾? ¾²??¹?·? ??½º. (?ö?ç ?ì¾î½?½º???º ??¹? ??½º?¬?¸·? ´ë?¼ µ? »ó????)
#endif
		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, "321");
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
#if defined(WJ_COMBAT_ZONE)	
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
	{
		ChatPacket(CHAT_TYPE_INFO, "322");
		return false;
	}
#endif
#ifdef TOURNAMENT_PVP_SYSTEM
	if (CTournamentPvP::instance().IsTournamentMap(this, TOURNAMENT_BLOCK_POLY))	
		return false;
#endif
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, "323");
		return false;
	}

	if (true == IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, "324");
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, "325");
		item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket(CHAT_TYPE_INFO, "325");
		item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{
				// µ?°©±¸ ?³¸®
				sys_log(0, "USE_POLYMORPH_BALL PID(%d) vnum(%d)", GetPlayerID(), dwVnum);

				// ·¹º§ ?¦?? ?¼?©
				int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, "326");
					return false;
				}

				int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				DWORD dwBonus = 0;

				dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID)/40) * 100;

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);

				item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				// º¸·ù

				// µ?°©¼­ ?³¸®
				// ¼???0                ¼???1           ¼???2
				// µ?°©?? ¸ó½º?? ¹ø?£   ¼ö·??¤µµ        µ?°©¼­ ·¹º§
				sys_log(0, "USE_POLYMORPH_BOOK: %s(%u) vnum(%u)", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
				{
					CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;
#ifdef ELEMENT_SPELL_WORLDARD
	if (IsOpenElementsSpell()) return false;
#endif
	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	/*
	LPITEM Unique1 = GetWear(WEAR_UNIQUE1);
	LPITEM Unique2 = GetWear(WEAR_UNIQUE2);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	LPITEM MountCostume = GetWear(WEAR_COSTUME_MOUNT);
#endif

	if( NULL != Unique1 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup() )
		{
			return UnequipItem(Unique1);
		}
	}

	if( NULL != Unique2 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup() )
		{
			return UnequipItem(Unique2);
		}
	}
	*/

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	LPITEM MountCostume = GetWear(WEAR_COSTUME_MOUNT);
	if (MountCostume)
		return UnequipItem(MountCostume);
#endif

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

#if defined(ENABLE_NEW_AUTOPOTION)
	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type && AFFECT_AUTO_ATTACK_SPEED_RECOVERY != type && AFFECT_AUTO_MOV_SPEED_RECOVERY != type && AFFECT_AUTO_CRITICAL_RECOVERY != type && AFFECT_AUTO_PENETRATE_RECOVERY != type && AFFECT_AUTO_VIT_RECOVERY != type && AFFECT_AUTO_STR_RECOVERY != type && AFFECT_AUTO_INT_RECOVERY != type && AFFECT_AUTO_DEX_RECOVERY != type && AFFECT_AUTO_RED_POTION_RECOVERY != type && AFFECT_AUTO_ORANGE_POTION_RECOVERY != type && AFFECT_AUTO_YELLOW_POTION_RECOVERY != type && AFFECT_AUTO_GREEN_POTION_RECOVERY != type && AFFECT_AUTO_BLUE_POTION_RECOVERY != type && AFFECT_AUTO_WHITE_POTION_RECOVERY != type)
		return;
#else
	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;
#endif

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(DWORD) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (!CArenaManager::instance().IsArenaMap(GetMapIndex())
#ifdef ENABLE_NEWSTUFF
				&& !(g_NoPotionsOnPVP && CPVPManager::instance().IsFighting(GetPlayerID()) && !IsAllowedPotionOnPVP(pItem->GetVnum()))
#endif
			)
			{
				const long amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);

				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}
#if defined(ENABLE_NEW_AUTOPOTION)
				else {
					switch (type)
					{
						case AFFECT_AUTO_ATTACK_SPEED_RECOVERY:
						case AFFECT_AUTO_MOV_SPEED_RECOVERY:
						case AFFECT_AUTO_CRITICAL_RECOVERY:
						case AFFECT_AUTO_PENETRATE_RECOVERY:
						case AFFECT_AUTO_VIT_RECOVERY:
						case AFFECT_AUTO_STR_RECOVERY:
						case AFFECT_AUTO_INT_RECOVERY:
						case AFFECT_AUTO_DEX_RECOVERY:
						case AFFECT_AUTO_RED_POTION_RECOVERY:
						case AFFECT_AUTO_ORANGE_POTION_RECOVERY:
						case AFFECT_AUTO_YELLOW_POTION_RECOVERY:
						case AFFECT_AUTO_GREEN_POTION_RECOVERY:
						case AFFECT_AUTO_BLUE_POTION_RECOVERY:
						case AFFECT_AUTO_WHITE_POTION_RECOVERY:
							{
								DWORD dwAffectType = pItem->GetValue(0);
								if (FindAffect(dwAffectType))
									return;

								long duration = INFINITE_AFFECT_DURATION;
								if (pItem->IsTimerAutoPotionEvent())
									duration = pItem->GetSocket(2);

								for (size_t applyIndex = 0; applyIndex < ITEM_APPLY_MAX_NUM; applyIndex++) {
									TItemApply itemApply = pItem->GetProto()->aApplies[applyIndex];
									
									if (itemApply.bType == 0 || itemApply.lValue == 0)
										continue;

									AddAffect(dwAffectType, aApplyInfo[itemApply.bType].bPointType, itemApply.lValue, AFF_NONE, duration, 0, true);
								}
							}
							break;
						default:
							break;
					}
				}
#endif

				if (amount > 0)
				{
					if (avail > amount)
					{
#if !defined(ENABLE_NEW_AUTOPOTION)
						const int pct_of_used = amount_of_used * 100 / amount_of_full;
						const int pct_of_will_used = (amount_of_used + amount) * 100 / amount_of_full;

						bool bLog = false;


						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount, bLog);
#endif
					}
					else
					{
						amount = avail;

						ITEM_MANAGER::instance().RemoveItem( pItem );
					}

					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange( POINT_HP_RECOVERY, amount );
						EffectPacket( SE_AUTO_HPUP );
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange( POINT_SP_RECOVERY, amount );
						EffectPacket( SE_AUTO_SPUP );
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}
		else
		{
			RemoveAffect( const_cast<CAffect*>(pAffect) );
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
#ifdef ENABLE_SPECIAL_STORAGE
	case UPGRADE_INVENTORY:
	case BOOK_INVENTORY:
	case STONE_INVENTORY:
	case CHANGE_INVENTORY:
	case COSTUME_INVENTORY:
		return cell < (SPECIAL_INVENTORY_MAX_NUM);
#endif
#ifdef ENABLE_SWITCHBOT
	case SWITCHBOT:
		return cell < SWITCHBOT_SLOT_COUNT;
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	case CHANGE_EQUIP:
		return cell < CHANGE_EQUIP_SLOT_COUNT;
#endif
		
	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;
	default:
		return false;
	}
}


// ±??ú¾?¼­ ¸¸µç ¸??©·?.. exp°¡ true¸é msg¸¦ ?â·???°í return false ??´? ¸??©·? (??¹??û?? verify ¿ëµµ¶û?º return ¶§¹®¿¡ ¾?°£ ¹?´ë¶ó ??¸§¶§¹®¿¡ ?ò°¥¸± ¼öµµ ??°?´?..)
#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

/// ?ö?ç ?³¸¯???? »ó??¸¦ ¹????¸·? ??¾î?ø item?» ?ø¿ë?? ¼ö ??´? ?ö ?®????°í, º?°¡´? ??´?¸é ?³¸¯??¿¡°? ???¯¸¦ ¾?·???´? ??¼ö
bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	//BYTE itemType = item->GetType();
	//BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break; // TODO: ¼ö???· ¾????? ?ø¿ë°¡´?¿©º? ?³¸®
#endif
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, "327");
					return false;
				}
				break;

			case LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, "328");
					return false;
				}
				break;

			case LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, "329");
					return false;
				}
				break;

			case LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, "330");
					return false;
				}
				break;

			case LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, "331");
					return false;
				}
				break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)))
		{
			ChatPacket(CHAT_TYPE_INFO, "332");
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) &&
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, "333");
			return false;
		}

	}

#ifdef ENABLE_MOUNT_SKIN
	if (item->IsCostumeMountSkin())
	{
		if(IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, "334");
			return false;
		}

		LPITEM normalMount = GetWear(WEAR_COSTUME_MOUNT);
		if(!normalMount)
		{
			ChatPacket(CHAT_TYPE_INFO, "1098");
			return false;
		}
	}
#endif

	return true;
}

/// ?ö?ç ?³¸¯???? »ó??¸¦ ¹????¸·? ?ø¿ë ???? item?» ¹þ?» ¼ö ??´? ?ö ?®????°í, º?°¡´? ??´?¸é ?³¸¯??¿¡°? ???¯¸¦ ¾?·???´? ??¼ö
bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{

	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "º§?® ??º¥?ä¸®¿¡ ¾??????? ?¸?ç??¸é ???¦?? ¼ö ¾ø½?´?´?.");

	// ¿µ¿ø?÷ ???¦?? ¼ö ¾ø´? ¾?????
	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (item->IsCostumeMount())
	{
		if(IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, "334");
			return false;
		}

#ifdef ENABLE_MOUNT_SKIN
		LPITEM skinMount = GetWear(WEAR_COSTUME_MOUNT_SKIN);
		if(skinMount)
		{
			ChatPacket(CHAT_TYPE_INFO, "1097");
			return false;
		}
#endif

	}
#endif

#ifdef ENABLE_MOUNT_SKIN
	if (item->IsCostumeMountSkin())
	{
		if(IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, "334");
			return false;
		}
	}
#endif

	// ¾????? unequip½? ??º¥?ä¸®·? ¿?±æ ¶§ ºó ??¸®°¡ ??´? ?ö ?®??
	{
		int pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG( -1 == pos, "¼??ö?°¿¡ ºó °ø°£?? ¾ø½?´?´?." );
	}


	return true;
}

#ifdef ENABLE_SHOW_CHEST_DROP
void CHARACTER::SendChestData(DWORD chestIndex)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(chestIndex);
	if (pGroup)
	{
		DWORD chestItemCount = pGroup->GetGroupSize();
		if(chestItemCount > 0)
		{
			TEMP_BUFFER buf;
			TPacketGCChestDrop p;
			p.header = HEADER_GC_CHESTDROP;
			p.size = sizeof(p)+sizeof(DWORD)+sizeof(DWORD)+(chestItemCount*(sizeof(DWORD)*2));

			buf.write(&p, sizeof(TPacketGCChestDrop));
			buf.write(&chestIndex, sizeof(DWORD));
			buf.write(&chestItemCount, sizeof(DWORD));
			for (int i = 0; i < pGroup->GetGroupSize(); i++)
			{
				DWORD itemVnum = pGroup->GetVnum(i);
				DWORD itemCount = pGroup->GetCount(i);
				buf.write(&itemVnum, sizeof(DWORD));
				buf.write(&itemCount, sizeof(DWORD));
			}
			GetDesc()->Packet(buf.read_peek(), buf.size());
		}
	}
}
#endif

#ifdef ENABLE_MULTI_CHEST
void CHARACTER::OpenChest(LPITEM item, WORD openCount)
{
	if(openCount > item->GetCount())
		openCount = item->GetCount();
	
	if(openCount > 1000)
		openCount =1000;

	std::map<DWORD, DWORD> dwItemVnums;
	const WORD openCountCache = openCount;
	if (GiveItemFromSpecialItemGroupNew(item->GetVnum(), openCount))
	{
		if(openCount != 0)
		{
			ChatPacket(CHAT_TYPE_INFO, "1096 %d", (openCountCache-openCount));
			item->SetCount(item->GetCount() - (openCountCache-openCount));
		}
		else
			item->SetCount(item->GetCount() - openCountCache);
	}
	else
		ChatPacket(CHAT_TYPE_INFO, "976");
}
bool CHARACTER::AutoGiveItemChest(LPITEM item, DWORD& itemCount, std::map<LPITEM, WORD>& vecUpdateItems)
{
	int cell = -1;
	if (item->IsDragonSoul())
		cell = GetEmptyDragonSoulInventory(item);
#ifdef ENABLE_SPECIAL_STORAGE
	else if (item->IsUpgradeItem())
		cell = GetEmptyUpgradeInventory(item);
	else if (item->IsBook())
		cell = GetEmptyBookInventory(item);
	else if (item->IsStone())
		cell = GetEmptyStoneInventory(item);
	else if (item->IsChange())
		cell = GetEmptyChangeInventory(item);
	else if (item->IsCostume())
		cell = GetEmptyCostumeInventory(item);
#endif
	else
		cell = GetEmptyInventory(item->GetSize());

	if (cell == -1)
		return false;

	const DWORD itemVnum = item->GetVnum();
	WORD wCount = item->GetCount();

	if (item->IsStackable() && item->GetType() != ITEM_BLEND)
	{
		for (auto it = vecUpdateItems.begin(); it != vecUpdateItems.end(); ++it)
		{
			LPITEM item2 = it->first;
			if (itemVnum == item2->GetVnum())
			{
				const DWORD item2Count = item2->GetCount();
				const WORD bCount2 = MIN(g_bItemCountLimit - item2Count, wCount);
				if (bCount2 > 0)
				{
					wCount -= bCount2;
					item2->SetCount(item2Count + bCount2);
					if (wCount == 0)
					{
						itemCount = 0;
						return true;
					}
				}
			}
		}
#ifdef ENABLE_SPECIAL_STORAGE
		if (item->IsUpgradeItem())
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item2 = GetUpgradeInventoryItem(i);
				if (!item2)
					continue;
				if (item2->GetVnum() == itemVnum)
				{
					BYTE j;
					for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						if (item2->GetSocket(j) != item->GetSocket(j))
							break;
					if (j != ITEM_SOCKET_MAX_NUM)
						continue;
					const DWORD item2Count = item2->GetCount();
					const WORD bCount2 = MIN(g_bItemCountLimit - item2Count, wCount);
					if (bCount2 > 0)
					{
						const auto itItem = vecUpdateItems.find(item2);
						if(itItem == vecUpdateItems.end())
							vecUpdateItems.emplace(item2, item2Count);
						wCount -= bCount2;
						item2->SetUpdateStatus(true);
						item2->SetCount(item2Count + bCount2);
						if (wCount == 0)
						{
							itemCount = 0;
							return true;
						}
					}
				}
			}
		}
		else if (item->IsBook())
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item2 = GetBookInventoryItem(i);
				if (!item2)
					continue;
				if (item2->GetVnum() == itemVnum)
				{
					BYTE j;
					for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						if (item2->GetSocket(j) != item->GetSocket(j))
							break;
					if (j != ITEM_SOCKET_MAX_NUM)
						continue;
					const DWORD item2Count = item2->GetCount();
					const WORD bCount2 = MIN(g_bItemCountLimit - item2Count, wCount);
					if (bCount2 > 0)
					{
						const auto itItem = vecUpdateItems.find(item2);
						if(itItem == vecUpdateItems.end())
							vecUpdateItems.emplace(item2, item2Count);
						wCount -= bCount2;
						item2->SetUpdateStatus(true);
						item2->SetCount(item2Count + bCount2);
						if (wCount == 0)
						{
							itemCount = 0;
							return true;
						}
					}
				}
			}
		}
		else if (item->IsStone())
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item2 = GetStoneInventoryItem(i);
				if (!item2)
					continue;
				if (item2->GetVnum() == itemVnum)
				{
					BYTE j;
					for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						if (item2->GetSocket(j) != item->GetSocket(j))
							break;
					if (j != ITEM_SOCKET_MAX_NUM)
						continue;
					const DWORD item2Count = item2->GetCount();
					const WORD bCount2 = MIN(g_bItemCountLimit - item2Count, wCount);
					if (bCount2 > 0)
					{
						const auto itItem = vecUpdateItems.find(item2);
						if(itItem == vecUpdateItems.end())
							vecUpdateItems.emplace(item2, item2Count);
						wCount -= bCount2;
						item2->SetUpdateStatus(true);
						item2->SetCount(item2Count + bCount2);
						if (wCount == 0)
						{
							itemCount = 0;
							return true;
						}
					}
				}
			}
		}
		else if (item->IsChange())
		{
			for (int i = 0; i < SPECIAL_INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item2 = GetChangeInventoryItem(i);
				if (!item2)
					continue;
				if (item2->GetVnum() == itemVnum)
				{
					BYTE j;
					for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						if (item2->GetSocket(j) != item->GetSocket(j))
							break;
					if (j != ITEM_SOCKET_MAX_NUM)
						continue;
					const DWORD item2Count = item2->GetCount();
					const WORD bCount2 = MIN(g_bItemCountLimit - item2Count, wCount);
					if (bCount2 > 0)
					{
						const auto itItem = vecUpdateItems.find(item2);
						if(itItem == vecUpdateItems.end())
							vecUpdateItems.emplace(item2, item2Count);
						wCount -= bCount2;
						item2->SetUpdateStatus(true);
						item2->SetCount(item2Count + bCount2);
						if (wCount == 0)
						{
							itemCount = 0;
							return true;
						}
					}
				}
			}
		}
#endif
		else
		{
			for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
			{
				LPITEM item2 = GetInventoryItem(i);
				if (!item2)
					continue;
				if (item2->GetVnum() == itemVnum)
				{
					BYTE j;
					for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
						if (item2->GetSocket(j) != item->GetSocket(j))
							break;
					if (j != ITEM_SOCKET_MAX_NUM)
						continue;
					const DWORD item2Count = item2->GetCount();
					const WORD bCount2 = MIN(g_bItemCountLimit - item2Count, wCount);
					if (bCount2 > 0)
					{
						const auto itItem = vecUpdateItems.find(item2);
						if(itItem == vecUpdateItems.end())
							vecUpdateItems.emplace(item2, item2Count);
						wCount -= bCount2;
						item2->SetUpdateStatus(true);
						item2->SetCount(item2Count + bCount2);
						if (wCount == 0)
						{
							itemCount = 0;
							return true;
						}
					}
				}
			}
		}
	}
	if (wCount > 0)
	{
		item->SetCount(wCount);
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
#ifdef ENABLE_SPECIAL_STORAGE
		else if (item->IsUpgradeItem())
			item->AddToCharacter(this, TItemPos(UPGRADE_INVENTORY, cell));
		else if (item->IsBook())
			item->AddToCharacter(this, TItemPos(BOOK_INVENTORY, cell));
		else if (item->IsStone())
			item->AddToCharacter(this, TItemPos(STONE_INVENTORY, cell));
		else if (item->IsChange())
			item->AddToCharacter(this, TItemPos(CHANGE_INVENTORY, cell));
		else if (item->IsCostume())
			item->AddToCharacter(this, TItemPos(COSTUME_INVENTORY, cell));
#endif
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));

		if(item->IsStackable())
		{
			const auto itItem = vecUpdateItems.find(item);
			if(itItem == vecUpdateItems.end())
				vecUpdateItems.emplace(item, wCount);
		}
		itemCount = wCount;
		return true;
	}
	return false;
}

bool CHARACTER::GiveItemFromSpecialItemGroupNew(DWORD dwGroupNum, WORD& loopCount)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);
	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}
	std::map<DWORD, WORD> vecItemCounts;
	std::map<LPITEM, WORD> vecUpdateItems;
	const WORD constCount = loopCount;
	WORD workCount = loopCount;

	int returnCount = 5;

	for (DWORD j = 0; j < constCount; ++j)
	{
		if (returnCount <= 0 || workCount <= 0)
			break;
		std::vector <int> idxes;
		const int n = pGroup->GetMultiIndex(idxes);

		bool isOkey = false;

		for (int i = 0; i < n; i++)
		{
			if (returnCount <= 0 || (workCount <= 0 && !isOkey))
				break;

			const int idx = idxes[i];
			const DWORD dwVnum = pGroup->GetVnum(idx);
			const DWORD dwEarnCount = pGroup->GetCount(idx);
			DWORD dwCount = pGroup->GetCount(idx);

			switch (dwVnum)
			{
				case CSpecialItemGroup::GOLD:
					PointChange(POINT_GOLD, dwCount);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
					break;
				case CSpecialItemGroup::POISON:
					AttackedByPoison(NULL);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
					break;
				case CSpecialItemGroup::EXP:
					PointChange(POINT_EXP, dwCount);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
					break;
				case CSpecialItemGroup::MOB:
				{
					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), GetX() + number(-500, 500), GetY() + number(-500, 500), 0, true, -1);
					if (ch)
						ch->SetAggressive();
					
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
				}
				break;
				case CSpecialItemGroup::MOB_GROUP:
					CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), GetX() - number(300, 500), GetY() - number(300, 500), GetX() + number(300, 500), GetY() + number(300, 500), NULL, true);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
					break;
				case CSpecialItemGroup::SLOW:
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
					break;
				case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP() * dwCount / 100;
					iDropHP = MIN(iDropHP, GetHP() - 1);
					PointChange(POINT_HP, -iDropHP);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
				}
				break;
#ifdef ENABLE_WOLFMAN
				case CSpecialItemGroup::BLEEDING:
				{
					AttackedByBleeding(NULL);
					if(isOkey == false)
					{
						isOkey = true;
						workCount -= 1;
					}
				}
				break;
#endif
				default:
				{
					LPITEM item = ITEM_MANAGER::Instance().CreateItem(dwVnum, dwCount);
					if (item)
					{
						if (AutoGiveItemChest(item, dwCount, vecUpdateItems))
						{
							if(isOkey == false)
							{
								isOkey = true;
								workCount -= 1;
							}

							auto itCount = vecItemCounts.find(dwVnum);
							if (itCount != vecItemCounts.end())
								itCount->second += dwEarnCount;
							else
								vecItemCounts.emplace(dwVnum, dwEarnCount);

							if (dwCount == 0)
							{
								M2_DESTROY_ITEM(item);
								continue;
							}
						}
						else
						{
							M2_DESTROY_ITEM(item);
							returnCount -= 1;
						}
					}
				}
			}
		}
	}

	for (auto it = vecUpdateItems.begin(); it != vecUpdateItems.end(); ++it)
	{
		LPITEM item = it->first;
		item->SetUpdateStatus(false);
		if (item->GetCount() != it->second)
			item->UpdatePacket();

		if (item->GetType() == ITEM_QUEST || item->GetType() == ITEM_MATERIAL)
			quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
	}

	for (auto it = vecItemCounts.begin(); it != vecItemCounts.end(); ++it)
		ChatPacket(CHAT_TYPE_ITEM_INFO, "165 I%d %d", it->first, it->second);

	loopCount = workCount;
	return (workCount != constCount);
}
#endif