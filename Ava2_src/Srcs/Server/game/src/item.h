#ifndef __INC_METIN_II_GAME_ITEM_H__
#define __INC_METIN_II_GAME_ITEM_H__

#include "entity.h"

class CItem : public CEntity
{
	protected:
		// override methods from ENTITY class
		virtual void	EncodeInsertPacket(LPENTITY entity);
		virtual void	EncodeRemovePacket(LPENTITY entity);

	public:
		CItem(DWORD dwVnum);
		virtual ~CItem();

		int			GetLevelLimit();

		bool		CheckItemUseLevel(int nLevel);

		bool		IsPCBangItem();

		long		FindApplyValue(BYTE bApplyType);

		bool		IsStackable()		{ return (GetFlag() & ITEM_FLAG_STACKABLE)?true:false; }

		void		Initialize();
		void		Destroy();

		void		Save();

		void		SetWindow(BYTE b)	{ m_bWindow = b; }
		BYTE		GetWindow()		{ return m_bWindow; }

		void		SetID(DWORD id)		{ m_dwID = id;	}
		DWORD		GetID()			{ return m_dwID; }

		void			SetProto(const TItemTable * table);
		TItemTable const *	GetProto()	{ return m_pProto; }

		long long	GetGold();

		long long	GetShopBuyPrice();

#ifdef ENABLE_MULTILANGUAGE_SYSTEM
		const char * GetName() {return m_pProto ? m_stName.c_str() : "";}
#else
		const char * GetName()			{ return m_pProto ? m_pProto->szLocaleName : NULL; }
#endif

		const char *	GetBaseName()		{ return m_pProto ? m_pProto->szName : NULL; }
		BYTE		GetSize()		{ return m_pProto ? m_pProto->bSize : 0;	}

		void		SetFlag(long flag)	{ m_lFlag = flag;	}
		long		GetFlag()		{ return m_lFlag;	}

		void		AddFlag(long bit);
		void		RemoveFlag(long bit);

		DWORD		GetWearFlag()		{ return m_pProto ? m_pProto->dwWearFlags : 0; }
		DWORD		GetAntiFlag()		{ return m_pProto ? m_pProto->dwAntiFlags : 0; }
		DWORD		GetImmuneFlag()		{ return m_pProto ? m_pProto->dwImmuneFlag : 0; }
#ifdef ENABLE_SPECIAL_COSTUME_ATTR
		bool		CanBonusCostume();
		BYTE		GetBonusFlag();
		BYTE		GetBonusMaxAttr();
#endif

		void		SetVID(DWORD vid)	{ m_dwVID = vid;	}
		DWORD		GetVID()		{ return m_dwVID;	}

#ifdef ENABLE_NEW_NAME_ITEM
		const char*	GetNewName() const	{return nameex;}
		void		SetNewName(const char* name);
#endif

		bool		SetCount(DWORD count);
		DWORD		GetCount();
#ifdef __SOULBINDING_SYSTEM__
		void	Bind(long lTime);
		long	GetBind()	{return m_lBind;}
		bool	IsBind();
		bool	IsUntilBind();
		void	SetUnBindingExpireEvent(LPEVENT pkEvent);
		void	StartUnBindingExpireEvent();
#endif
		// GetVnum�� GetOriginalVnum�� ���� comment
		// GetVnum�� Masking �� Vnum�̴�. �̸� ��������ν�, �������� ���� Vnum�� 10������, Vnum�� 20�� ��ó�� ������ �� �ִ� ���̴�.
		// Masking ���� ori_to_new.txt���� ���ǵ� ���̴�.
		// GetOriginalVnum�� ������ ������ Vnum����, �α� ���� ��, Ŭ���̾�Ʈ�� ������ ���� ���� ��, ������ ���� �� Vnum�� ����Ͽ��� �Ѵ�.
		//
		DWORD		GetVnum() const		{ return m_dwMaskVnum ? m_dwMaskVnum : m_dwVnum;	}
		DWORD		GetOriginalVnum() const		{ return m_dwVnum;	}
		BYTE		GetType() const		{ return m_pProto ? m_pProto->bType : 0;	}
		BYTE		GetSubType() const	{ return m_pProto ? m_pProto->bSubType : 0;	}
		BYTE		GetLimitType(DWORD idx) const { return m_pProto ? m_pProto->aLimits[idx].bType : 0;	}
		long		GetLimitValue(DWORD idx) const { return m_pProto ? m_pProto->aLimits[idx].lValue : 0;	}

