#include "stdafx.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "MountSystem.h"
#include "../../common/VnumHelper.h"
#include "packet.h"
#include "item_manager.h"
#include "item.h"
#include "db.h"
#include "fstream"
extern int passes_per_sec;
EVENTINFO(mount_event_info)
{
	CMountSystem* pMountSystem;
};


EVENTFUNC(mount_update_event)
{
	mount_event_info* info = dynamic_cast<mount_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "check_speedhack_event> <Factor> Null pointer" );
		return 0;
	}

	CMountSystem* pMountSystem = info->pMountSystem;

	if (NULL == pMountSystem)
		return 0;

	
	pMountSystem->Update(0);
	return PASSES_PER_SEC(1) / 4;
}


CMountActor::CMountActor(LPCHARACTER owner, DWORD vnum)
{
	m_dwVnum = vnum;
	m_dwVID = 0;
	m_pkCharacter = 0;
	m_pkOwner = owner;
#ifdef MOUNT_SYSTEM_BONUS
	m_dwSummonItemVID = 0;
	m_dwSummonItemVnum = 0;
	m_pSummonItem = 0;
#endif
	m_OMoveSpeed = 0;
	m_dwMounted = false;
}

CMountActor::~CMountActor()
{
	this->Unsummon();

	m_pkOwner = 0;
}

void CMountActor::SetName(const char* name)
{
	std::string mName = "";

	mName += name;

	if (true == IsSummoned())
		m_pkCharacter->SetName(mName);

	m_Name = mName;
}



DWORD CMountActor::Mount()
{
	long x = m_pkOwner->GetX(), y = m_pkOwner->GetY(), z = m_pkOwner->GetZ();

	
	x += number(-100, 100);
	y += number(-100, 100);
	
	if (0 == m_pkOwner)
		return 0;
	
	if (IsMounting() == true)
		return 0;

	if (NULL != m_pkCharacter)
		M2_DESTROY_CHARACTER(m_pkCharacter);
	
	m_pkCharacter = 0;

	m_pkOwner->MountVnum(m_dwVnum);
	m_pkOwner->SetQuestFlag("MountSystem.mounted", m_dwVnum);
#ifdef ENABLE_MOUNT_SYSTEM
	m_pkOwner->SetMountingM(true);
#endif
	m_dwMounted = true;
	
#ifdef MOUNT_SYSTEM_BONUS
	this->GiveBuff();
#endif

	return m_dwVnum;;
}

DWORD CMountActor::Unmount()
{
	if (0 == m_pkOwner || IsMounting() == false)
		return 0;

#ifdef MOUNT_SYSTEM_BONUS
	this->ClearBuff();
#endif
	m_pkOwner->SetQuestFlag("MountSystem.mounted", 0);
#ifdef ENABLE_MOUNT_SYSTEM
	m_pkOwner->SetMountingM(false);
#endif
	if (m_pkOwner->IsHorseRiding())
		m_pkOwner->StopRiding();
	
	m_pkOwner->MountVnum(0);	
	m_dwMounted = false;
	
#ifdef MOUNT_SYSTEM_BONUS
	this->Summon(m_pkOwner->GetName(), m_dwVnum, m_pSummonItem);
	// m_pkOwner->ComputePoints();
	// m_pkOwner->UpdatePacket();
	//m_pkOwner->PointChange(POINT_ST, 0);
	//m_pkOwner->PointChange(POINT_DX, 0);
	//m_pkOwner->PointChange(POINT_HT, 0);
	//m_pkOwner->PointChange(POINT_IQ, 0);
#else
	this->Summon(m_pkOwner->GetName(), m_dwVnum);
#endif

	return 1;
}

