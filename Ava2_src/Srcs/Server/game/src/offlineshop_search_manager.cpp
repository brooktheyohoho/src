#include "stdafx.h"

#ifdef ENABLE_OFFLINESHOP_SYSTEM
#ifdef ENABLE_SHOP_SEARCH_SYSTEM
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "item_manager.h"
#include "packet.h"
#include "offline_shop_search.h"
#include "offlineshop_manager.h"
#include "buffer_manager.h"
#include "desc_manager.h"

bool COfflineShopManager::CheckItemNames(LPCHARACTER ch, const char* searchInput, std::vector<DWORD>& m_vecItemVnums)
{
	const std::string searchedFileName(searchInput);
	const size_t searchTextLength = searchedFileName.length();
	if (searchTextLength < 1)
		return false;
	const auto it = m_mapItemNames.find(ch->GetCharacterLanguage());
	if (it != m_mapItemNames.end())
	{
		for (DWORD j = 0; j < it->second.size(); ++j)
		{
			if (it->second[j].first.compare(0, searchTextLength, searchedFileName) == 0)
			{
				const auto itItem = std::find(m_vecItemVnums.begin(), m_vecItemVnums.end(), it->second[j].second);
				if (itItem == m_vecItemVnums.end())
					m_vecItemVnums.push_back(it->second[j].second);
			}
		}
	}
	return m_vecItemVnums.size() > 0 ? true : false;
}
//agregar lenguajes offline search
void COfflineShopManager::LoadSearchLanguage()
{
	const std::map<BYTE, std::string> m_vecLang = {
		{LOCALE_EN, "en"},
		{LOCALE_TR, "tr"},
		{LOCALE_ES, "es"},
		{LOCALE_PL, "pl"},
		{LOCALE_PT, "pt"},
		{LOCALE_RO, "ro"},
	};
	m_mapItemNames.clear();
	for (auto it = m_vecLang.begin(); it != m_vecLang.end(); ++it)
	{
		char filename[40];
		snprintf(filename, sizeof(filename), "locale/multi_lang/%s_item_names.txt", it->second.c_str());
		char	one_line[256];
		FILE* fp = fopen(filename, "r");
		if (fp != NULL)
		{
			std::vector<std::pair<std::string, DWORD>> m_ItemNames;
			while (fgets(one_line, 256, fp))
			{
				std::vector<std::string> m_vec;
				split_argument_special(one_line, m_vec, "\t");
				if (m_vec.size() < 2)
					continue;
				else if (m_vec[0].length() < 1 || m_vec[1].length() < 1)
					continue;
				DWORD itemvnum;
				str_to_number(itemvnum, m_vec[0].c_str());
				if (!itemvnum)
					continue;
				std::transform(m_vec[1].begin(), m_vec[1].end(), m_vec[1].begin(), [](unsigned char c) { return std::tolower(c); });
				m_ItemNames.emplace_back(std::make_pair(m_vec[1], itemvnum));
			}
			fclose(fp);
			if (m_ItemNames.size())
				m_mapItemNames.emplace(it->first, m_ItemNames);
		}
	}
}

void COfflineShopManager::SaveInMemory(OFFLINE_SHOP_ITEM* item)
{
	TItemTable * table = ITEM_MANAGER::instance().GetTable(item->vnum);
	if (!table){return;}
	if(table->bType == ITEM_WEAPON)
		m_itemMapWeapon.insert(std::make_pair(item->id, item));
	//else if(table->bType == ITEM_ARMOR || table->bType == ITEM_BELT || table->bType == ITEM_PENDANT || table->bType == ITEM_NEW_RINGS)
	else if(table->bType == ITEM_ARMOR || table->bType == ITEM_BELT || table->bType == ITEM_NEW_RINGS)
		m_itemMapEquipment.insert(std::make_pair(item->id, item));
	else if(table->bType == ITEM_COSTUME)
	{
		// after the add pet i will modify here.
		//if(table->bSubType == COSTUME_MOUNT || table->bSubType == COSTUME_PET)
		//	m_itemMapPets.insert(std::make_pair(item->id, item));
		//else
		//	m_itemMapCostume.insert(std::make_pair(item->id, item));
	}
	else if(table->bType == ITEM_DS || (table->bType == ITEM_MATERIAL && table->bSubType == MATERIAL_DS_REFINE_NORMAL))
		m_itemMapDragonsoul.insert(std::make_pair(item->id, item));
	else if(table->bType == ITEM_SKILLBOOK || table->dwVnum == 70037)
		m_itemMapBook.insert(std::make_pair(item->id, item));
	else
		m_itemMapOther.insert(std::make_pair(item->id, item));
}

