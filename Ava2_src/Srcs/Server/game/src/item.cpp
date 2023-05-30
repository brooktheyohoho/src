#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"
#include "packet.h"
#include "protocol.h"
#include "log.h"
#include "skill.h"
#include "unique_item.h"
#include "profiler.h"
#include "marriage.h"
#include "item_addon.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "affect.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/VnumHelper.h"
#include "../../common/CommonDefines.h"
#include "MountSystem.h"
#ifdef ENABLE_NEW_PET_SYSTEM
#include "PetSystem.h"
#endif
CItem::CItem(DWORD dwVnum)
	: m_dwVnum(dwVnum), m_bWindow(0), m_dwID(0), m_bEquipped(false), m_dwVID(0), m_wCell(0), m_dwCount(0), 
#ifdef __SOULBINDING_SYSTEM__
		m_lBind(0),
		m_pkBindingExpireEvent(NULL), 
#endif

#if defined(ENABLE_NEW_AUTOPOTION)
	m_pkTimerAutoPotionEvent(NULL), 
#endif

	m_lFlag(0), m_dwLastOwnerPID(0),
	m_bExchanging(false), m_pkDestroyEvent(NULL), m_pkExpireEvent(NULL), m_pkUniqueExpireEvent(NULL),
	m_pkTimerBasedOnWearExpireEvent(NULL), m_pkRealTimeExpireEvent(NULL),
   	m_pkAccessorySocketExpireEvent(NULL), m_pkOwnershipEvent(NULL), m_dwOwnershipPID(0), m_bSkipSave(false), m_isLocked(false),
	m_dwMaskVnum(0), m_dwSIGVnum (0)
#ifdef ENABLE_MULTI_CHEST
	, m_dwUpdateStatus (false)
#endif
{
#ifdef ELEMENT_SPELL_WORLDARD
	m_grade_element = 0;
	memset( &m_attack_element, 0, sizeof(m_attack_element) );
	m_element_type_bonus = 0;
	memset( &m_elements_value_bonus, 0, sizeof(m_elements_value_bonus) );
#endif
	memset( &m_alSockets, 0, sizeof(m_alSockets) );
	memset( &m_aAttr, 0, sizeof(m_aAttr) );
#ifdef ENABLE_GLOVE_SYSTEM
	memset( &m_randomAttr, 0, sizeof(m_randomAttr) );
#endif
#ifdef ENABLE_NEW_NAME_ITEM
	strlcpy(nameex,"^",sizeof(nameex));
#endif

}

CItem::~CItem()
{
	Destroy();
}

void CItem::Initialize()
{
	CEntity::Initialize(ENTITY_ITEM);

	m_bWindow = RESERVED_WINDOW;
	m_pOwner = NULL;
	m_dwID = 0;
	m_bEquipped = false;
	m_dwVID = m_wCell = m_dwCount = m_lFlag = 0;
	m_pProto = NULL;
	m_bExchanging = false;

#ifdef ELEMENT_SPELL_WORLDARD
	m_grade_element = 0;
	memset( &m_attack_element, 0, sizeof(m_attack_element) );
	m_element_type_bonus = 0;
	memset( &m_elements_value_bonus, 0, sizeof(m_elements_value_bonus) );
#endif


	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));
#ifdef ENABLE_NEW_NAME_ITEM
	strlcpy(nameex,"^",sizeof(nameex));
#endif
#ifdef ENABLE_GLOVE_SYSTEM
	memset(&m_randomAttr, 0, sizeof(m_randomAttr));
#endif

#ifdef ENABLE_MULTI_CHEST
	m_dwUpdateStatus = false;
#endif

#ifdef __SOULBINDING_SYSTEM__
	m_lBind = 0;
	m_pkBindingExpireEvent = NULL;
#endif

#ifdef ENABLE_MULTILANGUAGE_SYSTEM
	m_stName = "I" + std::to_string(GetVnum());
#endif

	m_pkDestroyEvent = NULL;
	m_pkOwnershipEvent = NULL;
	m_dwOwnershipPID = 0;
	m_pkUniqueExpireEvent = NULL;

#if defined(ENABLE_NEW_AUTOPOTION)
	m_pkTimerAutoPotionEvent = NULL;
#endif

	m_pkTimerBasedOnWearExpireEvent = NULL;
	m_pkRealTimeExpireEvent = NULL;

	m_pkAccessorySocketExpireEvent = NULL;

	m_bSkipSave = false;
	m_dwLastOwnerPID = 0;
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	destroyTime = -1;
#endif
}

void CItem::Destroy()
{
	event_cancel(&m_pkDestroyEvent);
	event_cancel(&m_pkOwnershipEvent);
	event_cancel(&m_pkUniqueExpireEvent);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);
	event_cancel(&m_pkRealTimeExpireEvent);

#if defined(ENABLE_NEW_AUTOPOTION)
	event_cancel(&m_pkTimerAutoPotionEvent);
#endif

#ifdef __SOULBINDING_SYSTEM__
	event_cancel(&m_pkBindingExpireEvent);
#endif
	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);
}

EVENTFUNC(item_destroy_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "item_destroy_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetOwner())
		sys_err("item_destroy_event: Owner exist. (item %s owner %s)", pkItem->GetName(), pkItem->GetOwner()->GetName());

	pkItem->SetDestroyEvent(NULL);
	M2_DESTROY_ITEM(pkItem);
	return 0;
}

void CItem::SetDestroyEvent(LPEVENT pkEvent)
{
	m_pkDestroyEvent = pkEvent;
}

