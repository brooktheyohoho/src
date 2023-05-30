
#include "stdafx.h"
#include "Cache.h"

#include "QID.h"
#include "ClientManager.h"
#ifdef __AUCTION__
#include "AuctionManager.h"
#endif
#include "Main.h"

extern CPacketInfo g_item_info;
extern int g_iPlayerCacheFlushSeconds;
extern int g_iItemCacheFlushSeconds;
extern int g_test_server;

extern int g_item_count;
const int auctionMinFlushSec = 1800;

CItemCache::CItemCache()
{
	m_expireTime = MIN(1800, g_iItemCacheFlushSeconds);
}

CItemCache::~CItemCache()
{
}

// 이거 이상한데...
// Delete를 했으면, Cache도 해제해야 하는것 아닌가???
// 근데 Cache를 해제하는 부분이 없어.
// 못 찾은 건가?
// 이렇게 해놓으면, 계속 시간이 될 때마다 아이템을 계속 지워...
// 이미 사라진 아이템인데... 확인사살??????
// fixme
// by rtsummit
void CItemCache::Delete()
{
	if (m_data.vnum == 0)
		return;

	//char szQuery[QUERY_MAX_LEN];
	//szQuery[QUERY_MAX_LEN] = '\0';
	if (g_test_server)
		sys_log(0, "ItemCache::Delete : DELETE %u", m_data.id);

	m_data.vnum = 0;
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();

	//m_bNeedQuery = false;
	//m_lastUpdateTime = time(0) - m_expireTime; // 바로 타임아웃 되도록 하자.
}

void CItemCache::OnFlush()
{
	if (m_data.vnum == 0) // vnum이 0이면 삭제하라고 표시된 것이다.
	{
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item%s WHERE id=%u", GetTablePostfix(), m_data.id);
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_DESTROY, 0, NULL);

		if (g_test_server)
			sys_log(0, "ItemCache::Flush : DELETE %u %s", m_data.id, szQuery);
	}
	else
	{
		long alSockets[ITEM_SOCKET_MAX_NUM];
		TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
		bool isSocket = false, isAttr = false;
#ifdef ENABLE_GLOVE_SYSTEM
		TPlayerItemAttribute randomAttr[GLOVE_ATTR_MAX_NUM];
		bool isRandomAttr = false;
		memset(&randomAttr, 0, sizeof(TPlayerItemAttribute) * GLOVE_ATTR_MAX_NUM);
#endif

		memset(&alSockets, 0, sizeof(long) * ITEM_SOCKET_MAX_NUM);
		memset(&aAttr, 0, sizeof(TPlayerItemAttribute) * ITEM_ATTRIBUTE_MAX_NUM);

		TPlayerItem * p = &m_data;

		if (memcmp(alSockets, p->alSockets, sizeof(long) * ITEM_SOCKET_MAX_NUM))
			isSocket = true;

		if (memcmp(aAttr, p->aAttr, sizeof(TPlayerItemAttribute) * ITEM_ATTRIBUTE_MAX_NUM))
			isAttr = true;

#ifdef ENABLE_GLOVE_SYSTEM
		if (memcmp(randomAttr, p->randomAttr, sizeof(TPlayerItemAttribute) * GLOVE_ATTR_MAX_NUM))
			isRandomAttr = true;
#endif

		char szColumns[QUERY_MAX_LEN];
		char szValues[QUERY_MAX_LEN];
		char szUpdate[QUERY_MAX_LEN];

		int iLen = snprintf(szColumns, sizeof(szColumns), "id, owner_id, window, pos, count, vnum");

		int iValueLen = snprintf(szValues, sizeof(szValues), "%u, %u, %d, %d, %u, %u",
				p->id, p->owner, p->window, p->pos, p->count, p->vnum);

		int iUpdateLen = snprintf(szUpdate, sizeof(szUpdate), "owner_id=%u, window=%d, pos=%d, count=%u, vnum=%u",
				p->owner, p->window, p->pos, p->count, p->vnum);

#ifdef ELEMENT_SPELL_WORLDARD
		iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ", grade_elements, attackelement1, attackelement2, attackelement3, elementstype, elementsvalue1, elementsvalue2, elementsvalue3");
		iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,
					", %d, %d, %d, %d, %d, %d ,%d, %d", p->grade_element, p->attack_element[0], p->attack_element[1], p->attack_element[2] ,p->element_type_bonus, p->elements_value_bonus[0], p->elements_value_bonus[1], p->elements_value_bonus[2]);
		iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,
					", grade_elements=%d, attackelement1=%d, attackelement2=%d, attackelement3=%d, elementstype=%d, elementsvalue1=%d, elementsvalue2=%d, elementsvalue3=%d",p->grade_element,  p->attack_element[0], p->attack_element[1], p->attack_element[2], p->element_type_bonus, p->elements_value_bonus[0], p->elements_value_bonus[1], p->elements_value_bonus[2]);

#endif

#ifdef ENABLE_NEW_NAME_ITEM
		iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ", name");
		iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,", '%s'", p->name);
		iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,", name='%s'", p->name);
#endif

		if (isSocket)
		{
			for(BYTE j=0;j<ITEM_SOCKET_MAX_NUM;++j)
			{
				iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ", socket%d",j);
				iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,", %lu", p->alSockets[j]);
				iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,", socket%d=%lu", j, p->alSockets[j]);
			}
		}

		if (isAttr)
		{
			for(BYTE j=0;j<ITEM_ATTRIBUTE_MAX_NUM;++j)
			{
				iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen,", attrtype%d, attrvalue%d",j,j);
				iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,", %d, %d",p->aAttr[j].bType, p->aAttr[j].sValue);
				iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,", attrtype%d=%d, attrvalue%d=%d",j, p->aAttr[j].bType, j, p->aAttr[j].sValue);
			}
		}

