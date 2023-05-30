/*********************************************************************
* date        : 2016.09.20
* file        : refine.cpp
* author      : VegaS
* version	  : 0.1.6
*/
#include "stdafx.h"
#include "refine.h"
#include "char.h"
#include "item_manager.h"
#include "item.h"
#include "boost/format.hpp"
CRefineManager::CRefineManager()
{
}

CRefineManager::~CRefineManager()
{
}

#ifdef ENABLE_FEATURES_REFINE_SYSTEM
	int ReadTypePotionDict[3] = {REFINE_VNUM_POTION_LOW, REFINE_VNUM_POTION_MEDIUM, REFINE_VNUM_POTION_EXTRA};

	int CRefineManager::Result(LPCHARACTER ch)
	{
		return ch->GetQuestFlag(PADDER_INCREASE_VALUE);
	}

	bool CRefineManager::GetPercentage(LPCHARACTER ch, BYTE lLow, BYTE lMedium, BYTE lExtra, BYTE lTotal, LPITEM item)
	{
		BYTE dwListType[3] = {lLow, lMedium, lExtra};
		
		if (!item)
			return false;
		
		if (lTotal > 100 || dwListType[0] < PADDER_MIN || dwListType[1] < PADDER_MIN || dwListType[2] < PADDER_MIN)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "923");		
			return false;
		}

		for (int i=0;i<=JOURNAL_MAX_NUM; i++)
		{
			if (dwListType[i] > PADDER_MIN)
			{
				//@fix 12.01.2017
				if (item->GetType() == ITEM_METIN)
					return false;

				if (ch->CountSpecifyItem(ReadTypePotionDict[i]) < PADDER_SAVE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "924 %s", ITEM_MANAGER::instance().GetTable(ReadTypePotionDict[i])->szLocaleName);	
					return false;
				}
			}
		}	

		return true;
	}

	void CRefineManager::Reset(LPCHARACTER ch)
	{
		for (int i=0; i<=JOURNAL_MAX_NUM; i++)
		{
			std::string stringBuf = str(boost::format(PADDER_MODE) % i);
			
			if (ch->GetQuestFlag(stringBuf.c_str()) > PADDER_MIN)
			{
				ch->SetQuestFlag(PADDER_INCREASE_VALUE, PADDER_MIN);
				ch->SetQuestFlag(stringBuf.c_str(), PADDER_MIN);
				ch->RemoveSpecifyItem(ReadTypePotionDict[i], PADDER_SAVE);
			}
		}
	}	

	void CRefineManager::Increase(LPCHARACTER ch, BYTE lLow, BYTE lMedium, BYTE lExtra)
	{
		BYTE dwListType[3] = {lLow, lMedium, lExtra};
		int dwListPercentage[3] = {REFINE_PERCENTAGE_LOW, REFINE_PERCENTAGE_MEDIUM, REFINE_PERCENTAGE_EXTRA};
		int dwTotalPercentage = 0;

		for (int i=0; i<=JOURNAL_MAX_NUM; i++)
		{
			if (dwListType[i] > PADDER_MIN)
			{
				std::string stringBuf = str(boost::format(PADDER_MODE) % i);
				ch->SetQuestFlag(stringBuf.c_str(), PADDER_SAVE);
				dwTotalPercentage += dwListPercentage[i];		
			}
		}
		
		if (ch->GetQuestFlag(PADDER_INCREASE_VALUE) < PADDER_SAVE) {
			ch->SetQuestFlag(PADDER_INCREASE_VALUE, dwTotalPercentage);
		}
		else {
			ch->ChatPacket(CHAT_TYPE_INFO, "925");
		}
	}
#endif

bool CRefineManager::Initialize(TRefineTable * table, int size)
{
	for (int i=0; i<size; ++i, ++table)
	{
		m_map_RefineRecipe.insert(std::make_pair(table->id, *table));
	}

	return true;
}

const TRefineTable* CRefineManager::GetRefineRecipe(DWORD vnum)
{
	if (vnum == 0)
		return NULL;

	itertype(m_map_RefineRecipe) it = m_map_RefineRecipe.find(vnum);

	if (it == m_map_RefineRecipe.end())
	{
		return NULL;
	}

	return &it->second;
}
