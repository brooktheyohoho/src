#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "db.h"
#include "packet.h"
#include "desc_client.h"


void CHARACTER::OpenCalendarSystem()
{

	if (IsOpenCalendar()){return;}

	if (m_dwLastOpenCalendar > get_global_time())
	{
		ChatPacket(CHAT_TYPE_INFO, "952 %d", m_dwLastOpenCalendar - get_global_time());
		return;
	}
	m_dwLastOpenCalendar = get_global_time() + 10;

	SetOpenCalendarSystem(true);

	SendCalendarPackets(CALENDAR_RECEIVE_CLEAR_EVENTS);

	LoadEvents();

	SendCalendarPackets(CALENDAR_RECEIVE_OPEN);
}

void CHARACTER::OpenCalendarADM()
{
	SendCalendarPacketsADM(CALENDAR_RECEIVE_CLEAR_EVENTS_ADM);
	LoadEventsADM();
	SendCalendarPacketsADM(CALENDAR_RECEIVE_OPEN_ADM);
}

bool CHARACTER::GetEventsInfo(int day)
{
	char szQuery3[1024];
	snprintf(szQuery3, sizeof(szQuery3),
	"SELECT * FROM player.events WHERE day = %d", day);
	std::auto_ptr<SQLMsg> pMsg3(DBManager::Instance().DirectQuery(szQuery3));
	if (pMsg3->Get()->uiNumRows == 0){
		return false;
	}
	return true;
}

void CHARACTER::SendEvents(int day, int index_event, const char * hora_inicio, const char * hora_final)
{

	if (!IsGM()){
		return;
	}
	
	char hora_inicio_escape[6* 2 + 1];
	char hora_final_espace[6* 2 + 1];

	DBManager::instance().EscapeString(hora_inicio_escape, 	sizeof(hora_inicio_escape), hora_inicio, 	strlen(hora_inicio));
	DBManager::instance().EscapeString(hora_final_espace, 	sizeof(hora_final_espace), 	hora_final, 	strlen(hora_final));

	char szQuery2[1024];
	snprintf(szQuery2, sizeof(szQuery2),
	"SELECT event_name, event_descrip FROM player.events_info WHERE id_event = %d", index_event);
	std::auto_ptr<SQLMsg> pMsg2(DBManager::Instance().DirectQuery(szQuery2));
	if (pMsg2->Get()->uiNumRows > 0){
		MYSQL_ROW  row = mysql_fetch_row(pMsg2->Get()->pSQLResult);

		char * event_name = row[0];
		char * event_descrip = row[1];
	
		if (GetEventsInfo(day)){
			DBManager::instance().DirectQuery("UPDATE player.events SET event = '%s',event_descrip='%s',hora_inicio= '%s',hora_final = '%s' WHERE day = %d",event_name,event_descrip,hora_inicio_escape,hora_final_espace,day);

		}else{
			DBManager::instance().DirectQuery("INSERT INTO player.events(day, event, event_descrip, hora_inicio, hora_final) VALUES(%d,'%s','%s','%s','%s')",
			day,
			event_name,
			event_descrip,
			hora_inicio_escape,
			hora_final_espace);
		}
	}
}

void CHARACTER::LoadEvents()
{
	char szQuery[1024];
	//snprintf(szQuery, sizeof(szQuery),"SELECT day, event, event_descrip, banner, hora_inicio, hora_final, hora_inicio_2, hora_final_2 FROM %sevents", get_table_postfix());
	snprintf(szQuery, sizeof(szQuery), "SELECT EXTRACT(DAY FROM date) as day, name, image, TIME(DATE) as startAt, TIME(DATE_ADD(date, INTERVAL duration second)) as endAt, duration FROM %sevent_proto WHERE EXTRACT(MONTH FROM date) = EXTRACT(MONTH FROM CURDATE()) AND EXTRACT(YEAR FROM date) = EXTRACT(YEAR FROM CURDATE())", get_table_postfix());
	
	std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg->Get()->uiNumRows > 0){

		int day;
		for (int i = 0; i < mysql_num_rows(pMsg->Get()->pSQLResult); ++i)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;

			/*str_to_number(day,row[col++]);
			char * event = row[col++];
			char * event_descrip = row[col++];
			char * banner = row[col++];
			char * hora_inicio = row[col++];
			char * hora_final = row[col++];
			char * hora_inicio_2 = row[col++];
			char * hora_final_2 = row[col++];*/
			
			BYTE bDay = 0;
			str_to_number(bDay, row[col++]);
			
			char szName[100];
			strlcpy(szName, row[col++], sizeof(szName));
			
			char szImage[100];
			strlcpy(szImage, row[col++], sizeof(szImage));
			
			//DWORD dwTimeStamp = 0;
			//str_to_number(dwTimeStamp, row[col++]);

			char szStartAt[100];
			strlcpy(szStartAt, row[col++], sizeof(szStartAt));

			char szEndAt[100];
			strlcpy(szEndAt, row[col++], sizeof(szEndAt));
			
			DWORD dwDuration = 0;
			str_to_number(dwDuration, row[col++]);

			SendCalendarPackets(CALENDAR_RECEIVE_LOAD_EVENTS, bDay, szName, szImage, szStartAt, szEndAt, dwDuration);
		}
	}

	SendCalendarPackets(CALENDAR_RECEIVE_LOADING_EVENTS);
}

