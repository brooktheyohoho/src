
#include "stdafx.h"
#include "../../common/CommonDefines.h"

#include "../../common/billing.h"
#include "../../common/building.h"
#include "../../common/VnumHelper.h"
#include "../../libgame/include/grid.h"

#include "ClientManager.h"

#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "GuildManager.h"
#include "PrivManager.h"
#include "MoneyLog.h"
#include "ItemAwardManager.h"
#include "Marriage.h"
#include "Monarch.h"
#include "BlockCountry.h"
#include "ItemIDRangeManager.h"
#include "Cache.h"
#include <sstream>
#ifdef __AUCTION__
#include "AuctionManager.h"
#endif
extern int g_iPlayerCacheFlushSeconds;
extern int g_iItemCacheFlushSeconds;
extern int g_test_server;
extern int g_log;
extern std::string g_stLocale;
extern std::string g_stLocaleNameColumn;
bool CreateItemTableFromRes(MYSQL_RES * res, std::vector<TPlayerItem> * pVec, DWORD dwPID);

DWORD g_dwUsageMax = 0;
DWORD g_dwUsageAvg = 0;

CPacketInfo g_query_info;
CPacketInfo g_item_info;

int g_item_count = 0;
int g_query_count[2];
#ifdef ENABLE_PROTO_FROM_DB
bool g_bMirror2DB = false;
#endif

CClientManager::CClientManager() :
	m_pkAuthPeer(NULL),
	m_iPlayerIDStart(0),
	m_iPlayerDeleteLevelLimit(0),
	m_iPlayerDeleteLevelLimitLower(0),
	m_bChinaEventServer(false),
	m_iShopTableSize(0),
	m_pShopTable(NULL),
	m_iRefineTableSize(0),
	m_pRefineTable(NULL),
#ifdef ENABLE_MULTI_REFINE_WORLDARD
	m_iMultiRefineTableSize(0),
	m_pMultiRefineTable(NULL),
#endif
	m_bShutdowned(FALSE),
	m_iCacheFlushCount(0),
	m_iCacheFlushCountLimit(200)

{
	m_itemRange.dwMin = 0;
	m_itemRange.dwMax = 0;
	m_itemRange.dwUsableItemIDMin = 0;

	memset(g_query_count, 0, sizeof(g_query_count));
#ifdef ENABLE_PROTO_FROM_DB
	bIsProtoReadFromDB = false;
#endif
}

CClientManager::~CClientManager()
{
	Destroy();
}

void CClientManager::SetPlayerIDStart(int iIDStart)
{
	m_iPlayerIDStart = iIDStart;
}

void CClientManager::GetPeerP2PHostNames(std::string& peerHostNames)
{
	std::ostringstream oss(std::ostringstream::out);

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		CPeer * peer = *it;
		oss << peer->GetHost() << " " << peer->GetP2PPort() << " channel : " << (int)(peer->GetChannel()) << "\n";
	}

	peerHostNames += oss.str();
}

void CClientManager::Destroy()
{
#ifdef ENABLE_ITEMSHOP
	m_IShopManager.clear();
	m_IShopLogManager.clear();
#endif
	m_mChannelStatus.clear();
	for (itertype(m_peerList) i = m_peerList.begin(); i != m_peerList.end(); ++i)
		(*i)->Destroy();

	m_peerList.clear();

	if (m_fdAccept > 0)
	{
		socket_close(m_fdAccept);
		m_fdAccept = -1;
	}
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	for (auto it = m_Offlineshop.begin(); it != m_Offlineshop.end(); ++it) {
		if (it->second) {
			char szQuery[1024];
			snprintf(szQuery, sizeof(szQuery), "UPDATE player.offline_shop_npc SET time = %d WHERE owner_id = %u",it->second->time, it->second->owner_id);
			std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
			delete it->second;
		}
	}
	m_Offlineshop.clear();
#endif
}

#define ENABLE_DEFAULT_PRIV
#ifdef ENABLE_DEFAULT_PRIV
static bool bCleanOldPriv = true;
static bool __InitializeDefaultPriv()
{
	if (bCleanOldPriv)
	{
		std::auto_ptr<SQLMsg> pCleanStuff(CDBManager::instance().DirectQuery("DELETE FROM priv_settings WHERE value <= 0 OR duration <= NOW();", SQL_COMMON));
		printf("DEFAULT_PRIV_EMPIRE: removed %u expired priv settings.\n", pCleanStuff->Get()->uiAffectedRows);
	}
	std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery("SELECT priv_type, id, type, value, UNIX_TIMESTAMP(duration) FROM priv_settings", SQL_COMMON));
	if (pMsg->Get()->uiNumRows == 0)
		return false;
	MYSQL_ROW row = NULL;
	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		if (!strcmp(row[0], "EMPIRE"))
		{
			// init
			BYTE empire = 0;
			BYTE type = 1;
			int value = 0;
			time_t duration_sec = 0;
			// set
			str_to_number(empire, row[1]);
			str_to_number(type, row[2]);
			str_to_number(value, row[3]);
			str_to_number(duration_sec, row[4]);
			// recalibrate time
			time_t now_time_sec = CClientManager::instance().GetCurrentTime();
			if (now_time_sec>duration_sec)
				duration_sec = 0;
			else
				duration_sec -= now_time_sec;
			// send priv
			printf("DEFAULT_PRIV_EMPIRE: set empire(%u), type(%u), value(%d), duration(%u)\n", empire, type, value, duration_sec);
			CPrivManager::instance().AddEmpirePriv(empire, type, value, duration_sec);
		}
		else if (!strcmp(row[0], "GUILD"))
		{
			// init
			DWORD guild_id = 0;
			BYTE type = 1;
			int value = 0;
			time_t duration_sec = 0;
			// set
			str_to_number(guild_id, row[1]);
			str_to_number(type, row[2]);
			str_to_number(value, row[3]);
			str_to_number(duration_sec, row[4]);
			// recalibrate time
			time_t now_time_sec = CClientManager::instance().GetCurrentTime();
			if (now_time_sec>duration_sec)
				duration_sec = 0;
			else
				duration_sec -= now_time_sec;
			// send priv
			if (guild_id)
			{
				printf("DEFAULT_PRIV_GUILD: set guild_id(%u), type(%u), value(%d), duration(%u)\n", guild_id, type, value, duration_sec);
				CPrivManager::instance().AddGuildPriv(guild_id, type, value, duration_sec);
			}
		}
		else if (!strcmp(row[0], "PLAYER"))
		{
			// init
			DWORD pid = 0;
			BYTE type = 1;
			int value = 0;
			// set
			str_to_number(pid, row[1]);
			str_to_number(type, row[2]);
			str_to_number(value, row[3]);
			// send priv
			if (pid)
			{
				printf("DEFAULT_PRIV_PLAYER: set pid(%u), type(%u), value(%d)\n", pid, type, value);
				CPrivManager::instance().AddCharPriv(pid, type, value);
			}
		}
	}
	return true;
}

static bool __UpdateDefaultPriv(const char* priv_type, DWORD id, BYTE type, int value, time_t duration_sec)
{
	char szQuery[1024];
	snprintf(szQuery, 1024,
		"REPLACE INTO priv_settings SET priv_type='%s', id=%u, type=%u, value=%d, duration=DATE_ADD(NOW(), INTERVAL %u SECOND);",
		priv_type, id, type, value, duration_sec
	);
	std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_COMMON));
	return pMsg->Get()->uiAffectedRows;
}
#endif

bool CClientManager::Initialize()
{
	int tmpValue;

	//BOOT_LOCALIZATION
	if (!InitializeLocalization())
	{
		fprintf(stderr, "Failed Localization Infomation so exit\n");
		return false;
	}
#ifdef ENABLE_DEFAULT_PRIV
	if (!__InitializeDefaultPriv())
	{
		fprintf(stderr, "Failed Default Priv Setting so exit\n");
		// return false;
	}
#endif
	//END_BOOT_LOCALIZATION
	//ITEM_UNIQUE_ID

	if (!InitializeNowItemID())
	{
		fprintf(stderr, " Item range Initialize Failed. Exit DBCache Server\n");
		return false;
	}
	//END_ITEM_UNIQUE_ID

#ifdef ENABLE_PROTO_FROM_DB
	int iTemp;
	if (CConfig::instance().GetValue("PROTO_FROM_DB", &iTemp))
	{
		bIsProtoReadFromDB = !!iTemp;
		fprintf(stdout, "PROTO_FROM_DB: %s\n", (bIsProtoReadFromDB)?"Enabled":"Disabled");
	}
	if (!bIsProtoReadFromDB && CConfig::instance().GetValue("MIRROR2DB", &iTemp))
	{
		g_bMirror2DB = !!iTemp;
		fprintf(stdout, "MIRROR2DB: %s\n", (g_bMirror2DB)?"Enabled":"Disabled");
	}
#endif
	if (!InitializeTables())
	{
		sys_err("Table Initialize FAILED");
		return false;
	}

	CGuildManager::instance().BootReserveWar();

	if (!CConfig::instance().GetValue("BIND_PORT", &tmpValue))
		tmpValue = 5300;

	char szBindIP[128];

	if (!CConfig::instance().GetValue("BIND_IP", szBindIP, 128))
		strlcpy(szBindIP, "0", sizeof(szBindIP));

	m_fdAccept = socket_tcp_bind(szBindIP, tmpValue);

	if (m_fdAccept < 0)
	{
		perror("socket");
		return false;
	}

	sys_log(0, "ACCEPT_HANDLE: %u", m_fdAccept);
	fdwatch_add_fd(m_fdWatcher, m_fdAccept, NULL, FDW_READ, false);

	if (!CConfig::instance().GetValue("BACKUP_LIMIT_SEC", &tmpValue))
		tmpValue = 600;

	m_looping = true;

	if (!CConfig::instance().GetValue("PLAYER_DELETE_LEVEL_LIMIT", &m_iPlayerDeleteLevelLimit))
	{
		sys_err("conf.txt: Cannot find PLAYER_DELETE_LEVEL_LIMIT, use default level %d", PLAYER_MAX_LEVEL_CONST + 1);
		m_iPlayerDeleteLevelLimit = PLAYER_MAX_LEVEL_CONST + 1;
	}

	if (!CConfig::instance().GetValue("PLAYER_DELETE_LEVEL_LIMIT_LOWER", &m_iPlayerDeleteLevelLimitLower))
	{
		m_iPlayerDeleteLevelLimitLower = 0;
	}

	sys_log(0, "PLAYER_DELETE_LEVEL_LIMIT set to %d", m_iPlayerDeleteLevelLimit);
	sys_log(0, "PLAYER_DELETE_LEVEL_LIMIT_LOWER set to %d", m_iPlayerDeleteLevelLimitLower);

	m_bChinaEventServer = false;

	int	iChinaEventServer = 0;

	if (CConfig::instance().GetValue("CHINA_EVENT_SERVER", &iChinaEventServer))
		m_bChinaEventServer = (iChinaEventServer);

	sys_log(0, "CHINA_EVENT_SERVER %s", CClientManager::instance().IsChinaEventServer()?"true":"false");


	LoadEventFlag();

#ifdef ENABLE_AUTOMATIC_EVENT
	if (!InitializeAutomaticEvent())
	{
		sys_err("InitializeAutomaticEvent FAILED");
		return false;
	}
#endif

	// database character-set을 강제로 맞춤
	if (g_stLocale == "big5" || g_stLocale == "sjis")
	    CDBManager::instance().QueryLocaleSet();

	return true;
}

void CClientManager::MainLoop()
{
	SQLMsg * tmp;

	sys_log(0, "ClientManager pointer is %p", this);

	// 메인루프
	while (!m_bShutdowned)
	{
		while ((tmp = CDBManager::instance().PopResult()))
		{
			AnalyzeQueryResult(tmp);
			delete tmp;
		}

		if (!Process())
			break;

		log_rotate();
	}

	//
	// 메인루프 종료처리
	//
	sys_log(0, "MainLoop exited, Starting cache flushing");

	signal_timer_disable();

	itertype(m_map_playerCache) it = m_map_playerCache.begin();

	//플레이어 테이블 캐쉬 플러쉬
	while (it != m_map_playerCache.end())
	{
		CPlayerTableCache * c = (it++)->second;

		c->Flush();
		delete c;
	}
	m_map_playerCache.clear();


	itertype(m_map_itemCache) it2 = m_map_itemCache.begin();
	//아이템 플러쉬
	while (it2 != m_map_itemCache.end())
	{
		CItemCache * c = (it2++)->second;

		c->Flush();
		delete c;
	}
	m_map_itemCache.clear();
}

void CClientManager::Quit()
{
	m_bShutdowned = TRUE;
}

void CClientManager::QUERY_BOOT(CPeer* peer, TPacketGDBoot * p)
{
	const BYTE bPacketVersion = 6; // BOOT 패킷이 바뀔때마다 번호를 올리도록 한다.

	std::vector<tAdminInfo> vAdmin;
	std::vector<std::string> vHost;

	__GetHostInfo(vHost);
	__GetAdminInfo(p->szIP, vAdmin);

	sys_log(0, "QUERY_BOOT : AdminInfo (Request ServerIp %s) ", p->szIP);

	DWORD dwPacketSize =
		sizeof(DWORD) +
		sizeof(BYTE) +
		sizeof(WORD) + sizeof(WORD) + sizeof(TMobTable) * m_vec_mobTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TItemTable) * m_vec_itemTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TShopTable) * m_iShopTableSize +
		sizeof(WORD) + sizeof(WORD) + sizeof(TSkillTable) * m_vec_skillTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TRefineTable) * m_iRefineTableSize +

#ifdef ENABLE_MULTI_REFINE_WORLDARD
		sizeof(WORD) + sizeof(WORD) + sizeof(TMultiRefineTable) * m_iMultiRefineTableSize +
#endif

		sizeof(WORD) + sizeof(WORD) + sizeof(TItemAttrTable) * m_vec_itemAttrTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TItemAttrTable) * m_vec_itemRareTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TItemAttrTable) * m_vec_itemCostumeTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TItemAttrTable) * m_vec_itemCostumeHairTable.size() +

		sizeof(WORD) + sizeof(WORD) + sizeof(TItemAttrTable) * m_vec_itemRingsAttrTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TItemAttrTable) * m_vec_itemRingsRareTable.size() +

		sizeof(WORD) + sizeof(WORD) + sizeof(TBanwordTable) * m_vec_banwordTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(building::TLand) * m_vec_kLandTable.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(building::TObjectProto) * m_vec_kObjectProto.size() +
		sizeof(WORD) + sizeof(WORD) + sizeof(building::TObject) * m_map_pkObjectTable.size() +
#ifdef __AUCTION__
		sizeof(WORD) + sizeof(WORD) + sizeof(TPlayerItem) * AuctionManager::instance().GetAuctionItemSize() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TAuctionItemInfo) * AuctionManager::instance().GetAuctionSize() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TSaleItemInfo) * AuctionManager::instance().GetSaleSize() +
		sizeof(WORD) + sizeof(WORD) + sizeof(TWishItemInfo) * AuctionManager::instance().GetWishSize() +
		sizeof(WORD) + sizeof(WORD) + (sizeof(DWORD) + sizeof(DWORD) + sizeof(int)) * AuctionManager::instance().GetMyBidSize() +
#endif
		sizeof(time_t) +
		sizeof(WORD) + sizeof(WORD) + sizeof(TItemIDRangeTable)*2 +
		//ADMIN_MANAGER
		sizeof(WORD) + sizeof(WORD) + 16 * vHost.size() +
		sizeof(WORD) + sizeof(WORD) +  sizeof(tAdminInfo) *  vAdmin.size() +
		//END_ADMIN_MANAGER
		sizeof(WORD) + sizeof(WORD) + sizeof(TMonarchInfo) +
		sizeof(WORD) + sizeof(WORD) + sizeof(MonarchCandidacy)* CMonarch::instance().MonarchCandidacySize() +
#ifdef ENABLE_OFFLINESHOP_SYSTEM
		sizeof(WORD) + sizeof(WORD) + sizeof(TOfflineShop) * m_Offlineshop.size() +
#endif
		
		sizeof(WORD);

	peer->EncodeHeader(HEADER_DG_BOOT, 0, dwPacketSize);
	peer->Encode(&dwPacketSize, sizeof(DWORD));
	peer->Encode(&bPacketVersion, sizeof(BYTE));

	sys_log(0, "BOOT: PACKET: %d", dwPacketSize);
	sys_log(0, "BOOT: VERSION: %d", bPacketVersion);

	sys_log(0, "sizeof(TMobTable) = %d", sizeof(TMobTable));
	sys_log(0, "sizeof(TItemTable) = %d", sizeof(TItemTable));
	sys_log(0, "sizeof(TShopTable) = %d", sizeof(TShopTable));
	sys_log(0, "sizeof(TSkillTable) = %d", sizeof(TSkillTable));
	sys_log(0, "sizeof(TRefineTable) = %d", sizeof(TRefineTable));
#ifdef ENABLE_MULTI_REFINE_WORLDARD
	sys_log(0, "sizeof(TMultiRefineTable) = %d", sizeof(TMultiRefineTable));
#endif
	sys_log(0, "sizeof(TItemAttrTable) = %d", sizeof(TItemAttrTable));
	sys_log(0, "sizeof(TItemRareTable) = %d", sizeof(TItemAttrTable));
	sys_log(0, "sizeof(TItemCostumeTable) = %d", sizeof(TItemAttrTable));
	sys_log(0, "sizeof(TBanwordTable) = %d", sizeof(TBanwordTable));
	sys_log(0, "sizeof(TLand) = %d", sizeof(building::TLand));
	sys_log(0, "sizeof(TObjectProto) = %d", sizeof(building::TObjectProto));
	sys_log(0, "sizeof(TObject) = %d", sizeof(building::TObject));
	//ADMIN_MANAGER
	sys_log(0, "sizeof(tAdminInfo) = %d * %d ", sizeof(tAdminInfo) * vAdmin.size());
	//END_ADMIN_MANAGER
	sys_log(0, "sizeof(TMonarchInfo) = %d * %d", sizeof(TMonarchInfo));
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	sys_log(0, "sizeof(TOfflineShop) = %d ", sizeof(TOfflineShop));
#endif
	peer->EncodeWORD(sizeof(TMobTable));
	peer->EncodeWORD(m_vec_mobTable.size());
	peer->Encode(&m_vec_mobTable[0], sizeof(TMobTable) * m_vec_mobTable.size());

	peer->EncodeWORD(sizeof(TItemTable));
	peer->EncodeWORD(m_vec_itemTable.size());
	peer->Encode(&m_vec_itemTable[0], sizeof(TItemTable) * m_vec_itemTable.size());

	peer->EncodeWORD(sizeof(TShopTable));
	peer->EncodeWORD(m_iShopTableSize);
	peer->Encode(m_pShopTable, sizeof(TShopTable) * m_iShopTableSize);

	peer->EncodeWORD(sizeof(TSkillTable));
	peer->EncodeWORD(m_vec_skillTable.size());
	peer->Encode(&m_vec_skillTable[0], sizeof(TSkillTable) * m_vec_skillTable.size());

	peer->EncodeWORD(sizeof(TRefineTable));
	peer->EncodeWORD(m_iRefineTableSize);
	peer->Encode(m_pRefineTable, sizeof(TRefineTable) * m_iRefineTableSize);

#ifdef ENABLE_MULTI_REFINE_WORLDARD
	peer->EncodeWORD(sizeof(TMultiRefineTable));
	peer->EncodeWORD(m_iMultiRefineTableSize);
	peer->Encode(m_pMultiRefineTable, sizeof(TMultiRefineTable) * m_iMultiRefineTableSize);
#endif	
	
	peer->EncodeWORD(sizeof(TItemAttrTable));
	peer->EncodeWORD(m_vec_itemAttrTable.size());
	peer->Encode(&m_vec_itemAttrTable[0], sizeof(TItemAttrTable) * m_vec_itemAttrTable.size());

	peer->EncodeWORD(sizeof(TItemAttrTable));
	peer->EncodeWORD(m_vec_itemRareTable.size());
	peer->Encode(&m_vec_itemRareTable[0], sizeof(TItemAttrTable) * m_vec_itemRareTable.size());

	peer->EncodeWORD(sizeof(TItemAttrTable));
	peer->EncodeWORD(m_vec_itemCostumeTable.size());
	peer->Encode(&m_vec_itemCostumeTable[0], sizeof(TItemAttrTable) * m_vec_itemCostumeTable.size());

	peer->EncodeWORD(sizeof(TItemAttrTable));
	peer->EncodeWORD(m_vec_itemCostumeHairTable.size());
	peer->Encode(&m_vec_itemCostumeHairTable[0], sizeof(TItemAttrTable) * m_vec_itemCostumeHairTable.size());

	peer->EncodeWORD(sizeof(TItemAttrTable));
	peer->EncodeWORD(m_vec_itemRingsAttrTable.size());
	peer->Encode(&m_vec_itemRingsAttrTable[0], sizeof(TItemAttrTable) * m_vec_itemRingsAttrTable.size());

	peer->EncodeWORD(sizeof(TItemAttrTable));
	peer->EncodeWORD(m_vec_itemRingsRareTable.size());
	peer->Encode(&m_vec_itemRingsRareTable[0], sizeof(TItemAttrTable) * m_vec_itemRingsRareTable.size());

	peer->EncodeWORD(sizeof(TBanwordTable));
	peer->EncodeWORD(m_vec_banwordTable.size());
	peer->Encode(&m_vec_banwordTable[0], sizeof(TBanwordTable) * m_vec_banwordTable.size());

	peer->EncodeWORD(sizeof(building::TLand));
	peer->EncodeWORD(m_vec_kLandTable.size());
	peer->Encode(&m_vec_kLandTable[0], sizeof(building::TLand) * m_vec_kLandTable.size());

	peer->EncodeWORD(sizeof(building::TObjectProto));
	peer->EncodeWORD(m_vec_kObjectProto.size());
	peer->Encode(&m_vec_kObjectProto[0], sizeof(building::TObjectProto) * m_vec_kObjectProto.size());

	peer->EncodeWORD(sizeof(building::TObject));
	peer->EncodeWORD(m_map_pkObjectTable.size());

	itertype(m_map_pkObjectTable) it = m_map_pkObjectTable.begin();

	while (it != m_map_pkObjectTable.end())
		peer->Encode((it++)->second, sizeof(building::TObject));

	// Auction Boot
#ifdef __AUCTION__
	AuctionManager::instance().Boot (peer);
#endif
	time_t now = time(0);
	peer->Encode(&now, sizeof(time_t));

	TItemIDRangeTable itemRange = CItemIDRangeManager::instance().GetRange();
	TItemIDRangeTable itemRangeSpare = CItemIDRangeManager::instance().GetRange();

	peer->EncodeWORD(sizeof(TItemIDRangeTable));
	peer->EncodeWORD(1);
	peer->Encode(&itemRange, sizeof(TItemIDRangeTable));
	peer->Encode(&itemRangeSpare, sizeof(TItemIDRangeTable));

	peer->SetItemIDRange(itemRange);
	peer->SetSpareItemIDRange(itemRangeSpare);

	//ADMIN_MANAGER
	peer->EncodeWORD(16);
	peer->EncodeWORD(vHost.size());

	for (size_t n = 0; n < vHost.size(); ++n)
	{
		peer->Encode(vHost[n].c_str(), 16);
		sys_log(0, "GMHosts %s", vHost[n].c_str());
	}

	peer->EncodeWORD(sizeof(tAdminInfo));
	peer->EncodeWORD(vAdmin.size());

	for (size_t n = 0; n < vAdmin.size(); ++n)
	{
		peer->Encode(&vAdmin[n], sizeof(tAdminInfo));
		sys_log(0, "Admin name %s ConntactIP %s", vAdmin[n].m_szName, vAdmin[n].m_szContactIP);
	}
	//END_ADMIN_MANAGER

	//MONARCH
	peer->EncodeWORD(sizeof(TMonarchInfo));
	peer->EncodeWORD(1);
	peer->Encode(CMonarch::instance().GetMonarch(), sizeof(TMonarchInfo));

	CMonarch::VEC_MONARCHCANDIDACY & rVecMonarchCandidacy = CMonarch::instance().GetVecMonarchCandidacy();

	size_t num_monarch_candidacy = CMonarch::instance().MonarchCandidacySize();
	peer->EncodeWORD(sizeof(MonarchCandidacy));
	peer->EncodeWORD(num_monarch_candidacy);
	if (num_monarch_candidacy != 0) {
		peer->Encode(&rVecMonarchCandidacy[0], sizeof(MonarchCandidacy) * num_monarch_candidacy);
	}
	//END_MONARCE
	