void COfflineShopManager::RemoveInMemory(OFFLINE_SHOP_ITEM* item)
{
	TItemTable * table = ITEM_MANAGER::instance().GetTable(item->vnum);
	if (!table){return;}
	if(table->bType == ITEM_WEAPON)
	{
		auto it = m_itemMapWeapon.find(item->id);
		if(it!=m_itemMapWeapon.end())
			m_itemMapWeapon.erase(it);
	}

	//else if(table->bType == ITEM_ARMOR || table->bType == ITEM_PENDANT || table->bType == ITEM_BELT || table->bType == ITEM_NEW_RINGS)
	else if(table->bType == ITEM_ARMOR || table->bType == ITEM_BELT || table->bType == ITEM_NEW_RINGS)
	{
		auto it = m_itemMapEquipment.find(item->id);
		if(it!=m_itemMapEquipment.end())
			m_itemMapEquipment.erase(it);
	}
	else if(table->bType == ITEM_COSTUME)
	{
		//if(table->bSubType == COSTUME_PET || table->bSubType == COSTUME_MOUNT)
		//{
		//	auto it2 = m_itemMapPets.find(item->id);
		//	if(it2!=m_itemMapPets.end())
		//		m_itemMapPets.erase(it2);
		//}
		//else
		{
			auto it = m_itemMapCostume.find(item->id);
			if(it!=m_itemMapCostume.end())
				m_itemMapCostume.erase(it);
		}
	}
	else if(table->bType == ITEM_DS || (table->bType == ITEM_MATERIAL && table->bSubType == MATERIAL_DS_REFINE_NORMAL))
	{
		auto it2 = m_itemMapDragonsoul.find(item->id);
		if(it2!=m_itemMapDragonsoul.end())
			m_itemMapDragonsoul.erase(it2);
	}
	else if(table->bType == ITEM_SKILLBOOK || table->dwVnum == 70037)
	{
		auto it = m_itemMapBook.find(item->id);
		if (it != m_itemMapBook.end())
			m_itemMapBook.erase(it);
	}
	else
	{
		auto it = m_itemMapOther.find(item->id);
		if (it != m_itemMapOther.end())
			m_itemMapOther.erase(it);
	}
	
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	if (c_ref_set.size())
	{
		for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
		{
			auto desc = *it;
			if (desc)
			{
				LPCHARACTER ch = desc->GetCharacter();
				if (ch)
				{
					if (ch->IsLookingSearchItem(item->id))
						ch->ChatPacket(CHAT_TYPE_COMMAND, "OfflineShopBuyed %u", item->id);
				}
			}

		}
	}
}

bool ComparePrice(const TOfflineShopItem& i, const TOfflineShopItem& j){return (i.price < j.price);}
bool CompareCount(const TOfflineShopItem& i, const TOfflineShopItem& j){return (i.count < j.count);}

int GetRefineLevel(const char* name)
{
	char* p = const_cast<char*>(strrchr(name, '+'));
	if (!p)
		return 0;
	int	rtn = 0;
	str_to_number(rtn, p + 1);
	return rtn;
}