void CItem::StartDestroyEvent(int iSec)
{
	if (m_pkDestroyEvent)
		return;
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	destroyTime = time(0)+iSec;
	AddToGround(dropMapIndex, dropPos, dropskipOwnerCheck);
#endif
	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetDestroyEvent(event_create(item_destroy_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::EncodeInsertPacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	const PIXEL_POSITION & c_pos = GetXYZ();

	struct packet_item_ground_add pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_ADD;
	pack.x		= c_pos.x;
	pack.y		= c_pos.y;
	pack.z		= c_pos.z;
	pack.dwVnum		= GetVnum();
	pack.dwVID		= m_dwVID;
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	pack.itemCount = GetCount();
	pack.ownerTime = destroyTime-time(0);
	
	for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = GetSocket(i);

	thecore_memcpy(pack.aAttrs, GetAttributes(), sizeof(pack.aAttrs));
#endif
	//pack.count	= m_dwCount;


	d->Packet(&pack, sizeof(pack));

	if (m_pkOwnershipEvent != NULL)
	{
		item_event_info * info = dynamic_cast<item_event_info *>(m_pkOwnershipEvent->info);

		if ( info == NULL )
		{
			sys_err( "CItem::EncodeInsertPacket> <Factor> Null pointer" );
			return;
		}

		TPacketGCItemOwnership p;

		p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
		p.dwVID = m_dwVID;
		strlcpy(p.szName, info->szOwnerName, sizeof(p.szName));

		d->Packet(&p, sizeof(TPacketGCItemOwnership));
	}
}

void CItem::EncodeRemovePacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	struct packet_item_ground_del pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_DEL;
	pack.dwVID		= m_dwVID;

	d->Packet(&pack, sizeof(pack));
	sys_log(2, "Item::EncodeRemovePacket %s to %s", GetName(), ((LPCHARACTER) ent)->GetName());
}

void CItem::SetProto(const TItemTable * table)
{
	assert(table != NULL);
	m_pProto = table;
	SetFlag(m_pProto->dwFlags);
}

void CItem::UsePacketEncode(LPCHARACTER ch, LPCHARACTER victim, struct packet_item_use *packet)
{
	if (!GetVnum())
		return;

	packet->header 	= HEADER_GC_ITEM_USE;
	packet->ch_vid 	= ch->GetVID();
	packet->victim_vid 	= victim->GetVID();
	packet->Cell = TItemPos(GetWindow(), m_wCell);
	packet->vnum	= GetVnum();
}

void CItem::RemoveFlag(long bit)
{
	REMOVE_BIT(m_lFlag, bit);
}

void CItem::AddFlag(long bit)
{
	SET_BIT(m_lFlag, bit);
}

void CItem::UpdatePacket()
{
#ifdef ENABLE_MULTI_CHEST
	if(GetUpdateStatus())
		return;
#endif
	if (!m_pOwner || !m_pOwner->GetDesc())
		return;
#ifdef ENABLE_SWITCHBOT
	if (m_bWindow == SWITCHBOT)
		return;
#endif

	TPacketGCItemUpdate pack;

	pack.header = HEADER_GC_ITEM_UPDATE;
	pack.Cell = TItemPos(GetWindow(), m_wCell);
	pack.count	= m_dwCount;
#ifdef ENABLE_NEW_NAME_ITEM
	strlcpy(pack.name,nameex,sizeof(pack.name));
#endif
#ifdef ELEMENT_SPELL_WORLDARD
	pack.grade_element = m_grade_element;
	for (int i = 0; i < MAX_ELEMENTS_SPELL; ++i)
	{
		pack.attack_element[i] = m_attack_element[i];
		pack.elements_value_bonus[i] = m_elements_value_bonus[i];
	}
	pack.element_type_bonus = m_element_type_bonus;
#endif
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = m_alSockets[i];

	thecore_memcpy(pack.aAttr, GetAttributes(), sizeof(pack.aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
	thecore_memcpy(pack.randomAttr, GetRandomAttributes(), sizeof(pack.randomAttr));
#endif

#ifdef __SOULBINDING_SYSTEM__
	pack.bind = m_lBind;
#endif

	sys_log(2, "UpdatePacket %s -> %s", GetName(), m_pOwner->GetName());
	m_pOwner->GetDesc()->Packet(&pack, sizeof(pack));
}
#ifdef __SOULBINDING_SYSTEM__
void CItem::Bind(long lTime)
{
	if (lTime >= 0)
	{
		m_lBind = lTime;
		UpdatePacket();
		Save();
	}
}

bool CItem::IsBind()
{
	if (GetBind() == 1)
		return true;
	
	return false;
}

bool CItem::IsUntilBind()
{
	if (GetBind() >= 2)
		return true;
	
	return false;
}

EVENTFUNC(unbinding_expire_event)
{
	const item_vid_event_info * pInfo = reinterpret_cast<item_vid_event_info*>(event->info);
	if (!pInfo)
		return 0;
	
	const LPITEM pItem = ITEM_MANAGER::instance().FindByVID(pInfo->item_vid);
	if (!pItem)
		return 0;
	
	int iNextExpire = pItem->GetBind() - processing_time / passes_per_sec;
	if (iNextExpire <= 2)
	{
		pItem->Bind(0);
		pItem->SetUnBindingExpireEvent(NULL);
		return 0;
	}
	
	pItem->Bind(iNextExpire);
	return PASSES_PER_SEC(MIN(60, iNextExpire));
}

void CItem::SetUnBindingExpireEvent(LPEVENT pkEvent)
{
	m_pkBindingExpireEvent = pkEvent;
}

void CItem::StartUnBindingExpireEvent()
{
	if (m_pkBindingExpireEvent)
		return;
	
	item_vid_event_info * pInfo = AllocEventInfo<item_vid_event_info>();
	pInfo->item_vid = GetVID();
	SetUnBindingExpireEvent(event_create(unbinding_expire_event, pInfo, PASSES_PER_SEC(60)));
}
#endif


DWORD CItem::GetCount()
{
	if (GetType() == ITEM_ELK) return MIN(m_dwCount, INT_MAX);
	else
	{
		return MIN(m_dwCount, g_bItemCountLimit);
	}
}

bool CItem::SetCount(DWORD count)
{
	if (GetType() == ITEM_ELK)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
	else
	{
		m_dwCount = MIN(count, g_bItemCountLimit);
	}

	if (count == 0 && m_pOwner)
	{
		if (GetSubType() == USE_ABILITY_UP || GetSubType() == USE_POTION || GetVnum() == 70020)
		{
			LPCHARACTER pOwner = GetOwner();
			WORD wCell = GetCell();

			RemoveFromCharacter();

			if (!IsDragonSoul())
			{
				LPITEM pItem = pOwner->FindSpecifyItem(GetVnum());

				if (NULL != pItem)
				{
					pOwner->ChainQuickslotItem(pItem, QUICKSLOT_TYPE_ITEM, wCell);
				}
				else
				{
					pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wCell, 255);
				}
			}

			M2_DESTROY_ITEM(this);
		}
		else
		{
			if (!IsDragonSoul())
			{
				m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, m_wCell, 255);
			}
			M2_DESTROY_ITEM(RemoveFromCharacter());
		}

		return false;
	}

	UpdatePacket();

	Save();
	return true;
}

LPITEM CItem::RemoveFromCharacter()
{
	if (!m_pOwner)
	{
		sys_err("Item::RemoveFromCharacter owner null");
		return (this);
	}

	LPCHARACTER pOwner = m_pOwner;

	if (m_bEquipped)	// 장착되었는가?
	{
		Unequip();
		//pOwner->UpdatePacket();

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
	else
	{
		if (GetWindow() != SAFEBOX && GetWindow() != MALL)
		{
			if (IsDragonSoul())
			{
				if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= DRAGON_SOUL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
#ifdef ENABLE_SWITCHBOT
			else if (m_bWindow == SWITCHBOT)
			{
				if (m_wCell >= SWITCHBOT_SLOT_COUNT)
				{
					sys_err("CItem::RemoveFromCharacter: pos >= SWITCHBOT_SLOT_COUNT");
				}
				else
				{
					pOwner->SetItem(TItemPos(SWITCHBOT, m_wCell), NULL);
				}
			}
#endif

#ifdef CHANGE_EQUIP_WORLDARD
			else if (m_bWindow == CHANGE_EQUIP)
			{
				if (m_wCell >= CHANGE_EQUIP_SLOT_COUNT)
				{
					sys_err("CItem::RemoveFromCharacter: pos >= CHANGE_EQUIP_SLOT_COUNT");
				}
				else
				{
					pOwner->SetItem(TItemPos(CHANGE_EQUIP, m_wCell), NULL);
				}
			}
#endif

#ifdef ENABLE_SPECIAL_STORAGE
			else if (IsUpgradeItem() || IsBook() || IsStone() || IsChange() || (IsCostume() || IsCostumeSkin()))
			{
				if (m_wCell >= SPECIAL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= SPECIAL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
#endif

			else
			{
				TItemPos cell(INVENTORY, m_wCell);

				if (false == cell.IsDefaultInventoryPosition() && false == cell.IsBeltInventoryPosition()) // 아니면 소지품에?
					sys_err("CItem::RemoveFromCharacter: Invalid Item Position");
				else
				{
					pOwner->SetItem(cell, NULL);
				}
			}
		}

		m_pOwner = NULL;
		m_wCell = 0;

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
}

bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell)
{
	assert(GetSectree() == NULL);
	assert(m_pOwner == NULL);
	WORD pos = Cell.cell;
	BYTE window_type = Cell.window_type;

	if (INVENTORY == window_type)
	{
		if (m_wCell >= INVENTORY_MAX_NUM && BELT_INVENTORY_SLOT_START > m_wCell)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
	else if (DRAGON_SOUL_INVENTORY == window_type)
	{
		if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#ifdef ENABLE_SWITCHBOT
	else if (SWITCHBOT == window_type)
	{
		if (m_wCell >= SWITCHBOT_SLOT_COUNT)
		{
			sys_err("CItem::AddToCharacter:switchbot cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	else if (CHANGE_EQUIP == window_type)
	{
		if (m_wCell >= CHANGE_EQUIP_SLOT_COUNT)
		{
			sys_err("CItem::AddToCharacter:switchbot cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

#ifdef ENABLE_SPECIAL_STORAGE
	else if (UPGRADE_INVENTORY == window_type || BOOK_INVENTORY == window_type || STONE_INVENTORY == window_type || CHANGE_INVENTORY == window_type || COSTUME_INVENTORY == window_type)
	{
		if (m_wCell >= SPECIAL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

#ifdef ENABLE_NEWITEM_AFFECT
	bool isNewItem = this->GetLastOwnerPID() != ch->GetPlayerID();
#endif


	if (ch->GetDesc())
		m_dwLastOwnerPID = ch->GetPlayerID();
#ifdef __SASH_SYSTEM__
	if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_SASH) && (GetSocket(SASH_ABSORPTION_SOCKET) == 0))
	{
		long lVal = GetValue(SASH_GRADE_VALUE_FIELD);
		switch (lVal)
		{
			case 2:
				{
					lVal = SASH_GRADE_2_ABS;
				}
				break;
			case 3:
				{
					lVal = SASH_GRADE_3_ABS;
				}
				break;
			case 4:
				{
					lVal = number(SASH_GRADE_4_ABS_MIN, SASH_GRADE_4_ABS_MAX_COMB);
				}
				break;
			default:
				{
					lVal = SASH_GRADE_1_ABS;
				}
				break;
		}
		
		SetSocket(SASH_ABSORPTION_SOCKET, lVal);
	}
	#endif
	event_cancel(&m_pkDestroyEvent);

#ifdef ENABLE_NEWITEM_AFFECT
	ch->SetItem(TItemPos(window_type, pos), this, isNewItem);
#else
	ch->SetItem(TItemPos(window_type, pos), this);
#endif

	m_pOwner = ch;

	Save();
	return true;
}

LPITEM CItem::RemoveFromGround()
{
	if (GetSectree())
	{
		SetOwnership(NULL);

		GetSectree()->RemoveEntity(this);

		ViewCleanup();

		Save();
	}

	return (this);
}

bool CItem::AddToGround(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck)
{
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	if(destroyTime == -1)
	{
		thecore_memcpy(&dropPos, &pos, sizeof(dropPos));
		dropMapIndex = lMapIndex;
		dropskipOwnerCheck = skipOwnerCheck;
		return true;
	}
#endif

	if (0 == lMapIndex)
	{
		sys_err("wrong map index argument: %d", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		sys_err("sectree already assigned vnum: %d mapIndex: %ld",GetVnum(), lMapIndex);

		UpdateSectree();

		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		sys_err("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d", pos.x, pos.y);
		return false;
	}

	//tree->Touch();

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}

bool CItem::DistanceValid(LPCHARACTER ch)
{
	if (!GetSectree())
		return false;
	const int iDist = DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY());
#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
	const WORD distance = ch->FindAffect(AFFECT_AUTO_PICK_UP) != NULL ? 600 :  300;
	if (iDist > distance)
#else
	if (iDist > 300)
#endif
		return false;
	return true;
}

bool CItem::CanUsedBy(LPCHARACTER ch)
{
	// Anti flag check
	switch (ch->GetJob())
	{
		case JOB_WARRIOR:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break;
#endif
	}

	return true;
}

int CItem::FindEquipCell(LPCHARACTER ch, int iCandidateCell)
{
	// 코스츔 아이템(ITEM_COSTUME)은 WearFlag 없어도 됨. (sub type으로 착용위치 구분. 귀찮게 또 wear flag 줄 필요가 있나..)
	// 용혼석(ITEM_DS, ITEM_SPECIAL_DS)도  SUB_TYPE으로 구분. 신규 반지, 벨트는 ITEM_TYPE으로 구분 -_-

	if ((0 == GetWearFlag() || ITEM_TOTEM == GetType()) && ITEM_COSTUME != GetType() && ITEM_DS != GetType() && ITEM_SPECIAL_DS != GetType() && ITEM_RING != GetType() && ITEM_BELT != GetType() && ITEM_NEW_RINGS != GetType()
#ifdef __ITEM_SHINING__
		&& ITEM_SHINING != GetType()
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		&& ITEM_PET != GetType()
#endif
	)
		return -1;

	// 용혼석 슬롯을 WEAR로 처리할 수가 없어서(WEAR는 최대 32개까지 가능한데 용혼석을 추가하면 32가 넘는다.)
	// 인벤토리의 특정 위치((INVENTORY_MAX_NUM + WEAR_MAX_NUM)부터 (INVENTORY_MAX_NUM + WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX - 1)까지)를
	// 용혼석 슬롯으로 정함.
	// return 할 때에, INVENTORY_MAX_NUM을 뺀 이유는,
	// 본래 WearCell이 INVENTORY_MAX_NUM를 빼고 return 하기 때문.
	if (GetType() == ITEM_DS || GetType() == ITEM_SPECIAL_DS)
	{
		if (iCandidateCell < 0)
		{
			return WEAR_MAX_NUM + GetSubType();
		}
		else
		{
			for (int i = 0; i < DRAGON_SOUL_DECK_MAX_NUM; i++)
			{
				if (WEAR_MAX_NUM + i * DS_SLOT_MAX + GetSubType() == iCandidateCell)
				{
					return iCandidateCell;
				}
			}
			return -1;
		}
	}

#ifdef __ITEM_SHINING__
	else if (GetType() == ITEM_SHINING)
	{
		if (GetSubType() == SHINING_WEAPON) 
		{
			// for (int i = WEAR_SHINING_WEAPON_1; i <= WEAR_SHINING_WEAPON_3; i++)
			// {
				// if (!ch->GetWear(i))
					// return i;
			// }
			return WEAR_SHINING_WEAPON_1;
		}
		else if (GetSubType() == SHINING_ARMOR) 
		{
			// for (int i = WEAR_SHINING_ARMOR_1; i <= WEAR_SHINING_ARMOR_3; i++)
			// {
				// if (!ch->GetWear(i))
					// return i;
			// }
			return WEAR_SHINING_ARMOR_1;
		}
		else if (GetSubType() == SHINING_SPECIAL) 
		{
			return WEAR_SHINING_SPECIAL;
		}
	}
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
	else if (IsNewPetItem())
		return WEAR_PET;
#endif
	else if (GetType() == ITEM_NEW_RINGS)
	{
		if (ch->GetWear(WEAR_RINGS_1))
			return WEAR_RINGS_2;
		else
			return WEAR_RINGS_1;
	}

	else if (GetType() == ITEM_COSTUME)
	{
		if (GetSubType() == COSTUME_BODY)
			return WEAR_COSTUME_BODY;
		else if (GetSubType() == COSTUME_HAIR)
			return WEAR_COSTUME_HAIR;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_MOUNT)
			return WEAR_COSTUME_MOUNT;
#endif
#ifdef __SASH_SYSTEM__
		else if (GetSubType() == COSTUME_SASH)
			return WEAR_COSTUME_SASH;
#endif
#ifdef ENABLE_SASH_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_NEW_SASH)
			return WEAR_COSTUME_NEW_SASH;

#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_WEAPON)
			return WEAR_COSTUME_WEAPON;
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
		else if (GetSubType() == COSTUME_PET)
			return WEAR_OLD_PET;
#endif
#ifdef ENABLE_MOUNT_SKIN
		else if (GetSubType() == COSTUME_MOUNT_SKIN)
			return WEAR_COSTUME_MOUNT_SKIN;
#endif
	}
#if !defined(ENABLE_MOUNT_COSTUME_SYSTEM) && !defined(__SASH_SYSTEM__)
	else if (GetType() == ITEM_RING)
	{
		if (ch->GetWear(WEAR_RING1))
			return WEAR_RING2;
		else
			return WEAR_RING1;
	}
#endif
#ifdef ENABLE_GLOVE_SYSTEM
	else if (GetType() == ITEM_ARMOR && GetSubType() == ARMOR_GLOVE)
		return WEAR_GLOVE;
#endif

#ifdef ITEM_TALISMAN_EQUIPMENT
	else if (GetType() == ITEM_ARMOR && GetSubType() == ARMOR_PENDANT)
		return WEAR_PENDANT;
#endif

	else if (GetType() == ITEM_BELT)
		return WEAR_BELT;
	else if (GetWearFlag() & WEARABLE_BODY)
		return WEAR_BODY;
	else if (GetWearFlag() & WEARABLE_HEAD)
		return WEAR_HEAD;
	else if (GetWearFlag() & WEARABLE_FOOTS)
		return WEAR_FOOTS;
	else if (GetWearFlag() & WEARABLE_WRIST)
		return WEAR_WRIST;
	else if (GetWearFlag() & WEARABLE_WEAPON)
		return WEAR_WEAPON;
	else if (GetWearFlag() & WEARABLE_SHIELD)
		return WEAR_SHIELD;
	else if (GetWearFlag() & WEARABLE_NECK)
		return WEAR_NECK;
	else if (GetWearFlag() & WEARABLE_EAR)
		return WEAR_EAR;
	else if (GetWearFlag() & WEARABLE_ARROW)
		return WEAR_ARROW;

	else if (GetWearFlag() & WEARABLE_UNIQUE)
	{
		if (ch->GetWear(WEAR_UNIQUE1))
			return WEAR_UNIQUE2;
		else
			return WEAR_UNIQUE1;
	}


	// 수집 퀘스트를 위한 아이템이 박히는곳으로 한번 박히면 절대 뺼수 없다.
	else if (GetWearFlag() & WEARABLE_ABILITY)
	{
		if (!ch->GetWear(WEAR_ABILITY1))
		{
			return WEAR_ABILITY1;
		}
		else if (!ch->GetWear(WEAR_ABILITY2))
		{
			return WEAR_ABILITY2;
		}
		else if (!ch->GetWear(WEAR_ABILITY3))
		{
			return WEAR_ABILITY3;
		}
		else if (!ch->GetWear(WEAR_ABILITY4))
		{
			return WEAR_ABILITY4;
		}
		else if (!ch->GetWear(WEAR_ABILITY5))
		{
			return WEAR_ABILITY5;
		}
		else if (!ch->GetWear(WEAR_ABILITY6))
		{
			return WEAR_ABILITY6;
		}
		else if (!ch->GetWear(WEAR_ABILITY7))
		{
			return WEAR_ABILITY7;
		}
		else if (!ch->GetWear(WEAR_ABILITY8))
		{
			return WEAR_ABILITY8;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

void CItem::ModifyPoints(bool bAdd)
{
#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetType() == ITEM_PET && GetSubType() == PET_LEVELABLE)
	{
		int bonusTypes[][2] = { {APPLY_MAX_HP,4000},{APPLY_ATTBONUS_MONSTER,20},{APPLY_CRITICAL_PCT,10} };
		for (BYTE i = 0; i < 3; ++i)
		{
			long bonusLevel = GetSocket(POINT_PET_BONUS_1+i);
			if (bonusLevel == 0)
				continue;
			int value = bonusLevel == 20 ? bonusTypes[i][1]:bonusLevel*(float(bonusTypes[i][1])/20.0);
			m_pOwner->ApplyPoint(bonusTypes[i][0], bAdd ? value : -value);
		}

		for (BYTE j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; ++j)
		{
			BYTE skillType = GetAttributeType(j);
			if (skillType == 0 || skillType == 99)
				continue;
			auto it = petSkillBonus.find(skillType);
			if (it != petSkillBonus.end())
			{
				BYTE skillLevel = GetAttributeValue(j);
				for (BYTE i = 0; i < it->second.size(); ++i)
				{
					auto bonus = it->second[i];
					int value = skillLevel == 20 ? bonus.second : skillLevel * (float(bonus.second) / 20.0);
					m_pOwner->ApplyPoint(bonus.first, bAdd ? value : -value);
				}
			}
		}
		return;
	}
#endif

#ifdef ENABLE_RARITY
	long rarityValue = GetSocket(RARITY_VALUE_INDEX);
#endif

	int accessoryGrade;

	// 무기와 갑옷만 소켓을 적용시킨다.
	if (false == IsAccessoryForSocket())
	{
		if (m_pProto->bType == ITEM_WEAPON || m_pProto->bType == ITEM_ARMOR || m_pProto->bType == ITEM_NEW_RINGS)
		{
			// 소켓이 속성강화에 사용되는 경우 적용하지 않는다 (ARMOR_WRIST ARMOR_NECK ARMOR_EAR)
#ifdef ENABLE_RARITY
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM_EX; ++i)
#else
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
#endif
			{
#ifdef CLOSE_RARITY_METIN_BONUS
				if (IsRarityItem())
					if (rarityValue <= 0)
						continue;
#endif
				DWORD dwVnum;

				if ((dwVnum = GetSocket(i)) <= 2)
					continue;

				TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
				{
					if (m_pProto->bSubType != WEAPON_QUIVER)
						sys_err("cannot find table by vnum %u", dwVnum);
					continue;
				}

				if (ITEM_METIN == p->bType)
				{
					//m_pOwner->ApplyPoint(p->alValues[0], bAdd ? p->alValues[1] : -p->alValues[1]);
					for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
					{
						if (p->aApplies[i].bType == APPLY_NONE)
							continue;

						if (p->aApplies[i].bType == APPLY_SKILL)
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : p->aApplies[i].lValue ^ 0x00800000);
						else
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : -p->aApplies[i].lValue);
					}
				}
			}
		}

		accessoryGrade = 0;
	}
	else
	{
		accessoryGrade = MIN(GetAccessorySocketGrade(), ITEM_ACCESSORY_SOCKET_MAX_NUM);
	}
	#ifdef __SASH_SYSTEM__
	if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_SASH) && (GetSocket(SASH_ABSORBED_SOCKET)))
	{
		TItemTable * pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(SASH_ABSORBED_SOCKET));
		if (pkItemAbsorbed)
		{
			if ((pkItemAbsorbed->bType == ITEM_ARMOR) && (pkItemAbsorbed->bSubType == ARMOR_BODY))
			{
				long lDefGrade = pkItemAbsorbed->alValues[1] + long(pkItemAbsorbed->alValues[5] * 2);
				double dValue = lDefGrade * GetSocket(SASH_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				lDefGrade = (long) dValue;
				if ((pkItemAbsorbed->alValues[1] > 0) && (lDefGrade <= 0) || (pkItemAbsorbed->alValues[5] > 0) && (lDefGrade < 1))
					lDefGrade += 1;
				else if ((pkItemAbsorbed->alValues[1] > 0) || (pkItemAbsorbed->alValues[5] > 0))
					lDefGrade += 1;
				
				m_pOwner->ApplyPoint(APPLY_DEF_GRADE_BONUS, bAdd ? lDefGrade : -lDefGrade);
				
				long lDefMagicBonus = pkItemAbsorbed->alValues[0];
				dValue = lDefMagicBonus * GetSocket(SASH_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				lDefMagicBonus = (long) dValue;
				if ((pkItemAbsorbed->alValues[0] > 0) && (lDefMagicBonus < 1))
					lDefMagicBonus += 1;
				else if (pkItemAbsorbed->alValues[0] > 0)
					lDefMagicBonus += 1;
				
				m_pOwner->ApplyPoint(APPLY_MAGIC_DEF_GRADE, bAdd ? lDefMagicBonus : -lDefMagicBonus);
			}

			/*
			else if (pkItemAbsorbed->bType == ITEM_WEAPON)
			{
				long lAttGrade = pkItemAbsorbed->alValues[4] + pkItemAbsorbed->alValues[5];
				if (pkItemAbsorbed->alValues[3] > pkItemAbsorbed->alValues[4])
					lAttGrade = pkItemAbsorbed->alValues[3] + pkItemAbsorbed->alValues[5];
				
				double dValue = lAttGrade * GetSocket(SASH_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				lAttGrade = (long) dValue;
				if (((pkItemAbsorbed->alValues[3] > 0) && (lAttGrade < 1)) || ((pkItemAbsorbed->alValues[4] > 0) && (lAttGrade < 1)))
					lAttGrade += 1;
				else if ((pkItemAbsorbed->alValues[3] > 0) || (pkItemAbsorbed->alValues[4] > 0))
					lAttGrade += 1;
				
				m_pOwner->ApplyPoint(APPLY_ATT_GRADE_BONUS, bAdd ? lAttGrade : -lAttGrade);
				
				long lAttMagicGrade = pkItemAbsorbed->alValues[2] + pkItemAbsorbed->alValues[5];
				if (pkItemAbsorbed->alValues[1] > pkItemAbsorbed->alValues[2])
					lAttMagicGrade = pkItemAbsorbed->alValues[1] + pkItemAbsorbed->alValues[5];
				
				dValue = lAttMagicGrade * GetSocket(SASH_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				lAttMagicGrade = (long) dValue;
				if (((pkItemAbsorbed->alValues[1] > 0) && (lAttMagicGrade < 1)) || ((pkItemAbsorbed->alValues[2] > 0) && (lAttMagicGrade < 1)))
					lAttMagicGrade += 1;
				else if ((pkItemAbsorbed->alValues[1] > 0) || (pkItemAbsorbed->alValues[2] > 0))
					lAttMagicGrade += 1;
				
				m_pOwner->ApplyPoint(APPLY_MAGIC_ATT_GRADE, bAdd ? lAttMagicGrade : -lAttMagicGrade);
			}*/
		}
	}
	#endif
	
/*
	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{

		if (m_pProto->aApplies[i].bType == APPLY_NONE)
			continue;
		
		long value = m_pProto->aApplies[i].lValue;

		if (m_pProto->aApplies[i].bType == APPLY_SKILL)
		{
			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : value ^ 0x00800000);
		}
		else
		{
			if (0 != accessoryGrade)
				value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : -value);
		}
	}*/
	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		#ifdef __SASH_SYSTEM__
		if ((m_pProto->aApplies[i].bType == APPLY_NONE) && (GetType() != ITEM_COSTUME) && (GetSubType() != COSTUME_SASH))
		#else
		if (m_pProto->aApplies[i].bType == APPLY_NONE)
		#endif
			continue;
#ifdef CLOSE_RARITY_APPLY_BONUS
		if (IsRarityItem())
			if (rarityValue <= 0)
				continue;
#endif
		BYTE bType = m_pProto->aApplies[i].bType;
		long value = m_pProto->aApplies[i].lValue;
		#ifdef __SASH_SYSTEM__
		if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_SASH))
		{
			TItemTable * pkItemAbsorbed = ITEM_MANAGER::instance().GetTable(GetSocket(SASH_ABSORBED_SOCKET));
			if (pkItemAbsorbed)
			{
				if (pkItemAbsorbed->aApplies[i].bType == APPLY_NONE)
					continue;
				
				bType = pkItemAbsorbed->aApplies[i].bType;
				value = pkItemAbsorbed->aApplies[i].lValue;
				if (value < 0)
					continue;
				
				double dValue = value * GetSocket(SASH_ABSORPTION_SOCKET);
				dValue = (double)dValue / 100;
				dValue = (double)dValue + .5;
				value = (long) dValue;
				if ((pkItemAbsorbed->aApplies[i].lValue > 0) && (value <= 0))
					value += 1;
			}
			else
				continue;
		}
		#endif
		
#ifdef ENABLE_RARITY
		if (IsRarityItem() && GetSocket(RARITY_VALUE_INDEX) > 0)
		{
			int rarityValue = GetRarityPoint(bType, GetSocket(RARITY_TYPE_INDEX))*(float(value)/100.0);
			value += rarityValue;
		}
#endif
		
		if (bType != APPLY_SKILL)
		{
			if (accessoryGrade != 0)
				value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);
			
			m_pOwner->ApplyPoint(bType, bAdd ? value : -value);
		}
		else
			m_pOwner->ApplyPoint(bType, bAdd ? value : value ^ 0x00800000);
	}

	// 초승달의 반지, 할로윈 사탕, 행복의 반지, 영원한 사랑의 펜던트의 경우
	// 기존의 하드 코딩으로 강제로 속성을 부여했지만,
	// 그 부분을 제거하고 special item group 테이블에서 속성을 부여하도록 변경하였다.
	// 하지만 하드 코딩되어있을 때 생성된 아이템이 남아있을 수도 있어서 특수처리 해놓는다.
	// 이 아이템들의 경우, 밑에 ITEM_UNIQUE일 때의 처리로 속성이 부여되기 때문에,
	// 아이템에 박혀있는 attribute는 적용하지 않고 넘어간다.
	if (true == CItemVnumHelper::IsRamadanMoonRing(GetVnum()) || true == CItemVnumHelper::IsHalloweenCandy(GetVnum())
		|| true == CItemVnumHelper::IsHappinessRing(GetVnum()) || true == CItemVnumHelper::IsLovePendant(GetVnum()))
	{
		// Do not anything.
	}
	/*
	else
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (GetAttributeType(i))
			{
				const TPlayerItemAttribute& ia = GetAttribute(i);
				long sValue = ia.sValue;

				
				if (ia.bType == APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? sValue : sValue ^ 0x00800000);
				else
					m_pOwner->ApplyPoint(ia.bType, bAdd ? sValue : -sValue);
			}
		}


#ifdef ELEMENT_SPELL_WORLDARD
		if(GetElementsType() != 0){
			m_pOwner->ApplyPoint(GetElementsType(), bAdd ? GetElementsValue(GetElementGrade()-1) : -GetElementsValue(GetElementGrade()-1));			
			m_pOwner->PointChange(POINT_ATT_BONUS, bAdd ? GetElementAttack(GetElementGrade()-1) : -GetElementAttack(GetElementGrade()-1));
			m_pOwner->PointChange(POINT_ATT_ELEMENTS, bAdd ? GetElementAttack(GetElementGrade()-1) : -GetElementAttack(GetElementGrade()-1));
		}

#endif	
		
	}*/


	else
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
#ifdef CLOSE_RARITY_ATTR_BONUS
			if (IsRarityItem())
				if (rarityValue <= 0)
					continue;
#endif
			if (GetAttributeType(i))
			{
				const TPlayerItemAttribute& ia = GetAttribute(i);
				long sValue = ia.sValue;
				#ifdef __SASH_SYSTEM__
				if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_SASH))
				{
					double dValue = sValue * GetSocket(SASH_ABSORPTION_SOCKET);
					dValue = (double)dValue / 100;
					dValue = (double)dValue + .5;
					sValue = (long) dValue;
					if ((ia.sValue > 0) && (sValue <= 0))
						sValue += 1;
				}
				#endif
				
				if (ia.bType == APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? sValue : sValue ^ 0x00800000);
				else
					m_pOwner->ApplyPoint(ia.bType, bAdd ? sValue : -sValue);
			}
		}

#ifdef ELEMENT_SPELL_WORLDARD
		if(GetElementsType() != 0){
			long value_element = GetElementsValue(GetElementGrade()-1);
			long attack_element = GetElementAttack(GetElementGrade()-1);

			if ((GetType() == ITEM_COSTUME) && (GetSubType() == COSTUME_SASH))
			{
				double dValueElement = value_element * GetSocket(SASH_ABSORPTION_SOCKET);
				dValueElement = (double)dValueElement / 100;
				dValueElement = (double)dValueElement + .5;
				value_element = (long) dValueElement;
				if ((GetElementsValue(GetElementGrade()-1) > 0) && (value_element <= 0))
					value_element += 1;

				double dAttackElement = attack_element * GetSocket(SASH_ABSORPTION_SOCKET);
				dAttackElement = (double)dAttackElement / 100;
				dAttackElement = (double)dAttackElement + .5;
				attack_element = (long) dAttackElement;
				if ((GetElementAttack(GetElementGrade()-1) > 0) && (attack_element <= 0))
					attack_element += 1;
			}

			m_pOwner->ApplyPoint(GetElementsType(), bAdd ? value_element : -value_element);			
			m_pOwner->PointChange(POINT_ATT_BONUS, bAdd ? attack_element : -attack_element);
			m_pOwner->PointChange(POINT_ATT_ELEMENTS, bAdd ? attack_element : -attack_element);
		}

#endif
#ifdef ENABLE_GLOVE_SYSTEM
		for (int i = 0; i < GLOVE_ATTR_MAX_NUM; ++i)
		{
			if (m_pProto->aApplies[i].bType == APPLY_RANDOM && GetRandomAttributeType(i))
			{
				const TPlayerItemAttribute& ia = GetRandomAttribute(i);

				if (ia.bType == APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : ia.sValue ^ 0x00800000);
				else
				{
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : -ia.sValue);
				}
			}
		}
#endif

	}


	switch (m_pProto->bType)
	{
		case ITEM_PICK:
		case ITEM_ROD:
			{
				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, 0);
				}
			}
			break;

		case ITEM_WEAPON:
			{

				if (m_pProto->bSubType == WEAPON_ARROW || m_pProto->bSubType == WEAPON_QUIVER)
				{
					if (bAdd)
					{
						if (m_wCell == INVENTORY_MAX_NUM + WEAR_ARROW)
						{
							m_pOwner->SetPart(PART_ARROW_TYPE, m_pProto->bSubType);
							const CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
							if (pWeapon != NULL && pWeapon->GetSubType() == WEAPON_BOW)
							{
								const CItem* pCostumeWeapon = m_pOwner->GetWear(WEAR_COSTUME_WEAPON);
								if (pCostumeWeapon != NULL)
								{
									m_pOwner->SetPart(PART_WEAPON, pCostumeWeapon->GetVnum());
								}
								else
								{
									m_pOwner->SetPart(PART_WEAPON, pWeapon->GetVnum());
								}
							}
						}
					}
					else
					{
						if (m_wCell == INVENTORY_MAX_NUM + WEAR_ARROW)
							m_pOwner->SetPart(PART_ARROW_TYPE, m_pOwner->GetOriginalPart(PART_ARROW_TYPE));
					}
					
					break;
				}
				const CItem* pArrow = m_pOwner->GetWear(WEAR_ARROW);

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_WEAPON))
					break;
#endif

				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						if (pArrow != NULL)
							m_pOwner->SetPart(PART_ARROW_TYPE, pArrow->GetSubType());

					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, 0);
				}
			}
			break;

		case ITEM_ARMOR:
			{
				// 코스츔 body를 입고있다면 armor는 벗던 입던 상관 없이 비주얼에 영향을 주면 안 됨.
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_BODY))
					break;