#ifdef ENABLE_OFFLINESHOP_SYSTEM
	peer->EncodeWORD(sizeof(TOfflineShop));
	std::vector<TOfflineShop> offlineshop;
	offlineshop.clear();

	for (auto it = m_Offlineshop.begin(); it != m_Offlineshop.end(); it++)
	{
		TOfflineShop p;
		memcpy(&p, it->second, sizeof(TOfflineShop));
		offlineshop.push_back(p);
	}
	peer->EncodeWORD(offlineshop.size());
	if(offlineshop.size())
		peer->Encode(&offlineshop[0], sizeof(TOfflineShop) * offlineshop.size());
#endif

	if (g_test_server)
		sys_log(0, "MONARCHCandidacy Size %d", CMonarch::instance().MonarchCandidacySize());

	peer->EncodeWORD(0xffff);
#ifdef ENABLE_ITEMSHOP
	SendItemShopData(peer);
#endif
#ifdef ENABLE_EVENT_MANAGER
	SendEventData(peer);
#endif
}

void CClientManager::SendPartyOnSetup(CPeer* pkPeer)
{
	TPartyMap & pm = m_map_pkChannelParty[pkPeer->GetChannel()];

	for (itertype(pm) it_party = pm.begin(); it_party != pm.end(); ++it_party)
	{
		sys_log(0, "PARTY SendPartyOnSetup Party [%u]", it_party->first);
		pkPeer->EncodeHeader(HEADER_DG_PARTY_CREATE, 0, sizeof(TPacketPartyCreate));
		pkPeer->Encode(&it_party->first, sizeof(DWORD));

		for (itertype(it_party->second) it_member = it_party->second.begin(); it_member != it_party->second.end(); ++it_member)
		{
			sys_log(0, "PARTY SendPartyOnSetup Party [%u] Member [%u]", it_party->first, it_member->first);
			pkPeer->EncodeHeader(HEADER_DG_PARTY_ADD, 0, sizeof(TPacketPartyAdd));
			pkPeer->Encode(&it_party->first, sizeof(DWORD));
			pkPeer->Encode(&it_member->first, sizeof(DWORD));
			pkPeer->Encode(&it_member->second.bRole, sizeof(BYTE));

			pkPeer->EncodeHeader(HEADER_DG_PARTY_SET_MEMBER_LEVEL, 0, sizeof(TPacketPartySetMemberLevel));
			pkPeer->Encode(&it_party->first, sizeof(DWORD));
			pkPeer->Encode(&it_member->first, sizeof(DWORD));
			pkPeer->Encode(&it_member->second.bLevel, sizeof(BYTE));
		}
	}
}

void CClientManager::QUERY_PLAYER_COUNT(CPeer * pkPeer, TPlayerCountPacket * pPacket)
{
	pkPeer->SetUserCount(pPacket->dwCount);
}

void CClientManager::QUERY_QUEST_SAVE(CPeer * pkPeer, TQuestTable * pTable, DWORD dwLen)
{
	if (0 != (dwLen % sizeof(TQuestTable)))
	{
		sys_err("invalid packet size %d, sizeof(TQuestTable) == %d", dwLen, sizeof(TQuestTable));
		return;
	}

	int iSize = dwLen / sizeof(TQuestTable);

	char szQuery[1024];

	for (int i = 0; i < iSize; ++i, ++pTable)
	{
		if (pTable->lValue == 0)
		{
			snprintf(szQuery, sizeof(szQuery),
					"DELETE FROM quest%s WHERE dwPID=%d AND szName='%s' AND szState='%s'",
					GetTablePostfix(), pTable->dwPID, pTable->szName, pTable->szState);
		}
		else
		{
			snprintf(szQuery, sizeof(szQuery),
					"REPLACE INTO quest%s (dwPID, szName, szState, lValue) VALUES(%d, '%s', '%s', %ld)",
					GetTablePostfix(), pTable->dwPID, pTable->szName, pTable->szState, pTable->lValue);
		}

		CDBManager::instance().ReturnQuery(szQuery, QID_QUEST_SAVE, pkPeer->GetHandle(), NULL);
	}
}

void CClientManager::QUERY_SAFEBOX_LOAD(CPeer * pkPeer, DWORD dwHandle, TSafeboxLoadPacket * packet, bool bMall)
{
	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle);
	strlcpy(pi->safebox_password, packet->szPassword, sizeof(pi->safebox_password));
	pi->account_id = packet->dwID;
	pi->account_index = 0;
	pi->ip[0] = bMall ? 1 : 0;
	strlcpy(pi->login, packet->szLogin, sizeof(pi->login));

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT account_id, size, password FROM safebox%s WHERE account_id=%u",
			GetTablePostfix(), packet->dwID);

	if (g_log)
		sys_log(0, "HEADER_GD_SAFEBOX_LOAD (handle: %d account.id %u is_mall %d)", dwHandle, packet->dwID, bMall ? 1 : 0);

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_LOAD, pkPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_LOAD(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * pi = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = pi->dwHandle;

	// 여기에서 사용하는 account_index는 쿼리 순서를 말한다.
	// 첫번째 패스워드 알아내기 위해 하는 쿼리가 0
	// 두번째 실제 데이터를 얻어놓는 쿼리가 1

	if (pi->account_index == 0)
	{
		char szSafeboxPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
		strlcpy(szSafeboxPassword, pi->safebox_password, sizeof(szSafeboxPassword));

		TSafeboxTable * pSafebox = new TSafeboxTable;
		memset(pSafebox, 0, sizeof(TSafeboxTable));

		SQLResult * res = msg->Get();

		if (res->uiNumRows == 0)
		{
			if (strcmp("000000", szSafeboxPassword))
			{
				pkPeer->EncodeHeader(HEADER_DG_SAFEBOX_WRONG_PASSWORD, dwHandle, 0);
				delete pi;
				return;
			}
		}
		else
		{
			MYSQL_ROW row = mysql_fetch_row(res->pSQLResult);

			// 비밀번호가 틀리면..
			if (((!row[2] || !*row[2]) && strcmp("000000", szSafeboxPassword)) ||
				((row[2] && *row[2]) && strcmp(row[2], szSafeboxPassword)))
			{
				pkPeer->EncodeHeader(HEADER_DG_SAFEBOX_WRONG_PASSWORD, dwHandle, 0);
				delete pi;
				return;
			}

			if (!row[0])
				pSafebox->dwID = 0;
			else
				str_to_number(pSafebox->dwID, row[0]);

			if (!row[1])
				pSafebox->bSize = 0;
			else
				str_to_number(pSafebox->bSize, row[1]);
			/*
			   if (!row[3])
			   pSafebox->dwGold = 0;
			   else
			   pSafebox->dwGold = atoi(row[3]);
			   */
			if (pi->ip[0] == 1)
			{
				pSafebox->bSize = 1;
				sys_log(0, "MALL id[%d] size[%d]", pSafebox->dwID, pSafebox->bSize);
			}
			else
			{
				sys_log(0, "SAFEBOX id[%d] size[%d]", pSafebox->dwID, pSafebox->bSize);
				pSafebox->bSize = 4;
			}
		}

		if (0 == pSafebox->dwID)
			pSafebox->dwID = pi->account_id;

		pi->pSafebox = pSafebox;
		
		char szQuery[QUERY_MAX_LEN];

		int iLen = snprintf(szQuery, sizeof(szQuery),"SELECT id, window+0, pos, count, vnum");

#ifdef ELEMENT_SPELL_WORLDARD
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", grade_elements");
		for (BYTE j=1;j<4;j++)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", attackelement%d",j);
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", elementstype");
		for (BYTE j=1;j<4;j++)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", elementsvalue%d",j);
#endif

		for (BYTE j=0;j<ITEM_SOCKET_MAX_NUM;j++)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", socket%d",j);
		for (BYTE j=0;j<ITEM_ATTRIBUTE_MAX_NUM;j++)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", attrtype%d, attrvalue%d",j,j);
#ifdef ENABLE_GLOVE_SYSTEM
		for (BYTE j=0;j<GLOVE_ATTR_MAX_NUM;j++)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", randomtype%d, randomvalue%d",j,j);
#endif

#ifdef ENABLE_NEW_NAME_ITEM
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", name");
#endif

		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen," FROM player.item WHERE owner_id=%d AND window='%s'", pi->account_id, pi->ip[0] == 0 ? "SAFEBOX" : "MALL");
		pi->account_index = 1;
		CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_LOAD, pkPeer->GetHandle(), pi);
	}
	else
	{
		if (!pi->pSafebox)
		{
			sys_err("null safebox pointer!");
			delete pi;
			return;
		}


		// 쿼리에 에러가 있었으므로 응답할 경우 창고가 비어있는 것 처럼
		// 보이기 때문에 창고가 아얘 안열리는게 나음
		if (!msg->Get()->pSQLResult)
		{
			sys_err("null safebox result");
			delete pi;
			return;
		}

		static std::vector<TPlayerItem> s_items;
		CreateItemTableFromRes(msg->Get()->pSQLResult, &s_items, pi->account_id);

		std::set<TItemAward *> * pSet = ItemAwardManager::instance().GetByLogin(pi->login);

		if (pSet && !m_vec_itemTable.empty())
		{

			CGrid grid(5, MAX(1, pi->pSafebox->bSize) * 9);
			bool bEscape = false;

			for (DWORD i = 0; i < s_items.size(); ++i)
			{
				TPlayerItem & r = s_items[i];

				itertype(m_map_itemTableByVnum) it = m_map_itemTableByVnum.find(r.vnum);

				if (it == m_map_itemTableByVnum.end())
				{
					bEscape = true;
					sys_err("invalid item vnum %u in safebox: login %s", r.vnum, pi->login);
					break;
				}

				grid.Put(r.pos, 1, it->second->bSize);
			}

			if (!bEscape)
			{
				std::vector<std::pair<DWORD, DWORD> > vec_dwFinishedAwardID;

				__typeof(pSet->begin()) it = pSet->begin();

				char szQuery[512];

				while (it != pSet->end())
				{
					TItemAward * pItemAward = *(it++);
					const DWORD& dwItemVnum = pItemAward->dwVnum;

					if (pItemAward->bTaken)
						continue;

					if (pi->ip[0] == 0 && pItemAward->bMall)
						continue;

					if (pi->ip[0] == 1 && !pItemAward->bMall)
						continue;

					itertype(m_map_itemTableByVnum) it = m_map_itemTableByVnum.find(pItemAward->dwVnum);

					if (it == m_map_itemTableByVnum.end())
					{
						sys_err("invalid item vnum %u in item_award: login %s", pItemAward->dwVnum, pi->login);
						continue;
					}

					TItemTable * pItemTable = it->second;

					int iPos;

					if ((iPos = grid.FindBlank(1, it->second->bSize)) == -1)
						break;

					TPlayerItem item;
					memset(&item, 0, sizeof(TPlayerItem));

					DWORD dwSocket2 = 0;

					if (pItemTable->bType == ITEM_UNIQUE)
					{
						if (pItemAward->dwSocket2 != 0)
							dwSocket2 = pItemAward->dwSocket2;
						else
							dwSocket2 = pItemTable->alValues[0];
					}
					else if ((dwItemVnum == 50300 || dwItemVnum == 70037) && pItemAward->dwSocket0 == 0)
					{
						DWORD dwSkillIdx;
						DWORD dwSkillVnum;

						do
						{
							dwSkillIdx = number(0, m_vec_skillTable.size()-1);

							dwSkillVnum = m_vec_skillTable[dwSkillIdx].dwVnum;

							if (!dwSkillVnum > 120)
								continue;

							break;
						} while (1);

						pItemAward->dwSocket0 = dwSkillVnum;
					}
					else
					{
						switch (dwItemVnum)
						{
							case 72723: case 72724: case 72725: case 72726:
							case 72727: case 72728: case 72729: case 72730:
							// 무시무시하지만 이전에 하던 걸 고치기는 무섭고...
							// 그래서 그냥 하드 코딩. 선물 상자용 자동물약 아이템들.
							case 76004: case 76005: case 76021: case 76022:
							case 79012: case 79013:
								if (pItemAward->dwSocket2 == 0)
								{
									dwSocket2 = pItemTable->alValues[0];
								}
								else
								{
									dwSocket2 = pItemAward->dwSocket2;
								}
								break;
						}
					}

					if (GetItemID () > m_itemRange.dwMax)
					{
						sys_err("UNIQUE ID OVERFLOW!!");
						break;
					}

					{
						itertype(m_map_itemTableByVnum) it = m_map_itemTableByVnum.find (dwItemVnum);
						if (it == m_map_itemTableByVnum.end())
						{
							sys_err ("Invalid item(vnum : %d). It is not in m_map_itemTableByVnum.", dwItemVnum);
							continue;
						}
						TItemTable* item_table = it->second;
						if (item_table == NULL)
						{
							sys_err ("Invalid item_table (vnum : %d). It's value is NULL in m_map_itemTableByVnum.", dwItemVnum);
							continue;
						}
						if (0 == pItemAward->dwSocket0)
						{
							for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
							{
								if (LIMIT_REAL_TIME == item_table->aLimits[i].bType)
								{
									if (0 == item_table->aLimits[i].lValue)
										pItemAward->dwSocket0 = time(0) + 60 * 60 * 24 * 7;
									else
										pItemAward->dwSocket0 = time(0) + item_table->aLimits[i].lValue;

									break;
								}
								else if (LIMIT_REAL_TIME_START_FIRST_USE == item_table->aLimits[i].bType || LIMIT_TIMER_BASED_ON_WEAR == item_table->aLimits[i].bType)
								{
									if (0 == item_table->aLimits[i].lValue)
										pItemAward->dwSocket0 = 60 * 60 * 24 * 7;
									else
										pItemAward->dwSocket0 = item_table->aLimits[i].lValue;

									break;
								}
							}
						}

						snprintf(szQuery, sizeof(szQuery),
								"INSERT INTO item%s (id, owner_id, window, pos, vnum, count, socket0, socket1, socket2) "
								"VALUES(%u, %u, '%s', %d, %u, %u, %u, %u, %u)",
								GetTablePostfix(),
								GainItemID(),
								pi->account_id,
								pi->ip[0] == 0 ? "SAFEBOX" : "MALL",
								iPos,
								pItemAward->dwVnum, pItemAward->dwCount, pItemAward->dwSocket0, pItemAward->dwSocket1, dwSocket2);
					}

					std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));
					SQLResult * pRes = pmsg->Get();
					sys_log(0, "SAFEBOX Query : [%s]", szQuery);

					if (pRes->uiAffectedRows == 0 || pRes->uiInsertID == 0 || pRes->uiAffectedRows == (uint32_t)-1)
						break;

					item.id = pmsg->Get()->uiInsertID;
					item.window = pi->ip[0] == 0 ? SAFEBOX : MALL,
					item.pos = iPos;
					item.count = pItemAward->dwCount;
					item.vnum = pItemAward->dwVnum;
					item.alSockets[0] = pItemAward->dwSocket0;
					item.alSockets[1] = pItemAward->dwSocket1;
					item.alSockets[2] = dwSocket2;
					s_items.push_back(item);

					vec_dwFinishedAwardID.push_back(std::make_pair(pItemAward->dwID, item.id));
					grid.Put(iPos, 1, it->second->bSize);
				}

				for (DWORD i = 0; i < vec_dwFinishedAwardID.size(); ++i)
					ItemAwardManager::instance().Taken(vec_dwFinishedAwardID[i].first, vec_dwFinishedAwardID[i].second);
			}
		}

		pi->pSafebox->wItemCount = s_items.size();

		pkPeer->EncodeHeader(pi->ip[0] == 0 ? HEADER_DG_SAFEBOX_LOAD : HEADER_DG_MALL_LOAD, dwHandle, sizeof(TSafeboxTable) + sizeof(TPlayerItem) * s_items.size());

		pkPeer->Encode(pi->pSafebox, sizeof(TSafeboxTable));

		if (!s_items.empty())
			pkPeer->Encode(&s_items[0], sizeof(TPlayerItem) * s_items.size());

		delete pi;
	}
}

void CClientManager::QUERY_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, DWORD dwHandle, TSafeboxChangeSizePacket * p)
{
	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle);
	pi->account_index = p->bSize;	// account_index를 사이즈로 임시로 사용

	char szQuery[QUERY_MAX_LEN];

	if (p->bSize == 1)
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO safebox%s (account_id, size) VALUES(%u, %u)", GetTablePostfix(), p->dwID, p->bSize);
	else
		snprintf(szQuery, sizeof(szQuery), "UPDATE safebox%s SET size=%u WHERE account_id=%u", GetTablePostfix(), p->bSize, p->dwID);

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_SIZE, pkPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * p = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = p->dwHandle;
	BYTE bSize = p->account_index;

	delete p;

	if (msg->Get()->uiNumRows > 0)
	{
		pkPeer->EncodeHeader(HEADER_DG_SAFEBOX_CHANGE_SIZE, dwHandle, sizeof(BYTE));
		pkPeer->EncodeBYTE(bSize);
	}
}

void CClientManager::QUERY_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, DWORD dwHandle, TSafeboxChangePasswordPacket * p)
{
	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle);
	strlcpy(pi->safebox_password, p->szNewPassword, sizeof(pi->safebox_password));
	strlcpy(pi->login, p->szOldPassword, sizeof(pi->login));
	pi->account_id = p->dwID;

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT password FROM safebox%s WHERE account_id=%u", GetTablePostfix(), p->dwID);

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_PASSWORD, pkPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * p = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = p->dwHandle;

	if (msg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if ((row[0] && *row[0] && !strcasecmp(row[0], p->login)) || ((!row[0] || !*row[0]) && !strcmp("000000", p->login)))
		{
			char szQuery[QUERY_MAX_LEN];
			char escape_pwd[64];
			CDBManager::instance().EscapeString(escape_pwd, p->safebox_password, strlen(p->safebox_password));

			snprintf(szQuery, sizeof(szQuery), "UPDATE safebox%s SET password='%s' WHERE account_id=%u", GetTablePostfix(), escape_pwd, p->account_id);

			CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_PASSWORD_SECOND, pkPeer->GetHandle(), p);
			return;
		}
	}

	delete p;

	// Wrong old password
	pkPeer->EncodeHeader(HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER, dwHandle, sizeof(BYTE));
	pkPeer->EncodeBYTE(0);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * p = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = p->dwHandle;
	delete p;

	pkPeer->EncodeHeader(HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER, dwHandle, sizeof(BYTE));
	pkPeer->EncodeBYTE(1);
}

void CClientManager::QUERY_SAFEBOX_SAVE(CPeer * pkPeer, TSafeboxTable * pTable)
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery),
			"UPDATE safebox%s SET gold='%u' WHERE account_id=%u",
			GetTablePostfix(), pTable->dwGold, pTable->dwID);

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_SAVE, pkPeer->GetHandle(), NULL);
}

void CClientManager::QUERY_EMPIRE_SELECT(CPeer * pkPeer, DWORD dwHandle, TEmpireSelectPacket * p)
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), "UPDATE player_index%s SET empire=%u WHERE id=%u", GetTablePostfix(), p->bEmpire, p->dwAccountID);
	delete CDBManager::instance().DirectQuery(szQuery);

	sys_log(0, "EmpireSelect: %s", szQuery);
	{
#ifdef ENABLE_PLAYER_PER_ACCOUNT5
		snprintf(szQuery, sizeof(szQuery),
				"SELECT pid1, pid2, pid3, pid4, pid5 FROM player_index%s WHERE id=%u", GetTablePostfix(), p->dwAccountID);
#else
		snprintf(szQuery, sizeof(szQuery),
				"SELECT pid1, pid2, pid3, pid4 FROM player_index%s WHERE id=%u", GetTablePostfix(), p->dwAccountID);
#endif

		std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult * pRes = pmsg->Get();

		if (pRes->uiNumRows)
		{
			sys_log(0, "EMPIRE %lu", pRes->uiNumRows);

			MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);
			DWORD pids[3];

			UINT g_start_map[4] =
			{
				0,  // reserved
				1,  // 신수국
				21, // 천조국
				41  // 진노국
			};

			// FIXME share with game
			DWORD g_start_position[4][2]=
			{
				{      0,      0 },
				{ 469300, 964200 }, // 신수국
				{  55700, 157900 }, // 천조국
				{ 969600, 278400 }  // 진노국
			};

			for (int i = 0; i < 3; ++i)
			{
				str_to_number(pids[i], row[i]);
				sys_log(0, "EMPIRE PIDS[%d]", pids[i]);

				if (pids[i])
				{
					sys_log(0, "EMPIRE move to pid[%d] to villiage of %u, map_index %d",
							pids[i], p->bEmpire, g_start_map[p->bEmpire]);

					snprintf(szQuery, sizeof(szQuery), "UPDATE player%s SET map_index=%u,x=%u,y=%u WHERE id=%u",
							GetTablePostfix(),
							g_start_map[p->bEmpire],
							g_start_position[p->bEmpire][0],
							g_start_position[p->bEmpire][1],
							pids[i]);

					std::auto_ptr<SQLMsg> pmsg2(CDBManager::instance().DirectQuery(szQuery));
				}
			}
		}
	}

	pkPeer->EncodeHeader(HEADER_DG_EMPIRE_SELECT, dwHandle, sizeof(BYTE));
	pkPeer->EncodeBYTE(p->bEmpire);
}