#ifdef ENABLE_GLOVE_SYSTEM
		if (isRandomAttr)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen,
					", randomtype0, randomvalue0, randomtype1, randomvalue1, randomtype2, randomvalue2, randomtype3, randomvalue3"
					", randomtype4, randomvalue4, randomtype5, randomvalue5, randomtype6, randomvalue6");

			iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,
					", %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
					p->randomAttr[0].bType, p->randomAttr[0].sValue,
					p->randomAttr[1].bType, p->randomAttr[1].sValue,
					p->randomAttr[2].bType, p->randomAttr[2].sValue,
					p->randomAttr[3].bType, p->randomAttr[3].sValue,
					p->randomAttr[4].bType, p->randomAttr[4].sValue,
					p->randomAttr[5].bType, p->randomAttr[5].sValue,
					p->randomAttr[6].bType, p->randomAttr[6].sValue);

			iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,
					", randomtype0=%d, randomvalue0=%d"
					", randomtype1=%d, randomvalue1=%d"
					", randomtype2=%d, randomvalue2=%d"
					", randomtype3=%d, randomvalue3=%d"
					", randomtype4=%d, randomvalue4=%d"
					", randomtype5=%d, randomvalue5=%d"
					", randomtype6=%d, randomvalue6=%d",
					p->randomAttr[0].bType, p->randomAttr[0].sValue,
					p->randomAttr[1].bType, p->randomAttr[1].sValue,
					p->randomAttr[2].bType, p->randomAttr[2].sValue,
					p->randomAttr[3].bType, p->randomAttr[3].sValue,
					p->randomAttr[4].bType, p->randomAttr[4].sValue,
					p->randomAttr[5].bType, p->randomAttr[5].sValue,
					p->randomAttr[6].bType, p->randomAttr[6].sValue);
		}

#endif
		char szItemQuery[QUERY_MAX_LEN + QUERY_MAX_LEN];
		snprintf(szItemQuery, sizeof(szItemQuery), "REPLACE INTO item%s (%s) VALUES(%s)", GetTablePostfix(), szColumns, szValues);

		if (g_test_server)
			sys_log(0, "ItemCache::Flush :REPLACE  (%s)", szItemQuery);

		CDBManager::instance().ReturnQuery(szItemQuery, QID_ITEM_SAVE, 0, NULL);

		//g_item_info.Add(p->vnum);
		++g_item_count;
	}

	m_bNeedQuery = false;
}

//
// CPlayerTableCache
//
CPlayerTableCache::CPlayerTableCache()
{
	m_expireTime = MIN(1800, g_iPlayerCacheFlushSeconds);
}

CPlayerTableCache::~CPlayerTableCache()
{
}

void CPlayerTableCache::OnFlush()
{
	if (g_test_server)
		sys_log(0, "PlayerTableCache::Flush : %s", m_data.name);

	char szQuery[QUERY_MAX_LEN];
	CreatePlayerSaveQuery(szQuery, sizeof(szQuery), &m_data);
	CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, NULL);
}

#ifdef __AUCTION__
CAuctionItemInfoCache::CAuctionItemInfoCache()
{
	m_expireTime = MIN (auctionMinFlushSec, g_iItemCacheFlushSeconds);
}

CAuctionItemInfoCache::~CAuctionItemInfoCache()
{

}

void CAuctionItemInfoCache::Delete()
{
	if (m_data.item_num == 0)
		return;

	if (g_test_server)
		sys_log(0, "CAuctionItemInfoCache::Delete : DELETE %u", m_data.item_id);

	m_data.item_num = 0;
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
	delete this;
}

void CAuctionItemInfoCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];

	if (m_data.item_num == 0)
	{
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM auction where item_id = %d", m_data.item_id);
		CDBManager::instance().AsyncQuery(szQuery);
	}
	else
	{
		snprintf(szQuery, sizeof(szQuery), "REPLACE INTO auction VALUES (%u, %d, %d, %u, \"%s\", %u, %u, %u, %u)",
			m_data.item_num, m_data.offer_price, m_data.price, m_data.offer_id, m_data.shown_name, (DWORD)m_data.empire, (DWORD)m_data.expired_time,
			m_data.item_id, m_data.bidder_id);

		CDBManager::instance().AsyncQuery(szQuery);
	}
}

CSaleItemInfoCache::CSaleItemInfoCache()
{
	m_expireTime = MIN (auctionMinFlushSec, g_iItemCacheFlushSeconds);
}

CSaleItemInfoCache::~CSaleItemInfoCache()
{
}

void CSaleItemInfoCache::Delete()
{
}

void CSaleItemInfoCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO sale VALUES (%u, %d, %d, %u, \"%s\", %u, %u, %u, %u)",
		m_data.item_num, m_data.offer_price, m_data.price, m_data.offer_id, m_data.shown_name, (DWORD)m_data.empire, (DWORD)m_data.expired_time,
		m_data.item_id, m_data.wisher_id);

	CDBManager::instance().AsyncQuery(szQuery);
}

CWishItemInfoCache::CWishItemInfoCache()
{
	m_expireTime = MIN (auctionMinFlushSec, g_iItemCacheFlushSeconds);
}

CWishItemInfoCache::~CWishItemInfoCache()
{
}

void CWishItemInfoCache::Delete()
{
}

void CWishItemInfoCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO wish VALUES (%u, %d, %d, %u, \"%s\", %u, %d)",
		m_data.item_num, m_data.offer_price, m_data.price, m_data.offer_id, m_data.shown_name, (DWORD)m_data.empire, (DWORD)m_data.expired_time);

	CDBManager::instance().AsyncQuery(szQuery);
}
#endif