void CMountActor::Unsummon()
{
	if (true == this->IsSummoned())
	{	

		if(IsMounting() == true)
			this->Unmount();
		
 #ifdef MOUNT_SYSTEM_BONUS
			this->SetSummonItem(NULL);
 #endif
		if (NULL != m_pkOwner)
			m_pkOwner->ComputePoints();


		if (NULL != m_pkCharacter)
			M2_DESTROY_CHARACTER(m_pkCharacter);
		
		m_pkCharacter = 0;
		m_dwVID = 0;
		m_dwMounted = false;
#ifdef ENABLE_MOUNT_SYSTEM
		m_pkOwner->SetMountVnumM(0);
#endif
	}
}

#ifdef MOUNT_SYSTEM_BONUS
DWORD CMountActor::Summon(const char* mName, DWORD mobVnum, LPITEM pSummonItem)
#else
DWORD CMountActor::Summon(const char* mName, DWORD mobVnum)
#endif
{
	long x = m_pkOwner->GetX(), y = m_pkOwner->GetY(), z = m_pkOwner->GetZ();
#ifdef ENABLE_MOUNT_SYSTEM
	m_pkOwner->SetMountingM(false);
#endif
	if (m_pkOwner->IsHorseRiding())
		m_pkOwner->StopRiding();

	m_pkOwner->MountVnum(0);	
	m_dwMounted = false;

	
	x += number(-100, 100);
	y += number(-100, 100);

	if(this->IsMounting() == true && this->IsSummoned() == false)
	{
#ifdef ENABLE_MOUNT_SYSTEM
		m_pkOwner->SetMountingM(false);
#endif
		if (m_pkOwner->IsHorseRiding())
			m_pkOwner->StopRiding();

 #ifdef MOUNT_SYSTEM_BONUS
		this->SetSummonItem(NULL);
 #endif

		m_pkOwner->MountVnum(0);	
		m_dwMounted = false;
		m_pkCharacter = 0;
		m_dwVID = 0;

	}
	if(this->IsSummoned() == true)
	{
		this->Unsummon();
	}
	if (0 != m_pkCharacter)
	{
		m_pkCharacter->Show (m_pkOwner->GetMapIndex(), x, y);
		m_dwVID = m_pkCharacter->GetVID();

		return m_dwVID;
	}
	
	m_pkCharacter = CHARACTER_MANAGER::instance().SpawnMob(mobVnum, m_pkOwner->GetMapIndex(), x, y, z, false, (int)(m_pkOwner->GetRotation()+180), false);

	if (0 == m_pkCharacter)
	{
		sys_err("[CMountActor::Summon] Cannot summon mount - ERROR CODE [1]");
		return 0;
	}


	m_pkCharacter->SetEmpire(m_pkOwner->GetEmpire());
#ifdef ENABLE_MOUNT_SYSTEM
	m_pkCharacter->SetMount();
#endif
	
	m_dwVID = m_pkCharacter->GetVID();
	
	this->SetName(m_pkOwner->GetName());
#ifdef MOUNT_SYSTEM_BONUS
	this->SetSummonItem(pSummonItem);
	m_pSummonItem = pSummonItem;
#endif
#ifdef ENABLE_MOUNT_SYSTEM
	m_pkOwner->SetMountVnumM(m_dwVnum);
#endif
	m_pkOwner->ComputePoints();
	m_pkCharacter->Show(m_pkOwner->GetMapIndex(), x, y, z);
	
	return m_dwVID;
}

bool CMountActor::_UpdatAloneActionAI(float fMinDist, float fMaxDist)
{
	float fDist = number(fMinDist, fMaxDist), r = (float)number (0, 359), dest_x = GetOwner()->GetX() + fDist * cos(r), dest_y = GetOwner()->GetY() + fDist * sin(r);
	
	m_pkCharacter->SetNowWalking(true);

	if (!m_pkCharacter->IsStateMove() && m_pkCharacter->Goto(dest_x, dest_y))
		m_pkCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	m_dwLastActionTime = get_dword_time();
	return true;
}