#ifdef ENABLE_GLOVE_SYSTEM
#ifdef ITEM_TALISMAN_EQUIPMENT
				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD || GetSubType() == ARMOR_PENDANT || GetSubType() == ARMOR_GLOVE)
#else
				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD || GetSubType() == ARMOR_GLOVE)
#endif
#else
#ifdef ITEM_TALISMAN_EQUIPMENT
				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD || GetSubType() == ARMOR_PENDANT)
#else
				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD)
#endif
#endif
				{
					if (bAdd)
					{
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, GetVnum());
					}
					else
					{
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, m_pOwner->GetOriginalPart(PART_MAIN));
					}
				}
			}
			break;

		// 코스츔 아이템 입었을 때 캐릭터 parts 정보 세팅. 기존 스타일대로 추가함..
		case ITEM_COSTUME:
			{
				DWORD toSetValue = this->GetVnum();
				EParts toSetPart = PART_MAX_NUM;

				// 갑옷 코스츔
				if (GetSubType() == COSTUME_BODY)
				{
					toSetPart = PART_MAIN;

					if (false == bAdd)
					{
						// 코스츔 갑옷을 벗었을 때 원래 갑옷을 입고 있었다면 그 갑옷으로 look 세팅, 입지 않았다면 default look
						const CItem* pArmor = m_pOwner->GetWear(WEAR_BODY);
						toSetValue = (NULL != pArmor) ? pArmor->GetVnum() : m_pOwner->GetOriginalPart(PART_MAIN);
					}

				}

				// 헤어 코스츔
				else if (GetSubType() == COSTUME_HAIR)
				{
					toSetPart = PART_HAIR;

					// 코스츔 헤어는 shape값을 item proto의 value3에 세팅하도록 함. 특별한 이유는 없고 기존 갑옷(ARMOR_BODY)의 shape값이 프로토의 value3에 있어서 헤어도 같이 value3으로 함.
					// [NOTE] 갑옷은 아이템 vnum을 보내고 헤어는 shape(value3)값을 보내는 이유는.. 기존 시스템이 그렇게 되어있음...
					toSetValue = (true == bAdd) ? this->GetValue(3) : 0;
				}
/*
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_MOUNT)
				{

				}
#endif*/

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_WEAPON)
				{
					toSetPart = PART_WEAPON;
					if (false == bAdd)
					{
						const CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
						toSetValue = (NULL != pWeapon) ? pWeapon->GetVnum() : m_pOwner->GetOriginalPart(PART_WEAPON);
					}
				}
