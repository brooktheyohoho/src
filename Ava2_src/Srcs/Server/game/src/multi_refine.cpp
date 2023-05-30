#include "stdafx.h"
#include "multi_refine.h"
#include "char.h"
#include "item_manager.h"
#include "item.h"
#include "boost/format.hpp"
#include "packet.h"
#include "desc_client.h"

CMultiRefineManager::CMultiRefineManager()
{
}

CMultiRefineManager::~CMultiRefineManager()
{
}

bool CMultiRefineManager::Initialize(TMultiRefineTable * table, int size)
{
	for (int i=0; i<size; ++i, ++table)
	{
		m_map_MultiRefineRecipe.insert(std::make_pair(table->id, *table));
	}
	
	return true;
}

const TMultiRefineTable* CMultiRefineManager::GetRefineRecipeNew(int vnum)
{
	itertype(m_map_MultiRefineRecipe) it = m_map_MultiRefineRecipe.find(vnum);

	if (it == m_map_MultiRefineRecipe.end())
	{
		return NULL;
	}

	return &it->second;
}

int CMultiRefineManager::GetMultiRefineSize(int vnum)
{
	int count = 0;

	for (itertype(m_map_MultiRefineRecipe) it = m_map_MultiRefineRecipe.begin(); it != m_map_MultiRefineRecipe.end(); ++it)
	{
		const TMultiRefineTable* prt = &it->second;

		if (prt->vnum == vnum)
		{
			count++;
		}
	}

	return count;
}

void CMultiRefineManager::SetInformationRefineMulti(LPCHARACTER ch,BYTE bSubType)
{

	TPacketGCRefineInformation p;
	p.header = HEADER_GC_REFINE_INFORMATION;
	p.bSubType = bSubType;

	if(bSubType == MULTIREFINE_SEND_DATES)
	{
		for (itertype(m_map_MultiRefineRecipe) it = m_map_MultiRefineRecipe.begin(); it != m_map_MultiRefineRecipe.end(); ++it)
		{
			const TMultiRefineTable* prt = &it->second;
			p.src_vnum = prt->vnum;
			LPDESC d = ch->GetDesc();
			d->Packet(&p, sizeof(TPacketGCRefineInformation));
		}
	}else{
		LPDESC d = ch->GetDesc();
		d->Packet(&p, sizeof(TPacketGCRefineInformation));
	}
}