void CClientManager::QUERY_SETUP(CPeer * peer, DWORD dwHandle, const char * c_pData)
{
	TPacketGDSetup * p = (TPacketGDSetup *) c_pData;
	c_pData += sizeof(TPacketGDSetup);

	if (p->bAuthServer)
	{
		sys_log(0, "AUTH_PEER ptr %p", peer);

		m_pkAuthPeer = peer;
		SendAllLoginToBilling();
		return;
	}

	peer->SetPublicIP(p->szPublicIP);
	peer->SetChannel(p->bChannel);
	peer->SetListenPort(p->wListenPort);
	peer->SetP2PPort(p->wP2PPort);
	peer->SetMaps(p->alMaps);

	//
	// 어떤 맵이 어떤 서버에 있는지 보내기
	//
	TMapLocation kMapLocations;

	strlcpy(kMapLocations.szHost, peer->GetPublicIP(), sizeof(kMapLocations.szHost));
	kMapLocations.wPort = peer->GetListenPort();
	thecore_memcpy(kMapLocations.alMaps, peer->GetMaps(), sizeof(kMapLocations.alMaps));

	BYTE bMapCount;

	std::vector<TMapLocation> vec_kMapLocations;

	if (peer->GetChannel() == 1)
	{
		for (itertype(m_peerList) i = m_peerList.begin(); i != m_peerList.end(); ++i)
		{
			CPeer * tmp = *i;

			if (tmp == peer)
				continue;

			if (!tmp->GetChannel())
				continue;

			if (tmp->GetChannel() == GUILD_WARP_WAR_CHANNEL || tmp->GetChannel() == peer->GetChannel())
			{
				TMapLocation kMapLocation2;
				strlcpy(kMapLocation2.szHost, tmp->GetPublicIP(), sizeof(kMapLocation2.szHost));
				kMapLocation2.wPort = tmp->GetListenPort();
				thecore_memcpy(kMapLocation2.alMaps, tmp->GetMaps(), sizeof(kMapLocation2.alMaps));
				vec_kMapLocations.push_back(kMapLocation2);

				tmp->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(BYTE) + sizeof(TMapLocation));
				bMapCount = 1;
				tmp->EncodeBYTE(bMapCount);
				tmp->Encode(&kMapLocations, sizeof(TMapLocation));
			}
		}
	}
	else if (peer->GetChannel() == GUILD_WARP_WAR_CHANNEL)
	{
		for (itertype(m_peerList) i = m_peerList.begin(); i != m_peerList.end(); ++i)
		{
			CPeer * tmp = *i;

			if (tmp == peer)
				continue;

			if (!tmp->GetChannel())
				continue;

			if (tmp->GetChannel() == 1 || tmp->GetChannel() == peer->GetChannel())
			{
				TMapLocation kMapLocation2;
				strlcpy(kMapLocation2.szHost, tmp->GetPublicIP(), sizeof(kMapLocation2.szHost));
				kMapLocation2.wPort = tmp->GetListenPort();
				thecore_memcpy(kMapLocation2.alMaps, tmp->GetMaps(), sizeof(kMapLocation2.alMaps));
				vec_kMapLocations.push_back(kMapLocation2);
			}

			tmp->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(BYTE) + sizeof(TMapLocation));
			bMapCount = 1;
			tmp->EncodeBYTE(bMapCount);
			tmp->Encode(&kMapLocations, sizeof(TMapLocation));
		}
	}
	else
	{
		for (itertype(m_peerList) i = m_peerList.begin(); i != m_peerList.end(); ++i)
		{
			CPeer * tmp = *i;

			if (tmp == peer)
				continue;

			if (!tmp->GetChannel())
				continue;

			if (tmp->GetChannel() == GUILD_WARP_WAR_CHANNEL || tmp->GetChannel() == peer->GetChannel())
			{
				TMapLocation kMapLocation2;

				strlcpy(kMapLocation2.szHost, tmp->GetPublicIP(), sizeof(kMapLocation2.szHost));
				kMapLocation2.wPort = tmp->GetListenPort();
				thecore_memcpy(kMapLocation2.alMaps, tmp->GetMaps(), sizeof(kMapLocation2.alMaps));

				vec_kMapLocations.push_back(kMapLocation2);
			}

			if (tmp->GetChannel() == peer->GetChannel())
			{
				tmp->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(BYTE) + sizeof(TMapLocation));
				bMapCount = 1;
				tmp->EncodeBYTE(bMapCount);
				tmp->Encode(&kMapLocations, sizeof(TMapLocation));
			}
		}
	}

	vec_kMapLocations.push_back(kMapLocations);

	peer->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(BYTE) + sizeof(TMapLocation) * vec_kMapLocations.size());
	bMapCount = vec_kMapLocations.size();
	peer->EncodeBYTE(bMapCount);
	peer->Encode(&vec_kMapLocations[0], sizeof(TMapLocation) * vec_kMapLocations.size());

	//
	// 셋업 : 접속한 피어에 다른 피어들이 접속하게 만든다. (P2P 컨넥션 생성)
	//
	sys_log(0, "SETUP: channel %u listen %u p2p %u count %u", peer->GetChannel(), p->wListenPort, p->wP2PPort, bMapCount);

	TPacketDGP2P p2pSetupPacket;
	p2pSetupPacket.wPort = peer->GetP2PPort();
	p2pSetupPacket.bChannel = peer->GetChannel();
	strlcpy(p2pSetupPacket.szHost, peer->GetPublicIP(), sizeof(p2pSetupPacket.szHost));

	for (itertype(m_peerList) i = m_peerList.begin(); i != m_peerList.end();++i)
	{
		CPeer * tmp = *i;

		if (tmp == peer)
			continue;

		// 채널이 0이라면 아직 SETUP 패킷이 오지 않은 피어 또는 auth라고 간주할 수 있음
		if (0 == tmp->GetChannel())
			continue;

		tmp->EncodeHeader(HEADER_DG_P2P, 0, sizeof(TPacketDGP2P));
		tmp->Encode(&p2pSetupPacket, sizeof(TPacketDGP2P));
	}

	//
	// 로그인 및 빌링정보 보내기
	//
	TPacketLoginOnSetup * pck = (TPacketLoginOnSetup *) c_pData;;
	std::vector<TPacketBillingRepair> vec_repair;

	for (DWORD c = 0; c < p->dwLoginCount; ++c, ++pck)
	{
		CLoginData * pkLD = new CLoginData;

		pkLD->SetKey(pck->dwLoginKey);
		pkLD->SetClientKey(pck->adwClientKey);
		pkLD->SetIP(pck->szHost);

		TAccountTable & r = pkLD->GetAccountRef();

		r.id = pck->dwID;
		trim_and_lower(pck->szLogin, r.login, sizeof(r.login));
		strlcpy(r.social_id, pck->szSocialID, sizeof(r.social_id));
		strlcpy(r.passwd, "TEMP", sizeof(r.passwd));

		InsertLoginData(pkLD);

		if (InsertLogonAccount(pck->szLogin, peer->GetHandle(), pck->szHost))
		{
			sys_log(0, "SETUP: login %u %s login_key %u host %s", pck->dwID, pck->szLogin, pck->dwLoginKey, pck->szHost);
			pkLD->SetPlay(true);

			if (m_pkAuthPeer)
			{
				TPacketBillingRepair pck_repair;
				pck_repair.dwLoginKey = pkLD->GetKey();
				strlcpy(pck_repair.szLogin, pck->szLogin, sizeof(pck_repair.szLogin));
				strlcpy(pck_repair.szHost, pck->szHost, sizeof(pck_repair.szHost));
				vec_repair.push_back(pck_repair);
			}
		}
		else
			sys_log(0, "SETUP: login_fail %u %s login_key %u", pck->dwID, pck->szLogin, pck->dwLoginKey);
	}

	if (m_pkAuthPeer && !vec_repair.empty())
	{
		sys_log(0, "REPAIR size %d", vec_repair.size());

		m_pkAuthPeer->EncodeHeader(HEADER_DG_BILLING_REPAIR, 0, sizeof(DWORD) + sizeof(TPacketBillingRepair) * vec_repair.size());
		m_pkAuthPeer->EncodeDWORD(vec_repair.size());
		m_pkAuthPeer->Encode(&vec_repair[0], sizeof(TPacketBillingRepair) * vec_repair.size());
	}

	SendPartyOnSetup(peer);
	CGuildManager::instance().OnSetup(peer);
	CPrivManager::instance().SendPrivOnSetup(peer);
	SendEventFlagsOnSetup(peer);
	marriage::CManager::instance().OnSetup(peer);
}

void CClientManager::QUERY_ITEM_FLUSH(CPeer * pkPeer, const char * c_pData)
{
	DWORD dwID = *(DWORD *) c_pData;

	if (g_log)
		sys_log(0, "HEADER_GD_ITEM_FLUSH: %u", dwID);

	CItemCache * c = GetItemCache(dwID);

	if (c)
		c->Flush();
}

void CClientManager::QUERY_ITEM_SAVE(CPeer * pkPeer, const char * c_pData)
{
	TPlayerItem * p = (TPlayerItem *) c_pData;

	// 창고면 캐쉬하지 않고, 캐쉬에 있던 것도 빼버려야 한다.
	// auction은 이 루트를 타지 않아야 한다. EnrollInAuction을 타야한다.

	if (p->window == SAFEBOX || p->window == MALL)
	{
		CItemCache * c = GetItemCache(p->id);
		if (c)
		{
			TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(c->Get()->owner);
			if (it != m_map_pkItemCacheSetPtr.end())
			{
				if (g_test_server)
					sys_log(0, "ITEM_CACHE: safebox owner %u id %u", c->Get()->owner, c->Get()->id);
				it->second->erase(c);
			}
			m_map_itemCache.erase(p->id);
			delete c;
		}

		char szQuery[QUERY_MAX_LEN];
		int iLen = snprintf(szQuery, sizeof(szQuery),"REPLACE INTO player.item (id, owner_id, window, pos, count, vnum");
		for(BYTE j=0;j<ITEM_SOCKET_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", socket%d",j);
		for(BYTE j=0;j<ITEM_ATTRIBUTE_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", attrtype%d, attrvalue%d",j,j);
#ifdef ENABLE_GLOVE_SYSTEM
		for(BYTE j=0;j<GLOVE_ATTR_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", randomtype%d, randomvalue%d",j,j);
#endif
#ifdef ENABLE_NEW_NAME_ITEM
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", name");
#endif
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,") VALUES(%u, %u, %d, %d, %u, %u",p->id,p->owner,p->window,p->pos,p->count,p->vnum);
		for(BYTE j=0;j<ITEM_SOCKET_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", %ld",p->alSockets[j]);
		for(BYTE j=0;j<ITEM_ATTRIBUTE_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", %d, %d",p->aAttr[j].bType, p->aAttr[j].sValue);
#ifdef ENABLE_GLOVE_SYSTEM
		for(BYTE j=0;j<GLOVE_ATTR_MAX_NUM;++j)
			iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", %d, %d",p->randomAttr[j].bType, p->randomAttr[j].sValue);
#endif
#ifdef ENABLE_NEW_NAME_ITEM
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,", '%s'", p->name);
#endif
		iLen += snprintf(szQuery+iLen, sizeof(szQuery)-iLen,")");
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_SAVE, pkPeer->GetHandle(), NULL);
	}
#ifdef __AUCTION__
	else if (p->window == AUCTION)
	{
		sys_err("invalid window. how can you enter this route?");
		return ;
	}
#endif
	else
	{
		if (g_test_server)
			sys_log(0, "QUERY_ITEM_SAVE => PutItemCache() owner %d id %d vnum %d ", p->owner, p->id, p->vnum);

		PutItemCache(p);
	}
}

CClientManager::TItemCacheSet * CClientManager::GetItemCacheSet(DWORD pid)
{
	TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(pid);

	if (it == m_map_pkItemCacheSetPtr.end())
		return NULL;

	return it->second;
}

void CClientManager::CreateItemCacheSet(DWORD pid)
{
	if (m_map_pkItemCacheSetPtr.find(pid) != m_map_pkItemCacheSetPtr.end())
		return;

	TItemCacheSet * pSet = new TItemCacheSet;
	m_map_pkItemCacheSetPtr.insert(TItemCacheSetPtrMap::value_type(pid, pSet));

	if (g_log)
		sys_log(0, "ITEM_CACHE: new cache %u", pid);
}

void CClientManager::FlushItemCacheSet(DWORD pid)
{
	TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(pid);

	if (it == m_map_pkItemCacheSetPtr.end())
	{
		sys_log(0, "FLUSH_ITEMCACHESET : No ItemCacheSet pid(%d)", pid);
		return;
	}

	TItemCacheSet * pSet = it->second;
	TItemCacheSet::iterator it_set = pSet->begin();

	while (it_set != pSet->end())
	{
		CItemCache * c = *it_set++;
		c->Flush();

		m_map_itemCache.erase(c->Get()->id);
		delete c;
	}

	pSet->clear();
	delete pSet;

	m_map_pkItemCacheSetPtr.erase(it);

	if (g_log)
		sys_log(0, "FLUSH_ITEMCACHESET : Deleted pid(%d)", pid);
}

CItemCache * CClientManager::GetItemCache(DWORD id)
{
	TItemCacheMap::iterator it = m_map_itemCache.find(id);

	if (it == m_map_itemCache.end())
		return NULL;

	return it->second;
}

void CClientManager::PutItemCache(TPlayerItem * pNew, bool bSkipQuery)
{
	CItemCache * c;

	c = GetItemCache(pNew->id);

	// 아이템 새로 생성
	if (!c)
	{
		if (g_log)
			sys_log(0, "ITEM_CACHE: PutItemCache ==> New CItemCache id%d vnum%d new owner%d", pNew->id, pNew->vnum, pNew->owner);

		c = new CItemCache;
		m_map_itemCache.insert(TItemCacheMap::value_type(pNew->id, c));
	}
	// 있을시
	else
	{
		if (g_log)
			sys_log(0, "ITEM_CACHE: PutItemCache ==> Have Cache");
		// 소유자가 틀리면
		if (pNew->owner != c->Get()->owner)
		{
			// 이미 이 아이템을 가지고 있었던 유저로 부터 아이템을 삭제한다.
			TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(c->Get()->owner);

			if (it != m_map_pkItemCacheSetPtr.end())
			{
				if (g_log)
				sys_log(0, "ITEM_CACHE: delete owner %u id %u new owner %u", c->Get()->owner, c->Get()->id, pNew->owner);
				it->second->erase(c);
			}
		}
	}

	// 새로운 정보 업데이트
	c->Put(pNew, bSkipQuery);

	TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(c->Get()->owner);

	if (it != m_map_pkItemCacheSetPtr.end())
	{
		if (g_log)
			sys_log(0, "ITEM_CACHE: save %u id %u", c->Get()->owner, c->Get()->id);
		else
			sys_log(1, "ITEM_CACHE: save %u id %u", c->Get()->owner, c->Get()->id);
		it->second->insert(c);
	}
	else
	{
		// 현재 소유자가 없으므로 바로 저장해야 다음 접속이 올 때 SQL에 쿼리하여
		// 받을 수 있으므로 바로 저장한다.
		if (g_log)
			sys_log(0, "ITEM_CACHE: direct save %u id %u", c->Get()->owner, c->Get()->id);
		else
			sys_log(1, "ITEM_CACHE: direct save %u id %u", c->Get()->owner, c->Get()->id);

		c->OnFlush();
	}
}

bool CClientManager::DeleteItemCache(DWORD dwID)
{
	CItemCache * c = GetItemCache(dwID);

	if (!c)
		return false;

	c->Delete();
	return true;
}

void CClientManager::UpdatePlayerCache()
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.begin();

	while (it != m_map_playerCache.end())
	{
		CPlayerTableCache * c = (it++)->second;

		if (c->CheckTimeout())
		{
			if (g_log)
				sys_log(0, "UPDATE : UpdatePlayerCache() ==> FlushPlayerCache %d %s ", c->Get(false)->id, c->Get(false)->name);

			c->Flush();

			// Item Cache도 업데이트
			UpdateItemCacheSet(c->Get()->id);
		}
		else if (c->CheckFlushTimeout())
			c->Flush();
	}
}


void CClientManager::SetCacheFlushCountLimit(int iLimit)
{
	m_iCacheFlushCountLimit = MAX(10, iLimit);
	sys_log(0, "CACHE_FLUSH_LIMIT_PER_SECOND: %d", m_iCacheFlushCountLimit);
}

void CClientManager::UpdateItemCache()
{
	if (m_iCacheFlushCount >= m_iCacheFlushCountLimit)
		return;

	TItemCacheMap::iterator it = m_map_itemCache.begin();

	while (it != m_map_itemCache.end())
	{
		CItemCache * c = (it++)->second;

		// 아이템은 Flush만 한다.
		if (c->CheckFlushTimeout())
		{
			if (g_test_server)
				sys_log(0, "UpdateItemCache ==> Flush() vnum %d id owner %d", c->Get()->vnum, c->Get()->id, c->Get()->owner);

			c->Flush();

			if (++m_iCacheFlushCount >= m_iCacheFlushCountLimit)
				break;
		}
	}
}

void CClientManager::QUERY_ITEM_DESTROY(CPeer * pkPeer, const char * c_pData)
{
	DWORD dwID = *(DWORD *) c_pData;
	c_pData += sizeof(DWORD);

	DWORD dwPID = *(DWORD *) c_pData;

	if (!DeleteItemCache(dwID))
	{
		char szQuery[64];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item%s WHERE id=%u", GetTablePostfix(), dwID);

		if (g_log)
			sys_log(0, "HEADER_GD_ITEM_DESTROY: PID %u ID %u", dwPID, dwID);

		if (dwPID == 0) // 아무도 가진 사람이 없었다면, 비동기 쿼리
			CDBManager::instance().AsyncQuery(szQuery);
		else
			CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_DESTROY, pkPeer->GetHandle(), NULL);
	}
}

void CClientManager::QUERY_FLUSH_CACHE(CPeer * pkPeer, const char * c_pData)
{
	DWORD dwPID = *(DWORD *) c_pData;

	CPlayerTableCache * pkCache = GetPlayerCache(dwPID);

	if (!pkCache)
		return;

	sys_log(0, "FLUSH_CACHE: %u", dwPID);

	pkCache->Flush();
	FlushItemCacheSet(dwPID);

	m_map_playerCache.erase(dwPID);
	delete pkCache;
}

void CClientManager::QUERY_SMS(CPeer * pkPeer, TPacketGDSMS * pack)
{
	char szQuery[QUERY_MAX_LEN];

	char szMsg[256+1];
	//unsigned long len = CDBManager::instance().EscapeString(szMsg, pack->szMsg, strlen(pack->szMsg), SQL_ACCOUNT);
	unsigned long len = CDBManager::instance().EscapeString(szMsg, pack->szMsg, strlen(pack->szMsg));
	szMsg[len] = '\0';

	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO sms_pool (server, sender, receiver, mobile, msg) VALUES(%d, '%s', '%s', '%s', '%s')",
			(m_iPlayerIDStart + 2) / 3, pack->szFrom, pack->szTo, pack->szMobile, szMsg);

	CDBManager::instance().AsyncQuery(szQuery);
}

void CClientManager::QUERY_RELOAD_PROTO()
{
	if (!InitializeTables())
	{
		sys_err("QUERY_RELOAD_PROTO: cannot load tables");
		return;
	}

	for (TPeerList::iterator i = m_peerList.begin(); i != m_peerList.end(); ++i)
	{
		CPeer * tmp = *i;

		if (!tmp->GetChannel())
			continue;

		tmp->EncodeHeader(HEADER_DG_RELOAD_PROTO, 0,
				sizeof(WORD) + sizeof(TSkillTable) * m_vec_skillTable.size() +
				sizeof(WORD) + sizeof(TBanwordTable) * m_vec_banwordTable.size() +
				sizeof(WORD) + sizeof(TItemTable) * m_vec_itemTable.size() +
				sizeof(WORD) + sizeof(TMobTable) * m_vec_mobTable.size());

		tmp->EncodeWORD(m_vec_skillTable.size());
		tmp->Encode(&m_vec_skillTable[0], sizeof(TSkillTable) * m_vec_skillTable.size());

		tmp->EncodeWORD(m_vec_banwordTable.size());
		tmp->Encode(&m_vec_banwordTable[0], sizeof(TBanwordTable) * m_vec_banwordTable.size());

		tmp->EncodeWORD(m_vec_itemTable.size());
		tmp->Encode(&m_vec_itemTable[0], sizeof(TItemTable) * m_vec_itemTable.size());

		tmp->EncodeWORD(m_vec_mobTable.size());
		tmp->Encode(&m_vec_mobTable[0], sizeof(TMobTable) * m_vec_mobTable.size());
	}
}

// ADD_GUILD_PRIV_TIME
/**
 * @version	05/06/08 Bang2ni - 지속시간 추가
 */
void CClientManager::AddGuildPriv(TPacketGiveGuildPriv* p)
{
	CPrivManager::instance().AddGuildPriv(p->guild_id, p->type, p->value, p->duration_sec);
#ifdef ENABLE_DEFAULT_PRIV
	__UpdateDefaultPriv("GUILD", p->guild_id, p->type, p->value, p->duration_sec);
#endif
}

void CClientManager::AddEmpirePriv(TPacketGiveEmpirePriv* p)
{
	CPrivManager::instance().AddEmpirePriv(p->empire, p->type, p->value, p->duration_sec);
#ifdef ENABLE_DEFAULT_PRIV
	__UpdateDefaultPriv("EMPIRE", p->empire, p->type, p->value, p->duration_sec);
#endif
}
// END_OF_ADD_GUILD_PRIV_TIME

void CClientManager::AddCharacterPriv(TPacketGiveCharacterPriv* p)
{
	CPrivManager::instance().AddCharPriv(p->pid, p->type, p->value);
#ifdef ENABLE_DEFAULT_PRIV
	__UpdateDefaultPriv("PLAYER", p->pid, p->type, p->value, 0);
#endif
}

void CClientManager::MoneyLog(TPacketMoneyLog* p)
{
	CMoneyLog::instance().AddLog(p->type, p->vnum, p->gold);
}

CLoginData * CClientManager::GetLoginData(DWORD dwKey)
{
	TLoginDataByLoginKey::iterator it = m_map_pkLoginData.find(dwKey);

	if (it == m_map_pkLoginData.end())
		return NULL;

	return it->second;
}

CLoginData * CClientManager::GetLoginDataByLogin(const char * c_pszLogin)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	TLoginDataByLogin::iterator it = m_map_pkLoginDataByLogin.find(szLogin);

	if (it == m_map_pkLoginDataByLogin.end())
		return NULL;

	return it->second;
}

CLoginData * CClientManager::GetLoginDataByAID(DWORD dwAID)
{
	TLoginDataByAID::iterator it = m_map_pkLoginDataByAID.find(dwAID);

	if (it == m_map_pkLoginDataByAID.end())
		return NULL;

	return it->second;
}

void CClientManager::InsertLoginData(CLoginData * pkLD)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(pkLD->GetAccountRef().login, szLogin, sizeof(szLogin));

	m_map_pkLoginData.insert(std::make_pair(pkLD->GetKey(), pkLD));
	m_map_pkLoginDataByLogin.insert(std::make_pair(szLogin, pkLD));
	m_map_pkLoginDataByAID.insert(std::make_pair(pkLD->GetAccountRef().id, pkLD));
}

void CClientManager::DeleteLoginData(CLoginData * pkLD)
{
	m_map_pkLoginData.erase(pkLD->GetKey());
	m_map_pkLoginDataByLogin.erase(pkLD->GetAccountRef().login);
	m_map_pkLoginDataByAID.erase(pkLD->GetAccountRef().id);

	if (m_map_kLogonAccount.find(pkLD->GetAccountRef().login) == m_map_kLogonAccount.end())
		delete pkLD;
	else
		pkLD->SetDeleted(true);
}

void CClientManager::QUERY_AUTH_LOGIN(CPeer * pkPeer, DWORD dwHandle, TPacketGDAuthLogin * p)
{
	if (g_test_server)
		sys_log(0, "QUERY_AUTH_LOGIN %d %d %s", p->dwID, p->dwLoginKey, p->szLogin);
	CLoginData * pkLD = GetLoginDataByLogin(p->szLogin);

	if (pkLD)
	{
		DeleteLoginData(pkLD);
	}

	BYTE bResult;

	if (GetLoginData(p->dwLoginKey))
	{
		sys_err("LoginData already exist key %u login %s", p->dwLoginKey, p->szLogin);
		bResult = 0;

		pkPeer->EncodeHeader(HEADER_DG_AUTH_LOGIN, dwHandle, sizeof(BYTE));
		pkPeer->EncodeBYTE(bResult);
	}
	else
	{
		CLoginData * pkLD = new CLoginData;

		pkLD->SetKey(p->dwLoginKey);
		pkLD->SetClientKey(p->adwClientKey);
		pkLD->SetBillType(p->bBillType);
		pkLD->SetBillID(p->dwBillID);
		pkLD->SetPremium(p->iPremiumTimes);

		TAccountTable & r = pkLD->GetAccountRef();

		r.id = p->dwID;
		trim_and_lower(p->szLogin, r.login, sizeof(r.login));
		strlcpy(r.social_id, p->szSocialID, sizeof(r.social_id));
		strlcpy(r.passwd, "TEMP", sizeof(r.passwd));

		sys_log(0, "AUTH_LOGIN id(%u) login(%s) social_id(%s) login_key(%u), client_key(%u %u %u %u)",
				p->dwID, p->szLogin, p->szSocialID, p->dwLoginKey,
				p->adwClientKey[0], p->adwClientKey[1], p->adwClientKey[2], p->adwClientKey[3]);

		bResult = 1;

		InsertLoginData(pkLD);

		pkPeer->EncodeHeader(HEADER_DG_AUTH_LOGIN, dwHandle, sizeof(BYTE));
		pkPeer->EncodeBYTE(bResult);
	}
}

void CClientManager::BillingExpire(TPacketBillingExpire * p)
{
	char key[LOGIN_MAX_LEN + 1];
	trim_and_lower(p->szLogin, key, sizeof(key));

	switch (p->bBillType)
	{
		case BILLING_IP_TIME:
		case BILLING_IP_DAY:
			{
				DWORD dwIPID = 0;
				str_to_number(dwIPID, p->szLogin);

				TLogonAccountMap::iterator it = m_map_kLogonAccount.begin();

				while (it != m_map_kLogonAccount.end())
				{
					CLoginData * pkLD = (it++)->second;

					if (pkLD->GetBillID() == dwIPID)
					{
						CPeer * pkPeer = GetPeer(pkLD->GetConnectedPeerHandle());

						if (pkPeer)
						{
							strlcpy(p->szLogin, pkLD->GetAccountRef().login, sizeof(p->szLogin));
							pkPeer->EncodeHeader(HEADER_DG_BILLING_EXPIRE, 0, sizeof(TPacketBillingExpire));
							pkPeer->Encode(p, sizeof(TPacketBillingExpire));
						}
					}
				}
			}
			break;

		case BILLING_TIME:
		case BILLING_DAY:
			{
				TLogonAccountMap::iterator it = m_map_kLogonAccount.find(key);

				if (it != m_map_kLogonAccount.end())
				{
					CLoginData * pkLD = it->second;

					CPeer * pkPeer = GetPeer(pkLD->GetConnectedPeerHandle());

					if (pkPeer)
					{
						pkPeer->EncodeHeader(HEADER_DG_BILLING_EXPIRE, 0, sizeof(TPacketBillingExpire));
						pkPeer->Encode(p, sizeof(TPacketBillingExpire));
					}
				}
			}
			break;
	}
}