#endif
#ifdef __SASH_SYSTEM__
				else if (GetSubType() == COSTUME_SASH)
				{

					if (0 != m_pOwner->GetWear(WEAR_COSTUME_NEW_SASH))
						break;

					toSetValue -= 85000;

#ifndef FIX_19_PERCENT
					if (GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
						toSetValue += 1000;
#endif


					toSetValue = (bAdd == true) ? toSetValue : 0;
					toSetPart = PART_SASH;

				}
#endif

				else if(GetSubType() == COSTUME_NEW_SASH)
				{

					LPITEM itemSash = m_pOwner->GetWear(WEAR_COSTUME_SASH);
					if (false == bAdd)
					{
                        if(NULL != itemSash)
                        {
                        	toSetValue = itemSash->GetVnum() - 85000;
							if (itemSash->GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
								toSetValue += 1000;
                        }else{
                        	toSetValue = 0;
                        }
					}else{
						toSetValue -= 85000;
						//if (GetSocket(SASH_ABSORPTION_SOCKET) >= SASH_EFFECT_FROM_ABS)
						//	toSetValue += 1000;
					}

					toSetPart = PART_SASH;
				}

				if (PART_MAX_NUM != toSetPart)
				{
					m_pOwner->SetPart((BYTE)toSetPart, toSetValue);
					m_pOwner->UpdatePacket();
				}
			}
			break;
		case ITEM_UNIQUE:
			{
				if (0 != GetSIGVnum())
				{
					const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(GetSIGVnum());
					if (NULL == pItemGroup)
						break;
					DWORD dwAttrVnum = pItemGroup->GetAttrVnum(GetVnum());
					const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(dwAttrVnum);
					if (NULL == pAttrGroup)
						break;
					for (itertype (pAttrGroup->m_vecAttrs) it = pAttrGroup->m_vecAttrs.begin(); it != pAttrGroup->m_vecAttrs.end(); it++)
					{
						m_pOwner->ApplyPoint(it->apply_type, bAdd ? it->apply_value : -it->apply_value);
					}
				}
			}
			break;
	}
}

bool CItem::IsEquipable() const
{
	switch (this->GetType())
	{
	case ITEM_COSTUME:
	case ITEM_ARMOR:
	case ITEM_WEAPON:
	case ITEM_ROD:
	case ITEM_PICK:
	case ITEM_UNIQUE:
	case ITEM_DS:
	case ITEM_SPECIAL_DS:
	case ITEM_RING:
	case ITEM_BELT:
#ifdef __ITEM_SHINING__
	case ITEM_SHINING:
#endif
	case ITEM_NEW_RINGS:
#ifdef ENABLE_NEW_PET_SYSTEM
	case ITEM_PET:
#endif
		return true;
	}

	return false;
}