void CHARACTER::LoadEventsADM()
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery),"SELECT id_event, event_name, event_descrip FROM %sevents_info", get_table_postfix());
	std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery(szQuery));
	if (pMsg->Get()->uiNumRows > 0){

		int id_event;
		for (int i = 0; i < mysql_num_rows(pMsg->Get()->pSQLResult); ++i)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;

			str_to_number(id_event,row[col++]);
			char * event_name = row[col++];
			char * event_descrip = row[col++];

			SendCalendarPacketsADM(CALENDAR_RECEIVE_LOAD_EVENTS_ADM, id_event, event_name, event_descrip);
		}
	}

	SendCalendarPacketsADM(CALENDAR_RECEIVE_LOADING_EVENTS_ADM);
}


void CHARACTER::SendCalendarPackets(BYTE subheader, BYTE bDay,  const char* name, const char* image, const char* szStartAt, const char* szEndAt, DWORD dwDuration)
{
	TPacketGCCalendarReceive pack;
	pack.bSubType = subheader;

	if(subheader == CALENDAR_RECEIVE_LOAD_EVENTS)
	{
		/*pack.day = day;
		memcpy (pack.event, 		event_escape, 	sizeof(pack.event));
		memcpy (pack.event_descript, event_descrip, 	sizeof(pack.event_descript));
		memcpy (pack.banner, banner, 	sizeof(pack.banner));
		memcpy (pack.hora_inicio, 	hora_inicio, 	sizeof(pack.hora_inicio));
		memcpy (pack.hora_final, 	hora_final, 	sizeof(pack.hora_final));
		memcpy (pack.hora_inicio_2, hora_inicio_2, 	sizeof(pack.hora_inicio_2));
		memcpy (pack.hora_final_2, 	hora_final_2, 	sizeof(pack.hora_final_2));*/

		pack.bDay = bDay;
		strlcpy(pack.szName, name != NULL ?name:"", sizeof(pack.szName));
		strlcpy(pack.szImage, image != NULL ?image:"", sizeof(pack.szImage));
		strlcpy(pack.szStartAt, szStartAt != NULL?szStartAt:"", sizeof(pack.szStartAt));
		strlcpy(pack.szEndAt, szEndAt != NULL?szEndAt:"", sizeof(pack.szEndAt));
		pack.dwDuration = dwDuration;
	}

	if (subheader == CALENDAR_RECEIVE_OPEN){
		pack.dwCurrentTimeStamp = get_global_time();
	}

	LPDESC d = GetDesc();

	if (NULL == d)
	{
		sys_err ("User CHARACTER::SendCalendarPackets (%s)'s DESC is NULL POINT.", GetName());
		return ;
	}

	d->Packet(&pack, sizeof(pack));

}


void CHARACTER::SendCalendarPacketsADM(BYTE subheader, int index_event , const char* event_escape, const char* event_descrip)
{
	TPacketGCCalendarReceiveADM pack;
	pack.bSubType = subheader;

	if(subheader == CALENDAR_RECEIVE_LOAD_EVENTS_ADM)
	{
		pack.index_event = index_event;
		memcpy (pack.event,event_escape!=NULL?event_escape:"", 	sizeof(pack.event));
		memcpy (pack.event_descript,event_escape!=NULL?event_descrip:"", 	sizeof(pack.event_descript));
	}

	LPDESC d = GetDesc();

	if (NULL == d)
	{
		sys_err ("User CHARACTER::SendCalendarPackets (%s)'s DESC is NULL POINT.", GetName());
		return ;
	}

	d->Packet(&pack, sizeof(pack));

}