void CClientManager::BillingCheck(const char * data)
{
	if (!m_pkAuthPeer)
		return;

	time_t curTime = GetCurrentTime();

	DWORD dwCount = *(DWORD *) data;
	data += sizeof(DWORD);

	std::vector<DWORD> vec;

	sys_log(0, "BillingCheck: size %u", dwCount);

	for (DWORD i = 0; i < dwCount; ++i)
	{
		DWORD dwKey = *(DWORD *) data;
		data += sizeof(DWORD);

		sys_log(0, "BillingCheck: %u", dwKey);

		TLoginDataByLoginKey::iterator it = m_map_pkLoginData.find(dwKey);

		if (it == m_map_pkLoginData.end())
		{
			sys_log(0, "BillingCheck: key not exist: %u", dwKey);
			vec.push_back(dwKey);
		}
		else
		{
			CLoginData * pkLD = it->second;

			if (!pkLD->IsPlay() && curTime - pkLD->GetLastPlayTime() > 180)
			{
				sys_log(0, "BillingCheck: not login: %u", dwKey);
				vec.push_back(dwKey);
			}
		}
	}

	m_pkAuthPeer->EncodeHeader(HEADER_DG_BILLING_CHECK, 0, sizeof(DWORD) + sizeof(DWORD) * vec.size());
	m_pkAuthPeer->EncodeDWORD(vec.size());

	if (!vec.empty())
		m_pkAuthPeer->Encode(&vec[0], sizeof(DWORD) * vec.size());
}

void CClientManager::GuildDepositMoney(TPacketGDGuildMoney* p)
{
	CGuildManager::instance().DepositMoney(p->dwGuild, p->iGold);
}

void CClientManager::GuildWithdrawMoney(CPeer* peer, TPacketGDGuildMoney* p)
{
	CGuildManager::instance().WithdrawMoney(peer, p->dwGuild, p->iGold);
}

void CClientManager::GuildWithdrawMoneyGiveReply(TPacketGDGuildMoneyWithdrawGiveReply* p)
{
	CGuildManager::instance().WithdrawMoneyReply(p->dwGuild, p->bGiveSuccess, p->iChangeGold);
}

void CClientManager::GuildWarBet(TPacketGDGuildWarBet * p)
{
	CGuildManager::instance().Bet(p->dwWarID, p->szLogin, p->dwGold, p->dwGuild);
}

void CClientManager::SendAllLoginToBilling()
{
	if (!m_pkAuthPeer)
		return;

	std::vector<TPacketBillingRepair> vec;
	TPacketBillingRepair p;

	TLogonAccountMap::iterator it = m_map_kLogonAccount.begin();

	while (it != m_map_kLogonAccount.end())
	{
		CLoginData * pkLD = (it++)->second;

		p.dwLoginKey = pkLD->GetKey();
		strlcpy(p.szLogin, pkLD->GetAccountRef().login, sizeof(p.szLogin));
		strlcpy(p.szHost, pkLD->GetIP(), sizeof(p.szHost));
		sys_log(0, "SendAllLoginToBilling %s %s", pkLD->GetAccountRef().login, pkLD->GetIP());
		vec.push_back(p);
	}

	if (!vec.empty())
	{
		m_pkAuthPeer->EncodeHeader(HEADER_DG_BILLING_REPAIR, 0, sizeof(DWORD) + sizeof(TPacketBillingRepair) * vec.size());
		m_pkAuthPeer->EncodeDWORD(vec.size());
		m_pkAuthPeer->Encode(&vec[0], sizeof(TPacketBillingRepair) * vec.size());
	}
}

void CClientManager::SendLoginToBilling(CLoginData * pkLD, bool bLogin)
{
	if (!m_pkAuthPeer)
		return;

	TPacketBillingLogin p;

	p.dwLoginKey = pkLD->GetKey();
	p.bLogin = bLogin ? 1 : 0;

	DWORD dwCount = 1;
	m_pkAuthPeer->EncodeHeader(HEADER_DG_BILLING_LOGIN, 0, sizeof(DWORD) + sizeof(TPacketBillingLogin));
	m_pkAuthPeer->EncodeDWORD(dwCount);
	m_pkAuthPeer->Encode(&p, sizeof(TPacketBillingLogin));
}

void CClientManager::CreateObject(TPacketGDCreateObject * p)
{
	using namespace building;

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO object%s (land_id, vnum, map_index, x, y, x_rot, y_rot, z_rot) VALUES(%u, %u, %d, %d, %d, %f, %f, %f)",
			GetTablePostfix(), p->dwLandID, p->dwVnum, p->lMapIndex, p->x, p->y, p->xRot, p->yRot, p->zRot);

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiInsertID == 0)
	{
		sys_err("cannot insert object");
		return;
	}

	TObject * pkObj = new TObject;

	memset(pkObj, 0, sizeof(TObject));

	pkObj->dwID = pmsg->Get()->uiInsertID;
	pkObj->dwVnum = p->dwVnum;
	pkObj->dwLandID = p->dwLandID;
	pkObj->lMapIndex = p->lMapIndex;
	pkObj->x = p->x;
	pkObj->y = p->y;
	pkObj->xRot = p->xRot;
	pkObj->yRot = p->yRot;
	pkObj->zRot = p->zRot;
	pkObj->lLife = 0;

	ForwardPacket(HEADER_DG_CREATE_OBJECT, pkObj, sizeof(TObject));

	m_map_pkObjectTable.insert(std::make_pair(pkObj->dwID, pkObj));
}

void CClientManager::DeleteObject(DWORD dwID)
{
	char szQuery[128];

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM object%s WHERE id=%u", GetTablePostfix(), dwID);

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		sys_err("no object by id %u", dwID);
		return;
	}

	itertype(m_map_pkObjectTable) it = m_map_pkObjectTable.find(dwID);

	if (it != m_map_pkObjectTable.end())
	{
		delete it->second;
		m_map_pkObjectTable.erase(it);
	}

	ForwardPacket(HEADER_DG_DELETE_OBJECT, &dwID, sizeof(DWORD));
}

void CClientManager::UpdateLand(DWORD * pdw)
{
	DWORD dwID = pdw[0];
	DWORD dwGuild = pdw[1];

	building::TLand * p = &m_vec_kLandTable[0];

	DWORD i;

	for (i = 0; i < m_vec_kLandTable.size(); ++i, ++p)
	{
		if (p->dwID == dwID)
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "UPDATE land%s SET guild_id=%u WHERE id=%u", GetTablePostfix(), dwGuild, dwID);
			CDBManager::instance().AsyncQuery(buf);

			p->dwGuildID = dwGuild;
			break;
		}
	}

	if (i < m_vec_kLandTable.size())
		ForwardPacket(HEADER_DG_UPDATE_LAND, p, sizeof(building::TLand));
}

void CClientManager::VCard(TPacketGDVCard * p)
{
	sys_log(0, "VCARD: %u %s %s %s %s",
			p->dwID, p->szSellCharacter, p->szSellAccount, p->szBuyCharacter, p->szBuyAccount);

	m_queue_vcard.push(*p);
}

void CClientManager::VCardProcess()
{
	if (!m_pkAuthPeer)
		return;

	while (!m_queue_vcard.empty())
	{
		m_pkAuthPeer->EncodeHeader(HEADER_DG_VCARD, 0, sizeof(TPacketGDVCard));
		m_pkAuthPeer->Encode(&m_queue_vcard.front(), sizeof(TPacketGDVCard));

		m_queue_vcard.pop();
	}
}

// BLOCK_CHAT
void CClientManager::BlockChat(TPacketBlockChat* p)
{
	char szQuery[256];

	if (g_stLocale == "sjis")
		snprintf(szQuery, sizeof(szQuery), "SELECT id FROM player%s WHERE name = '%s' collate sjis_japanese_ci", GetTablePostfix(), p->szName);
	else
		snprintf(szQuery, sizeof(szQuery), "SELECT id FROM player%s WHERE name = '%s'", GetTablePostfix(), p->szName);
	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));
	SQLResult * pRes = pmsg->Get();

	if (pRes->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);
		DWORD pid = strtoul(row[0], NULL, 10);

		TPacketGDAddAffect pa;
		pa.dwPID = pid;
		pa.elem.dwType = 223;
		pa.elem.bApplyOn = 0;
		pa.elem.lApplyValue = 0;
		pa.elem.dwFlag = 0;
		pa.elem.lDuration = p->lDuration;
		pa.elem.lSPCost = 0;
		QUERY_ADD_AFFECT(NULL, &pa);
	}
	else
	{
		// cannot find user with that name
	}
}
// END_OF_BLOCK_CHAT

void CClientManager::MarriageAdd(TPacketMarriageAdd * p)
{
	sys_log(0, "MarriageAdd %u %u %s %s", p->dwPID1, p->dwPID2, p->szName1, p->szName2);
	marriage::CManager::instance().Add(p->dwPID1, p->dwPID2, p->szName1, p->szName2);
}

void CClientManager::MarriageUpdate(TPacketMarriageUpdate * p)
{
	sys_log(0, "MarriageUpdate PID:%u %u LP:%d ST:%d", p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
	marriage::CManager::instance().Update(p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
}

void CClientManager::MarriageRemove(TPacketMarriageRemove * p)
{
	sys_log(0, "MarriageRemove %u %u", p->dwPID1, p->dwPID2);
	marriage::CManager::instance().Remove(p->dwPID1, p->dwPID2);
}

void CClientManager::WeddingRequest(TPacketWeddingRequest * p)
{
	sys_log(0, "WeddingRequest %u %u", p->dwPID1, p->dwPID2);
	ForwardPacket(HEADER_DG_WEDDING_REQUEST, p, sizeof(TPacketWeddingRequest));
	//marriage::CManager::instance().RegisterWedding(p->dwPID1, p->szName1, p->dwPID2, p->szName2);
}

void CClientManager::WeddingReady(TPacketWeddingReady * p)
{
	sys_log(0, "WeddingReady %u %u", p->dwPID1, p->dwPID2);
	ForwardPacket(HEADER_DG_WEDDING_READY, p, sizeof(TPacketWeddingReady));
	marriage::CManager::instance().ReadyWedding(p->dwMapIndex, p->dwPID1, p->dwPID2);
}

void CClientManager::WeddingEnd(TPacketWeddingEnd * p)
{
	sys_log(0, "WeddingEnd %u %u", p->dwPID1, p->dwPID2);
	marriage::CManager::instance().EndWedding(p->dwPID1, p->dwPID2);
}

void CPacketInfo::Add(int header)
{
	itertype(m_map_info) it = m_map_info.find(header);

	if (it == m_map_info.end())
		m_map_info.insert(std::map<int, int>::value_type(header, 1));
	else
		++it->second;
}

void CPacketInfo::Reset()
{
	m_map_info.clear();
}

void CClientManager::ProcessPackets(CPeer * peer)
{
	BYTE		header;
	DWORD		dwHandle;
	DWORD		dwLength;
	const char * data = NULL;
	int			i = 0;
	int			iCount = 0;

	while (peer->PeekPacket(i, header, dwHandle, dwLength, &data))
	{
		// DISABLE_DB_HEADER_LOG
		// sys_log(0, "header %d %p size %d", header, this, dwLength);
		// END_OF_DISABLE_DB_HEADER_LOG
		m_bLastHeader = header;
		++iCount;

#ifdef _TEST
		if (header != 10)
			sys_log(0, " ProcessPacket Header [%d] Handle[%d] Length[%d] iCount[%d]", header, dwHandle, dwLength, iCount);
#endif
		if (g_test_server)
		{
			if (header != 10)
				sys_log(0, " ProcessPacket Header [%d] Handle[%d] Length[%d] iCount[%d]", header, dwHandle, dwLength, iCount);
		}


		// test log by mhh
		{
			if (HEADER_GD_BLOCK_COUNTRY_IP == header)
				sys_log(0, "recved : HEADER_GD_BLOCK_COUNTRY_IP");
		}

		switch (header)
		{
			case HEADER_GD_BOOT:
				QUERY_BOOT(peer, (TPacketGDBoot *) data);
				break;

			case HEADER_GD_HAMMER_OF_TOR:
				break;

			case HEADER_GD_LOGIN_BY_KEY:
				QUERY_LOGIN_BY_KEY(peer, dwHandle, (TPacketGDLoginByKey *) data);
				break;

			case HEADER_GD_LOGOUT:
				//sys_log(0, "HEADER_GD_LOGOUT (handle: %d length: %d)", dwHandle, dwLength);
				QUERY_LOGOUT(peer, dwHandle, data);
				break;

			case HEADER_GD_PLAYER_LOAD:
				sys_log(1, "HEADER_GD_PLAYER_LOAD (handle: %d length: %d)", dwHandle, dwLength);
				QUERY_PLAYER_LOAD(peer, dwHandle, (TPlayerLoadPacket *) data);
				break;

			case HEADER_GD_PLAYER_SAVE:
				sys_log(1, "HEADER_GD_PLAYER_SAVE (handle: %d length: %d)", dwHandle, dwLength);
				QUERY_PLAYER_SAVE(peer, dwHandle, (TPlayerTable *) data);
				break;

			case HEADER_GD_PLAYER_CREATE:
				sys_log(0, "HEADER_GD_PLAYER_CREATE (handle: %d length: %d)", dwHandle, dwLength);
				__QUERY_PLAYER_CREATE(peer, dwHandle, (TPlayerCreatePacket *) data);
				sys_log(0, "END");
				break;

			case HEADER_GD_PLAYER_DELETE:
				sys_log(1, "HEADER_GD_PLAYER_DELETE (handle: %d length: %d)", dwHandle, dwLength);
				__QUERY_PLAYER_DELETE(peer, dwHandle, (TPlayerDeletePacket *) data);
				break;

			case HEADER_GD_PLAYER_COUNT:
				QUERY_PLAYER_COUNT(peer, (TPlayerCountPacket *) data);
				break;

			case HEADER_GD_QUEST_SAVE:
				sys_log(1, "HEADER_GD_QUEST_SAVE (handle: %d length: %d)", dwHandle, dwLength);
				QUERY_QUEST_SAVE(peer, (TQuestTable *) data, dwLength);
				break;

			case HEADER_GD_SAFEBOX_LOAD:
				QUERY_SAFEBOX_LOAD(peer, dwHandle, (TSafeboxLoadPacket *) data, 0);
				break;

			case HEADER_GD_SAFEBOX_SAVE:
				sys_log(1, "HEADER_GD_SAFEBOX_SAVE (handle: %d length: %d)", dwHandle, dwLength);
				QUERY_SAFEBOX_SAVE(peer, (TSafeboxTable *) data);
				break;

			case HEADER_GD_SAFEBOX_CHANGE_SIZE:
				QUERY_SAFEBOX_CHANGE_SIZE(peer, dwHandle, (TSafeboxChangeSizePacket *) data);
				break;

			case HEADER_GD_SAFEBOX_CHANGE_PASSWORD:
				QUERY_SAFEBOX_CHANGE_PASSWORD(peer, dwHandle, (TSafeboxChangePasswordPacket *) data);
				break;

			case HEADER_GD_MALL_LOAD:
				QUERY_SAFEBOX_LOAD(peer, dwHandle, (TSafeboxLoadPacket *) data, 1);
				break;

			case HEADER_GD_EMPIRE_SELECT:
				QUERY_EMPIRE_SELECT(peer, dwHandle, (TEmpireSelectPacket *) data);
				break;

			case HEADER_GD_SETUP:
				QUERY_SETUP(peer, dwHandle, data);
				break;

			case HEADER_GD_GUILD_CREATE:
				GuildCreate(peer, *(DWORD *) data);
				break;

			case HEADER_GD_GUILD_SKILL_UPDATE:
				GuildSkillUpdate(peer, (TPacketGuildSkillUpdate *) data);
				break;

			case HEADER_GD_GUILD_EXP_UPDATE:
				GuildExpUpdate(peer, (TPacketGuildExpUpdate *) data);
				break;

			case HEADER_GD_GUILD_ADD_MEMBER:
				GuildAddMember(peer, (TPacketGDGuildAddMember*) data);
				break;

			case HEADER_GD_GUILD_REMOVE_MEMBER:
				GuildRemoveMember(peer, (TPacketGuild*) data);
				break;

			case HEADER_GD_GUILD_CHANGE_GRADE:
				GuildChangeGrade(peer, (TPacketGuild*) data);
				break;

			case HEADER_GD_GUILD_CHANGE_MEMBER_DATA:
				GuildChangeMemberData(peer, (TPacketGuildChangeMemberData*) data);
				break;

			case HEADER_GD_GUILD_DISBAND:
				GuildDisband(peer, (TPacketGuild*) data);
				break;

			case HEADER_GD_GUILD_WAR:
				GuildWar(peer, (TPacketGuildWar*) data);
				break;

			case HEADER_GD_GUILD_WAR_SCORE:
				GuildWarScore(peer, (TPacketGuildWarScore*) data);
				break;

			case HEADER_GD_GUILD_CHANGE_LADDER_POINT:
				GuildChangeLadderPoint((TPacketGuildLadderPoint*) data);
				break;

			case HEADER_GD_GUILD_USE_SKILL:
				GuildUseSkill((TPacketGuildUseSkill*) data);
				break;

			case HEADER_GD_FLUSH_CACHE:
				QUERY_FLUSH_CACHE(peer, data);
				break;

			case HEADER_GD_ITEM_SAVE:
				QUERY_ITEM_SAVE(peer, data);
				break;

			case HEADER_GD_ITEM_DESTROY:
				QUERY_ITEM_DESTROY(peer, data);
				break;

			case HEADER_GD_ITEM_FLUSH:
				QUERY_ITEM_FLUSH(peer, data);
				break;

			case HEADER_GD_ADD_AFFECT:
				sys_log(1, "HEADER_GD_ADD_AFFECT");
				QUERY_ADD_AFFECT(peer, (TPacketGDAddAffect *) data);
				break;

			case HEADER_GD_REMOVE_AFFECT:
				sys_log(1, "HEADER_GD_REMOVE_AFFECT");
				QUERY_REMOVE_AFFECT(peer, (TPacketGDRemoveAffect *) data);
				break;

			case HEADER_GD_HIGHSCORE_REGISTER:
				QUERY_HIGHSCORE_REGISTER(peer, (TPacketGDHighscore *) data);
				break;

			case HEADER_GD_PARTY_CREATE:
				QUERY_PARTY_CREATE(peer, (TPacketPartyCreate*) data);
				break;

			case HEADER_GD_PARTY_DELETE:
				QUERY_PARTY_DELETE(peer, (TPacketPartyDelete*) data);
				break;

			case HEADER_GD_PARTY_ADD:
				QUERY_PARTY_ADD(peer, (TPacketPartyAdd*) data);
				break;

			case HEADER_GD_PARTY_REMOVE:
				QUERY_PARTY_REMOVE(peer, (TPacketPartyRemove*) data);
				break;

			case HEADER_GD_PARTY_STATE_CHANGE:
				QUERY_PARTY_STATE_CHANGE(peer, (TPacketPartyStateChange*) data);
				break;

			case HEADER_GD_PARTY_SET_MEMBER_LEVEL:
				QUERY_PARTY_SET_MEMBER_LEVEL(peer, (TPacketPartySetMemberLevel*) data);
				break;

			case HEADER_GD_RELOAD_PROTO:
				QUERY_RELOAD_PROTO();
				break;

			case HEADER_GD_CHANGE_NAME:
				QUERY_CHANGE_NAME(peer, dwHandle, (TPacketGDChangeName *) data);
				break;

			case HEADER_GD_SMS:
				QUERY_SMS(peer, (TPacketGDSMS *) data);
				break;

			case HEADER_GD_AUTH_LOGIN:
				QUERY_AUTH_LOGIN(peer, dwHandle, (TPacketGDAuthLogin *) data);
				break;

			case HEADER_GD_REQUEST_GUILD_PRIV:
				AddGuildPriv((TPacketGiveGuildPriv*)data);
				break;

			case HEADER_GD_REQUEST_EMPIRE_PRIV:
				AddEmpirePriv((TPacketGiveEmpirePriv*)data);
				break;

			case HEADER_GD_REQUEST_CHARACTER_PRIV:
				AddCharacterPriv((TPacketGiveCharacterPriv*) data);
				break;

			case HEADER_GD_MONEY_LOG:
				MoneyLog((TPacketMoneyLog*)data);
				break;

			case HEADER_GD_GUILD_DEPOSIT_MONEY:
				GuildDepositMoney((TPacketGDGuildMoney*)data);
				break;

			case HEADER_GD_GUILD_WITHDRAW_MONEY:
				GuildWithdrawMoney(peer, (TPacketGDGuildMoney*)data);
				break;

			case HEADER_GD_GUILD_WITHDRAW_MONEY_GIVE_REPLY:
				GuildWithdrawMoneyGiveReply((TPacketGDGuildMoneyWithdrawGiveReply*)data);
				break;

			case HEADER_GD_GUILD_WAR_BET:
				GuildWarBet((TPacketGDGuildWarBet *) data);
				break;

			case HEADER_GD_SET_EVENT_FLAG:
				SetEventFlag((TPacketSetEventFlag*) data);
				break;

			case HEADER_GD_BILLING_EXPIRE:
				BillingExpire((TPacketBillingExpire *) data);
				break;

			case HEADER_GD_BILLING_CHECK:
				BillingCheck(data);
				break;

			case HEADER_GD_CREATE_OBJECT:
				CreateObject((TPacketGDCreateObject *) data);
				break;

			case HEADER_GD_DELETE_OBJECT:
				DeleteObject(*(DWORD *) data);
				break;

			case HEADER_GD_UPDATE_LAND:
				UpdateLand((DWORD *) data);
				break;

			case HEADER_GD_VCARD:
				VCard((TPacketGDVCard *) data);
				break;

			case HEADER_GD_MARRIAGE_ADD:
				MarriageAdd((TPacketMarriageAdd *) data);
				break;

			case HEADER_GD_MARRIAGE_UPDATE:
				MarriageUpdate((TPacketMarriageUpdate *) data);
				break;

			case HEADER_GD_MARRIAGE_REMOVE:
				MarriageRemove((TPacketMarriageRemove *) data);
				break;

			case HEADER_GD_WEDDING_REQUEST:
				WeddingRequest((TPacketWeddingRequest *) data);
				break;

			case HEADER_GD_WEDDING_READY:
				WeddingReady((TPacketWeddingReady *) data);
				break;

			case HEADER_GD_WEDDING_END:
				WeddingEnd((TPacketWeddingEnd *) data);
				break;

				// BLOCK_CHAT
			case HEADER_GD_BLOCK_CHAT:
				BlockChat((TPacketBlockChat *) data);
				break;
				// END_OF_BLOCK_CHAT

				//RELOAD_ADMIN
			case HEADER_GD_RELOAD_ADMIN:
				ReloadAdmin(peer, (TPacketReloadAdmin*)data);
				break;
				//END_RELOAD_ADMIN

			case HEADER_GD_BREAK_MARRIAGE:
				BreakMarriage(peer, data);
				break;

			//MOANRCH
			case HEADER_GD_ELECT_MONARCH:
				Election(peer, dwHandle, data);
				break;

			case HEADER_GD_CANDIDACY:
				Candidacy(peer, dwHandle, data);
				break;

			case HEADER_GD_ADD_MONARCH_MONEY:
				AddMonarchMoney(peer, dwHandle, data);
				break;

			case HEADER_GD_DEC_MONARCH_MONEY:
				DecMonarchMoney(peer, dwHandle, data);
				break;

			case HEADER_GD_TAKE_MONARCH_MONEY:
				TakeMonarchMoney(peer, dwHandle, data);
				break;

			case HEADER_GD_COME_TO_VOTE:
				ComeToVote(peer, dwHandle, data);
				break;

			case HEADER_GD_RMCANDIDACY:		//< 후보 제거 (운영자)
				RMCandidacy(peer, dwHandle, data);
				break;

			case HEADER_GD_SETMONARCH:		///<군주설정 (운영자)
				SetMonarch(peer, dwHandle, data);
				break;

			case HEADER_GD_RMMONARCH:		///<군주삭제
				RMMonarch(peer, dwHandle, data);
				break;
			//END_MONARCH

			case HEADER_GD_CHANGE_MONARCH_LORD :
				ChangeMonarchLord(peer, dwHandle, (TPacketChangeMonarchLord*)data);
				break;

			case HEADER_GD_BLOCK_COUNTRY_IP:
				sys_log(0, "HEADER_GD_BLOCK_COUNTRY_IP received");
				CBlockCountry::instance().SendBlockedCountryIp(peer);
				CBlockCountry::instance().SendBlockException(peer);
				break;

			case HEADER_GD_BLOCK_EXCEPTION:
				sys_log(0, "HEADER_GD_BLOCK_EXCEPTION received");
				BlockException((TPacketBlockException*) data);
				break;

			case HEADER_GD_REQ_SPARE_ITEM_ID_RANGE :
				SendSpareItemIDRange(peer);
				break;

			case HEADER_GD_REQ_CHANGE_GUILD_MASTER :
				GuildChangeMaster((TPacketChangeGuildMaster*) data);
				break;

			case HEADER_GD_UPDATE_HORSE_NAME :
				UpdateHorseName((TPacketUpdateHorseName*) data, peer);
				break;

			case HEADER_GD_REQ_HORSE_NAME :
				AckHorseName(*(DWORD*)data, peer);
				break;

			case HEADER_GD_DC:
				DeleteLoginKey((TPacketDC*) data);
				break;

			case HEADER_GD_VALID_LOGOUT:
				ResetLastPlayerID((TPacketNeedLoginLogInfo*)data);
				break;
#if defined(WJ_COMBAT_ZONE)	
			case HEADER_GD_COMBAT_ZONE_RESET_RANKING:
				CombatZoneResetRanking((TPacketGDCombatZoneResetRanking*)data);
				break;
				
			case HEADER_GD_COMBAT_ZONE_SKILLS_CACHE:
				UpdateSkillsCache((TPacketGDCombatZoneSkillsCache*)data);
				break;
#endif
			case HEADER_GD_REQUEST_CHARGE_CASH:
				ChargeCash((TRequestChargeCash*)data);
				break;

			//delete gift notify icon

			case HEADER_GD_DELETE_AWARDID:
				DeleteAwardId((TPacketDeleteAwardID*) data);
				break;

			case HEADER_GD_UPDATE_CHANNELSTATUS:
				UpdateChannelStatus((SChannelStatus*) data);
				break;
			case HEADER_GD_REQUEST_CHANNELSTATUS:
				RequestChannelStatus(peer, dwHandle);
				break;
#ifdef __AUCTION__
			case HEADER_GD_COMMAND_AUCTION:
			{
				TPacketGDCommnadAuction* auction_data = (TPacketGDCommnadAuction*)data;

				switch (auction_data->get_cmd())
				{
				case AUCTION_ENR_AUC:
					EnrollInAuction (peer, dwHandle, (AuctionEnrollProductInfo*)data);
					break;
				case AUCTION_ENR_SALE:
					EnrollInSale (peer, dwHandle, (AuctionEnrollSaleInfo*)data);
					break;
				case AUCTION_ENR_WISH:
					EnrollInWish (peer, dwHandle, (AuctionEnrollWishInfo*)data);
					break;
				case AUCTION_BID:
					AuctionBid (peer, dwHandle, (AuctionBidInfo*)data);
					break;
				case AUCTION_IMME_PUR:
					AuctionImpur (peer, dwHandle, (AuctionImpurInfo*)data);
					break;
				case AUCTION_GET_AUC:
					AuctionGetAuctionedItem (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_BUY_SOLD:
					AuctionBuySoldItem (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_CANCEL_AUC:
					AuctionCancelAuction (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_CANCEL_WISH:
					AuctionCancelWish (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_CANCEL_SALE:
					AuctionCancelSale (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_DELETE_AUCTION_ITEM:
					AuctionDeleteAuctionItem (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_DELETE_SALE_ITEM:
					AuctionDeleteSaleItem (peer, dwHandle, auction_data->get_item());
					break;
				case AUCTION_REBID:
					AuctionReBid (peer, dwHandle, (AuctionBidInfo*)data);
					break;

//				case AUCTION_BID_CANCEL:
//					AuctionBidCancel (peer, dwHandle, data->get_item());
				default :
					break;
				}
			}
			break;
#endif
#ifdef ENABLE_OFFLINESHOP_SYSTEM
		case HEADER_GD_OFFLINESHOP:
			RecvPackets(data);
			break;
#endif
#ifdef ENABLE_ITEMSHOP
		case HEADER_GD_ITEMSHOP:
			RecvItemShop(peer, dwHandle, data);
			break;
#endif
#ifdef ENABLE_EVENT_MANAGER
		case HEADER_GD_EVENT_MANAGER:
			RecvEventManagerPacket(data);
			break;
#endif
			default:
				sys_err("Unknown header (header: %d handle: %d length: %d)", header, dwHandle, dwLength);
				break;
		}
	}

	peer->RecvEnd(i);
}

void CClientManager::AddPeer(socket_t fd)
{
	CPeer * pPeer = new CPeer;

	if (pPeer->Accept(fd))
		m_peerList.push_front(pPeer);
	else
		delete pPeer;
}

void CClientManager::RemovePeer(CPeer * pPeer)
{
	if (m_pkAuthPeer == pPeer)
	{
		m_pkAuthPeer = NULL;
	}
	else
	{
		TLogonAccountMap::iterator it = m_map_kLogonAccount.begin();

		while (it != m_map_kLogonAccount.end())
		{
			CLoginData * pkLD = it->second;

			if (pkLD->GetConnectedPeerHandle() == pPeer->GetHandle())
			{
				if (pkLD->IsPlay())
				{
					pkLD->SetPlay(false);
					SendLoginToBilling(pkLD, false);
				}

				if (pkLD->IsDeleted())
				{
					sys_log(0, "DELETING LoginData");
					delete pkLD;
				}

				m_map_kLogonAccount.erase(it++);
			}
			else
				++it;
		}
	}

	m_peerList.remove(pPeer);
	delete pPeer;
}

CPeer * CClientManager::GetPeer(IDENT ident)
{
	for (itertype(m_peerList) i = m_peerList.begin(); i != m_peerList.end();++i)
	{
		CPeer * tmp = *i;

		if (tmp->GetHandle() == ident)
			return tmp;
	}

	return NULL;
}

CPeer * CClientManager::GetAnyPeer()
{
	if (m_peerList.empty())
		return NULL;

	return m_peerList.front();
}

// DB 매니저로 부터 받은 결과를 처리한다.
//
// @version	05/06/10 Bang2ni - 가격정보 관련 쿼리(QID_ITEMPRICE_XXX) 추가
int CClientManager::AnalyzeQueryResult(SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	CPeer * peer = GetPeer(qi->dwIdent);

#ifdef _TEST
	if (qi->iType != QID_ITEM_AWARD_LOAD)
	sys_log(0, "AnalyzeQueryResult %d", qi->iType);
#endif
	switch (qi->iType)
	{
		case QID_ITEM_AWARD_LOAD:
			ItemAwardManager::instance().Load(msg);
			delete qi;
			return true;

		case QID_GUILD_RANKING:
			CGuildManager::instance().ResultRanking(msg->Get()->pSQLResult);
			break;
	}

	if (!peer)
	{
		//sys_err("CClientManager::AnalyzeQueryResult: peer not exist anymore. (ident: %d)", qi->dwIdent);
		delete qi;
		return true;
	}

	switch (qi->iType)
	{
		case QID_PLAYER:
		case QID_ITEM:
		case QID_QUEST:
		case QID_AFFECT:

			RESULT_COMPOSITE_PLAYER(peer, msg, qi->iType);
			break;

		case QID_LOGIN:
			RESULT_LOGIN(peer, msg);
			break;

		case QID_SAFEBOX_LOAD:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_LOAD");
			RESULT_SAFEBOX_LOAD(peer, msg);
			break;

		case QID_SAFEBOX_CHANGE_SIZE:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_SIZE");
			RESULT_SAFEBOX_CHANGE_SIZE(peer, msg);
			break;

		case QID_SAFEBOX_CHANGE_PASSWORD:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_PASSWORD %p", msg);
			RESULT_SAFEBOX_CHANGE_PASSWORD(peer, msg);
			break;

		case QID_SAFEBOX_CHANGE_PASSWORD_SECOND:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_PASSWORD %p", msg);
			RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(peer, msg);
			break;

		case QID_HIGHSCORE_REGISTER:
			sys_log(0, "QUERY_RESULT: HEADER_GD_HIGHSCORE_REGISTER %p", msg);
			RESULT_HIGHSCORE_REGISTER(peer, msg);
			break;

		case QID_SAFEBOX_SAVE:
		case QID_ITEM_SAVE:
		case QID_ITEM_DESTROY:
		case QID_QUEST_SAVE:
		case QID_PLAYER_SAVE:
		case QID_ITEM_AWARD_TAKEN:
			break;

			// PLAYER_INDEX_CREATE_BUG_FIX
		case QID_PLAYER_INDEX_CREATE:
			RESULT_PLAYER_INDEX_CREATE(peer, msg);
			break;
			// END_PLAYER_INDEX_CREATE_BUG_FIX

		case QID_PLAYER_DELETE:
			__RESULT_PLAYER_DELETE(peer, msg);
			break;

		case QID_LOGIN_BY_KEY:
			RESULT_LOGIN_BY_KEY(peer, msg);
			break;

		default:
			sys_log(0, "CClientManager::AnalyzeQueryResult unknown query result type: %d, str: %s", qi->iType, msg->stQuery.c_str());
			break;
	}

	delete qi;
	return true;
}

void UsageLog()
{
	FILE* fp = NULL;

	time_t      ct;
	char        *time_s;
	struct tm   lt;

	int         avg = g_dwUsageAvg / 3600; // 60 초 * 60 분

	fp = fopen("usage.txt", "a+");

	if (!fp)
		return;

	ct = time(0);
	lt = *localtime(&ct);
	time_s = asctime(&lt);

	time_s[strlen(time_s) - 1] = '\0';

	fprintf(fp, "| %4d %-15.15s | %5d | %5u |", lt.tm_year + 1900, time_s + 4, avg, g_dwUsageMax);

	fprintf(fp, "\n");
	fclose(fp);

	g_dwUsageMax = g_dwUsageAvg = 0;
}

//#define ENABLE_ITEMAWARD_REFRESH
int CClientManager::Process()
{
	int pulses;

	if (!(pulses = thecore_idle()))
		return 0;

	while (pulses--)
	{
		++thecore_heart->pulse;

		/*
		//30분마다 변경
		if (((thecore_pulse() % (60 * 30 * 10)) == 0))
		{
			g_iPlayerCacheFlushSeconds = MAX(60, rand() % 180);
			g_iItemCacheFlushSeconds = MAX(60, rand() % 180);
			sys_log(0, "[SAVE_TIME]Change saving time item %d player %d", g_iPlayerCacheFlushSeconds, g_iItemCacheFlushSeconds);
		}
		*/

		if (!(thecore_heart->pulse % thecore_heart->passes_per_sec))
		{
			if (g_test_server)
			{

				if (!(thecore_heart->pulse % thecore_heart->passes_per_sec * 10))

				{
					pt_log("[%9d] return %d/%d/%d/%d async %d/%d/%d/%d",
							thecore_heart->pulse,
							CDBManager::instance().CountReturnQuery(SQL_PLAYER),
							CDBManager::instance().CountReturnResult(SQL_PLAYER),
							CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
							CDBManager::instance().CountReturnCopiedQuery(SQL_PLAYER),
							CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
							CDBManager::instance().CountAsyncResult(SQL_PLAYER),
							CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER),
							CDBManager::instance().CountAsyncCopiedQuery(SQL_PLAYER));

					if ((thecore_heart->pulse % 50) == 0)
						sys_log(0, "[%9d] return %d/%d/%d async %d/%d/%d",
								thecore_heart->pulse,
								CDBManager::instance().CountReturnQuery(SQL_PLAYER),
								CDBManager::instance().CountReturnResult(SQL_PLAYER),
								CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
								CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
								CDBManager::instance().CountAsyncResult(SQL_PLAYER),
								CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER));
				}
			}
			else
			{
				pt_log("[%9d] return %d/%d/%d/%d async %d/%d/%d%/%d",
						thecore_heart->pulse,
						CDBManager::instance().CountReturnQuery(SQL_PLAYER),
						CDBManager::instance().CountReturnResult(SQL_PLAYER),
						CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
						CDBManager::instance().CountReturnCopiedQuery(SQL_PLAYER),
						CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
						CDBManager::instance().CountAsyncResult(SQL_PLAYER),
						CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER),
						CDBManager::instance().CountAsyncCopiedQuery(SQL_PLAYER));

						if ((thecore_heart->pulse % 50) == 0)
						sys_log(0, "[%9d] return %d/%d/%d async %d/%d/%d",
							thecore_heart->pulse,
							CDBManager::instance().CountReturnQuery(SQL_PLAYER),
							CDBManager::instance().CountReturnResult(SQL_PLAYER),
							CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
							CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
							CDBManager::instance().CountAsyncResult(SQL_PLAYER),
							CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER));
						}

			CDBManager::instance().ResetCounter();

			DWORD dwCount = CClientManager::instance().GetUserCount();

			g_dwUsageAvg += dwCount;
			g_dwUsageMax = MAX(g_dwUsageMax, dwCount);

			memset(&thecore_profiler[0], 0, sizeof(thecore_profiler));

			if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 3600)))
				UsageLog();

			m_iCacheFlushCount = 0;


			//플레이어 플러쉬
			UpdatePlayerCache();
			//아이템 플러쉬
			UpdateItemCache();
			//로그아웃시 처리- 캐쉬셋 플러쉬
			UpdateLogoutPlayer();


#ifdef ENABLE_AUTOMATIC_EVENT
			ProcessEventTable();
#endif

			CGuildManager::instance().Update();
			CPrivManager::instance().Update();
			marriage::CManager::instance().Update();
#ifdef ENABLE_EVENT_MANAGER
			UpdateEventManager();
#endif
		}
#ifdef ENABLE_ITEMAWARD_REFRESH
		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 5)))
		{
			ItemAwardManager::instance().RequestLoad();
		}