#define ENABLE_IMMUNE_FIX
// return false on error state
bool CItem::EquipTo(LPCHARACTER ch, BYTE bWearCell)
{
	if (!ch)
	{
		sys_err("EquipTo: nil character");
		return false;
	}

	// 용혼석 슬롯 index는 WEAR_MAX_NUM 보다 큼.
	if (IsDragonSoul())
	{
		if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
		{
			sys_err("EquipTo: invalid dragon soul cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM);
			return false;
		}
	}
	else
	{
		if (bWearCell >= WEAR_MAX_NUM)
		{
			sys_err("EquipTo: invalid wear cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
	}

	if (ch->GetWear(bWearCell))
	{
		sys_err("EquipTo: item already exist (this: #%d %s cell: %d %s)", GetOriginalVnum(), GetName(), bWearCell, ch->GetWear(bWearCell)->GetName());
		return false;
	}

	if (GetOwner())
		RemoveFromCharacter();

	ch->SetWear(bWearCell, this); // 여기서 패킷 나감

	m_pOwner = ch;
	m_bEquipped = true;
	m_wCell	= INVENTORY_MAX_NUM + bWearCell;

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

	if (IsDragonSoul())
	{
		DSManager::instance().ActivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(true);
		StartUniqueExpireEvent();
		if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
			StartTimerBasedOnWearExpireEvent();

		// ACCESSORY_REFINE
		StartAccessorySocketExpireEvent();
		// END_OF_ACCESSORY_REFINE
	}
#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetOwner())
	{
		if (IsCostumePetItem())
		{
			CPetSystem* petSystem = GetOwner()->GetPetSystem();
			if (petSystem)
				petSystem->HandlePetCostumeItem();
		}
		if (IsNewPetItem())
		{
			CPetSystem* petSystem = GetOwner()->GetPetSystem();
			if (petSystem)
				petSystem->HandleNewPetItem();
		}
	}
#endif
	ch->BuffOnAttr_AddBuffsFromItem(this);

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	Save();

	return (true);
}

bool CItem::Unequip()
{
	if (!m_pOwner || GetCell() < INVENTORY_MAX_NUM)
	{
		// ITEM_OWNER_INVALID_PTR_BUG
		sys_err("%s %u m_pOwner %p, GetCell %d",
				GetName(), GetID(), get_pointer(m_pOwner), GetCell());
		// END_OF_ITEM_OWNER_INVALID_PTR_BUG
		return false;
	}

	if (this != m_pOwner->GetWear(GetCell() - INVENTORY_MAX_NUM))
	{
		sys_err("m_pOwner->GetWear() != this");
		return false;
	}

	//신규 말 아이템 제거시 처리
	if (IsRideItem())
		ClearMountAttributeAndAffect();

#ifdef ENABLE_NEW_PET_SYSTEM
	if (GetOwner())
	{
		if (IsCostumePetItem() || IsNewPetItem())
		{
			CPetSystem* petSystem = GetOwner()->GetPetSystem();
			if (petSystem)
				petSystem->DeletePet(GetID());
		}
	}
#endif

	if (IsDragonSoul())
	{
		DSManager::instance().DeactivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(false);
	}

	StopUniqueExpireEvent();

	if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
		StopTimerBasedOnWearExpireEvent();

	// ACCESSORY_REFINE
	StopAccessorySocketExpireEvent();
	// END_OF_ACCESSORY_REFINE


	m_pOwner->BuffOnAttr_RemoveBuffsFromItem(this);

	m_pOwner->SetWear(GetCell() - INVENTORY_MAX_NUM, NULL);

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	m_pOwner = NULL;
	m_wCell = 0;
	m_bEquipped	= false;

	return true;
}

long CItem::GetValue(DWORD idx)
{
	assert(idx < ITEM_VALUES_MAX_NUM);
	return GetProto()->alValues[idx];
}

void CItem::SetExchanging(bool bOn)
{
	m_bExchanging = bOn;
}

void CItem::Save()
{
	if (m_bSkipSave)
		return;

	ITEM_MANAGER::instance().DelayedSave(this);
}

bool CItem::CreateSocket(BYTE bSlot, BYTE bGold)
{
	assert(bSlot < ITEM_SOCKET_MAX_NUM);

	if (m_alSockets[bSlot] != 0)
	{
		sys_err("Item::CreateSocket : socket already exist %s %d", GetName(), bSlot);
		return false;
	}

	if (bGold)
		m_alSockets[bSlot] = 2;
	else
		m_alSockets[bSlot] = 1;

	UpdatePacket();

	Save();
	return true;
}

void CItem::SetSockets(const long * c_al)
{
	thecore_memcpy(m_alSockets, c_al, sizeof(m_alSockets));
	Save();
}

void CItem::SetSocket(int i, long v, bool bLog)
{
	assert(i < ITEM_SOCKET_MAX_NUM);
	m_alSockets[i] = v;
	UpdatePacket();
	Save();
	if (bLog)
	{
#ifdef ENABLE_NEWSTUFF
		if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
		LogManager::instance().ItemLog(i, v, 0, GetID(), "SET_SOCKET", "", "", GetOriginalVnum());
	}
}

long long CItem::GetGold()
{
	if (IS_SET(GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (GetProto()->dwGold == 0)
			return GetCount();
		else
			return GetCount() / GetProto()->dwGold;
	}
	else
		return GetProto()->dwGold;
}

long long CItem::GetShopBuyPrice()
{
	return GetProto()->dwShopBuyPrice;
}

bool CItem::IsOwnership(LPCHARACTER ch)
{
	if (!m_pkOwnershipEvent)
		return true;

	return m_dwOwnershipPID == ch->GetPlayerID() ? true : false;
}

EVENTFUNC(ownership_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "ownership_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	pkItem->SetOwnershipEvent(NULL);

	TPacketGCItemOwnership p;

	p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID	= pkItem->GetVID();
	p.szName[0]	= '\0';

	pkItem->PacketAround(&p, sizeof(p));
	return 0;
}

void CItem::SetOwnershipEvent(LPEVENT pkEvent)
{
	m_pkOwnershipEvent = pkEvent;
}

void CItem::SetOwnership(LPCHARACTER ch, int iSec)
{
	if (!ch)
	{
		if (m_pkOwnershipEvent)
		{
			event_cancel(&m_pkOwnershipEvent);
			m_dwOwnershipPID = 0;

			TPacketGCItemOwnership p;

			p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
			p.dwVID	= m_dwVID;
			p.szName[0]	= '\0';

			PacketAround(&p, sizeof(p));
		}
		return;
	}

	if (m_pkOwnershipEvent)
		return;

	if (iSec <= 10)
		iSec = 30;

	m_dwOwnershipPID = ch->GetPlayerID();

	item_event_info* info = AllocEventInfo<item_event_info>();
	strlcpy(info->szOwnerName, ch->GetName(), sizeof(info->szOwnerName));
	info->item = this;

	SetOwnershipEvent(event_create(ownership_event, info, PASSES_PER_SEC(iSec)));

	TPacketGCItemOwnership p;

	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = m_dwVID;
	strlcpy(p.szName, ch->GetName(), sizeof(p.szName));

	PacketAround(&p, sizeof(p));
}

int CItem::GetSocketCount()
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		if (GetSocket(i) == 0)
			return i;
	}
	return ITEM_SOCKET_MAX_NUM;
}

bool CItem::AddSocket()
{
	int count = GetSocketCount();
	if (count == ITEM_SOCKET_MAX_NUM)
		return false;
	m_alSockets[count] = 1;
	return true;
}

void CItem::AlterToSocketItem(int iSocketCount)
{
	if (iSocketCount >= ITEM_SOCKET_MAX_NUM)
	{
		sys_log(0, "Invalid Socket Count %d, set to maximum", ITEM_SOCKET_MAX_NUM);
		iSocketCount = ITEM_SOCKET_MAX_NUM;
	}

	for (int i = 0; i < iSocketCount; ++i)
		SetSocket(i, 1);
}

void CItem::AlterToMagicItem()
{
	int idx = GetAttributeSetIndex();

	if (idx < 0)
		return;

	//      Appeariance Second Third
	// Weapon 50        20     5
	// Armor  30        10     2
	// Acc    20        10     1

	int iSecondPct;
	int iThirdPct;

	switch (GetType())
	{
		case ITEM_WEAPON:
			iSecondPct = 20;
			iThirdPct = 5;
			break;

		case ITEM_ARMOR:
		case ITEM_COSTUME:
			if (GetSubType() == ARMOR_BODY)
			{
				iSecondPct = 10;
				iThirdPct = 2;
			}
			else
			{
				iSecondPct = 10;
				iThirdPct = 1;
			}
			break;

		default:
			return;
	}

	if(GetType() == ITEM_COSTUME)
	{
		if(GetSubType() == COSTUME_HAIR)
		{
			PutAttributeCostumeHair(aiItemMagicAttributePercentHigh);
		}else{
			PutAttributeCostume(aiItemMagicAttributePercentHigh);
		}
	}else{
		PutAttribute(aiItemMagicAttributePercentHigh);
	}	

	if (number(1, 100) <= iSecondPct){
		if(GetType() == ITEM_COSTUME)
		{
			if(GetSubType() == COSTUME_HAIR)
			{
				PutAttributeCostumeHair(aiItemMagicAttributePercentLow);
			}else{
				PutAttributeCostume(aiItemMagicAttributePercentLow);
			}
		}else{
			PutAttribute(aiItemMagicAttributePercentLow);
		}
	}

	if (number(1, 100) <= iThirdPct){
		if(GetType() == ITEM_COSTUME)
		{
			if(GetSubType() == COSTUME_HAIR)
			{
				PutAttributeCostumeHair(aiItemMagicAttributePercentLow);
			}else{
				PutAttributeCostume(aiItemMagicAttributePercentLow);
			}
		}else{
			PutAttribute(aiItemMagicAttributePercentLow);
		}
	}
}

DWORD CItem::GetRefineFromVnum()
{
	return ITEM_MANAGER::instance().GetRefineFromVnum(GetVnum());
}

int CItem::GetRefineLevel()
{
	const char* name = GetBaseName();
	char* p = const_cast<char*>(strrchr(name, '+'));

	if (!p)
		return 0;

	int	rtn = 0;
	str_to_number(rtn, p+1);

	const char* locale_name = GetName();
	p = const_cast<char*>(strrchr(locale_name, '+'));

	if (p)
	{
		int	locale_rtn = 0;
		str_to_number(locale_rtn, p+1);
		if (locale_rtn != rtn)
		{
			sys_err("refine_level_based_on_NAME(%d) is not equal to refine_level_based_on_LOCALE_NAME(%d).", rtn, locale_rtn);
		}
	}

	return rtn;
}

bool CItem::IsPolymorphItem()
{
	return GetType() == ITEM_POLYMORPH;
}

EVENTFUNC(unique_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "unique_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetValue(2) == 0)
	{
		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= 1)
		{
			sys_log(0, "UNIQUE_ITEM: expire %s %u", pkItem->GetName(), pkItem->GetID());
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			pkItem->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - 1);
			return PASSES_PER_SEC(60);
		}
	}
	else
	{
		time_t cur = get_global_time();

		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= cur)
		{
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			// 게임 내에 시간제 아이템들이 빠릿빠릿하게 사라지지 않는 버그가 있어
			// 수정
			// by rtsummit
			if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur < 600)
				return PASSES_PER_SEC(pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur);
			else
				return PASSES_PER_SEC(600);
		}
	}
}

// 시간 후불제
// timer를 시작할 때에 시간 차감하는 것이 아니라,
// timer가 발화할 때에 timer가 동작한 시간 만큼 시간 차감을 한다.
EVENTFUNC(timer_based_on_wear_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "expire_event <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(ITEM_SOCKET_REMAIN_SEC) - processing_time/passes_per_sec;
	if (remain_time <= 0)
	{
		sys_log(0, "ITEM EXPIRED : expired %s %u", pkItem->GetName(), pkItem->GetID());
		pkItem->SetTimerBasedOnWearExpireEvent(NULL);
		pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, 0);

		// 일단 timer based on wear 용혼석은 시간 다 되었다고 없애지 않는다.
		if (pkItem->IsDragonSoul())
		{
			DSManager::instance().DeactivateDragonSoul(pkItem);
		}
		else
		{
			ITEM_MANAGER::instance().RemoveItem(pkItem, "TIMER_BASED_ON_WEAR_EXPIRE");
		}
		return 0;
	}
	pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	return PASSES_PER_SEC (MIN (60, remain_time));
}