		long		GetValue(DWORD idx);

		void		SetCell(LPCHARACTER ch, WORD pos)	{ m_pOwner = ch, m_wCell = pos;	}
		WORD		GetCell()				{ return m_wCell;	}

		LPITEM		RemoveFromCharacter();
		
		bool		AddToCharacter(LPCHARACTER ch, TItemPos Cell);

		LPCHARACTER	GetOwner()		{ return m_pOwner; }

		LPITEM		RemoveFromGround();
		bool		AddToGround(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck = false);

		int			FindEquipCell(LPCHARACTER ch, int bCandidateCell = -1);
		bool		IsEquipped() const		{ return m_bEquipped;	}
		bool		EquipTo(LPCHARACTER ch, BYTE bWearCell);
		bool		IsEquipable() const;

		bool		CanUsedBy(LPCHARACTER ch);

		bool		DistanceValid(LPCHARACTER ch);

		void		UpdatePacket();
		void		UsePacketEncode(LPCHARACTER ch, LPCHARACTER victim, struct packet_item_use * packet);

		void		SetExchanging(bool isOn = true);
		bool		IsExchanging()		{ return m_bExchanging;	}

		bool		IsTwohanded();

		bool		IsPolymorphItem();

		void		ModifyPoints(bool bAdd);	// �������� ȿ���� ĳ���Ϳ� �ο� �Ѵ�. bAdd�� false�̸� ������

		bool		CreateSocket(BYTE bSlot, BYTE bGold);
		const long *	GetSockets()		{ return &m_alSockets[0];	}
		long		GetSocket(int i)	{ return m_alSockets[i];	}

		void		SetSockets(const long * al);
		void		SetSocket(int i, long v, bool bLog = true);

		int		GetSocketCount();
		bool		AddSocket();
#ifdef ENABLE_GLOVE_SYSTEM
		const TPlayerItemAttribute* GetRandomAttributes()		{ return m_randomAttr;	}
		const TPlayerItemAttribute& GetRandomAttribute(int i)	{ return m_randomAttr[i];	}
		BYTE		GetRandomAttributeType(int i)	{ return m_randomAttr[i].bType;	}
		short		GetRandomAttributeValue(int i){ return m_randomAttr[i].sValue;	}

		int		GetRandomAttributeCount();
		void		SetRandomAttributes(const TPlayerItemAttribute* c_pAttribute);
#endif


		const TPlayerItemAttribute* GetAttributes()		{ return m_aAttr;	}
		const TPlayerItemAttribute& GetAttribute(int i)	{ return m_aAttr[i];	}

		BYTE		GetAttributeType(int i)	{ return m_aAttr[i].bType;	}
		short		GetAttributeValue(int i){ return m_aAttr[i].sValue;	}

		void		SetAttributes(const TPlayerItemAttribute* c_pAttribute);

		int		FindAttribute(BYTE bType);
		bool		RemoveAttributeAt(int index);
		bool		RemoveAttributeType(BYTE bType);

		bool		HasAttr(BYTE bApply);
		bool		HasRareAttr(BYTE bApply);
#ifdef ENABLE_GLOVE_SYSTEM
		bool		HasRandomAttr(BYTE bApply);
#endif

		void		SetDestroyEvent(LPEVENT pkEvent);
		void		StartDestroyEvent(int iSec=300);

		DWORD		GetRefinedVnum()	{ return m_pProto ? m_pProto->dwRefinedVnum : 0; }
		DWORD		GetRefineFromVnum();
		int		GetRefineLevel();

		void		SetSkipSave(bool b)	{ m_bSkipSave = b; }
		bool		GetSkipSave()		{ return m_bSkipSave; }

		bool		IsOwnership(LPCHARACTER ch);
		void		SetOwnership(LPCHARACTER ch, int iSec = 10);
		void		SetOwnershipEvent(LPEVENT pkEvent);

		void		SetLastOwnerPID(DWORD pid) { m_dwLastOwnerPID = pid; }
		DWORD		GetLastOwnerPID()	{ return m_dwLastOwnerPID; }