#endif
		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 10)))
		{
			/*
			char buf[4096 + 1];
			int len
			itertype(g_query_info.m_map_info) it;

			/////////////////////////////////////////////////////////////////
			buf[0] = '\0';
			len = 0;

			it = g_query_info.m_map_info.begin();

			int count = 0;

			while (it != g_query_info.m_map_info.end())
			{
				len += snprintf(buf + len, sizeof(buf) - len, "%2d %3d\n", it->first, it->second);
				count += it->second;
				it++;
			}

			pt_log("QUERY:\n%s-------------------- MAX : %d\n", buf, count);
			g_query_info.Reset();
			*/
			pt_log("QUERY: MAIN[%d] ASYNC[%d]", g_query_count[0], g_query_count[1]);
			g_query_count[0] = 0;
			g_query_count[1] = 0;
			/////////////////////////////////////////////////////////////////

			/////////////////////////////////////////////////////////////////
			/*
			buf[0] = '\0';
			len = 0;

			it = g_item_info.m_map_info.begin();

			count = 0;
			while (it != g_item_info.m_map_info.end())
			{
				len += snprintf(buf + len, sizeof(buf) - len, "%5d %3d\n", it->first, it->second);
				count += it->second;
				it++;
			}

			pt_log("ITEM:\n%s-------------------- MAX : %d\n", buf, count);
			g_item_info.Reset();
			*/
			pt_log("ITEM:%d\n", g_item_count);
			g_item_count = 0;
			/////////////////////////////////////////////////////////////////
		}

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 60)))	// 60초에 한번
		{
			// 유니크 아이템을 위한 시간을 보낸다.
			CClientManager::instance().SendTime();

			// 현재 연결된 peer의 host 및 port를 출력한다.
			std::string st;
			CClientManager::instance().GetPeerP2PHostNames(st);
			sys_log(0, "Current Peer host names...\n%s", st.c_str());
		}

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 3600)))	// 한시간에 한번
		{
			CMoneyLog::instance().Save();
		}




	}

	int num_events = fdwatch(m_fdWatcher, 0);
	int idx;
	CPeer * peer;

	for (idx = 0; idx < num_events; ++idx) // 인풋
	{
		peer = (CPeer *) fdwatch_get_client_data(m_fdWatcher, idx);

		if (!peer)
		{
			if (fdwatch_check_event(m_fdWatcher, m_fdAccept, idx) == FDW_READ)
			{
				AddPeer(m_fdAccept);
				fdwatch_clear_event(m_fdWatcher, m_fdAccept, idx);
			}
			else
			{
				sys_log(0, "FDWATCH: peer null in event: ident %d", fdwatch_get_ident(m_fdWatcher, idx)); // @warme012
			}

			continue;
		}

		switch (fdwatch_check_event(m_fdWatcher, peer->GetFd(), idx))
		{
			case FDW_READ:
				if (peer->Recv() < 0)
				{
					sys_err("Recv failed");
					RemovePeer(peer);
				}
				else
				{
					if (peer == m_pkAuthPeer)
						if (g_log)
							sys_log(0, "AUTH_PEER_READ: size %d", peer->GetRecvLength());

					ProcessPackets(peer);
				}
				break;

			case FDW_WRITE:
				if (peer == m_pkAuthPeer)
					if (g_log)
						sys_log(0, "AUTH_PEER_WRITE: size %d", peer->GetSendLength());

				if (peer->Send() < 0)
				{
					sys_err("Send failed");
					RemovePeer(peer);
				}

				break;

			case FDW_EOF:
				RemovePeer(peer);
				break;

			default:
				sys_err("fdwatch_check_fd returned unknown result");
				RemovePeer(peer);
				break;
		}
	}

#ifdef __WIN32__
	if (_kbhit()) {
		int c = _getch();
		switch (c) {
			case 0x1b: // Esc
				return 0; // shutdown
				break;
			default:
				break;
		}
	}
#endif

	VCardProcess();
	return 1;
}

DWORD CClientManager::GetUserCount()
{
	// 단순히 로그인 카운트를 센다.. --;
	return m_map_kLogonAccount.size();
}

void CClientManager::SendAllGuildSkillRechargePacket()
{
	ForwardPacket(HEADER_DG_GUILD_SKILL_RECHARGE, NULL, 0);
}

void CClientManager::SendTime()
{
	time_t now = GetCurrentTime();
	ForwardPacket(HEADER_DG_TIME, &now, sizeof(time_t));
}

void CClientManager::ForwardPacket(BYTE header, const void* data, int size, BYTE bChannel, CPeer* except)
{
	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		CPeer * peer = *it;

		if (peer == except)
			continue;

		if (!peer->GetChannel())
			continue;

		if (bChannel && peer->GetChannel() != bChannel)
			continue;

		peer->EncodeHeader(header, 0, size);

		if (size > 0 && data)
			peer->Encode(data, size);
	}
}

void CClientManager::SendNotice(const char * c_pszFormat, ...)
{
	char szBuf[255+1];
	va_list args;

	va_start(args, c_pszFormat);
	int len = vsnprintf(szBuf, sizeof(szBuf), c_pszFormat, args);
	va_end(args);
	szBuf[len] = '\0';

	ForwardPacket(HEADER_DG_NOTICE, szBuf, len + 1);
}

time_t CClientManager::GetCurrentTime()
{
	return time(0);
}

// ITEM_UNIQUE_ID
bool CClientManager::InitializeNowItemID()
{
	DWORD dwMin, dwMax;

	//아이템 ID를 초기화 한다.
	if (!CConfig::instance().GetTwoValue("ITEM_ID_RANGE", &dwMin, &dwMax))
	{
		sys_err("conf.txt: Cannot find ITEM_ID_RANGE [start_item_id] [end_item_id]");
		return false;
	}

	sys_log(0, "ItemRange From File %u ~ %u ", dwMin, dwMax);

	if (CItemIDRangeManager::instance().BuildRange(dwMin, dwMax, m_itemRange) == false)
	{
		sys_err("Can not build ITEM_ID_RANGE");
		return false;
	}

	sys_log(0, " Init Success Start %u End %u Now %u\n", m_itemRange.dwMin, m_itemRange.dwMax, m_itemRange.dwUsableItemIDMin);

	return true;
}

DWORD CClientManager::GainItemID()
{
	return m_itemRange.dwUsableItemIDMin++;
}

DWORD CClientManager::GetItemID()
{
	return m_itemRange.dwUsableItemIDMin;
}
// ITEM_UNIQUE_ID_END
//BOOT_LOCALIZATION

bool CClientManager::InitializeLocalization()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "SELECT mValue, mKey FROM locale");
	SQLMsg * pMsg = CDBManager::instance().DirectQuery(szQuery, SQL_COMMON);

	if (pMsg->Get()->uiNumRows == 0)
	{
		sys_err("InitializeLocalization() ==> DirectQuery failed(%s)", szQuery);
		delete pMsg;
		return false;
	}

	sys_log(0, "InitializeLocalization() - LoadLocaleTable(count:%d)", pMsg->Get()->uiNumRows);

	m_vec_Locale.clear();

	MYSQL_ROW row = NULL;

	for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
	{
		int col = 0;
		tLocale locale;

		strlcpy(locale.szValue, row[col++], sizeof(locale.szValue));
		strlcpy(locale.szKey, row[col++], sizeof(locale.szKey));

		//DB_NAME_COLUMN Setting
		if (strcmp(locale.szKey, "LOCALE") == 0)
		{
			if (strcmp(locale.szValue, "cibn") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "gb2312");

				g_stLocale = "gb2312";
				g_stLocaleNameColumn = "gb2312name";
			}
			else if (strcmp(locale.szValue, "ymir") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "euckr";
				g_stLocaleNameColumn = "name";
			}
			else if (strcmp(locale.szValue, "japan") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "sjis");

				g_stLocale = "sjis";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "english") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "germany") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "france") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "italy") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "spain") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "uk") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "turkey") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin5";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "poland") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin2";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "portugal") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "hongkong") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "big5");

				g_stLocale = "big5";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "newcibn") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "gb2312");

				g_stLocale = "gb2312";
				g_stLocaleNameColumn = "gb2312name";
			}
			else if (strcmp(locale.szValue, "korea") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "euckr";
				g_stLocaleNameColumn = "name";
			}
			else if (strcmp(locale.szValue, "canada") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "gb2312name";
			}
			else if (strcmp(locale.szValue, "brazil") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "greek") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "greek";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "russia") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "cp1251";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "denmark") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "bulgaria") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "cp1251";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "croatia") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "cp1251";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "mexico") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "arabia") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "cp1256";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "czech") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin2";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "hungary") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin2";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "romania") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin2";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "netherlands") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "singapore") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "vietnam") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "thailand") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "usa") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "latin1");

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else if (strcmp(locale.szValue, "we_korea") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "euckr");

				g_stLocale = "euckr";
				g_stLocaleNameColumn = "name";
			}
			else if (strcmp(locale.szValue, "taiwan") == 0)
			{
				sys_log(0, "locale[LOCALE] = %s", locale.szValue);

				if (g_stLocale != locale.szValue)
					sys_log(0, "Changed g_stLocale %s to %s", g_stLocale.c_str(), "big5");
				g_stLocale = "big5";
				g_stLocaleNameColumn = "locale_name";
			}
			else
			{
				sys_err("locale[LOCALE] = UNKNOWN(%s)", locale.szValue);
				exit(0);
			}
			// @warme007
			// sys_log(0,"before call SetLocale: %s",g_stLocale.c_str());
			// CDBManager::instance().SetLocale(g_stLocale.c_str());
			// sys_log(0,"Called SetLocale");
		}
		else if (strcmp(locale.szKey, "DB_NAME_COLUMN") == 0)
		{
			sys_log(0, "locale[DB_NAME_COLUMN] = %s", locale.szValue);
			g_stLocaleNameColumn = locale.szValue;
		}
		else
		{
			sys_log(0, "locale[UNKNOWN_KEY(%s)] = %s", locale.szKey, locale.szValue);
		}
		m_vec_Locale.push_back(locale);
	}

	delete pMsg;

	return true;
}
//END_BOOT_LOCALIZATION
//ADMIN_MANAGER