bool getFilter(TPacketCGShopSearch* pinfo, TItemTable* table, const OFFLINE_SHOP_ITEM* item)
{
	if (!table)
		return false;

	/// This check for sub category!!!!!!
	if (pinfo->ItemCat == 0)//Weapons
	{
		if (pinfo->SubCat == 0 && table->bSubType != WEAPON_SWORD)//sword
			return false;
		else if (pinfo->SubCat == 1 && table->bSubType != WEAPON_DAGGER)//dagger
			return false;
		else if (pinfo->SubCat == 2 && table->bSubType != WEAPON_BOW)//bow
			return false;
		else if (pinfo->SubCat == 3 && table->bSubType != WEAPON_TWO_HANDED)//two-handed
			return false;
		else if (pinfo->SubCat == 4 && table->bSubType != WEAPON_BELL)//bell
			return false;
		else if (pinfo->SubCat == 5 && table->bSubType != WEAPON_FAN)//fan
			return false;
	}
	else if (pinfo->ItemCat == 1)//Equipment
	{
		if (pinfo->SubCat == 0 && table->bSubType != ARMOR_BODY)
			return false;
		else if (pinfo->SubCat == 1 && table->bSubType != ARMOR_HEAD)
			return false;
		else if (pinfo->SubCat == 2 && table->bSubType != ARMOR_SHIELD)
			return false;
		else if (pinfo->SubCat == 3 && table->bSubType != ARMOR_WRIST)
			return false;
		else if (pinfo->SubCat == 4 && table->bSubType != ARMOR_FOOTS)
			return false;
		else if (pinfo->SubCat == 5 && table->bSubType != ARMOR_NECK)
			return false;
		else if (pinfo->SubCat == 6 && table->bSubType != ARMOR_EAR)
			return false;
		else if (pinfo->SubCat == 7 && table->bType != ITEM_NEW_RINGS)
			return false;
		else if (pinfo->SubCat == 8 && table->bSubType != ARMOR_PENDANT)
			return false;
	}


	for (int x = 0; x < ITEM_LIMIT_MAX_NUM; ++x)
	{
		if (table->aLimits[x].bType == LIMIT_LEVEL)
			if (table->aLimits[x].lValue < pinfo->level[0] || table->aLimits[x].lValue > pinfo->level[1])
				return false;
	}

	const int refineLevel = GetRefineLevel(table->szLocaleName);
	if (refineLevel != 0)
	{
		if (refineLevel < pinfo->refine[0] || refineLevel > pinfo->refine[1])
			return false;
	}

	if(!(item->price >= pinfo->price[0] && item->price <= pinfo->price[1]))
		return false;
	
	if (table->sAddonType == -1)
	{
		if (item->aAttr[0].bType == APPLY_NORMAL_HIT_DAMAGE_BONUS)
		{
			if (item->aAttr[0].sValue < pinfo->avg[0] || item->aAttr[0].sValue > pinfo->avg[1])
				return false;
		}
		if (item->aAttr[1].bType == APPLY_SKILL_DAMAGE_BONUS)
		{
			if (item->aAttr[1].sValue < pinfo->skill[0] || item->aAttr[1].sValue > pinfo->skill[1])
				return false;
		}
	}
	if (table->bType == ITEM_COSTUME && table->bSubType==COSTUME_SASH)
	{
		if (item->alSockets[0] < pinfo->abs[0] || item->alSockets[0] > pinfo->abs[1])
			return false;
	}
	if (pinfo->sex != 0)
	{
		if (pinfo->sex == 1 && IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_FEMALE))
			return false;
		if (pinfo->sex == 2 && IS_SET(table->dwAntiFlags, ITEM_ANTIFLAG_MALE))
			return false;
	}
	if (pinfo->enchant != 0)
	{
		if (pinfo->enchant == 1 && item->aAttr[0].bType >= 1)
			return false;
		if (pinfo->enchant == 2 && item->aAttr[0].bType < 1)
			return false;
	}
	if (table->bType == ITEM_DS)
	{
		if (pinfo->alchemy != 0 && table->bSubType != pinfo->alchemy)
			return false;
	}
	if (pinfo->character != 0)
	{
		if (pinfo->character == 1)//warrior
		{
			if (table->dwAntiFlags & ITEM_ANTIFLAG_WARRIOR)
				return false;
		}
		else if (pinfo->character == 2)//assassin
		{
			if (table->dwAntiFlags & ITEM_ANTIFLAG_ASSASSIN)
				return false;
		}
		else if (pinfo->character == 3)//shaman
		{
			if (table->dwAntiFlags & ITEM_ANTIFLAG_SHAMAN)
				return false;
		}
		else if (pinfo->character == 4)//sura
		{
			if (table->dwAntiFlags & ITEM_ANTIFLAG_SURA)
				return false;
		}
	}
	return true;
}