		int		GetAttributeSetIndex(); // �Ӽ� �ٴ°��� ������ �迭�� ��� �ε����� ����ϴ��� �����ش�.
		void		AlterToMagicItem();
		void		AlterToSocketItem(int iSocketCount);

		WORD		GetRefineSet()		{ return m_pProto ? m_pProto->wRefineSet : 0;	}

		void		StartUniqueExpireEvent();
		void		SetUniqueExpireEvent(LPEVENT pkEvent);

		void		StartTimerBasedOnWearExpireEvent();
		void		SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent);

		void		StartRealTimeExpireEvent();
		bool		IsRealTimeItem();

#if defined(ENABLE_NEW_AUTOPOTION)
		void		StartTimeAutoPotionEvent();
		bool		IsNewAutoPotionItem();
		bool		IsTimerAutoPotionEvent() const { return m_pkTimerAutoPotionEvent != NULL; }
#endif

		void		StopUniqueExpireEvent();
		void		StopTimerBasedOnWearExpireEvent();
		void		StopAccessorySocketExpireEvent();

		//			�ϴ� REAL_TIME�� TIMER_BASED_ON_WEAR �����ۿ� ���ؼ��� ����� ������.
		int			GetDuration();

		int		GetAttributeCount();
		void		ClearAttribute();
		void		ClearAttributeDragonSoul();
		void		ChangeAttribute(const int* aiChangeProb=NULL);
		void		AddAttribute();
		void		AddAttribute(BYTE bType, short sValue);
#ifdef ENABLE_GLOVE_SYSTEM
		void		AddRandomAttribute(BYTE bApplyRandom, int iRefineVnum);
#endif

		bool 		IsAttrRings(){return is_attr_rings;}
		void 		SetAttrRings(bool rings){is_attr_rings = rings;}

		void		ApplyAddon(int iAddonType);

		int		GetSpecialGroup() const;
		bool	IsSameSpecialGroup(const LPITEM item) const;

		// ACCESSORY_REFINE
		// �׼������� ������ ���� ������ �߰�
		bool		IsAccessoryForSocket();

		int		GetAccessorySocketGrade();
		int		GetAccessorySocketMaxGrade();
		int		GetAccessorySocketDownGradeTime();

		void		SetAccessorySocketGrade(int iGrade);
		void		SetAccessorySocketMaxGrade(int iMaxGrade);
		void		SetAccessorySocketDownGradeTime(DWORD time);

		void		AccessorySocketDegrade();

		// �Ǽ��縮 �� �����ۿ� �۾����� Ÿ�̸� ���ư��°�( ����, �� )
		void		StartAccessorySocketExpireEvent();
		void		SetAccessorySocketExpireEvent(LPEVENT pkEvent);

		bool		CanPutInto(LPITEM item);
		// END_OF_ACCESSORY_REFINE

		int 		GetAccessorySocketVnumRings(LPITEM item);


		void		CopyAttributeTo(LPITEM pItem);
		void		CopySocketTo(LPITEM pItem);

		int			GetRareAttrCount();
		bool		AddRareAttribute();
		bool		ChangeRareAttribute();



		void 		ChangeAttrCostume();
		void 		AddAttrCostume();
		void		AddAttrCostumeNew(BYTE bApply, BYTE bLevel);
		void		PutAttributeCostume(const int * aiAttrPercentTable);
		void		PutAttributeWithLevelCostume(BYTE bLevel);


		void 		ChangeAttrCostumeHair();
		void 		AddAttrCostumeHair();
		void		AddAttrCostumeHairNew(BYTE bApply, BYTE bLevel);
		void		PutAttributeCostumeHair(const int * aiAttrPercentTable);
		void		PutAttributeWithLevelCostumeHair(BYTE bLevel);



		void		AttrLog();

		void		Lock(bool f) { m_isLocked = f; }
		bool		isLocked() const { return m_isLocked; }