bool CClientManager::__GetAdminInfo(const char *szIP, std::vector<tAdminInfo> & rAdminVec)
{
	//szIP == NULL 일경우  모든서버에 운영자 권한을 갖는다.
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT mID,mAccount,mName,mContactIP,mServerIP,mAuthority FROM gmlist WHERE mServerIP='ALL' or mServerIP='%s'",
		   	szIP ? szIP : "ALL");

	SQLMsg * pMsg = CDBManager::instance().DirectQuery(szQuery, SQL_COMMON);

	if (pMsg->Get()->uiNumRows == 0)
	{
		sys_err("__GetAdminInfo() ==> DirectQuery failed(%s)", szQuery);
		delete pMsg;
		return false;
	}

	MYSQL_ROW row;
	rAdminVec.reserve(pMsg->Get()->uiNumRows);

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		int idx = 0;
		tAdminInfo Info;

		str_to_number(Info.m_ID, row[idx++]);
		trim_and_lower(row[idx++], Info.m_szAccount, sizeof(Info.m_szAccount));
		strlcpy(Info.m_szName, row[idx++], sizeof(Info.m_szName));
		strlcpy(Info.m_szContactIP, row[idx++], sizeof(Info.m_szContactIP));
		strlcpy(Info.m_szServerIP, row[idx++], sizeof(Info.m_szServerIP));
		std::string stAuth = row[idx++];

		if (!stAuth.compare("IMPLEMENTOR"))
			Info.m_Authority = GM_IMPLEMENTOR;
		else if (!stAuth.compare("GOD"))
			Info.m_Authority = GM_GOD;
		else if (!stAuth.compare("HIGH_WIZARD"))
			Info.m_Authority = GM_HIGH_WIZARD;
		else if (!stAuth.compare("LOW_WIZARD"))
			Info.m_Authority = GM_LOW_WIZARD;
		else if (!stAuth.compare("WIZARD"))
			Info.m_Authority = GM_WIZARD;
		else
			continue;

		rAdminVec.push_back(Info);

		sys_log(0, "GM: PID %u Login %s Character %s ContactIP %s ServerIP %s Authority %d[%s]",
			   	Info.m_ID, Info.m_szAccount, Info.m_szName, Info.m_szContactIP, Info.m_szServerIP, Info.m_Authority, stAuth.c_str());
	}

	delete pMsg;

	return true;
}

bool CClientManager::__GetHostInfo(std::vector<std::string> & rIPVec)
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "SELECT mIP FROM gmhost");
	SQLMsg * pMsg = CDBManager::instance().DirectQuery(szQuery, SQL_COMMON);

	if (pMsg->Get()->uiNumRows == 0)
	{
		sys_err("__GetHostInfo() ==> DirectQuery failed(%s)", szQuery);
		delete pMsg;
		return false;
	}

	rIPVec.reserve(pMsg->Get()->uiNumRows);

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		if (row[0] && *row[0])
		{
			rIPVec.push_back(row[0]);
			sys_log(0, "GMHOST: %s", row[0]);
		}
	}

	delete pMsg;
	return true;
}
//END_ADMIN_MANAGER

void CClientManager::ReloadAdmin(CPeer*, TPacketReloadAdmin* p)
{
	std::vector<tAdminInfo> vAdmin;
	std::vector<std::string> vHost;

	__GetHostInfo(vHost);
	__GetAdminInfo(p->szIP, vAdmin);

	DWORD dwPacketSize = sizeof(WORD) + sizeof (WORD) + sizeof(tAdminInfo) * vAdmin.size() +
		  sizeof(WORD) + sizeof(WORD) + 16 * vHost.size();

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		CPeer * peer = *it;

		if (!peer->GetChannel())
			continue;

		peer->EncodeHeader(HEADER_DG_RELOAD_ADMIN, 0, dwPacketSize);

		peer->EncodeWORD(16);
		peer->EncodeWORD(vHost.size());

		for (size_t n = 0; n < vHost.size(); ++n)
			peer->Encode(vHost[n].c_str(), 16);

		peer->EncodeWORD(sizeof(tAdminInfo));
		peer->EncodeWORD(vAdmin.size());

		for (size_t n = 0; n < vAdmin.size(); ++n)
			peer->Encode(&vAdmin[n], sizeof(tAdminInfo));
	}

	sys_log(0, "ReloadAdmin End %s", p->szIP);
}

//BREAK_MARRIAGE
void CClientManager::BreakMarriage(CPeer * peer, const char * data)
{
	DWORD pid1, pid2;

	pid1 = *(int *) data;
	data += sizeof(int);

	pid2 = *(int *) data;
	data += sizeof(int);

	sys_log(0, "Breaking off a marriage engagement! pid %d and pid %d", pid1, pid2);
	marriage::CManager::instance().Remove(pid1, pid2);
}
//END_BREAK_MARIIAGE

void CClientManager::UpdateItemCacheSet(DWORD pid)
{
	itertype(m_map_pkItemCacheSetPtr) it = m_map_pkItemCacheSetPtr.find(pid);

	if (it == m_map_pkItemCacheSetPtr.end())
	{
		if (g_test_server)
			sys_log(0, "UPDATE_ITEMCACHESET : UpdateItemCacheSet ==> No ItemCacheSet pid(%d)", pid);
		return;
	}

	TItemCacheSet * pSet = it->second;
	TItemCacheSet::iterator it_set = pSet->begin();

	while (it_set != pSet->end())
	{
		CItemCache * c = *it_set++;
		c->Flush();
	}

	if (g_log)
		sys_log(0, "UPDATE_ITEMCACHESET : UpdateItemCachsSet pid(%d)", pid);
}

void CClientManager::Election(CPeer * peer, DWORD dwHandle, const char* data)
{
	DWORD idx;
	DWORD selectingpid;

	idx = *(DWORD *) data;
	data += sizeof(DWORD);

	selectingpid = *(DWORD *) data;
	data += sizeof(DWORD);

	int Success = 0;

	if (!(Success = CMonarch::instance().VoteMonarch(selectingpid, idx)))
	{
		if (g_test_server)
		sys_log(0, "[MONARCH_VOTE] Failed %d %d", idx, selectingpid);
		peer->EncodeHeader(HEADER_DG_ELECT_MONARCH, dwHandle, sizeof(int));
		peer->Encode(&Success, sizeof(int));
		return;
	}
	else
	{
		if (g_test_server)
		sys_log(0, "[MONARCH_VOTE] Success %d %d", idx, selectingpid);
		peer->EncodeHeader(HEADER_DG_ELECT_MONARCH, dwHandle, sizeof(int));
		peer->Encode(&Success, sizeof(int));
		return;
	}

}
void CClientManager::Candidacy(CPeer *  peer, DWORD dwHandle, const char* data)
{
	DWORD pid;

	pid = *(DWORD *) data;
	data += sizeof(DWORD);

	if (!CMonarch::instance().AddCandidacy(pid, data))
	{
		if (g_test_server)
			sys_log(0, "[MONARCH_CANDIDACY] Failed %d %s", pid, data);

		peer->EncodeHeader(HEADER_DG_CANDIDACY, dwHandle, sizeof(int) + 32);
		peer->Encode(0, sizeof(int));
		peer->Encode(data, 32);
		return;
	}
	else
	{
		if (g_test_server)
			sys_log(0, "[MONARCH_CANDIDACY] Success %d %s", pid, data);

		for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
		{
			CPeer * p = *it;

			if (!p->GetChannel())
				continue;

			if (0 && p->GetChannel() != 0)
				continue;

			if (p == peer)
			{
				p->EncodeHeader(HEADER_DG_CANDIDACY, dwHandle, sizeof(int) + 32);
				p->Encode(&pid, sizeof(int));
				p->Encode(data, 32);
			}
			else
			{
				p->EncodeHeader(HEADER_DG_CANDIDACY, 0, sizeof(int) + 32);
				p->Encode(&pid, sizeof(int));
				p->Encode(data, 32);
			}
		}
	}
}

void CClientManager::AddMonarchMoney(CPeer * peer, DWORD dwHandle, const char * data)
{
	int Empire = *(int *) data;
	data += sizeof(int);

	int Money = *(int *) data;
	data += sizeof(int);

	if (g_test_server)
		sys_log(0, "[MONARCH] Add money Empire(%d) Money(%d)", Empire, Money);

	CMonarch::instance().AddMoney(Empire, Money);

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		CPeer * p = *it;

		if (!p->GetChannel())
			continue;

		if (p == peer)
		{
			p->EncodeHeader(HEADER_DG_ADD_MONARCH_MONEY, dwHandle, sizeof(int) + sizeof(int));
			p->Encode(&Empire, sizeof(int));
			p->Encode(&Money, sizeof(int));
		}
		else
		{
			p->EncodeHeader(HEADER_DG_ADD_MONARCH_MONEY, 0, sizeof(int) + sizeof(int));
			p->Encode(&Empire, sizeof(int));
			p->Encode(&Money, sizeof(int));
		}

	}
}
void CClientManager::DecMonarchMoney(CPeer * peer, DWORD dwHandle, const char * data)
{
	int Empire = *(int *) data;
	data += sizeof(int);

	int Money = *(int *) data;
	data += sizeof(int);

	if (g_test_server)
		sys_log(0, "[MONARCH] Dec money Empire(%d) Money(%d)", Empire, Money);

	CMonarch::instance().DecMoney(Empire, Money);

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		CPeer * p = *it;

		if (!p->GetChannel())
			continue;

		if (p == peer)
		{
			p->EncodeHeader(HEADER_DG_DEC_MONARCH_MONEY, dwHandle, sizeof(int) + sizeof(int));
			p->Encode(&Empire, sizeof(int));
			p->Encode(&Money, sizeof(int));
		}
		else
		{
			p->EncodeHeader(HEADER_DG_DEC_MONARCH_MONEY, 0, sizeof(int) + sizeof(int));
			p->Encode(&Empire, sizeof(int));
			p->Encode(&Money, sizeof(int));
		}
	}
}

void CClientManager::TakeMonarchMoney(CPeer * peer, DWORD dwHandle, const char * data)
{
	int Empire = *(int *) data;
	data += sizeof(int);

	DWORD pid = *(DWORD *) data;
	data += sizeof(int);

	int Money = *(int *) data;
	data += sizeof(int);

	if (g_test_server)
		sys_log(0, "[MONARCH] Take money Empire(%d) Money(%d)", Empire, Money);

	if (CMonarch::instance().TakeMoney(Empire, pid, Money) == true)
	{
		peer->EncodeHeader(HEADER_DG_TAKE_MONARCH_MONEY, dwHandle, sizeof(int) + sizeof(int));
		peer->Encode(&Empire, sizeof(int));
		peer->Encode(&Money, sizeof(int));
	}
	else
	{
		Money = 0;
		peer->EncodeHeader(HEADER_DG_TAKE_MONARCH_MONEY, dwHandle, sizeof(int) + sizeof(int));
		peer->Encode(&Empire, sizeof(int));
		peer->Encode(&Money, sizeof(int));
	}
}

void CClientManager::ComeToVote(CPeer * peer, DWORD dwHandle, const char * data)
{
	CMonarch::instance().ElectMonarch();
}

void CClientManager::RMCandidacy(CPeer * peer, DWORD dwHandle, const char * data)
{
	char szName[32];

	strlcpy(szName, data, sizeof(szName));
	sys_log(0, "[MONARCH_GM] Remove candidacy name(%s)", szName);

	int iRet = CMonarch::instance().DelCandidacy(szName) ? 1 : 0;

	if (1 == iRet)
	{
		for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
		{
			CPeer * p = *it;

			if (!p->GetChannel())
				continue;

			if (p == peer)
			{
				p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
				p->Encode(&iRet, sizeof(int));
				p->Encode(szName, sizeof(szName));
			}
			else
			{
				p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
				p->Encode(&iRet, sizeof(int));
				p->Encode(szName, sizeof(szName));
			}
		}
	}
	else
	{
		CPeer * p = peer;
		p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
		p->Encode(&iRet, sizeof(int));
		p->Encode(szName, sizeof(szName));
	}
}

void CClientManager::SetMonarch(CPeer * peer, DWORD dwHandle, const char * data)
{
	char szName[32];

	strlcpy(szName, data, sizeof(szName));

	if (g_test_server)
		sys_log(0, "[MONARCH_GM] Set Monarch name(%s)", szName);

	int iRet = CMonarch::instance().SetMonarch(szName) ? 1 : 0;

	if (1 == iRet)
	{
		for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
		{
			CPeer * p = *it;

			if (!p->GetChannel())
				continue;

			if (p == peer)
			{
				p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
				p->Encode(&iRet, sizeof(int));
				p->Encode(szName, sizeof(szName));
			}
			else
			{
				p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
				p->Encode(&iRet, sizeof(int));
				p->Encode(szName, sizeof(szName));
			}
		}
	}
	else
	{
		CPeer * p = peer;
		p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
		p->Encode(&iRet, sizeof(int));
		p->Encode(szName, sizeof(szName));
	}
}

void CClientManager::RMMonarch(CPeer * peer, DWORD dwHandle, const char * data)
{
	char szName[32];

	strlcpy(szName, data, sizeof(szName));

	if (g_test_server)
		sys_log(0, "[MONARCH_GM] Remove Monarch name(%s)", szName);

	CMonarch::instance().DelMonarch(szName);

	int iRet = CMonarch::instance().DelMonarch(szName) ? 1 : 0;

	if (1 == iRet)
	{
		for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
		{
			CPeer * p = *it;

			if (!p->GetChannel())
				continue;

			if (p == peer)
			{
				p->EncodeHeader(HEADER_DG_RMMONARCH, dwHandle, sizeof(int) + sizeof(szName));
				p->Encode(&iRet, sizeof(int));
				p->Encode(szName, sizeof(szName));
			}
			else
			{
				p->EncodeHeader(HEADER_DG_RMMONARCH, dwHandle, sizeof(int) + sizeof(szName));
				p->Encode(&iRet, sizeof(int));
				p->Encode(szName, sizeof(szName));
			}
		}
	}
	else
	{
		CPeer * p = peer;
		p->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int) + sizeof(szName));
		p->Encode(&iRet, sizeof(int));
		p->Encode(szName, sizeof(szName));
	}
}

void CClientManager::ChangeMonarchLord(CPeer * peer, DWORD dwHandle, TPacketChangeMonarchLord* info)
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT a.name, NOW() FROM player%s AS a, player_index%s AS b WHERE (a.account_id=b.id AND a.id=%u AND b.empire=%u) AND "
#ifdef ENABLE_PLAYER_PER_ACCOUNT5
		    "(b.pid1=%u OR b.pid2=%u OR b.pid3=%u OR b.pid4=%u OR b.pid5=%u)",
#else
		    "(b.pid1=%u OR b.pid2=%u OR b.pid3=%u OR b.pid4=%u)",
#endif
			GetTablePostfix(), GetTablePostfix(), info->dwPID, info->bEmpire,
#ifdef ENABLE_PLAYER_PER_ACCOUNT5
		   	info->dwPID, info->dwPID, info->dwPID, info->dwPID, info->dwPID);
#else
		   	info->dwPID, info->dwPID, info->dwPID, info->dwPID);
#endif

	SQLMsg * pMsg = CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER);

	if (pMsg->Get()->uiNumRows != 0)
	{
		TPacketChangeMonarchLordACK ack;
		ack.bEmpire = info->bEmpire;
		ack.dwPID = info->dwPID;

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		strlcpy(ack.szName, row[0], sizeof(ack.szName));
		strlcpy(ack.szDate, row[1], sizeof(ack.szDate));

		snprintf(szQuery, sizeof(szQuery), "UPDATE monarch SET pid=%u, windate=NOW() WHERE empire=%d", ack.dwPID, ack.bEmpire);
		SQLMsg* pMsg2 = CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER);

		if (pMsg2->Get()->uiAffectedRows > 0)
		{
			CMonarch::instance().LoadMonarch();

			TMonarchInfo* newInfo = CMonarch::instance().GetMonarch();

			for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); it++)
			{
				CPeer* client = *it;

				client->EncodeHeader(HEADER_DG_CHANGE_MONARCH_LORD_ACK, 0, sizeof(TPacketChangeMonarchLordACK));
				client->Encode(&ack, sizeof(TPacketChangeMonarchLordACK));

				client->EncodeHeader(HEADER_DG_UPDATE_MONARCH_INFO, 0, sizeof(TMonarchInfo));
				client->Encode(newInfo, sizeof(TMonarchInfo));
			}
		}

		delete pMsg2;
	}

	delete pMsg;
}

void CClientManager::BlockException(TPacketBlockException *data)
{
	sys_log(0, "[BLOCK_EXCEPTION] CMD(%d) login(%s)", data->cmd, data->login);

	// save sql
	{
		char buf[1024];

		switch (data->cmd)
		{
			case BLOCK_EXCEPTION_CMD_ADD:
				snprintf(buf, sizeof(buf), "INSERT INTO block_exception VALUES('%s')", data->login);
				CDBManager::instance().AsyncQuery(buf, SQL_ACCOUNT);
				CBlockCountry::instance().AddBlockException(data->login);
				break;
			case BLOCK_EXCEPTION_CMD_DEL:
				snprintf(buf, sizeof(buf), "DELETE FROM block_exception VALUES('%s')", data->login);
				CDBManager::instance().AsyncQuery(buf, SQL_ACCOUNT);
				CBlockCountry::instance().DelBlockException(data->login);
				break;
			default:
				return;
		}

	}

	for (itertype(m_peerList) it = m_peerList.begin(); it != m_peerList.end(); ++it)
	{
		CPeer	*peer = *it;

		if (!peer->GetChannel())
			continue;

		CBlockCountry::instance().SendBlockExceptionOne(peer, data->login, data->cmd);
	}
}

void CClientManager::SendSpareItemIDRange(CPeer* peer)
{
	peer->SendSpareItemIDRange();
}

//
// Login Key만 맵에서 지운다.
//
void CClientManager::DeleteLoginKey(TPacketDC *data)
{
	char login[LOGIN_MAX_LEN+1] = {0};
	trim_and_lower(data->login, login, sizeof(login));

	CLoginData *pkLD = GetLoginDataByLogin(login);

	if (pkLD)
	{
		TLoginDataByLoginKey::iterator it = m_map_pkLoginData.find(pkLD->GetKey());

		if (it != m_map_pkLoginData.end())
			m_map_pkLoginData.erase(it);
	}
}

// delete gift notify icon
void CClientManager::DeleteAwardId(TPacketDeleteAwardID *data)
{
	//sys_log(0,"data from game server arrived %d",data->dwID);
	std::map<DWORD, TItemAward *>::iterator it;
	it = ItemAwardManager::Instance().GetMapAward().find(data->dwID);
	if ( it != ItemAwardManager::Instance().GetMapAward().end() )
	{
		std::set<TItemAward *> & kSet = ItemAwardManager::Instance().GetMapkSetAwardByLogin()[it->second->szLogin];
		if(kSet.erase(it->second))
			sys_log(0,"erase ItemAward id: %d from cache", data->dwID);
		ItemAwardManager::Instance().GetMapAward().erase(data->dwID);
	}
	else
	{
		sys_log(0,"DELETE_AWARDID : could not find the id: %d", data->dwID);
	}

}

void CClientManager::UpdateChannelStatus(TChannelStatus* pData)
{
	TChannelStatusMap::iterator it = m_mChannelStatus.find(pData->nPort);
	if (it != m_mChannelStatus.end()) {
		it->second = pData->bStatus;
	}
	else {
		m_mChannelStatus.insert(TChannelStatusMap::value_type(pData->nPort, pData->bStatus));
	}
}

void CClientManager::RequestChannelStatus(CPeer* peer, DWORD dwHandle)
{
	const int nSize = m_mChannelStatus.size();
	peer->EncodeHeader(HEADER_DG_RESPOND_CHANNELSTATUS, dwHandle, sizeof(TChannelStatus)*nSize+sizeof(int));
	peer->Encode(&nSize, sizeof(int));
	for (TChannelStatusMap::iterator it = m_mChannelStatus.begin(); it != m_mChannelStatus.end(); it++) {
		peer->Encode(&it->first, sizeof(short));
		peer->Encode(&it->second, sizeof(BYTE));
	}
}

void CClientManager::ResetLastPlayerID(const TPacketNeedLoginLogInfo* data)
{
	CLoginData* pkLD = GetLoginDataByAID( data->dwPlayerID );

	if (NULL != pkLD)
	{
		pkLD->SetLastPlayerID( 0 );
	}
}
#if defined(WJ_COMBAT_ZONE)	
void CClientManager::CombatZoneResetRanking(const TPacketGDCombatZoneResetRanking* p)
{
	static int last_reset = 0;
	static int dif = 60*60*24; // 1gg

	if (time(NULL) < last_reset + dif)
		return;
	
	last_reset = time(NULL);

	CDBManager::instance().DirectQuery("UPDATE player.player SET combat_zone_rank = 0 WHERE combat_zone_rank > 0");

	std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery("SELECT * FROM player.combat_zone_ranking_weekly ORDER BY memberPoints DESC LIMIT 3"));	
	if (pMsg->Get()->uiNumRows == 3)
	{
		MYSQL_ROW row;
		int memberRank = 1;
		char szQuery[512 + 1];
		while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
		{
			sprintf(szQuery, "UPDATE player.player SET combat_zone_rank = '%d' WHERE name = '%s'", memberRank, row[0]);
			sys_err("Succes update rank grade %d for player %s.", memberRank, row[0]);
			CDBManager::instance().DirectQuery(szQuery);
			memberRank++;
		}
	}
	else {
		sys_err("The giving ranking medals not was possible because not was exist 3 players on ranking weekly.");
	}

	CDBManager::instance().DirectQuery("TRUNCATE TABLE player.combat_zone_ranking_weekly");
}

void CClientManager::UpdateSkillsCache(const TPacketGDCombatZoneSkillsCache* p)
{
	char szQuery[2048 + 1];
	sprintf(szQuery, 
		"INSERT INTO player.combat_zone_skills_cache (pid, skillLevel1, skillLevel2, skillLevel3, skillLevel4, skillLevel5, skillLevel6) "
			"VALUES('%d', '%d', '%d', '%d', '%d', '%d', '%d') "
				"ON DUPLICATE KEY UPDATE skillLevel1 = '%d', skillLevel2 = '%d', skillLevel3 = '%d', skillLevel4 = '%d', skillLevel5 = '%d', skillLevel6 = '%d'", 
					p->dwPID, p->dwSkillLevel1, p->dwSkillLevel2, p->dwSkillLevel3, p->dwSkillLevel4, p->dwSkillLevel5, p->dwSkillLevel6, p->dwSkillLevel1, p->dwSkillLevel2, p->dwSkillLevel3, p->dwSkillLevel4, p->dwSkillLevel5, p->dwSkillLevel6);
	CDBManager::instance().DirectQuery(szQuery);
}
#endif
void CClientManager::ChargeCash(const TRequestChargeCash* packet)
{
	char szQuery[512];

	if (ERequestCharge_Cash == packet->eChargeType)
		sprintf(szQuery, "update account set cash = cash + %d where id = %d limit 1", packet->dwAmount, packet->dwAID);
	else if(ERequestCharge_Mileage == packet->eChargeType)
		sprintf(szQuery, "update account set mileage = mileage + %d where id = %d limit 1", packet->dwAmount, packet->dwAID);
	else
	{
		sys_err ("Invalid request charge type (type : %d, amount : %d, aid : %d)", packet->eChargeType, packet->dwAmount, packet->dwAID);
		return;
	}

	sys_err ("Request Charge (type : %d, amount : %d, aid : %d)", packet->eChargeType, packet->dwAmount, packet->dwAID);

	CDBManager::Instance().AsyncQuery(szQuery, SQL_ACCOUNT);
}