bool CMountActor::_UpdateFollowAI()
{
	if (0 == m_pkCharacter->m_pkMobData)
		return false;
	
	if (0 == m_OMoveSpeed)
	{
		const CMob* mobData = CMobManager::Instance().Get(m_dwVnum);

		if (0 != mobData)
			m_OMoveSpeed = mobData->m_table.sMovingSpeed;
	}
	float	START_FOLLOW_DISTANCE = 1000.0f, START_RUN_DISTANCE = 1500.0f, RESPAWN_DISTANCE = 4500.f;
	int		APPROACH = 400;						
	
	bool bDoMoveAlone = true, bRun = false;
	
	DWORD currentTime = get_dword_time();

	long ownerX = m_pkOwner->GetX(), ownerY = m_pkOwner->GetY(), charX = m_pkCharacter->GetX(), charY = m_pkCharacter->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_pkOwner->GetRotation() * 3.141592f / 180.f, fx = -APPROACH * cos(fOwnerRot), fy = -APPROACH * sin(fOwnerRot);
		if (m_pkCharacter->Show(m_pkOwner->GetMapIndex(), ownerX + fx, ownerY + fy))
			return true;
	}
	
	
	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if( fDist >= START_RUN_DISTANCE)
		{
			bRun = true;
		}
		m_pkCharacter->SetNowWalking(!bRun);
		Follow(APPROACH);
		m_pkCharacter->SetLastAttacked(currentTime);
		m_dwLastActionTime = currentTime;
	}
	else 
		m_pkCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	
	return true;
}


bool CMountActor::Update(DWORD deltaTime)
{
	bool bResult = true;

#ifdef MOUNT_SYSTEM_BONUS
	if (m_pkOwner->IsDead() || (IsSummoned() && m_pkCharacter->IsDead())
		|| NULL == ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())
		|| ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetOwner() != this->GetOwner()
		)
#else
		if (m_pkOwner->IsDead() || (IsSummoned() && m_pkCharacter->IsDead())
		
		)	
#endif
	{
		if(IsMounting() == true)
			this->Unmount();
		this->Unsummon();
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "ManagerMountSystem 0 0");
		return true;
	}

	if (this->IsSummoned())
		bResult = bResult && this->_UpdateFollowAI();

	return bResult;
}

bool CMountActor::Follow(float fMinDistance)
{
	if( !m_pkOwner || !m_pkCharacter) 
		return false;

	float fOwnerX = m_pkOwner->GetX(), fOwnerY = m_pkOwner->GetY(), fMountX = m_pkCharacter->GetX(), fMountY = m_pkCharacter->GetY(), fDist = DISTANCE_SQRT(fOwnerX - fMountX, fOwnerY - fMountY);
	
	if (fDist <= fMinDistance)
		return false;

	m_pkCharacter->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy, fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_pkCharacter->GetRotation(), fDistToGo, &fx, &fy);
	
	if (!m_pkCharacter->Goto((int)(fMountX+fx+0.5f), (int)(fMountY+fy+0.5f)) )
		return false;

	m_pkCharacter->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

#ifdef MOUNT_SYSTEM_BONUS
void CMountActor::SetSummonItem(LPITEM pItem)
{
	if (NULL == pItem)
	{

		LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(m_dwSummonItemVID);
		if (NULL != pSummonItem)
			pSummonItem->SetSocket(2, 0);

		m_dwSummonItemVID = 0;
		m_dwSummonItemVnum = 0;
		return;
	}

	pItem->SetSocket(2, 1);	
	m_dwSummonItemVID = pItem->GetVID();
	m_dwSummonItemVnum = pItem->GetVnum();
}

void CMountActor::GiveBuff()
{
	// ÆÄÈ² Æê ¹öÇÁ´Â ´øÀü¿¡¼­¸¸ ¹ß»ýÇÔ.
	LPITEM item = ITEM_MANAGER::instance().FindByVID(m_dwSummonItemVID);
	if (NULL != item){
		item->ModifyPoints(true);
	}
	return ;
}