#ifdef ELEMENT_SPELL_WORLDARD
		void		SetElement(BYTE value_grade_element,const DWORD * attack_element, BYTE element_type_bonus, const short * elements_value_bonus);
		void 		SetElementNew(BYTE value_grade_element, DWORD attack_element, BYTE element_type_bonus, short elements_value_bonus);
		BYTE 		GetElementGrade() {return m_grade_element;}
		void 		DeleteAllElement(BYTE value_grade_element);
		void 		ChangeElement(BYTE element_type_bonus);

		const DWORD *	GetElementAttacks() {return &m_attack_element[0];}
		DWORD 			GetElementAttack(int index) {return m_attack_element[index];}

		BYTE				GetElementsType()	{ return m_element_type_bonus;	}
		const short *		GetElementsValues(){ return &m_elements_value_bonus[0];	}
		short				GetElementsValue(int index){ return m_elements_value_bonus[index];	}
#endif
		
	private :
		void		SetAttribute(int i, BYTE bType, short sValue);
#ifdef ENABLE_GLOVE_SYSTEM
		void		SetRandomAttribute(int i, BYTE bType, short sValue);
#endif
#ifdef ENABLE_RARITY
	public:
		bool IsRarityItem();
		void GiveRandomRarity();
		long GetRarityMaxValue();
		bool IncreaseRarity(BYTE increasePercent);
		void SetRarityValue(LPCHARACTER ch, long decreaseValue);
#endif
	public:
		void		SetForceAttribute(int i, BYTE bType, short sValue);

	protected:
		bool		EquipEx(bool is_equip);
		bool		Unequip();

		void		AddAttr(BYTE bApply, BYTE bLevel);
		void		PutAttribute(const int * aiAttrPercentTable);
		void		PutAttributeWithLevel(BYTE bLevel);

	public:
		void		AddRareAttribute2(const int * aiAttrPercentTable = NULL);
	protected:
		void		AddRareAttr(BYTE bApply, BYTE bLevel);
		void		PutRareAttribute(const int * aiAttrPercentTable);
		void		PutRareAttributeWithLevel(BYTE bLevel);

	protected:
		friend class CInputDB;
		bool		OnAfterCreatedItem();			// ������ �������� ��� ������ �Բ� ������ ����(�ε�)�� �� �Ҹ���� �Լ�.

	public:
		bool		IsRideItem();
		bool		IsRamadanRing();

		void		ClearMountAttributeAndAffect();
		bool		IsNewMountItem();

#ifdef __AUCTION__
		// �����
		bool		MoveToAuction ();
		void		CopyToRawData (TPlayerItem* item);
#endif
		// ���Ͽ��� ���� ĳ�� �����۰� ������, ��ȯ ������ ĳ�� �������� ����ٰ� �Ͽ�,
		// �������� �����ۿ�, ��ȯ ���� �÷��׸� ������ ���ο� �����۵��� ���ο� ������ �뿪�� �Ҵ��Ͽ���.
		// ������ ���ο� �����۵� �������� �����۰� ���� ȿ���� �����ϴµ�,
		// ������, Ŭ���, vnum ������� �Ǿ��־�
		// ���ο� vnum�� �˴� ������ ���� �� �ھƾ��ϴ� ��Ÿ��� ��Ȳ�� �´�Ҵ�.
		// �׷��� �� vnum�� �������̸�, �������� ���ư� ���� �������� ������ vnum���� �ٲ㼭 ���� �ϰ�,
		// ������ ���� ���� vnum���� �ٲ��ֵ��� �Ѵ�.

		// Mask vnum�� � ����(ex. ���� ��Ȳ)�� ���� vnum�� �ٲ�� ���ư��� �������� ���� �ִ�.
		void		SetMaskVnum(DWORD vnum)	{	m_dwMaskVnum = vnum; }
		DWORD		GetMaskVnum()			{	return m_dwMaskVnum; }
		bool		IsMaskedItem()	{	return m_dwMaskVnum != 0;	}
		bool		IsTalisman();
#ifdef ENABLE_NEW_PET_SYSTEM
		bool		IsCostumePetItem();
		bool		IsNewPetItem();
#endif
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		bool		IsCostumeMount();
#endif

#ifdef ENABLE_MOUNT_SKIN
		bool		IsCostumeMountSkin();
#endif

		// ��ȥ��
		bool		IsDragonSoul();
		int			GiveMoreTime_Per(float fPercent);
		int			GiveMoreTime_Fix(DWORD dwTime);
#ifdef ENABLE_LOCK_ATTR
		bool		IsAddonItem() { return (GetProto() && GetProto()->sAddonType) ? true : false; }
		BYTE		GetAttrLockIndex(BYTE index);
		int			GetEmptyAttrIndex();