#ifdef __AUCTION__
void CClientManager::EnrollInAuction (CPeer * peer, DWORD owner_id, AuctionEnrollProductInfo* data)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (owner_id);

	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", owner_id);
		return;
	}
	CItemCache* c = GetItemCache (data->get_item_id());

	if (c == NULL)
	{
		sys_err ("Item %d doesn't exist in db cache.", data->get_item_id());
		return;
	}
	TPlayerItem* item = c->Get(false);

	if (item->owner != owner_id)
	{
		sys_err ("Player id %d doesn't have item %d.", owner_id, data->get_item_id());
		return;
	}
	// 현재 시각 + 24시간 후.
	time_t expired_time = time(0) + 24 * 60 * 60;
	TAuctionItemInfo auctioned_item_info (item->vnum, data->get_bid_price(),
		data->get_impur_price(), owner_id, "", expired_time, data->get_item_id(), 0, data->get_empire());

	AuctionResult result = AuctionManager::instance().EnrollInAuction( c, auctioned_item_info );

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_ENR_AUC;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;
		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, owner_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
		peer->Encode(c->Get(false), sizeof(TPlayerItem));
	}
	else
	{
		// 아이템 케시를 Auction에 등록 했으니 ClientManager에서는 뺀다.
		TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(item->owner);

		if (it != m_map_pkItemCacheSetPtr.end())
		{
			it->second->erase(c);
		}
		m_map_itemCache.erase(item->id);
		sys_log(0, "Enroll In Auction Success. owner_id item_id %d %d", owner_id, item->id);

		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_ENR_AUC;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;
		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader(HEADER_DG_AUCTION_RESULT, owner_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem) + sizeof(TAuctionItemInfo));
			(*it)->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode(c->Get(false), sizeof(TPlayerItem));
			(*it)->Encode(&auctioned_item_info, sizeof(TAuctionItemInfo));
		}
	}

	return;
}

void CClientManager::EnrollInSale (CPeer * peer, DWORD owner_id, AuctionEnrollSaleInfo* data)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (owner_id);

	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", owner_id);
		return;
	}

	CPlayerTableCache* player_cache = it->second;
	TPlayerTable* player = player_cache->Get(false);

	CItemCache* c = GetItemCache (data->get_item_id());

	if (c == NULL)
	{
		sys_err ("Item %d doesn't exist in db cache.", data->get_item_id());
		return;
	}
	TPlayerItem* item = c->Get(false);

	if (item->owner != owner_id)
	{
		sys_err ("Player id %d doesn't have item %d.", owner_id, data->get_item_id());
		return;
	}
	// 현재 시각 + 24시간 후.
	time_t expired_time = time(0) + 24 * 60 * 60;
	TSaleItemInfo sold_item_info (item->vnum, data->get_sale_price(),
		owner_id, player->name, data->get_item_id(), data->get_wisher_id());

	AuctionResult result = AuctionManager::instance().EnrollInSale( c, sold_item_info );

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_ENR_SALE;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;
		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, owner_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
		peer->Encode(c->Get(false), sizeof(TPlayerItem));
	}
	else
	{
		// 아이템 케시를 Auction에 등록 했으니 ClientManager에서는 뺀다.
		TItemCacheSetPtrMap::iterator it = m_map_pkItemCacheSetPtr.find(item->owner);

		if (it != m_map_pkItemCacheSetPtr.end())
		{
			it->second->erase(c);
		}
		m_map_itemCache.erase(item->id);
		sys_log(0, "Enroll In Sale Success. owner_id item_id %d %d", owner_id, item->id);

		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_ENR_SALE;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader(HEADER_DG_AUCTION_RESULT, owner_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem) + sizeof(TSaleItemInfo));
			(*it)->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode(c->Get(false), sizeof(TPlayerItem));
			(*it)->Encode(&sold_item_info, sizeof(TSaleItemInfo));
		}
	}

	return;
}

void CClientManager::EnrollInWish (CPeer * peer, DWORD wisher_id, AuctionEnrollWishInfo* data)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (wisher_id);

	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", wisher_id);
		return;
	}

	CPlayerTableCache* player_cache = it->second;
	TPlayerTable* player = player_cache->Get(false);

	// 현재 시각 + 24시간 후.
	time_t expired_time = time(0) + 24 * 60 * 60;
	TWishItemInfo wished_item_info (data->get_item_num(), data->get_wish_price(), wisher_id, player->name, expired_time, data->get_empire());

	AuctionResult result = AuctionManager::instance().EnrollInWish ( wished_item_info );

	if (result <= AUCTION_FAIL)
	{
		sys_log(0, "Enroll In Wish Success. wisher_id item_num %d %d", wisher_id, data->get_item_num());

		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_ENR_WISH;
		enroll_result.target = data->get_item_num();
		enroll_result.result = result;
		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, wisher_id, sizeof(TPacketDGResultAuction));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		sys_log(0, "Enroll In Wish Fail. wisher_id item_num %d %d", wisher_id, data->get_item_num());

		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_ENR_WISH;
		enroll_result.target = data->get_item_num();
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader(HEADER_DG_AUCTION_RESULT, wisher_id, sizeof(TPacketDGResultAuction) + sizeof(TWishItemInfo));
			(*it)->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode(&wished_item_info, sizeof(TWishItemInfo));
		}
	}

	return;
}

void CClientManager::AuctionBid (CPeer * peer, DWORD bidder_id, AuctionBidInfo* data)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (bidder_id);

	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", bidder_id);
		return;
	}

	CPlayerTableCache* player_cache = it->second;
	TPlayerTable* player = player_cache->Get(false);

	AuctionResult result = AuctionManager::instance().Bid(bidder_id, player->name, data->get_item_id(), data->get_bid_price());

	if (result == AUCTION_FAIL)
	{
		sys_log(0, "Bid Fail. bidder_id item_id %d %d", bidder_id, data->get_item_id());
	}
	else
	{
		sys_log(0, "Bid Success. bidder_id item_id %d %d", bidder_id, data->get_item_id());
	}

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_BID;
		enroll_result.target = data->get_bid_price();
		enroll_result.result = result;

		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, bidder_id, sizeof(TPacketDGResultAuction) + sizeof(AuctionBidInfo));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_BID;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;

		TAuctionItemInfo* auctioned_item_info = AuctionManager::instance().GetAuctionItemInfoCache(data->get_item_id())->Get(false);

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader(HEADER_DG_AUCTION_RESULT, bidder_id, sizeof(TPacketDGResultAuction) + sizeof(TAuctionItemInfo));
			(*it)->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode(auctioned_item_info, sizeof(TAuctionItemInfo));
		}

	}
	return;
}

void CClientManager::AuctionImpur (CPeer * peer, DWORD purchaser_id, AuctionImpurInfo* data)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (purchaser_id);

	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", purchaser_id);
		return;
	}

	CPlayerTableCache* player_cache = it->second;
	TPlayerTable* player = player_cache->Get(false);

	AuctionResult result = AuctionManager::instance().Impur(purchaser_id, player->name, data->get_item_id());

	if (result == AUCTION_FAIL)
	{
		sys_log(0, "Impur Fail. purchaser_id item_id %d %d", purchaser_id, data->get_item_id());
	}
	else
	{
		sys_log(0, "Impur Success. purchaser_id item_id %d %d", purchaser_id, data->get_item_id());
	}

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_IMME_PUR;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;

		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, purchaser_id, sizeof(TPacketDGResultAuction));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_IMME_PUR;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;

		TAuctionItemInfo* auctioned_item_info = AuctionManager::instance().GetAuctionItemInfoCache(data->get_item_id())->Get(false);
		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader(HEADER_DG_AUCTION_RESULT, purchaser_id, sizeof(TPacketDGResultAuction) + sizeof(TAuctionItemInfo));
			(*it)->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode(auctioned_item_info, sizeof(TAuctionItemInfo));
		}
	}
	return;
}

void CClientManager::AuctionGetAuctionedItem (CPeer * peer, DWORD actor_id, DWORD item_id)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	TPlayerItem item;
	result = AuctionManager::instance().GetAuctionedItem(actor_id, item_id, item);

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_GET_AUC;
		enroll_result.target = item_id;
		enroll_result.result = result;

		peer->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction));
		peer->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_GET_AUC;
		enroll_result.target = item_id;
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem));
			(*it)->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode (&item, sizeof(TPlayerItem));
		}
	}
	return;
}

void CClientManager::AuctionBuySoldItem (CPeer * peer, DWORD actor_id, DWORD item_id)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	TPlayerItem item;
	result = AuctionManager::instance().BuySoldItem(actor_id, item_id, item);

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_BUY_SOLD;
		enroll_result.target = item_id;
		enroll_result.result = result;

		peer->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction));
		peer->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_BUY_SOLD;
		enroll_result.target = item_id;
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem));
			(*it)->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode (&item, sizeof(TPlayerItem));
		}
	}
	return;
}

void CClientManager::AuctionCancelAuction (CPeer * peer, DWORD actor_id, DWORD item_id)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	TPlayerItem item;
	result = AuctionManager::instance().CancelAuction(actor_id, item_id, item);

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_CANCEL_AUC;
		enroll_result.target = item_id;
		enroll_result.result = result;

		peer->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction));
		peer->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_CANCEL_AUC;
		enroll_result.target = item_id;
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem));
			(*it)->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode (&item, sizeof(TPlayerItem));
		}
	}
	return;
}

void CClientManager::AuctionCancelWish (CPeer * peer, DWORD actor_id, DWORD item_num)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	TPlayerItem item;
	result = AuctionManager::instance().CancelWish(actor_id, item_num);

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_CANCEL_WISH;
		enroll_result.target = item_num;
		enroll_result.result = result;

		peer->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction));
		peer->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_CANCEL_WISH;
		enroll_result.target = item_num;
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction));
			(*it)->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
		}
	}
	return;
}

void CClientManager::AuctionCancelSale (CPeer * peer, DWORD actor_id, DWORD item_id)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	TPlayerItem item;
	result = AuctionManager::instance().CancelSale(actor_id, item_id, item);

	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_CANCEL_SALE;
		enroll_result.target = item_id;
		enroll_result.result = result;

		peer->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction));
		peer->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_CANCEL_SALE;
		enroll_result.target = item_id;
		enroll_result.result = result;

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader (HEADER_DG_AUCTION_RESULT, actor_id, sizeof(TPacketDGResultAuction) + sizeof(TPlayerItem));
			(*it)->Encode (&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode (&item, sizeof(TPlayerItem));
		}
	}
	return;
}

void CClientManager::AuctionDeleteAuctionItem (CPeer * peer, DWORD actor_id, DWORD item_id)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	AuctionManager::instance().DeleteAuctionItem (actor_id, item_id);
}
void CClientManager::AuctionDeleteSaleItem (CPeer * peer, DWORD actor_id, DWORD item_id)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (actor_id);
	AuctionResult result = AUCTION_FAIL;
	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", actor_id);
		return;
	}

	AuctionManager::instance().DeleteSaleItem (actor_id, item_id);
}

// ReBid는 이전 입찰금액에 더해서 입찰한다.
// ReBid에선 data->bid_price가 이전 입찰가에 더해져서
// 그 금액으로 rebid하는 것.
// 이렇게 한 이유는 rebid에 실패 했을 때,
// 유저의 호주머니에서 뺀 돈을 돌려주기 편하게 하기 위함이다.

void CClientManager::AuctionReBid (CPeer * peer, DWORD bidder_id, AuctionBidInfo* data)
{
	TPlayerTableCacheMap::iterator it = m_map_playerCache.find (bidder_id);

	if (it == m_map_playerCache.end())
	{
		sys_err ("Invalid Player id %d. how can you get it?", bidder_id);
		return;
	}

	CPlayerTableCache* player_cache = it->second;
	TPlayerTable* player = player_cache->Get(false);

	AuctionResult result = AuctionManager::instance().ReBid(bidder_id, player->name, data->get_item_id(), data->get_bid_price());

	if (result == AUCTION_FAIL)
	{
		sys_log(0, "ReBid Fail. bidder_id item_id %d %d", bidder_id, data->get_item_id());
	}
	else
	{
		sys_log(0, "ReBid Success. bidder_id item_id %d %d", bidder_id, data->get_item_id());
	}
	// 이건 FAIL이 떠서는 안돼.
	// FAIL이 뜰 수가 없는게, MyBid에 있는 bidder_id에 대한 컨텐츠는 bidder_id만이 접근 할 수 있거든?
	// 그러므로 다른 것이 다 정상적으로 작동한다고 가정 한다면
	// 한 게임 서버 내에서 bidder_id로 MyBid를 수정한다 할 지라도, 그건 동기화 문제가 없어.
	// 다른 게임 서버에 똑같은 bidder_id를 가진 놈이 있을 수가 없으니까.
	// 그러므로 그 게임 서버에서 BidCancel 명령을 db에 날렸다는 것은,
	// 이미 그 부분에 대해서는 검사가 완벽하다는 것이야.
	// 그래도 혹시나 싶어서, 디버깅을 위해 fail 코드를 남겨둔다.
	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_REBID;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;

		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, bidder_id, sizeof(TPacketDGResultAuction) + sizeof(int));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
		peer->EncodeDWORD(data->get_bid_price());
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_REBID;
		enroll_result.target = data->get_item_id();
		enroll_result.result = result;

		TAuctionItemInfo* auctioned_item_info = AuctionManager::instance().GetAuctionItemInfoCache(data->get_item_id())->Get(false);

		for (TPeerList::iterator it = m_peerList.begin(); it != m_peerList.end(); it++)
		{
			(*it)->EncodeHeader(HEADER_DG_AUCTION_RESULT, bidder_id, sizeof(TPacketDGResultAuction) + sizeof(TAuctionItemInfo));
			(*it)->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
			(*it)->Encode(auctioned_item_info, sizeof(TAuctionItemInfo));
		}
	}
	return;
}

void CClientManager::AuctionBidCancel (CPeer * peer, DWORD bidder_id, DWORD item_id)
{
	AuctionResult result = AuctionManager::instance().BidCancel (bidder_id, item_id);

	// 이건 FAIL이 떠서는 안돼.
	// FAIL이 뜰 수가 없는게, MyBid에 있는 bidder_id에 대한 컨텐츠는 bidder_id만이 접근 할 수 있거든?
	// 그러므로 다른 것이 다 정상적으로 작동한다고 가정 한다면
	// 한 게임 서버 내에서 bidder_id로 MyBid를 수정한다 할 지라도, 그건 동기화 문제가 없어.
	// 다른 게임 서버에 똑같은 bidder_id를 가진 놈이 있을 수가 없으니까.
	// 그러므로 그 게임 서버에서 BidCancel 명령을 db에 날렸다는 것은,
	// 이미 그 부분에 대해서는 검사가 완벽하다는 것이야.
	// 그래도 혹시나 싶어서, 디버깅을 위해 fail 코드를 남겨둔다.
	if (result <= AUCTION_FAIL)
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_BID_CANCEL;
		enroll_result.target = item_id;
		enroll_result.result = result;

		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, bidder_id, sizeof(TPacketDGResultAuction));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
	}
	else
	{
		TPacketDGResultAuction enroll_result;
		enroll_result.cmd = AUCTION_BID_CANCEL;
		enroll_result.target = item_id;
		enroll_result.result = result;

		peer->EncodeHeader(HEADER_DG_AUCTION_RESULT, bidder_id, sizeof(TPacketDGResultAuction));
		peer->Encode(&enroll_result, sizeof(TPacketDGResultAuction));
	}
}
#endif

#ifdef ENABLE_AUTOMATIC_EVENT
bool CClientManager::InitializeAutomaticEvent()
{
	CDBManager::instance().DirectQuery("DELETE FROM event_proto WHERE event_proto.date < DATE_SUB(NOW() , INTERVAL 30 DAY)");
	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery("SELECT id, type+0, UNIX_TIMESTAMP(date), duration, priv_empire+0, priv_type+0, priv_value, flag_name, message_id, message_interval FROM player.event_proto"));
	m_vec_eventTable.clear();

	SQLResult* pRes = pmsg->Get();
	if (pRes->uiNumRows)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)))
		{
			int cur = 0;
			TEventTable ev;

			str_to_number(ev.dwVnum, row[cur++]);
			str_to_number(ev.bType, row[cur++]);
			str_to_number(ev.iDate, row[cur++]);
			str_to_number(ev.iDuration, row[cur++]);

			str_to_number(ev.bPrivEmpire, row[cur++]);
			ev.bPrivEmpire = MINMAX(0, ev.bPrivEmpire - 2, 3);

			str_to_number(ev.bPrivType, row[cur++]);
			ev.bPrivType = MINMAX(0, ev.bPrivType - 1, MAX_PRIV_NUM);

			str_to_number(ev.iPrivValue, row[cur++]);
			ev.iPrivValue = MINMAX(0, ev.iPrivValue, 200);

			strlcpy(ev.szFlagName, row[cur++], sizeof(ev.szFlagName));
			str_to_number(ev.dwMessageID, row[cur++]);
			
			str_to_number(ev.iMessageInterval, row[cur++]);

			m_vec_eventTable.push_back(ev);
		}
	}

	bAutomaticEventFirstScan = true;
	return true;
}

void CClientManager::ProcessEventTable()
{
	time_t currentTime_t = CClientManager::instance().GetCurrentTime();

	for (size_t i = 0; i < m_vec_eventTable.size(); i++)
	{
		TEventTable *ev = &m_vec_eventTable[i];

		time_t timeStart = ev->iDate;
		time_t timeEnd = timeStart + ev->iDuration;
		if (currentTime_t >= timeStart && currentTime_t <= timeEnd)
		{
			if (bAutomaticEventFirstScan == false)
			{
				if (ev->IsActivated == true)
				{
					if (ev->iNextNotice <= currentTime_t && ev->dwMessageID != 0)
					{
						ev->iNextNotice = currentTime_t + ev->iMessageInterval;
						int input_seconds = timeEnd - currentTime_t;
						int days = input_seconds / 60 / 60 / 24;
						int hours = (input_seconds / 60 / 60) % 24;
						int minutes = (input_seconds / 60) % 60;
						TPacketDGMessageAutomaticEvent message = TPacketDGMessageAutomaticEvent(ev->dwMessageID, 2, days, hours, minutes);
						ForwardPacket(HEADER_DG_MESSAGE_AUTOMATIC_EVENT, &message, sizeof(TPacketDGMessageAutomaticEvent));
					}
					continue;
				}
			}

			bool bSendMessage = true;
			if (ev->bType == MANUAL_EVENT) {
				TPacketDGAutomaticEvent p = TPacketDGAutomaticEvent(ev->dwVnum, true);
				ForwardPacket(HEADER_DG_AUTOMATIC_EVENT, &p, sizeof(TPacketDGAutomaticEvent));
			}
			else if (ev->bType == PRIV_EMPIRE) {
				int iSeconds = currentTime_t - timeStart;
				if (iSeconds >= ev->iDuration) continue;
				int iDuration = ev->iDuration - iSeconds;
				iDuration = MINMAX(0, iDuration, 60 * 60 * 24 * 7);
				CPrivManager::instance().AddEmpirePriv(ev->bPrivEmpire, ev->bPrivType, ev->iPrivValue, iDuration);
				bSendMessage = false;
			}

			TPacketSetEventFlag *p = new TPacketSetEventFlag();
			strlcpy(p->szFlagName, ev->szFlagName, sizeof(p->szFlagName));
			p->lValue = 1;

			if(ev->bType == GAME_EVENT_FLAG && ev->bType != PRIV_EMPIRE)
				SetEventFlag(p);
			else if (strcmp(ev->szFlagName, "NONE") != 0 && ev->bType != PRIV_EMPIRE)
				SetEventFlag(p);

			if (bSendMessage){
				TPacketDGMessageAutomaticEvent message = TPacketDGMessageAutomaticEvent(ev->dwMessageID, 0, 0, 0, 0);
				ForwardPacket(HEADER_DG_MESSAGE_AUTOMATIC_EVENT, &message, sizeof(TPacketDGMessageAutomaticEvent));
			}
			ev->IsActivated = true;
			ev->iNextNotice = currentTime_t + ev->iMessageInterval;
		}
		else {
			if (bAutomaticEventFirstScan == false)
				if (ev->IsActivated == false)
					continue;
			
			bool bSendMessage = true;
			if (ev->bType == MANUAL_EVENT) {
				TPacketDGAutomaticEvent p = TPacketDGAutomaticEvent(ev->dwVnum, false);
				ForwardPacket(HEADER_DG_AUTOMATIC_EVENT, &p, sizeof(TPacketDGAutomaticEvent));
			}
			else if (ev->bType == PRIV_EMPIRE) {
				bSendMessage = false;
			}

			TPacketSetEventFlag *p = new TPacketSetEventFlag();
			strlcpy(p->szFlagName, ev->szFlagName, sizeof(p->szFlagName));
			p->lValue = 0;

			if (ev->bType == GAME_EVENT_FLAG && ev->bType != PRIV_EMPIRE)
				SetEventFlag(p);
			else if (strcmp(ev->szFlagName, "NONE") != 0 && ev->bType != PRIV_EMPIRE)
				SetEventFlag(p);

			if (bSendMessage) {
				TPacketDGMessageAutomaticEvent message = TPacketDGMessageAutomaticEvent(ev->dwMessageID, 1, 0, 0, 0);
				ForwardPacket(HEADER_DG_MESSAGE_AUTOMATIC_EVENT, &message, sizeof(TPacketDGMessageAutomaticEvent));
			}
			ev->IsActivated = false;
			ev->iNextNotice = 0;
		}
	}

	bAutomaticEventFirstScan = false;
}
#endif