void CItem::SetUniqueExpireEvent(LPEVENT pkEvent)
{
	m_pkUniqueExpireEvent = pkEvent;
}

void CItem::SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent)
{
	m_pkTimerBasedOnWearExpireEvent = pkEvent;
}

EVENTFUNC(real_time_expire_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (NULL == info)
		return 0;

	const LPITEM item = ITEM_MANAGER::instance().FindByVID( info->item_vid );

	if (NULL == item)
		return 0;

	const time_t current = get_global_time();

	if (current > item->GetSocket(0))
	{
		switch (item->GetVnum())
		{
			if(item->IsNewMountItem())
			{
				if (item->GetSocket(2) != 0)
					item->ClearMountAttributeAndAffect();
			}
			break;
		}
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		if(item->IsCostumeMount())
		{
			LPCHARACTER owner = item->GetOwner();
			if (owner)
			{
				owner->StopRiding();
				owner->HorseSummon(false);
			}
		}
#endif
		
		if(item->IsNewPetItem())
			return 0;


#ifdef ENABLE_MOUNT_SKIN
		const bool isNeedRefresh = item->IsCostumeMountSkin() ? true : false;
		LPCHARACTER itOwner = item->GetOwner();
#endif
		ITEM_MANAGER::instance().RemoveItem(item, "REAL_TIME_EXPIRE");

#ifdef ENABLE_MOUNT_SKIN
		if(isNeedRefresh && itOwner)
		{
			itOwner->StopRiding();
			itOwner->HorseSummon(false);
		}
#endif

		return 0;
	}

	return PASSES_PER_SEC(1);
}

void CItem::StartRealTimeExpireEvent()
{
	if (m_pkRealTimeExpireEvent)
		return;
//#ifdef ENABLE_ITEMSHOP
//	if (GetWindow() == RESERVED_WINDOW)
//		return;
//#endif
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->aLimits[i].bType)
		{
			item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
			info->item_vid = GetVID();

			m_pkRealTimeExpireEvent = event_create( real_time_expire_event, info, PASSES_PER_SEC(1));

			sys_log(0, "REAL_TIME_EXPIRE: StartRealTimeExpireEvent");

			return;
		}
	}
}

#if defined(ENABLE_NEW_AUTOPOTION)
EVENTFUNC(time_auto_potion_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (!info)
		return 0;

	const LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);

	if (!item)
		return 0;

	if (item->GetSocket(0) == false)
		return PASSES_PER_SEC(1);

	int remain_time = item->GetSocket(2) - processing_time / passes_per_sec;
	if (remain_time <= 0) {
		ITEM_MANAGER::instance().RemoveItem(item, "TIME_AUTO_POTION_EXPIRE");
		return 0;
	}

	item->SetSocket(2, remain_time);
	return PASSES_PER_SEC(MIN(60, remain_time));
}

void CItem::StartTimeAutoPotionEvent()
{
	if (m_pkTimerAutoPotionEvent)
		return;

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	m_pkTimerAutoPotionEvent = event_create(time_auto_potion_event, info, PASSES_PER_SEC(1));

	sys_log(0, "TIME_EXPIRE: StartTimeAutoPotionEvent");
}

bool CItem::IsNewAutoPotionItem()
{
	switch (GetVnum())
	{
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
			return true;

		default:
			return false;
	}
}
#endif

bool CItem::IsRealTimeItem()
{
	if(!GetProto())
		return false;
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return true;
	}
	return false;
}

void CItem::StartUniqueExpireEvent()
{
	if (GetType() != ITEM_UNIQUE)
		return;

	if (m_pkUniqueExpireEvent)
		return;

	//기간제 아이템일 경우 시간제 아이템은 동작하지 않는다
	if (IsRealTimeItem())
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(false);

	int iSec = GetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME);

	if (iSec == 0)
		iSec = 60;
	else
		iSec = MIN(iSec, 60);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, 0);

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetUniqueExpireEvent(event_create(unique_expire_event, info, PASSES_PER_SEC(iSec)));
}

// 시간 후불제
// timer_based_on_wear_expire_event 설명 참조
void CItem::StartTimerBasedOnWearExpireEvent()
{
	if (m_pkTimerBasedOnWearExpireEvent)
		return;

	//기간제 아이템일 경우 시간제 아이템은 동작하지 않는다
	if (IsRealTimeItem())
		return;

	if (-1 == GetProto()->cLimitTimerBasedOnWearIndex)
		return;

	int iSec = GetSocket(0);

	// 남은 시간을 분단위로 끊기 위해...
	if (0 != iSec)
	{
		iSec %= 60;
		if (0 == iSec)
			iSec = 60;
	}

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetTimerBasedOnWearExpireEvent(event_create(timer_based_on_wear_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopUniqueExpireEvent()
{
	if (!m_pkUniqueExpireEvent)
		return;

	if (GetValue(2) != 0) // 게임시간제 이외의 아이템은 UniqueExpireEvent를 중단할 수 없다.
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(true);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, event_time(m_pkUniqueExpireEvent) / passes_per_sec);
	event_cancel(&m_pkUniqueExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::StopTimerBasedOnWearExpireEvent()
{
	if (!m_pkTimerBasedOnWearExpireEvent)
		return;

	int remain_time = GetSocket(ITEM_SOCKET_REMAIN_SEC) - event_processing_time(m_pkTimerBasedOnWearExpireEvent) / passes_per_sec;

	SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::ApplyAddon(int iAddonType)
{
	CItemAddonManager::instance().ApplyAddonTo(iAddonType, this);
}

int CItem::GetSpecialGroup() const
{
	return ITEM_MANAGER::instance().GetSpecialGroupFromItem(GetVnum());
}

//
// 악세서리 소켓 처리.
//
bool CItem::IsAccessoryForSocket()
{
#ifdef STONE_INTO_COSTUMES
	return (m_pProto->bType == ITEM_ARMOR && (m_pProto->bSubType == ARMOR_WRIST || m_pProto->bSubType == ARMOR_NECK || m_pProto->bSubType == ARMOR_EAR)) ||
		(m_pProto->bType == ITEM_BELT) || (m_pProto->bType == ITEM_COSTUME && m_pProto->bSubType == COSTUME_BODY) || (m_pProto->bType == ITEM_NEW_RINGS);			// 2013년 2월 새로 추가된 '벨트' 아이템의 경우 기획팀에서 악세서리 소켓 시스템을 그대로 이용하자고 함.
#else
	return (m_pProto->bType == ITEM_ARMOR && (m_pProto->bSubType == ARMOR_WRIST || m_pProto->bSubType == ARMOR_NECK || m_pProto->bSubType == ARMOR_EAR )) ||
		(m_pProto->bType == ITEM_BELT) || (m_pProto->bType == ITEM_NEW_RINGS);				// 2013년 2월 새로 추가된 '벨트' 아이템의 경우 기획팀에서 악세서리 소켓 시스템을 그대로 이용하자고 함.
#endif
}

void CItem::SetAccessorySocketGrade(int iGrade)
{
	SetSocket(0, MINMAX(0, iGrade, GetAccessorySocketMaxGrade()));

	int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

	//if (test_server)
	//	iDownTime /= 60;

	SetAccessorySocketDownGradeTime(iDownTime);
}

void CItem::SetAccessorySocketMaxGrade(int iMaxGrade)
{
	SetSocket(1, MINMAX(0, iMaxGrade, ITEM_ACCESSORY_SOCKET_MAX_NUM));
}

void CItem::SetAccessorySocketDownGradeTime(DWORD time)
{
	SetSocket(2, time);

	if (test_server && GetOwner())
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, "822 %s %d", GetName(), time);
}

EVENTFUNC(accessory_socket_expire_event)
{
	item_vid_event_info* info = dynamic_cast<item_vid_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "accessory_socket_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);
	if(!item)
		return 0;

	if (item->GetAccessorySocketDownGradeTime() <= 1)
	{
degrade:
		item->SetAccessorySocketExpireEvent(NULL);
		item->AccessorySocketDegrade();
		return 0;
	}
	else
	{
		int iTime = item->GetAccessorySocketDownGradeTime() - 60;

		if (iTime <= 1)
			goto degrade;

		item->SetAccessorySocketDownGradeTime(iTime);

		if (iTime > 60)
			return PASSES_PER_SEC(60);
		else
			return PASSES_PER_SEC(iTime);
	}
}

void CItem::StartAccessorySocketExpireEvent()
{
	if (!IsAccessoryForSocket())
		return;

	if (m_pkAccessorySocketExpireEvent)
		return;

	if (GetAccessorySocketMaxGrade() == 0)
		return;

	if (GetAccessorySocketGrade() == 0)
		return;

	int iSec = GetAccessorySocketDownGradeTime();
	SetAccessorySocketExpireEvent(NULL);

	if (iSec <= 1)
		iSec = 5;
	else
		iSec = MIN(iSec, 60);

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	SetAccessorySocketExpireEvent(event_create(accessory_socket_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopAccessorySocketExpireEvent()
{
	if (!m_pkAccessorySocketExpireEvent)
		return;

	if (!IsAccessoryForSocket())
		return;

	int new_time = GetAccessorySocketDownGradeTime() - (60 - event_time(m_pkAccessorySocketExpireEvent) / passes_per_sec);

	event_cancel(&m_pkAccessorySocketExpireEvent);

	if (new_time <= 1)
	{
		AccessorySocketDegrade();
	}
	else
	{
		SetAccessorySocketDownGradeTime(new_time);
	}
}

bool CItem::IsRideItem()
{
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_RIDE == GetSubType())
		return true;
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType())
		return true;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ITEM_COSTUME == GetType() && COSTUME_MOUNT == GetSubType())
		return true;
#endif
#ifdef ENABLE_MOUNT_SKIN
	if (ITEM_COSTUME == GetType() && COSTUME_MOUNT_SKIN == GetSubType())
		return true;
#endif
	return false;
}

bool CItem::IsRamadanRing()
{
	if (GetVnum() == UNIQUE_ITEM_RAMADAN_RING)
		return true;
	return false;
}

void CItem::ClearMountAttributeAndAffect()
{
	LPCHARACTER ch = GetOwner();

	ch->RemoveAffect(AFFECT_MOUNT);
	ch->RemoveAffect(AFFECT_MOUNT_BONUS);

	ch->MountVnum(0);

	ch->PointChange(POINT_ST, 0);
	ch->PointChange(POINT_DX, 0);
	ch->PointChange(POINT_HT, 0);
	ch->PointChange(POINT_IQ, 0);
}

// fixme
// 이거 지금은 안쓴데... 근데 혹시나 싶어서 남겨둠.
// by rtsummit
bool CItem::IsNewMountItem()
{
	switch(GetVnum())
	{
		case 76000: case 76001: case 76002: case 76003:
		case 76004: case 76005: case 76006: case 76007:
		case 76008: case 76009: case 76010: case 76011:
		case 76012: case 76013: case 76014:
			return true;
	}
	return false;
}

void CItem::SetAccessorySocketExpireEvent(LPEVENT pkEvent)
{
	m_pkAccessorySocketExpireEvent = pkEvent;
}

void CItem::AccessorySocketDegrade()
{
	if (GetAccessorySocketGrade() > 0)
	{
		LPCHARACTER ch = GetOwner();

		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "823 %s", GetName());
		}

		ModifyPoints(false);
		SetAccessorySocketGrade(GetAccessorySocketGrade()-1);
		ModifyPoints(true);

		int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

		if (test_server)
			iDownTime /= 60;

		SetAccessorySocketDownGradeTime(iDownTime);

		if (iDownTime)
			StartAccessorySocketExpireEvent();
	}
}

// ring에 item을 박을 수 있는지 여부를 체크해서 리턴
static const bool CanPutIntoRing(LPITEM ring, LPITEM item)
{
	//const DWORD vnum = item->GetVnum();
	return false;
}

bool CItem::CanPutInto(LPITEM item)
{
	if (item->GetType() == ITEM_BELT)
		return this->GetSubType() == USE_PUT_INTO_BELT_SOCKET;

	else if(item->GetType() == ITEM_RING)
		return CanPutIntoRing(item, this);

	if (item->GetType() != ITEM_NEW_RINGS && item->GetType() != ITEM_ARMOR)
	{
		return false;
	}

	DWORD vnum = item->GetVnum();

	struct JewelAccessoryInfo
	{
		DWORD jewel;
		DWORD wrist;
		DWORD neck;
		DWORD ear;
		DWORD rings;
	};
	const static JewelAccessoryInfo infos[] = {
		{ 50634, 14420, 16220, 17220, 19220},
		{ 50635, 14500, 16500, 17500, 19500},
		{ 50636, 14520, 16520, 17520, 19520},
		{ 50637, 14540, 16540, 17540, 19540},
		{ 50638, 14560, 16560, 17560, 19560},
		{ 50639, 14570, 16570, 17570, 19570},
	};

	DWORD item_type = (item->GetVnum() / 10) * 10;
	for (size_t i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch(item->GetSubType())
		{
		case ARMOR_WRIST:
			if (info.wrist == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_NECK:
			if (info.neck == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_EAR:
			if (info.ear == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;

		}


		if (item->GetType() == ITEM_NEW_RINGS){
			if (info.rings == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		}
	}

	if (item->GetType() == ITEM_NEW_RINGS){
		vnum -= 19000;
	}
	else
	{
		if (item->GetSubType() == ARMOR_WRIST)
			vnum -= 14000;
		else if (item->GetSubType() == ARMOR_NECK)
			vnum -= 16000;
		else if (item->GetSubType() == ARMOR_EAR)
			vnum -= 17000;	
		else
			return false;
	}

	DWORD type = vnum / 20;

	if (type < 0 || type > 11)
	{
		type = (vnum - 170) / 20;

		if (50623 + type != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16210 && item->GetVnum() <= 16219)
	{
		if (50625 != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16230 && item->GetVnum() <= 16239)
	{
		if (50626 != GetVnum())
			return false;
		else
			return true;
	}

	return 50623 + type == GetVnum();
}


int CItem::GetAccessorySocketVnumRings(LPITEM item)
{

	struct RingsAccessoryInfo
	{
		DWORD jewel;
		DWORD rings;
	};

	const int ACCESSORY_MATERIAL_LIST[] = {50623, 50624, 50625, 50626, 50627, 50628, 50629, 50630, 50631, 50632, 50633, 50634, 50635, 50636, 50637, 50638, 50639};
	const static RingsAccessoryInfo infos[] = {
		{ 50634,  19220},
		{ 50635,  19500},
		{ 50636,  19520},
		{ 50637,  19540},
		{ 50638,  19560},
		{ 50639,  19570},
	};

	DWORD vnum = item->GetVnum();

	DWORD item_type = (item->GetVnum() / 10) * 10;
	for (size_t i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const RingsAccessoryInfo& info = infos[i];
		if (item->GetType() == ITEM_NEW_RINGS){
			if (info.rings == item_type)
			{
				return info.jewel;
			}
		}
	}

	if (item->GetType() == ITEM_NEW_RINGS){
		vnum -= 19000;
	}

	DWORD type = vnum / 20;

	if (type < 0 || type >= (sizeof(ACCESSORY_MATERIAL_LIST) / sizeof(ACCESSORY_MATERIAL_LIST[0])))
	{
		type = (vnum - 170) / 20;

		if (type < 0 || type >= (sizeof(ACCESSORY_MATERIAL_LIST) / sizeof(ACCESSORY_MATERIAL_LIST[0])))
			return 0;
	}

	return ACCESSORY_MATERIAL_LIST[type];

}

// PC_BANG_ITEM_ADD
bool CItem::IsPCBangItem()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (m_pProto->aLimits[i].bType == LIMIT_PCBANG)
			return true;
	}
	return false;
}
// END_PC_BANG_ITEM_ADD

bool CItem::CheckItemUseLevel(int nLevel)
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			if (this->m_pProto->aLimits[i].lValue > nLevel) return false;
			else return true;
		}
	}
	return true;
}

long CItem::FindApplyValue(BYTE bApplyType)
{
	if (m_pProto == NULL)
		return 0;

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == bApplyType)
			return m_pProto->aApplies[i].lValue;
	}

	return 0;
}

void CItem::CopySocketTo(LPITEM pItem)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		pItem->m_alSockets[i] = m_alSockets[i];
	}
}