#endif
#ifdef ENABLE_SPECIAL_STORAGE
		bool 		IsUpgradeItem();
		bool 		IsBook();
		bool 		IsStone();
		bool 		IsChange();
		bool 		IsCostume();
		bool 		IsCostumeSkin();
#endif
	private:
		TItemTable const * m_pProto;		// ������ Ÿ��

		DWORD		m_dwVnum;
		LPCHARACTER	m_pOwner;

		BYTE		m_bWindow;		// ���� �������� ��ġ�� ������
		DWORD		m_dwID;			// ������ȣ
		bool		m_bEquipped;	// ���� �Ǿ��°�?
		DWORD		m_dwVID;		// VID
		WORD		m_wCell;		// ��ġ
		DWORD		m_dwCount;		// ����
#ifdef __SOULBINDING_SYSTEM__
		long		m_lBind;
		LPEVENT		m_pkBindingExpireEvent;
#endif
		long		m_lFlag;		// �߰� flag
		DWORD		m_dwLastOwnerPID;	// ������ ������ �־��� ����� PID

		bool		m_bExchanging;	///< ���� ��ȯ�� ����

		long		m_alSockets[ITEM_SOCKET_MAX_NUM];	// ������ ��Ĺ
		TPlayerItemAttribute	m_aAttr[ITEM_ATTRIBUTE_MAX_NUM];
#ifdef ENABLE_GLOVE_SYSTEM
		TPlayerItemAttribute	m_randomAttr[GLOVE_ATTR_MAX_NUM];
#endif

#ifdef ENABLE_MULTILANGUAGE_SYSTEM
		std::string m_stName;
#endif

		LPEVENT		m_pkDestroyEvent;
		LPEVENT		m_pkExpireEvent;
		LPEVENT		m_pkUniqueExpireEvent;
		LPEVENT		m_pkTimerBasedOnWearExpireEvent;
		LPEVENT		m_pkRealTimeExpireEvent;
		
#if defined(ENABLE_NEW_AUTOPOTION)
		LPEVENT		m_pkTimerAutoPotionEvent;
#endif

		LPEVENT		m_pkAccessorySocketExpireEvent;
		LPEVENT		m_pkOwnershipEvent;

		DWORD		m_dwOwnershipPID;

		bool		m_bSkipSave;

		bool		m_isLocked;

		DWORD		m_dwMaskVnum;
		DWORD		m_dwSIGVnum;
#ifdef ELEMENT_SPELL_WORLDARD
		BYTE 	    m_grade_element;
		DWORD 	 	m_attack_element[MAX_ELEMENTS_SPELL];
		BYTE 		m_element_type_bonus;
		short 		m_elements_value_bonus[MAX_ELEMENTS_SPELL];
#endif

#ifdef ENABLE_NEW_NAME_ITEM
		char		nameex[ITEM_NAME_MAX_LEN+1];
#endif
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
		int			destroyTime;
		long		dropMapIndex;
		PIXEL_POSITION		dropPos;
		bool		dropskipOwnerCheck;
#endif
		bool		is_attr_rings;
	public:
		void SetSIGVnum(DWORD dwSIG)
		{
			m_dwSIGVnum = dwSIG;
		}
		DWORD	GetSIGVnum() const
		{
			return m_dwSIGVnum;
		}
#ifdef ENABLE_MULTI_CHEST
public:
	bool	GetUpdateStatus(){return m_dwUpdateStatus;}
	void	SetUpdateStatus(bool bFlag){m_dwUpdateStatus=bFlag;}
protected:
	bool	m_dwUpdateStatus;
#endif

#ifdef ENABLE_COPY_ATTR_ITEM
public:
	bool	CanCopyAttrItem();
#endif
};

EVENTINFO(item_event_info)
{
	LPITEM item;
	char szOwnerName[CHARACTER_NAME_MAX_LEN];

	item_event_info()
	: item( 0 )
	{
		::memset( szOwnerName, 0, CHARACTER_NAME_MAX_LEN );
	}
};

EVENTINFO(item_vid_event_info)
{
	DWORD item_vid;

	item_vid_event_info()
	: item_vid( 0 )
	{
	}
};

#endif