#ifdef ENABLE_ITEMSHOP
#include "buffer_manager.h"
void stringToRealTime(struct tm& t, const std::string& strDateTime)
{
	int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
	if (sscanf(strDateTime.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec) == 6)
	{
		t.tm_year = year - 1900;
		t.tm_mon = month - 1;
		t.tm_mday = day;
		t.tm_isdst = 0;
		t.tm_hour = hour;
		t.tm_min = min;
		t.tm_sec = sec;
	}
}
bool sortItemShop(const TIShopData& first, const TIShopData& second)
{
	return first.sellCount > second.sellCount;
}
bool CClientManager::InitializeItemShop()
{
	m_IShopManager.clear();
	char szQuery[64];
	snprintf(szQuery, sizeof(szQuery), "SELECT * FROM player.ishop_data");
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
	if (pMsg->Get()->uiNumRows != 0)
	{
		std::vector<TIShopData> m_sortCache;

		MYSQL_ROW row = NULL;
		for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
		{
			int col = 0;
			TIShopData ishopData;

			BYTE categoryType, categorySubType;
			str_to_number(ishopData.id, row[col++]);
			str_to_number(categoryType, row[col++]);
			str_to_number(categorySubType, row[col++]);
			str_to_number(ishopData.itemVnum, row[col++]);
			str_to_number(ishopData.itemPrice, row[col++]);
			str_to_number(ishopData.discount, row[col++]);

			char eventTime[40];
			strlcpy(eventTime, row[col++], sizeof(eventTime));
			struct tm offerTimeTm;
			stringToRealTime(offerTimeTm, eventTime);
			time_t offerTime = mktime(&offerTimeTm);

			strlcpy(eventTime, row[col++], sizeof(eventTime));
			struct tm addedTimeTm;
			stringToRealTime(addedTimeTm, eventTime);
			time_t addedTime = mktime(&addedTimeTm);

			ishopData.offerTime = offerTime;
			if (ishopData.offerTime < 0)
				ishopData.offerTime = 0;
			ishopData.addedTime = addedTime;
			if (ishopData.addedTime < 0)
				ishopData.addedTime = 0;

			str_to_number(ishopData.sellCount, row[col++]);
			str_to_number(ishopData.week_limit, row[col++]);
			str_to_number(ishopData.month_limit, row[col++]);
			str_to_number(ishopData.maxSellCount, row[col++]);

			ishopData.topSellingIndex = -1;
			if(ishopData.sellCount > 0)
				m_sortCache.emplace_back(ishopData);

			auto itType = m_IShopManager.find(categoryType);
			if (itType != m_IShopManager.end())
			{
				auto itSubType = itType->second.find(categorySubType);
				if (itSubType != itType->second.end())
					itSubType->second.emplace_back(ishopData);
				else
				{
					std::vector<TIShopData> m_vec;
					m_vec.emplace_back(ishopData);
					itType->second.emplace(categorySubType, m_vec);
				}
			}
			else
			{
				std::vector<TIShopData> m_vec;
				std::map<BYTE, std::vector<TIShopData>> m_map;
				m_vec.emplace_back(ishopData);
				m_map.emplace(categorySubType, m_vec);
				m_IShopManager.emplace(categoryType, m_map);

			}
		}

		if (m_sortCache.size())
		{
			std::stable_sort(m_sortCache.begin(), m_sortCache.end(), sortItemShop);
			for (DWORD j = 0; j < m_sortCache.size(); ++j)
			{
				DWORD id = m_sortCache[j].id;
				for (auto itCategory = m_IShopManager.begin(); itCategory != m_IShopManager.end(); ++itCategory)
				{
					if (itCategory->second.size())
					{
						for (auto itSubCategory = itCategory->second.begin(); itSubCategory != itCategory->second.end(); ++itSubCategory)
						{
							for (DWORD x = 0; x < itSubCategory->second.size(); ++x)
							{
								if (itSubCategory->second[x].id == id)
									itSubCategory->second[x].topSellingIndex = j + 1;
							}
						}
					}
				}
			}
		}
	}
	itemShopUpdateTime = time(0);
	return true;
}
void CClientManager::SetDragonCoin(DWORD id, long long amount)
{
	char szQuery[84];
	snprintf(szQuery, sizeof(szQuery), "UPDATE account.account SET coins = %lld WHERE id = %d", amount, id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
}

void CClientManager::ItemShopIncreaseSellCount(DWORD itemID, int itemCount)
{
	long long sellCount = 0;
	char szQuery[84];
	snprintf(szQuery, sizeof(szQuery), "SELECT sellCount FROM player.ishop_data WHERE id = %u", itemID);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
	if (pMsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		str_to_number(sellCount, row[0]);
	}
	//sellCount += itemCount;
	sellCount += 1;
	snprintf(szQuery, sizeof(szQuery), "UPDATE player.ishop_data SET sellCount = %lld WHERE id = %u", sellCount, itemID);
	std::unique_ptr<SQLMsg> pMsgLast(CDBManager::instance().DirectQuery(szQuery));
}
long long CClientManager::GetDragonCoin(DWORD id)
{
	char szQuery[84];
	snprintf(szQuery, sizeof(szQuery), "SELECT coins FROM account.account WHERE id = %d", id);
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));
	if (pMsg->Get()->uiNumRows == 0)
		return 0;
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	long long dc = 0;
	str_to_number(dc, row[0]);
	return dc;
}
void CClientManager::RecvItemShop(CPeer* pkPeer, DWORD dwHandle, const char* data)
{
	const BYTE subIndex = *(BYTE*)data;
	data += sizeof(BYTE);
	if (subIndex == ITEMSHOP_LOG)
	{
		if (!pkPeer)
			return;

		const DWORD accountID = *(DWORD*)data;
		data += sizeof(DWORD);

		auto it = m_IShopLogManager.find(accountID);
		if (it == m_IShopLogManager.end())
		{
			char szQuery[84];
			snprintf(szQuery, sizeof(szQuery), "SELECT * FROM player.ishop_log WHERE accountID = %u", accountID);
			std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
			if (pMsg->Get()->uiNumRows != 0)
			{
				std::vector<TIShopLogData> m_vec;

				MYSQL_ROW row = NULL;
				for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
				{
					int col = 0;
					TIShopLogData ishopData;
					str_to_number(ishopData.accountID, row[col++]);
					strlcpy(ishopData.playerName, row[col++], sizeof(ishopData.playerName));
					strlcpy(ishopData.buyDate, row[col++], sizeof(ishopData.buyDate));
					str_to_number(ishopData.buyTime, row[col++]);
					strlcpy(ishopData.ipAdress, row[col++], sizeof(ishopData.ipAdress));
					str_to_number(ishopData.itemID, row[col++]);
					str_to_number(ishopData.itemVnum, row[col++]);
					str_to_number(ishopData.itemCount, row[col++]);
					str_to_number(ishopData.itemPrice, row[col++]);

					m_vec.emplace_back(ishopData);
				}
				if (m_vec.size())
					m_IShopLogManager.emplace(accountID, m_vec);
			}
		}

		it = m_IShopLogManager.find(accountID);
		if (it == m_IShopLogManager.end())
		{
			int logCount = 0;
			pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, sizeof(BYTE) + sizeof(int));
			pkPeer->Encode(&subIndex, sizeof(BYTE));
			pkPeer->Encode(&logCount, sizeof(int));
		}
		else
		{
			int logCount = it->second.size();

			pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle,sizeof(BYTE)+sizeof(int)+(sizeof(TIShopLogData)*logCount));
			pkPeer->Encode(&subIndex, sizeof(BYTE));
			pkPeer->Encode(&logCount, sizeof(int));
			if(logCount)
				pkPeer->Encode(&it->second[0],sizeof(TIShopLogData)*logCount);
		}
	}
	else if (subIndex == ITEMSHOP_RELOAD)
	{
		InitializeItemShop();
		SendItemShopData(NULL, true);
	}
	else if (subIndex == ITEMSHOP_LOG_ADD)
	{
		const DWORD accountID = *(DWORD*)data;
		data += sizeof(DWORD);
		char playerName[CHARACTER_NAME_MAX_LEN+1];
		strlcpy(playerName, data, sizeof(playerName));
		data+= sizeof(playerName);

		char ipAdress[16];
		strlcpy(ipAdress, data, sizeof(ipAdress));
		data += sizeof(ipAdress);

		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.ishop_log (accountID, playerName, buyDate, buyTime, ipAdress, itemVnum, itemCount, itemPrice) VALUES(%u, '%s', NOW(), %d, '%s', %u, %d, %lld)", accountID, playerName, time(0), ipAdress, 1, 1, 5);
		delete CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER);

		auto it = m_IShopLogManager.find(accountID);
		if (it != m_IShopLogManager.end())
		{
			char       timeText[21];
			time_t     now = time(0);
			struct tm  tstruct = *localtime(&now);
			strftime(timeText, sizeof(timeText), "%Y-%m-%d %X", &tstruct);

			TIShopLogData logData;
			logData.accountID = accountID;
			strlcpy(logData.playerName, playerName, sizeof(logData.playerName));
			strlcpy(logData.buyDate, timeText, sizeof(logData.buyDate));
			logData.buyTime = time(0);
			strlcpy(logData.ipAdress, ipAdress, sizeof(logData.ipAdress));
			logData.itemVnum = 1;
			logData.itemCount = 1;
			logData.itemPrice = 5;
			it->second.emplace_back(logData);
		}
	}
	else if (subIndex == ITEMSHOP_BUY)
	{
		const DWORD accountID = *(DWORD*)data;
		data += sizeof(DWORD);

		char playerName[CHARACTER_NAME_MAX_LEN + 1];
		thecore_memcpy(&playerName, data, sizeof(playerName));
		data += sizeof(playerName);

		char ipAdress[16];
		thecore_memcpy(&ipAdress, data, sizeof(ipAdress));
		data += sizeof(ipAdress);

		const int itemID = *(int*)data;
		data += sizeof(int);

		const int itemCount = *(int*)data;
		data += sizeof(int);

		if(itemCount <= 0 || itemCount > 20)
			return;

		const bool isLogOpen = *(bool*)data;
		data += sizeof(bool);

		if (m_IShopManager.size())
		{
			for (auto it = m_IShopManager.begin(); it != m_IShopManager.end(); ++it)
			{
				if (it->second.size())
				{
					for (auto itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
					{
						if (itEx->second.size())
						{
							for (auto itReal = itEx->second.begin(); itReal != itEx->second.end(); ++itReal)
							{
								TIShopData& itemData = *itReal;
								if (itemData.id == itemID)
								{
									long long accountDragonCoin = GetDragonCoin(accountID);

									long long itemPrice = itemData.itemPrice * itemCount;
									if (itemData.discount > 0)
										itemPrice = long((float(itemPrice) / 100.0) * float(100 - itemData.discount));

									bool needUpdatePacket = false;
									if (itemData.maxSellCount != -1)
									{
										if(itemData.maxSellCount==0)
										{
											BYTE returnType = 4;
											pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, sizeof(BYTE) + sizeof(BYTE));
											pkPeer->Encode(&subIndex, sizeof(BYTE));
											pkPeer->Encode(&returnType, sizeof(BYTE));
											return;
										}
										needUpdatePacket = true;
										itemData.maxSellCount-=1;
									}
									
									if (itemPrice > accountDragonCoin)
									{
										int returnType = 0;
										pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, sizeof(BYTE) + sizeof(BYTE));
										pkPeer->Encode(&subIndex, sizeof(BYTE));
										pkPeer->Encode(&returnType, sizeof(BYTE));
										return;
									}

									if (itemData.week_limit > 0)
									{
										DWORD weekCount = 0;
										char szQuery[254];
										snprintf(szQuery, sizeof(szQuery), "SELECT itemCount FROM srv1_player.ishop_log WHERE itemID  = %u and buyDate > DATE_SUB(NOW(), INTERVAL 1 WEEK) and accountID = %u", itemID, accountID);
										std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
										if (pMsg->Get()->uiNumRows != 0)
										{
											MYSQL_ROW row = NULL;
											for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
											{
												int buyCount;
												str_to_number(buyCount, row[0]);
												weekCount += buyCount;
											}
										}
										if (weekCount >= itemData.week_limit || weekCount+itemCount > itemData.week_limit)
										{
											int returnType = 1;
											pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, sizeof(BYTE) + sizeof(BYTE) + sizeof(int));
											pkPeer->Encode(&subIndex, sizeof(BYTE));
											pkPeer->Encode(&returnType, sizeof(BYTE));
											pkPeer->Encode(&itemData.week_limit, sizeof(int));
											return;
										}
									}

									if (itemData.month_limit > 0)
									{
										DWORD monthCount = 0;
										char szQuery[254];
										snprintf(szQuery, sizeof(szQuery), "SELECT itemCount FROM srv1_player.ishop_log WHERE accountID = %u and itemID = %u and buyDate > DATE_SUB(NOW(), INTERVAL 1 MONTH)", accountID, itemID);
										std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
										if (pMsg->Get()->uiNumRows != 0)
										{
											MYSQL_ROW row = NULL;
											for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
											{
												int buyCount;
												str_to_number(buyCount, row[0]);
												monthCount += buyCount;
											}
										}

										if (monthCount >= itemData.month_limit || monthCount+itemCount > itemData.month_limit)
										{
											int returnType = 2;
											pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, sizeof(BYTE) + sizeof(BYTE) + sizeof(int));
											pkPeer->Encode(&subIndex, sizeof(BYTE));
											pkPeer->Encode(&returnType, sizeof(BYTE));
											pkPeer->Encode(&itemData.month_limit, sizeof(int));
											return;
										}
									}

									//ItemShopIncreaseSellCount(itemID, itemCount);
									SetDragonCoin(accountID, accountDragonCoin - itemPrice);
									char szQuery[512];

#ifndef ENABLE_ITEMSHOP_TO_INVENTORY
									DWORD newItemID = GetEventFlag("SPECIAL_ITEM_ID") + 1;
									SetEventFlag("SPECIAL_ITEM_ID", newItemID);
									snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.item (id, owner_id, window, count, vnum) VALUES(%u, %u, %d, %d, %d)", newItemID, accountID, 4, itemCount, itemData.itemVnum);
									delete CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER);
#endif

									snprintf(szQuery, sizeof(szQuery), "INSERT INTO player.ishop_log (accountID, playerName, buyDate, buyTime, ipAdress, itemID, itemVnum, itemCount, itemPrice) VALUES(%u, '%s', NOW(), %d, '%s', %d, %u, %d, %lld)", accountID, playerName, time(0), ipAdress, itemID, itemData.itemVnum, itemCount, itemPrice);
									delete CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER);
									
									itemData.sellCount += 1;
									snprintf(szQuery, sizeof(szQuery), "UPDATE player.ishop_data SET sellCount = %lld, maxSellCount = %d WHERE id = %u", itemData.sellCount, itemData.maxSellCount, itemID);
									std::unique_ptr<SQLMsg> pMsgLast(CDBManager::instance().DirectQuery(szQuery));

									char       timeText[21];
									time_t     now = time(0);
									struct tm  tstruct = *localtime(&now);
									strftime(timeText, sizeof(timeText), "%Y-%m-%d %X", &tstruct);

									TIShopLogData logData;
									logData.accountID = accountID;
									strlcpy(logData.playerName, playerName, sizeof(logData.playerName));
									strlcpy(logData.buyDate, timeText, sizeof(logData.buyDate));
									logData.buyTime = time(0);
									strlcpy(logData.ipAdress, ipAdress, sizeof(logData.ipAdress));
									logData.itemVnum = itemData.itemVnum;
									logData.itemCount = itemCount;
									logData.itemPrice = itemPrice;

									auto it = m_IShopLogManager.find(accountID);
									if (it == m_IShopLogManager.end())
									{
										if (isLogOpen)
										{
											std::vector<TIShopLogData> m_vec;
											m_vec.emplace_back(logData);
											m_IShopLogManager.emplace(accountID, m_vec);
										}
									}
									else
									{
										it->second.emplace_back(logData);
									}

									int returnType = 3;
									int packetSize = sizeof(BYTE) + sizeof(BYTE) + sizeof(bool)+ sizeof(DWORD)+sizeof(int)+sizeof(long long);
									if (isLogOpen)
										packetSize+= sizeof(TIShopLogData);
									
									if (needUpdatePacket)
									{
										BYTE updatePacket = ITEMSHOP_UPDATE_ITEM;
										TEMP_BUFFER buf;
										buf.write(&updatePacket, sizeof(BYTE));
										buf.write(&itemData, sizeof(itemData));
										pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, buf.size());
										pkPeer->Encode(buf.read_peek(), buf.size());
									}

									pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, dwHandle, packetSize);
									pkPeer->Encode(&subIndex, sizeof(BYTE));
									pkPeer->Encode(&returnType, sizeof(BYTE));
									pkPeer->Encode(&isLogOpen, sizeof(bool));
									pkPeer->Encode(&logData.itemVnum, sizeof(DWORD));
									pkPeer->Encode(&logData.itemCount, sizeof(int));
									pkPeer->Encode(&logData.itemPrice, sizeof(long long));
									if (isLogOpen)
										pkPeer->Encode(&logData, sizeof(TIShopLogData));

									return;
								}
							}
						}
					}
				}
			}
		}
	}
}
void CClientManager::SendItemShopData(CPeer* pkPeer, bool isPacket)
{
	TEMP_BUFFER buf;
	buf.reset();

	BYTE subIndex = ITEMSHOP_LOAD;
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&itemShopUpdateTime, sizeof(int));
	buf.write(&isPacket, sizeof(bool));
	int categoryTotalSize = m_IShopManager.size();
	buf.write(&categoryTotalSize, sizeof(int));

	if (categoryTotalSize)
	{
		for (auto it = m_IShopManager.begin(); it != m_IShopManager.end(); ++it)
		{
			BYTE categoryIndex = it->first;
			buf.write(&categoryIndex, sizeof(BYTE));

			BYTE categorySize = it->second.size();
			buf.write(&categorySize, sizeof(BYTE));

			if (categorySize)
			{
				for (auto itEx = it->second.begin(); itEx != it->second.end(); ++itEx)
				{
					BYTE categorySubIndex = itEx->first;
					buf.write(&categorySubIndex, sizeof(BYTE));

					BYTE categorySubSize = itEx->second.size();
					buf.write(&categorySubSize, sizeof(BYTE));

					if (categorySubSize)
						buf.write(itEx->second.data(), sizeof(TIShopData) * categorySubSize);
				}
			}
		}
	}
	if (pkPeer != NULL)
	{
		pkPeer->EncodeHeader(HEADER_DG_ITEMSHOP, 0, buf.size());
		pkPeer->Encode(buf.read_peek(), buf.size());
	}
	else
	{
		ForwardPacket(HEADER_DG_ITEMSHOP, buf.read_peek(), buf.size());
	}
}
#endif

#ifdef ENABLE_EVENT_MANAGER
#include "buffer_manager.h"
void SetTimeToString(TEventManagerData* eventData)
{
	if (eventData->startTime <= 0)
		snprintf(eventData->startTimeText, sizeof(eventData->startTimeText), "1970-01-01 00:00:00");
	else
	{
		const time_t startTime = eventData->startTime;
		const struct tm vEventStartKey = *localtime(&startTime);
		snprintf(eventData->startTimeText, sizeof(eventData->startTimeText), "%d-%02d-%02d %02d:%02d:%02d", vEventStartKey.tm_year + 1900, vEventStartKey.tm_mon + 1, vEventStartKey.tm_mday, vEventStartKey.tm_hour, vEventStartKey.tm_min, vEventStartKey.tm_sec);
	}

	if (eventData->endTime <= 0)
		snprintf(eventData->endTimeText, sizeof(eventData->endTimeText), "1970-01-01 00:00:00");
	else
	{
		const time_t endTime = eventData->endTime;
		const struct tm vEventEndKey = *localtime(&endTime);
		snprintf(eventData->endTimeText, sizeof(eventData->endTimeText), "%d-%02d-%02d %02d:%02d:%02d", vEventEndKey.tm_year + 1900, vEventEndKey.tm_mon + 1, vEventEndKey.tm_mday, vEventEndKey.tm_hour, vEventEndKey.tm_min, vEventEndKey.tm_sec);
	}
}
bool IsDontHaveEndTimeEvent(BYTE eventIndex)
{
	switch (eventIndex)
	{
		case EMPIRE_WAR_EVENT:
		case TOURNAMENT_EVENT:
			return true;
			break;
	}
	return false;
}
void CClientManager::RecvEventManagerPacket(const char* data)
{
	const BYTE subIndex = *(BYTE*)data;
	data += sizeof(BYTE);
	if (subIndex == EVENT_MANAGER_UPDATE)
		InitializeEventManager(true);
	else if (subIndex == EVENT_MANAGER_REMOVE_EVENT)
	{
		const WORD index = *(WORD*)data;
		data += sizeof(WORD);

		if (m_EventManager.size())
		{
			for (auto it = m_EventManager.begin(); it != m_EventManager.end(); ++it)
			{
				for (DWORD j = 0; j < it->second.size(); ++j)
				{
					TEventManagerData& eventPtr = it->second[j];
					if (eventPtr.eventID == index)
					{
						eventPtr.endTime = time(0);
						SetTimeToString(&eventPtr);
						UpdateEventManager();
						char szQuery[QUERY_MAX_LEN];
						snprintf(szQuery, sizeof(szQuery), "UPDATE player.event_table SET endTime = NOW() WHERE id = %u", index);
						std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
						return;
					}
				}
			}
		}
	}
}
void CClientManager::UpdateEventManager()
{
	const time_t now = time(0);
	const struct tm vKey = *localtime(&now);
	if (vKey.tm_mday == 1 && vKey.tm_hour == 0 && vKey.tm_min == 0 && vKey.tm_sec == 0) // Month Update!
	{
		InitializeEventManager(true);
		return;
	}
	//const auto it = m_EventManager.find(vKey.tm_mday);
	auto it = m_EventManager.begin();
	while (it != m_EventManager.end())
	{
		if (it->second.size())
		{
			for(DWORD j=0;j<it->second.size();++j)
			{
				TEventManagerData& pData = it->second[j];
				int leftTime = pData.startTime - time(0);
				bool sendStatusPacket = false;

				//sys_err("id: %d eventIndex: %d lefTime: %d",pData.eventID ,pData.eventIndex, leftTime);

				if (leftTime == 0 && pData.eventStatus == false)
				{
					pData.eventStatus = true;
					sendStatusPacket = true;
				}

				leftTime = pData.endTime - now;
				if (leftTime == 0 && pData.endTime != 0 && pData.eventStatus == true)
				{
					pData.eventStatus = false;
					sendStatusPacket = true;
				}

				if (sendStatusPacket)
				{
					const BYTE subIndex = EVENT_MANAGER_EVENT_STATUS;
					TEMP_BUFFER buf;
					buf.write(&subIndex, sizeof(BYTE));
					buf.write(&pData.eventID, sizeof(WORD));
					buf.write(&pData.eventStatus, sizeof(bool));
					buf.write(&pData.endTime, sizeof(int));
					buf.write(&pData.endTimeText, sizeof(pData.endTimeText));
					ForwardPacket(HEADER_DG_EVENT_MANAGER, buf.read_peek(), buf.size());
				}
			}
		}
		++it;
	}
}

bool SortWithTime(const TEventManagerData& a, const TEventManagerData& b)
{
	return (a.startTime < b.startTime);
}
bool CClientManager::InitializeEventManager(bool updateFromGameMaster)
{
	m_EventManager.clear();

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT id, eventIndex+0, UNIX_TIMESTAMP(startTime), UNIX_TIMESTAMP(endTime), empireFlag, channelFlag, value0, value1, value2, value3 FROM player.event_table");
	std::unique_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
	if (pMsg->Get()->uiNumRows != 0)
	{
		const time_t cur_Time = time(NULL);
		const struct tm vKey = *localtime(&cur_Time);

		MYSQL_ROW row = NULL;
		for (int n = 0; (row = mysql_fetch_row(pMsg->Get()->pSQLResult)) != NULL; ++n)
		{
			int col = 0;
			TEventManagerData p;
			str_to_number(p.eventID, row[col++]);
			str_to_number(p.eventIndex, row[col++]);
			str_to_number(p.startTime, row[col++]);
			str_to_number(p.endTime, row[col++]);
			str_to_number(p.empireFlag, row[col++]);
			str_to_number(p.channelFlag, row[col++]);
			for (DWORD j = 0; j < 4; ++j)
				str_to_number(p.value[j], row[col++]);

			const time_t eventEndTime = p.endTime;
			const struct tm vEventEndKey = *localtime(&eventEndTime);
			if (vEventEndKey.tm_mon < vKey.tm_mon && p.endTime != 0)
				continue;

			p.eventTypeOnlyStart = IsDontHaveEndTimeEvent(p.eventIndex);
			//p.eventStatus = p.eventTypeOnlyStart ? false : (cur_Time >= p.startTime && cur_Time <= p.endTime);
			p.eventStatus = p.eventTypeOnlyStart ? false : (p.startTime <= cur_Time && cur_Time <= p.endTime);
			const time_t eventStartTime = p.startTime;
			const struct tm vEventStartKey = *localtime(&eventStartTime);

			bool insertWithStart = true;
			if (vEventStartKey.tm_mon < vKey.tm_mon)
				insertWithStart = false;
			SetTimeToString(&p);
			if (insertWithStart)
			{
				const auto it = m_EventManager.find(vEventStartKey.tm_mday);
				if (it != m_EventManager.end())
					it->second.emplace_back(p);
				else
				{
					std::vector<TEventManagerData> m_vec;
					m_vec.emplace_back(p);
					m_EventManager.emplace(vEventStartKey.tm_mday, m_vec);
				}
			}
			else
			{
				const auto it = m_EventManager.find(vEventEndKey.tm_mday);
				if (it != m_EventManager.end())
					it->second.emplace_back(p);
				else
				{
					std::vector<TEventManagerData> m_vec;
					m_vec.emplace_back(p);
					m_EventManager.emplace(vEventEndKey.tm_mday, m_vec);
				}
			}
		}
		if (m_EventManager.size())
		{
			for (auto it = m_EventManager.begin(); it != m_EventManager.end(); ++it)
				std::stable_sort(it->second.begin(), it->second.end(), SortWithTime);
		}
	}
	SendEventData(NULL, updateFromGameMaster);
	return true;
}

void CClientManager::SendEventData(CPeer* pkPeer, bool updateFromGameMaster)
{
	const BYTE subIndex = EVENT_MANAGER_LOAD;
	const BYTE dayCount = m_EventManager.size();
	TEMP_BUFFER buf;
	buf.write(&subIndex, sizeof(BYTE));
	buf.write(&dayCount, sizeof(BYTE));
	buf.write(&updateFromGameMaster, sizeof(bool));

	for(auto it = m_EventManager.begin();it != m_EventManager.end();++it)
	{
		const auto& dayIndex = it->first;
		const auto& dayData = it->second;
		BYTE dayEventCount = dayData.size();
		buf.write(&dayIndex, sizeof(BYTE));
		buf.write(&dayEventCount, sizeof(BYTE));
		if (dayEventCount > 0)
			buf.write(dayData.data(), dayEventCount * sizeof(TEventManagerData));
	}

	if (pkPeer != NULL)
	{
		pkPeer->EncodeHeader(HEADER_DG_EVENT_MANAGER, 0, buf.size());
		pkPeer->Encode(buf.read_peek(), buf.size());
	}
	else
		ForwardPacket(HEADER_DG_EVENT_MANAGER, buf.read_peek(), buf.size());
}
#endif