bool COfflineShopManager::getMap(TPacketCGShopSearch* pinfo, std::unordered_map<DWORD, OFFLINE_SHOP_ITEM*>::iterator& it, std::unordered_map<DWORD, OFFLINE_SHOP_ITEM*>::iterator& itend)
{
	if (pinfo->ItemCat == -1)
	{
		it = m_itemMap.begin();
		itend = m_itemMap.end();
	}
	else if (pinfo->ItemCat == 0)
	{
		it = m_itemMapWeapon.begin();
		itend = m_itemMapWeapon.end();
	}
	else if (pinfo->ItemCat == 1)
	{
		it = m_itemMapEquipment.begin();
		itend = m_itemMapEquipment.end();
	}
	else if (pinfo->ItemCat == 2)
	{
		it = m_itemMapCostume.begin();
		itend = m_itemMapCostume.end();
	}
	else if (pinfo->ItemCat == 3)
	{
		it = m_itemMapDragonsoul.begin();
		itend = m_itemMapDragonsoul.end();
	}
	else if (pinfo->ItemCat == 4)
	{
		it = m_itemMapOther.begin();
		itend = m_itemMapOther.end();
	}
	else if (pinfo->ItemCat == 5)
	{
		it = m_itemMapBook.begin();
		itend = m_itemMapBook.end();
	}
	else if (pinfo->ItemCat == 6)
	{
		it = m_itemMapPets.begin();
		itend = m_itemMapPets.end();
	}
	return true;
}