void CMountActor::ClearBuff()
{
	if (NULL == m_pkOwner)
		return ;
	TItemTable* item_proto = ITEM_MANAGER::instance().GetTable(m_dwSummonItemVnum);
	if (NULL == item_proto)
		return;

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; i++)
	{
		if (item_proto->aApplies[i].bType == APPLY_NONE || item_proto->aApplies[i].bType == APPLY_MAX_HP)
			continue;
		m_pkOwner->ApplyPoint(item_proto->aApplies[i].bType, -item_proto->aApplies[i].lValue);
	}

	return ;
}
#endif

CMountSystem::CMountSystem(LPCHARACTER owner)
{
	m_pkOwner = owner;
	m_dwUpdatePeriod = 400;
	m_dwLastUpdateTime = 0;
}

CMountSystem::~CMountSystem()
{
	Destroy();
}


void CMountSystem::Destroy()
{
	for (TMountActorMap::iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		CMountActor* MountActor = iter->second;

		if (0 != MountActor)
			delete MountActor;
	}
	event_cancel(&m_pkMountSystemUpdateEvent);
	m_MountActorMap.clear();
}

bool CMountSystem::Update(DWORD deltaTime)
{
	bool bResult = true;

	DWORD currentTime = get_dword_time();
	
	if (m_dwUpdatePeriod > currentTime - m_dwLastUpdateTime)
		return true;
	
	std::vector <CMountActor*> v_garbageActor;

	for (TMountActorMap::iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		CMountActor* MountActor = iter->second;

		if (0 != MountActor && MountActor->IsSummoned())
		{
			LPCHARACTER pMount = MountActor->GetCharacter();
			
			if (NULL == CHARACTER_MANAGER::instance().Find(pMount->GetVID()))
				v_garbageActor.push_back(MountActor);
			else
				bResult = bResult && MountActor->Update(deltaTime);
		}
	}
	for (std::vector<CMountActor*>::iterator it = v_garbageActor.begin(); it != v_garbageActor.end(); it++)
		DeleteMount(*it);

	m_dwLastUpdateTime = currentTime;

	return bResult;
}

void CMountSystem::DeleteMount(DWORD mobVnum)
{
	TMountActorMap::iterator iter = m_MountActorMap.find(mobVnum);

	if (m_MountActorMap.end() == iter)
	{
		sys_err("CMountSystem::DeleteMount ERROR CODE [1]");
		return;
	}
	CMountActor* MountActor = iter->second;

	if (0 == MountActor)
		sys_err("CMountSystem::DeleteMount ERROR CODE [2]");
	else
		delete MountActor;

	m_MountActorMap.erase(iter);	
}

void CMountSystem::DeleteMount(CMountActor* MountActor)
{
	for (TMountActorMap::iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		if (iter->second == MountActor)
		{
			delete MountActor;
			m_MountActorMap.erase(iter);

			return;
		}
	}

	sys_err("CMountSystem::DeleteMount ERROR CODE [2]");
}

void CMountSystem::Unsummon(DWORD vnum, bool bDeleteFromList)
{
	CMountActor* actor = this->GetByVnum(vnum);
	if (0 == actor)
	{
		sys_err("[CMountSystem::GetByVnum(%d)] Null Pointer (MountActor)", vnum);
		return;
	}
	actor->Unsummon();

	if (true == bDeleteFromList)
		this->DeleteMount(actor);

	bool bActive = false;
	for (TMountActorMap::iterator it = m_MountActorMap.begin(); it != m_MountActorMap.end(); it++)
		bActive |= it->second->IsSummoned();
	
	if (false == bActive)
	{
		event_cancel(&m_pkMountSystemUpdateEvent);
		m_pkMountSystemUpdateEvent = NULL;
	}
}