int CItem::GetAccessorySocketGrade()
{
   	return MINMAX(0, GetSocket(0), GetAccessorySocketMaxGrade());
}

int CItem::GetAccessorySocketMaxGrade()
{
   	return MINMAX(0, GetSocket(1), ITEM_ACCESSORY_SOCKET_MAX_NUM);
}

int CItem::GetAccessorySocketDownGradeTime()
{
	return MINMAX(0, GetSocket(2), aiAccessorySocketDegradeTime[GetAccessorySocketGrade()]);
}

void CItem::AttrLog()
{
	const char * pszIP = NULL;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (m_alSockets[i])
		{
#ifdef ENABLE_NEWSTUFF
			if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
			LogManager::instance().ItemLog(i, m_alSockets[i], 0, GetID(), "INFO_SOCKET", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}

	for (int i = 0; i<ITEM_ATTRIBUTE_MAX_NUM; ++i)
	{
		int	type	= m_aAttr[i].bType;
		int value	= m_aAttr[i].sValue;

		if (type)
		{
#ifdef ENABLE_NEWSTUFF
			if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
			LogManager::instance().ItemLog(i, type, value, GetID(), "INFO_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}
#ifdef ENABLE_GLOVE_SYSTEM
	for (int i = 0; i<ITEM_ATTRIBUTE_MAX_NUM; ++i)
	{
		int	type	= m_randomAttr[i].bType;
		int value	= m_randomAttr[i].sValue;

		if (type)
		{
#ifdef ENABLE_NEWSTUFF
			if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
			LogManager::instance().ItemLog(i, type, value, GetID(), "INFO_RANDOM_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}
#endif

}

int CItem::GetLevelLimit()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			return this->m_pProto->aLimits[i].lValue;
		}
	}
	return 0;
}

bool CItem::OnAfterCreatedItem()
{
	// 아이템을 한 번이라도 사용했다면, 그 이후엔 사용 중이지 않아도 시간이 차감되는 방식
	if (-1 != this->GetProto()->cLimitRealTimeFirstUseIndex)
	{
		// Socket1에 아이템의 사용 횟수가 기록되어 있으니, 한 번이라도 사용한 아이템은 타이머를 시작한다.
		if (0 != GetSocket(1))
		{
			StartRealTimeExpireEvent();
		}
	}
#ifdef __SOULBINDING_SYSTEM__
	if (IsUntilBind())
		StartUnBindingExpireEvent();
#endif
	return true;
}


#ifdef __AUCTION__

// 경매장
// window를 경매장으로 한다.

bool CItem::MoveToAuction()
{
	LPCHARACTER owner = GetOwner();
	if (owner == NULL)
	{
		sys_err ("Item those owner is not exist cannot regist in auction");
		return false;
	}

	if (GetWindow() == AUCTION)
	{
		sys_err ("Item is already in auction.");
	}

	SetWindow(AUCTION);
	owner->SetItem(m_bCell, NULL);
	Save();
	ITEM_MANAGER::instance().FlushDelayedSave(this);

	return true;
}

void CItem::CopyToRawData (TPlayerItem* new_item)
{
	if (new_item != NULL)
		return;

	new_item->id = m_dwID;
	new_item->window = m_bWindow;
	new_item->pos = m_bCell;
	new_item->count = m_dwCount;
#ifdef ENABLE_NEW_NAME_ITEM
	strlcpy(new_item->name,nameex,sizeof(new_item->name));
#endif
	new_item->vnum = GetVnum();
	thecore_memcpy (new_item->alSockets, m_alSockets, sizeof (m_alSockets));
	thecore_memcpy (new_item->aAttr, m_aAttr, sizeof (m_aAttr));
#ifdef ENABLE_GLOVE_SYSTEM
	thecore_memcpy (new_item->randomAttr, m_randomAttr, sizeof (m_randomAttr));
#endif

	new_item->owner = m_pOwner->GetPlayerID();
}
#endif

bool CItem::IsDragonSoul()
{
	return GetType() == ITEM_DS;
}

#ifdef ENABLE_SPECIAL_STORAGE
bool CItem::IsUpgradeItem()
{
	return ((GetType() == ITEM_MATERIAL && GetSubType() == MATERIAL_LEATHER) || (GetVnum() == 27987 || GetVnum() == 27799 || GetVnum() == 70031 || (GetVnum() >= 27992 && GetVnum() <= 27994)));
}
bool CItem::IsBook()
{
	return (GetType() == ITEM_SKILLBOOK);
}

bool CItem::IsStone()
{
	return (GetType() == ITEM_METIN && GetSubType() == METIN_NORMAL);
}

bool CItem::IsChange()
{
	switch(GetVnum())
	{
		case 72346:
			return true;
	}
	//agragar items para que vayan al inventario normal del agregar bonus skin mount
	switch(GetVnum())
	{
		case 54001 ... 54267:
		case 77927:
		case 50348:
		case 50349:
			return false;
	}
	return (GetType() == ITEM_USE && GetSubType() == USE_CHANGE_ATTRIBUTE);
}

bool CItem::IsCostumeSkin()
{
	return GetType() == ITEM_COSTUME && GetSubType() != COSTUME_MOUNT && GetSubType() != COSTUME_PET && GetSubType() != COSTUME_SASH;
}
bool CItem::IsCostume()
{
	return false;//when create false for to inv! bro this function last line return <-

	switch(GetVnum())
	{
		//case 85005:
		case 41004:
		case 40101:
		case 45005:
		case 87012:
			return false;
	}
	return GetType() == ITEM_COSTUME && GetSubType() != COSTUME_MOUNT && GetSubType() != COSTUME_PET;
}
#endif

int CItem::GiveMoreTime_Per(float fPercent)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		DWORD given_time = fPercent * duration / 100u;
		if (remain_sec == duration)
			return false;
		if ((given_time + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, given_time + remain_sec);
			return given_time;
		}
	}
	// 우선 용혼석에 관해서만 하도록 한다.
	else
		return 0;
}

int CItem::GiveMoreTime_Fix(DWORD dwTime)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		if (remain_sec == duration)
			return false;
		if ((dwTime + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, dwTime + remain_sec);
			return dwTime;
		}
	}
	// 우선 용혼석에 관해서만 하도록 한다.
	else
		return 0;
}


int	CItem::GetDuration()
{
	if(!GetProto())
		return -1;

	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return GetProto()->aLimits[i].lValue;
	}

	if (GetProto()->cLimitTimerBasedOnWearIndex >= 0)
	{
		BYTE cLTBOWI = GetProto()->cLimitTimerBasedOnWearIndex;
		return GetProto()->aLimits[cLTBOWI].lValue;
	}

	return -1;
}

bool CItem::IsSameSpecialGroup(const LPITEM item) const
{
	// 서로 VNUM이 같다면 같은 그룹인 것으로 간주
	if (this->GetVnum() == item->GetVnum())
		return true;

	if (GetSpecialGroup() && (item->GetSpecialGroup() == GetSpecialGroup()))
		return true;

	return false;
}
#ifdef ELEMENT_SPELL_WORLDARD	
void CItem::SetElement(BYTE value_grade_element, const DWORD * attack_element, BYTE element_type_bonus, const short * elements_value_bonus)
{
	m_grade_element = value_grade_element;
	thecore_memcpy(m_attack_element, attack_element, sizeof(m_attack_element));
	m_element_type_bonus = element_type_bonus;
	thecore_memcpy(m_elements_value_bonus, elements_value_bonus, sizeof(m_elements_value_bonus));
	UpdatePacket();
	Save();
}