void COfflineShopManager::SearchItem(LPCHARACTER ch, const char * data)
{
	if(!ch || !data)
		return;
	TPacketCGShopSearch* pinfo = (TPacketCGShopSearch *)data;
	if (pinfo->ItemCat > ITEM_PET
		||  pinfo->avg[0] > 80||  pinfo->avg[1] > 80
		||  pinfo->skill[0] > 40||  pinfo->skill[1] > 40
		|| pinfo->abs[0] > 40 || pinfo->abs[1] > 40
		|| pinfo->level[0] > PLAYER_MAX_LEVEL_CONST || pinfo->level[1] > PLAYER_MAX_LEVEL_CONST
		|| pinfo->refine[0] > 200 || pinfo->refine[1] > 200
		|| pinfo->sex > 2 || pinfo->enchant > 2 || pinfo->alchemy > 7
		)
		return;

	const WORD maxSearchCount = 150;
	const char* playerName = ch->GetName();
	std::vector<const OFFLINE_SHOP_ITEM*> sendItems;
	std::unordered_map<DWORD, OFFLINE_SHOP_ITEM*>::iterator it;
	std::vector<DWORD> m_vecSearchItemVnums;
	if (CheckItemNames(ch, pinfo->searchInput, m_vecSearchItemVnums))
	{
		it = m_itemMap.begin();
		for (; it != m_itemMap.end(); ++it)
		{
			if (sendItems.size() >= maxSearchCount)
				break;
			const OFFLINE_SHOP_ITEM* item = it->second;
			if (!item)
				continue;
			else if (item->vnum == 0)
				continue;
			else if (strstr(playerName, item->szOwnerName))
				continue;
			const auto itCheck = std::find(m_vecSearchItemVnums.begin(), m_vecSearchItemVnums.end(), item->vnum);
			if (itCheck == m_vecSearchItemVnums.end())
				continue;
			if (getFilter(pinfo, ITEM_MANAGER::instance().GetTable(item->vnum), item))
				sendItems.push_back(item);
		}
	}
	else
	{
		std::unordered_map<DWORD, OFFLINE_SHOP_ITEM*>::iterator itend;
		if (getMap(pinfo, it, itend))
		{
			for (; it != itend; ++it)
			{
				if (sendItems.size() >= maxSearchCount)
					break;
				OFFLINE_SHOP_ITEM* item = it->second;
				if (!item)
					continue;
				else if (item->vnum == 0)
					continue;
				else if (strstr(playerName, item->szOwnerName))
					continue;

				if (getFilter(pinfo, ITEM_MANAGER::instance().GetTable(item->vnum), item))
					sendItems.push_back(item);
			}
		}
	}

	ch->SetLookingSearch(sendItems);

	if (sendItems.size() > 0)
	{
		TEMP_BUFFER buf;
		const WORD itemCount = sendItems.size();

		TPacketGCShopSearchItemSet pack;
		pack.header = HEADER_GC_SHOPSEARCH_SET;
		pack.size = sizeof(TPacketGCShopSearchItemSet)+sizeof(WORD)+(itemCount*sizeof(TOfflineShopItem));

		std::vector<TOfflineShopItem> m_vecData;

		for (BYTE j = 0; j < sendItems.size(); j++)
		{
			const OFFLINE_SHOP_ITEM*& item = sendItems[j];
			TOfflineShopItem p;

			p.id = item->id;
			p.vnum = item->vnum;
			p.price = item->price;
			p.count = item->count;
			p.display_pos = item->pos;
			p.owner_id = item->owner_id;
			p.status = 0;
			thecore_memcpy(&p.alSockets, &item->alSockets, sizeof(p.alSockets));
			thecore_memcpy(&p.aAttr, &item->aAttr, sizeof(p.aAttr));
			strlcpy(p.szBuyerName, item->szOwnerName, sizeof(p.szBuyerName));
#ifdef ENABLE_CHANGELOOK_SYSTEM
			p.transmutation = item->transmutation;
#endif
			m_vecData.emplace_back(p);
		}

		std::stable_sort(m_vecData.begin(), m_vecData.end(), ComparePrice);
		std::stable_sort(m_vecData.begin(), m_vecData.end(), CompareCount);

		buf.write(&pack, sizeof(TPacketGCShopSearchItemSet));
		buf.write(&itemCount, sizeof(WORD));
		buf.write(m_vecData.data(), itemCount * sizeof(TOfflineShopItem));
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
	}

	/*
	if (bVnum)
		sys_log(0, "SHOP_SEARCH: CharName: %s Search: Cat: %d SubCat: %d MaxRefine: %d MinLevel: %d MaxLevel: %d Enchant : %d alchemySub: %d Sex: %d Value: %d itemVnum: %d ", ch->GetName(), pinfo->ItemCat, pinfo->SubCat, pinfo->MaxRefine,pinfo->MinLevel,pinfo->MaxLevel,pinfo->Enchant,pinfo->alchemySub,pinfo->Sex,pinfo->Value,pinfo->itemVnum);
	else
		sys_log(0, "SHOP_SEARCH: CharName: %s Search: Cat: %d SubCat: %d MaxRefine: %d MinLevel: %d MaxLevel: %d Enchant : %d alchemySub: %d Sex: %d Value: %d  ", ch->GetName(), pinfo->ItemCat, pinfo->SubCat, pinfo->MaxRefine,pinfo->MinLevel,pinfo->MaxLevel,pinfo->Enchant,pinfo->alchemySub,pinfo->Sex,pinfo->Value);
	*/
}

void COfflineShopManager::ClearItem(DWORD id)
{
	auto it = m_itemMap.find(id);
	if (it != m_itemMap.end()) {
		OFFLINE_SHOP_ITEM* item = it->second;
		RemoveInMemory(item);
		m_itemMap.erase(it);
		M2_DELETE(item);
	}
}

void COfflineShopManager::InsertItem(OFFLINE_SHOP_ITEM* p)
{
	auto it = m_itemMap.find(p->id);
	if (it != m_itemMap.end()){
		sys_err("wtf have 2 id in game? item id %u",p->id);
		return;
	}
	OFFLINE_SHOP_ITEM* item = new OFFLINE_SHOP_ITEM;
	thecore_memcpy(item, p, sizeof(OFFLINE_SHOP_ITEM));
	m_itemMap.insert(std::make_pair(p->id, item));
	SaveInMemory(item);
}

#endif
#endif