bool CMountSystem::IsActiveMount()
{
	bool state = false;
	for (TMountActorMap::iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		CMountActor* MountActor = iter->second;
		if (MountActor != 0)
		{
			if (MountActor->IsSummoned()) 
			{
				state = true;
				break;
			}			
		}
	}
	return state;
}

bool CMountSystem::IsMounting(DWORD mobVnum)
{
	CMountActor* MountActor = this->GetByVnum(mobVnum);
	if (MountActor->IsMounting())
		return true;
	else
		return false;
	// return MountActor->IsMounting();
}
#ifdef MOUNT_SYSTEM_BONUS
CMountActor* CMountSystem::Summon(DWORD mobVnum, const char* mName, LPITEM pSummonItem)
#else
CMountActor* CMountSystem::Summon(DWORD mobVnum, const char* mName)
#endif
{
	CMountActor* MountActor = this->GetByVnum(mobVnum);
	if (0 == MountActor)
	{
		MountActor = M2_NEW CMountActor(m_pkOwner, mobVnum);
		m_MountActorMap.insert(std::make_pair(mobVnum, MountActor));
	}
#ifdef MOUNT_SYSTEM_BONUS
	DWORD MountVID = MountActor->Summon(mName, mobVnum, pSummonItem);
#else
	DWORD MountVID = MountActor->Summon(mName, mobVnum);
#endif
	if (!MountVID)
		sys_err("[CPetSystem::Summon(%d)] Null Pointer (mobVnum)", mobVnum);

	if (NULL == m_pkMountSystemUpdateEvent)
	{
		mount_event_info* info = AllocEventInfo<mount_event_info>();

		info->pMountSystem = this;

		m_pkMountSystemUpdateEvent = event_create(mount_update_event, info, PASSES_PER_SEC(1) / 4);	
	}
	return MountActor;
}


CMountActor* CMountSystem::Mount(DWORD mobVnum)
{
	CMountActor* MountActor = this->GetByVnum(mobVnum);
	if (MountActor == NULL){
		sys_err("NULL MOUNTACTOR");
		return 0;
	}

	DWORD MountVID = MountActor->Mount();
	return MountActor;
}

CMountActor* CMountSystem::Unmount(DWORD mobVnum)
{
	CMountActor* MountActor = this->GetByVnum(mobVnum);
	if (MountActor == NULL){
		sys_err("NULL MOUNTACTOR");
		return 0;
	}

	DWORD MountVID = MountActor->Unmount();
	return MountActor;
}

CMountActor* CMountSystem::GetByVID(DWORD vid) const
{
	CMountActor* MountActor = 0;
	bool bFound = false;
	
	for (TMountActorMap::const_iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		MountActor = iter->second;

		if (0 == MountActor)
		{
			sys_err("CMountSystem:: CMountSystem ERROR[1]", vid);
			continue;
		}

		bFound = MountActor->GetVID() == vid;

		if (true == bFound)
			break;
	}
	return bFound ? MountActor : 0;
}

CMountActor* CMountSystem::GetByVnum(DWORD vnum) const
{
	CMountActor* MountActor = 0;

	TMountActorMap::const_iterator iter = m_MountActorMap.find(vnum);

	if (m_MountActorMap.end() != iter)
		MountActor = iter->second;

	return MountActor;
}

size_t CMountSystem::CountSummoned() const
{
	size_t count = 0;

	for (TMountActorMap::const_iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		CMountActor* MountActor = iter->second;

		if (0 != MountActor)
		{
			if (MountActor->IsSummoned())
				++count;
		}
	}
	return count;
}

#ifdef MOUNT_SYSTEM_BONUS
void CMountSystem::RefreshBuff()
{
	for (TMountActorMap::const_iterator iter = m_MountActorMap.begin(); iter != m_MountActorMap.end(); ++iter)
	{
		CMountActor* MountActor = iter->second;

		if (0 != MountActor)
		{
			if (MountActor->IsSummoned())
			{
				MountActor->GiveBuff();
			}
		}
	}
}
#endif