void CItem::SetElementNew(BYTE value_grade_element, DWORD attack_element, BYTE element_type_bonus, short elements_value_bonus)
{
	m_grade_element = value_grade_element;
	m_attack_element[value_grade_element-1] = attack_element;
	m_element_type_bonus = element_type_bonus;
	m_elements_value_bonus[value_grade_element-1] = elements_value_bonus;
	UpdatePacket();
	Save();
}

void CItem::DeleteAllElement(BYTE value_grade_element)
{

	m_grade_element = value_grade_element;
	m_attack_element[value_grade_element] = 0;
	m_elements_value_bonus[value_grade_element] = 0;
	if(value_grade_element == 0)
	{
		m_element_type_bonus = 0;
	}
	UpdatePacket();
	Save();
}


void CItem::ChangeElement(BYTE element_type_bonus)
{
	m_element_type_bonus = element_type_bonus;
	UpdatePacket();
	Save();
}

#endif



#ifdef ENABLE_NEW_NAME_ITEM
void CItem::SetNewName(const char* name)
{
	strlcpy(nameex,name,sizeof(nameex));
}
#endif
#ifdef ENABLE_NEW_PET_SYSTEM
bool CItem::IsCostumePetItem()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_PET;
}
bool CItem::IsNewPetItem()
{
	return GetType() == ITEM_PET && GetSubType() == PET_LEVELABLE;
}
#endif

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
bool CItem::IsCostumeMount()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_MOUNT;
}
#endif

#ifdef ENABLE_MOUNT_SKIN
bool CItem::IsCostumeMountSkin()
{
	return GetType() == ITEM_COSTUME && GetSubType() == COSTUME_MOUNT_SKIN;
}
#endif

#ifdef ENABLE_RARITY
void CItem::SetRarityValue(LPCHARACTER ch, long decreaseValue)
{
	long oldValue = GetSocket(RARITY_VALUE_INDEX);
	if (oldValue <= 0)
		return;

	long newValue = oldValue-decreaseValue;
	if (newValue <= 0)
	{
		newValue = 0;
		ModifyPoints(false);
		SetSocket(RARITY_VALUE_INDEX, newValue);
		ModifyPoints(true);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RarityMessage 0 0");
	}
	else
	{
		long rarityMaxValue = GetRarityMaxValue();
		long onePercent = float(float(rarityMaxValue) / 100.0);
		if(onePercent <= 0 || rarityMaxValue <= 0)
			return;
		if ((newValue % onePercent) == 0)
		{
			float checkPercent = (float(newValue) / float(rarityMaxValue)) * 100;
			long checkPercentValue = floor(checkPercent * (float(rarityMaxValue)) / 100.0);
			if (checkPercentValue == newValue)
			{
	
				ModifyPoints(false);
				SetSocket(RARITY_VALUE_INDEX, newValue);
				ModifyPoints(true);
				
				int checkPercentIn = int(checkPercent);
				if (checkPercentIn <= 10)
					ch->ChatPacket(CHAT_TYPE_COMMAND, "RarityMessage 1 %d", checkPercentIn);

			}
			else
				m_alSockets[RARITY_VALUE_INDEX] = newValue;
		}
		else
			m_alSockets[RARITY_VALUE_INDEX] = newValue;
	}

	if(test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s - rarity: %ld", GetName(), newValue);
}
long CItem::GetRarityMaxValue()
{
	if (!IsRarityItem())
		return 0;

	switch (GetSocket(RARITY_TYPE_INDEX))
	{
		case RARITY_NORMAL:
			if (GetType() == ITEM_WEAPON)
				return RARITY_WEAPON_MAX_VALUE_NORMAL;
			else
				return RARITY_ARMOR_MAX_VALUE_NORMAL;
			break;
		case RARITY_UNCOMMUN:
			if (GetType() == ITEM_WEAPON)
				return RARITY_WEAPON_MAX_VALUE_UNCOMMUN;
			else
				return RARITY_ARMOR_MAX_VALUE_UNCOMMUN;
			break;
		case RARITY_RARE:
			if (GetType() == ITEM_WEAPON)
				return RARITY_WEAPON_MAX_VALUE_RARE;
			else
				return RARITY_ARMOR_MAX_VALUE_RARE;
			break;
		case RARITY_EPIC:
			if (GetType() == ITEM_WEAPON)
				return RARITY_WEAPON_MAX_VALUE_EPIC;
			else
				return RARITY_ARMOR_MAX_VALUE_EPIC;
			break;
		case RARITY_RELIC:
			if (GetType() == ITEM_WEAPON)
				return RARITY_WEAPON_MAX_VALUE_RELIC;
			else
				return RARITY_ARMOR_MAX_VALUE_RELIC;
			break;
		case RARITY_LEGENDARY:
			if (GetType() == ITEM_WEAPON)
				return RARITY_WEAPON_MAX_VALUE_LEGENDARY;
			else
				return RARITY_ARMOR_MAX_VALUE_LEGENDARY;
			break;
	}
	return 0;
}
bool CItem::IsRarityItem()
{
	if (GetType() == ITEM_ARMOR)
		return true;
	else if (GetType() == ITEM_NEW_RINGS)
		return true;
	else if (GetType() == ITEM_WEAPON)
	{
		if (GetSubType() == WEAPON_QUIVER || WEAPON_ARROW == GetSubType())
			return false;
		return true;
	}
	return false;
}
bool CItem::IncreaseRarity(BYTE increasePercent)
{
	long rarityValue = GetSocket(RARITY_VALUE_INDEX);
	long rarityMaxValue = GetRarityMaxValue();
	if (rarityValue >= rarityMaxValue)
		return false;
	long newRarity = long(float(increasePercent) * (float(rarityMaxValue) / 100.0));
	newRarity += rarityValue;
	if (newRarity > rarityMaxValue)
		newRarity = rarityMaxValue;
	SetSocket(RARITY_VALUE_INDEX, newRarity);
	return true;
}
void CItem::GiveRandomRarity()
{
	std::map<DWORD, float>m_vecProbs = {
		{RARITY_NORMAL,22.0},//Normal
		{RARITY_UNCOMMUN,50.0},//UnCommun
		{RARITY_RARE,22.0},//Rare
		{RARITY_EPIC,3.5},//Epic
		{RARITY_RELIC,1.7},//Reliq
		{RARITY_LEGENDARY,1.0},//Legendary
	};

	std::vector<int> m_vecProbsReal;
	if(m_vecProbs.size())
	{
		for(auto it = m_vecProbs.begin();it!=m_vecProbs.end();++it)
		{
			if ((DWORD)(10000.0f * it->second) >= (DWORD)number(1, 1000000))
				m_vecProbsReal.emplace_back(it->first);
		}
	}

	int randomRarity = RARITY_NORMAL;
	if(m_vecProbsReal.size() > 1)
		randomRarity = m_vecProbsReal[number(0,m_vecProbsReal.size()-1)];

	if(randomRarity < RARITY_NORMAL || randomRarity > RARITY_LEGENDARY)
		randomRarity = RARITY_NORMAL;

	SetSocket(RARITY_TYPE_INDEX, randomRarity);

	/*
	// Old algoritm
	int randomRarity = number(RARITY_NORMAL, RARITY_LEGENDARY);
	bool giveLowRarity = true;
	if (randomRarity >= RARITY_UNCOMMUN)
	{
		for (BYTE j = 0; j < randomRarity; ++j)
		{
			if (number(1, 3) == 2)
			{
				if (j == randomRarity-1)
					giveLowRarity = false;
			}
		}
	}
	if (giveLowRarity)
		randomRarity = RARITY_NORMAL;
	SetSocket(RARITY_TYPE_INDEX, randomRarity);
	*/


	long itemMaxRarity = GetRarityMaxValue();
#ifdef GIVING_RANDOMLY_RARITY_VALUE
	long rarirtyPercent = number(5, 100);
	bool giveLowValue = true;
	if (rarirtyPercent > 70)
	{
		int forCount = rarirtyPercent / 10;
		for (BYTE j = 0; j < forCount; ++j)
		{
			if (number(1, 3) == 2)
			{
				if (j == forCount - 1)
					giveLowValue = false;
			}
		}
	}
	if(giveLowValue)
		rarirtyPercent = number(5, 40);
	long newRarityValue = long(float(rarirtyPercent) * (float(itemMaxRarity) / 100.0));
#else
	long newRarityValue = itemMaxRarity;
#endif
	
	SetSocket(RARITY_VALUE_INDEX, newRarityValue);
}
#endif


#ifdef ENABLE_SPECIAL_COSTUME_ATTR
bool CItem::CanBonusCostume()
{
#ifdef __ITEM_SHINING__
	if(GetType() == ITEM_SHINING)
	{
		switch (GetSubType())
		{
			case SHINING_WEAPON:
			case SHINING_ARMOR:
			case SHINING_SPECIAL:
				return true;
		}
	}
#endif
	else if (GetType() == ITEM_COSTUME)
	{
		switch (GetSubType())
		{
			case COSTUME_BODY:
				case COSTUME_HAIR:
				case COSTUME_WEAPON:
#ifdef ENABLE_MOUNT_SKIN
				case COSTUME_MOUNT_SKIN:
#endif
#ifdef ENABLE_SASH_COSTUME_SYSTEM
				case COSTUME_NEW_SASH:
#endif
					return true;
		}
	}
	return false;
}

BYTE CItem::GetBonusFlag()
{
#ifdef __ITEM_SHINING__
	if(GetType() == ITEM_SHINING)
	{
		switch (GetSubType())
		{
			case SHINING_WEAPON:
			case SHINING_ARMOR:
			case SHINING_SPECIAL:
				return 6;
		}
	}
#endif
	else if (GetType() == ITEM_COSTUME)
	{
		switch (GetSubType())
		{
			case COSTUME_BODY:
				return 1;
			case COSTUME_HAIR:
				return 2;
			case COSTUME_WEAPON:
				return 3;
#ifdef ENABLE_MOUNT_SKIN
			case COSTUME_MOUNT_SKIN:
				return 4;
#endif
#ifdef ENABLE_SASH_COSTUME_SYSTEM
			case COSTUME_NEW_SASH:
				return 5;
#endif
		}
	}
	return 0;
}
BYTE CItem::GetBonusMaxAttr()
{
#ifdef __ITEM_SHINING__
	if(GetType() == ITEM_SHINING)
	{
		switch (GetSubType())
		{
			case SHINING_WEAPON:
			case SHINING_ARMOR:
			case SHINING_SPECIAL:
				return SPECIAL_ATTR_COSTUME_SHINING_LIMIT;
		}
	}
#endif
	else if (GetType() == ITEM_COSTUME)
	{
		switch (GetSubType())
		{
			case COSTUME_BODY:
				return SPECIAL_ATTR_COSTUME_BODY_LIMIT;
			case COSTUME_HAIR:
				return SPECIAL_ATTR_COSTUME_HAIR_LIMIT;
			case COSTUME_WEAPON:
				return SPECIAL_ATTR_COSTUME_WEAPON_LIMIT;
#ifdef ENABLE_MOUNT_SKIN
			case COSTUME_MOUNT_SKIN:
				return SPECIAL_ATTR_COSTUME_MOUNT_SKIN_LIMIT;
#endif
#ifdef ENABLE_SASH_COSTUME_SYSTEM
			case COSTUME_NEW_SASH:
				return SPECIAL_ATTR_COSTUME_SASH_SKIN_LIMIT;
#endif
		}
	}
	return 0;
}
#endif


bool CItem::IsTalisman()
{
	return (GetType() == ITEM_ARMOR && GetSubType() == ARMOR_PENDANT);
}


#ifdef ENABLE_COPY_ATTR_ITEM
bool CItem::CanCopyAttrItem()
{
	switch(GetType())
	{
		//case ITEM_WEAPON:
		//	if(GetSubType() != WEAPON_ARROW && GetSubType() != WEAPON_QUIVER)
		//		return true;
		//	break;
		case ITEM_COSTUME:
		//case ITEM_ARMOR:
			return true;
	}
	return false;
}
#endif