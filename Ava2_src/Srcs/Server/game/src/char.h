#ifndef __INC_METIN_II_CHAR_H__
#define __INC_METIN_II_CHAR_H__

#include <boost/unordered_map.hpp>

#include "../../common/stl.h"
#include "../../common/CommonDefines.h"

#ifdef ENABLE_CSHIELD
#include "cshieldLib.h"
#endif

#include "entity.h"
#include "FSM.h"
#include "horse_rider.h"
#include "vid.h"
#include "constants.h"
#include "affect.h"
#include "affect_flag.h"

#ifndef ENABLE_CUBE_RENEWAL_WORLDARD
#include "cube.h"
#else
#include "cuberenewal.h"
#endif
#include "utils.h"

# ifdef ENABLE_MINI_GAME_CATCH_KING
#include "packet.h"
#endif

#include "mining.h"
#define ENABLE_ANTI_CMD_FLOOD
#define ENABLE_OPEN_SHOP_WITH_ARMOR
enum eMountType {MOUNT_TYPE_NONE=0, MOUNT_TYPE_NORMAL=1, MOUNT_TYPE_COMBAT=2, MOUNT_TYPE_MILITARY=3};
eMountType GetMountLevelByVnum(DWORD dwMountVnum, bool IsNew);
const DWORD GetRandomSkillVnum(BYTE bJob = JOB_MAX_NUM);

#ifdef __NEWPET_SYSTEM__
class CNewPetSystem;
#endif

#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
enum eAutomaticPickUP
{
	AUTOMATIC_PICK_UP_ACTIVATE = (1 << 0),

	AUTOMATIC_PICK_UP_WEAPON = (1 << 1),
	AUTOMATIC_PICK_UP_ARMOR = (1 << 2),
	AUTOMATIC_PICK_UP_SHIELD = (1 << 3),
	AUTOMATIC_PICK_UP_RINGS = (1 << 4),
	AUTOMATIC_PICK_UP_HELMETS = (1 << 5),
	AUTOMATIC_PICK_UP_BRACELETS = (1 << 6),
	AUTOMATIC_PICK_UP_NECKLACE = (1 << 7),
	AUTOMATIC_PICK_UP_EARRINGS = (1 << 8),
	AUTOMATIC_PICK_UP_COSTUMES = (1 << 9),
	AUTOMATIC_PICK_UP_PET_MOUNT = (1 << 10),
	AUTOMATIC_PICK_UP_SASH = (1 << 11),
	AUTOMATIC_PICK_UP_TALISMAN = (1 << 12),
	AUTOMATIC_PICK_UP_BOOKS = (1 << 13),
	AUTOMATIC_PICK_UP_STONES = (1 << 14),
	AUTOMATIC_PICK_UP_YANG = (1 << 15),
	AUTOMATIC_PICK_UP_CHESTS = (1 << 16),
	AUTOMATIC_PICK_UP_SHOES = (1 << 17),
	AUTOMATIC_PICK_UP_BELTS = (1 << 18),
};
#endif

class CBuffOnAttributes;
class CPetSystem;
#ifdef ENABLE_SUPPORT_SYSTEM
class CSupportSystem;
#endif
#ifdef ENABLE_MOUNT_SYSTEM
class CMountSystem;
#endif
#ifdef ENABLE_OFFLINESHOP_SYSTEM
class COfflineShop;
typedef class COfflineShop* LPOFFLINESHOP;
#endif


#define INSTANT_FLAG_DEATH_PENALTY		(1 << 0)
#define INSTANT_FLAG_SHOP			(1 << 1)
#define INSTANT_FLAG_EXCHANGE			(1 << 2)
#define INSTANT_FLAG_STUN			(1 << 3)
#define INSTANT_FLAG_NO_REWARD			(1 << 4)

#define AI_FLAG_NPC				(1 << 0)
#define AI_FLAG_AGGRESSIVE			(1 << 1)
#define AI_FLAG_HELPER				(1 << 2)
#define AI_FLAG_STAYZONE			(1 << 3)
#define MAX_CARDS_IN_HAND	5
#define MAX_CARDS_IN_FIELD	3


#define SET_OVER_TIME(ch, time)	(ch)->SetOverTime(time)

extern int g_nPortalLimitTime;

enum
{
	MAIN_RACE_WARRIOR_M,
	MAIN_RACE_ASSASSIN_W,
	MAIN_RACE_SURA_M,
	MAIN_RACE_SHAMAN_W,
	MAIN_RACE_WARRIOR_W,
	MAIN_RACE_ASSASSIN_M,
	MAIN_RACE_SURA_W,
	MAIN_RACE_SHAMAN_M,
#ifdef ENABLE_WOLFMAN_CHARACTER
	MAIN_RACE_WOLFMAN_M,
#endif
	MAIN_RACE_MAX_NUM,
};

enum
{
	POISON_LENGTH = 30,
#ifdef ENABLE_WOLFMAN_CHARACTER
	BLEEDING_LENGTH = 30,
#endif
	STAMINA_PER_STEP = 1,
	SAFEBOX_PAGE_SIZE = 9,
	AI_CHANGE_ATTACK_POISITION_TIME_NEAR = 10000,
	AI_CHANGE_ATTACK_POISITION_TIME_FAR = 1000,
	AI_CHANGE_ATTACK_POISITION_DISTANCE = 100,
	SUMMON_MONSTER_COUNT = 3,
};

enum
{
	FLY_NONE,
	FLY_EXP,
	FLY_HP_MEDIUM,
	FLY_HP_BIG,
	FLY_SP_SMALL,
	FLY_SP_MEDIUM,
	FLY_SP_BIG,
	FLY_FIREWORK1,
	FLY_FIREWORK2,
	FLY_FIREWORK3,
	FLY_FIREWORK4,
	FLY_FIREWORK5,
	FLY_FIREWORK6,
	FLY_FIREWORK_CHRISTMAS,
	FLY_CHAIN_LIGHTNING,
	FLY_HP_SMALL,
	FLY_SKILL_MUYEONG,
#ifdef ENABLE_CONQUEROR_LEVEL
	FLY_CONQUEROR_EXP,
#endif

};

enum EDamageType
{
	DAMAGE_TYPE_NONE,
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_NORMAL_RANGE,
	//스킬
	DAMAGE_TYPE_MELEE,
	DAMAGE_TYPE_RANGE,
	DAMAGE_TYPE_FIRE,
	DAMAGE_TYPE_ICE,
	DAMAGE_TYPE_ELEC,
	DAMAGE_TYPE_MAGIC,
	DAMAGE_TYPE_POISON,
	DAMAGE_TYPE_SPECIAL,
#ifdef ENABLE_WOLFMAN_CHARACTER
	DAMAGE_TYPE_BLEEDING,
#endif
};

enum DamageFlag
{
	DAMAGE_NORMAL	= (1 << 0),
	DAMAGE_POISON	= (1 << 1),
	DAMAGE_DODGE	= (1 << 2),
	DAMAGE_BLOCK	= (1 << 3),
	DAMAGE_PENETRATE= (1 << 4),
	DAMAGE_CRITICAL = (1 << 5),
#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_MOB_BLEEDING_AS_POISON)
	DAMAGE_BLEEDING	= (1 << 6),
#endif
};

enum EPointTypes
{
	POINT_NONE,                 // 0
	POINT_LEVEL,                // 1
	POINT_VOICE,                // 2
	POINT_EXP,                  // 3
	POINT_NEXT_EXP,             // 4
	POINT_HP,                   // 5
	POINT_MAX_HP,               // 6
	POINT_SP,                   // 7
	POINT_MAX_SP,               // 8
	POINT_STAMINA,              // 9  스테미너
	POINT_MAX_STAMINA,          // 10 최대 스테미너

	POINT_GOLD,                 // 11
	POINT_ST,                   // 12 근력
	POINT_HT,                   // 13 체력
	POINT_DX,                   // 14 민첩성
	POINT_IQ,                   // 15 정신력
	POINT_DEF_GRADE,		// 16 ...
	POINT_ATT_SPEED,            // 17 공격속도
	POINT_ATT_GRADE,		// 18 공격력 MAX
	POINT_MOV_SPEED,            // 19 이동속도
	POINT_CLIENT_DEF_GRADE,	// 20 방어등급
	POINT_CASTING_SPEED,        // 21 주문속도 (쿨다운타임*100) / (100 + 이값) = 최종 쿨다운 타임
	POINT_MAGIC_ATT_GRADE,      // 22 마법공격력
	POINT_MAGIC_DEF_GRADE,      // 23 마법방어력
	POINT_EMPIRE_POINT,         // 24 제국점수
	POINT_LEVEL_STEP,           // 25 한 레벨에서의 단계.. (1 2 3 될 때 보상, 4 되면 레벨 업)
	POINT_STAT,                 // 26 능력치 올릴 수 있는 개수
	POINT_SUB_SKILL,		// 27 보조 스킬 포인트
	POINT_SKILL,		// 28 액티브 스킬 포인트
	POINT_WEAPON_MIN,		// 29 무기 최소 데미지
	POINT_WEAPON_MAX,		// 30 무기 최대 데미지
	POINT_PLAYTIME,             // 31 플레이시간
	POINT_HP_REGEN,             // 32 HP 회복률
	POINT_SP_REGEN,             // 33 SP 회복률

	POINT_BOW_DISTANCE,         // 34 활 사정거리 증가치 (meter)

	POINT_HP_RECOVERY,          // 35 체력 회복 증가량
	POINT_SP_RECOVERY,          // 36 정신력 회복 증가량

	POINT_POISON_PCT,           // 37 독 확률
	POINT_STUN_PCT,             // 38 기절 확률
	POINT_SLOW_PCT,             // 39 슬로우 확률
	POINT_CRITICAL_PCT,         // 40 크리티컬 확률
	POINT_PENETRATE_PCT,        // 41 관통타격 확률
	POINT_CURSE_PCT,            // 42 저주 확률

	POINT_ATTBONUS_HUMAN,       // 43 인간에게 강함
	POINT_ATTBONUS_ANIMAL,      // 44 동물에게 데미지 % 증가
	POINT_ATTBONUS_ORC,         // 45 웅귀에게 데미지 % 증가
	POINT_ATTBONUS_MILGYO,      // 46 밀교에게 데미지 % 증가
	POINT_ATTBONUS_UNDEAD,      // 47 시체에게 데미지 % 증가
	POINT_ATTBONUS_DEVIL,       // 48 마귀(악마)에게 데미지 % 증가
	POINT_ATTBONUS_INSECT,      // 49 벌레족
	POINT_ATTBONUS_FIRE,        // 50 화염족
	POINT_ATTBONUS_ICE,         // 51 빙설족
	POINT_ATTBONUS_DESERT,      // 52 사막족
	POINT_ATTBONUS_MONSTER,     // 53 모든 몬스터에게 강함
	POINT_ATTBONUS_WARRIOR,     // 54 무사에게 강함
	POINT_ATTBONUS_ASSASSIN,	// 55 자객에게 강함
	POINT_ATTBONUS_SURA,		// 56 수라에게 강함
	POINT_ATTBONUS_SHAMAN,		// 57 무당에게 강함
	POINT_ATTBONUS_TREE,     	// 58 나무에게 강함 20050729.myevan UNUSED5

	POINT_RESIST_WARRIOR,		// 59 무사에게 저항
	POINT_RESIST_ASSASSIN,		// 60 자객에게 저항
	POINT_RESIST_SURA,			// 61 수라에게 저항
	POINT_RESIST_SHAMAN,		// 62 무당에게 저항

	POINT_STEAL_HP,             // 63 생명력 흡수
	POINT_STEAL_SP,             // 64 정신력 흡수

	POINT_MANA_BURN_PCT,        // 65 마나 번

	/// 피해시 보너스 ///

	POINT_DAMAGE_SP_RECOVER,    // 66 공격당할 시 정신력 회복 확률

	POINT_BLOCK,                // 67 블럭율
	POINT_DODGE,                // 68 회피율

	POINT_RESIST_SWORD,         // 69
	POINT_RESIST_TWOHAND,       // 70
	POINT_RESIST_DAGGER,        // 71
	POINT_RESIST_BELL,          // 72
	POINT_RESIST_FAN,           // 73
	POINT_RESIST_BOW,           // 74  화살   저항   : 대미지 감소
	POINT_RESIST_FIRE,          // 75  화염   저항   : 화염공격에 대한 대미지 감소
	POINT_RESIST_ELEC,          // 76  전기   저항   : 전기공격에 대한 대미지 감소
	POINT_RESIST_MAGIC,         // 77  술법   저항   : 모든술법에 대한 대미지 감소
	POINT_RESIST_WIND,          // 78  바람   저항   : 바람공격에 대한 대미지 감소

	POINT_REFLECT_MELEE,        // 79 공격 반사

	/// 특수 피해시 ///
	POINT_REFLECT_CURSE,		// 80 저주 반사
	POINT_POISON_REDUCE,		// 81 독데미지 감소

	/// 적 소멸시 ///
	POINT_KILL_SP_RECOVER,		// 82 적 소멸시 MP 회복
	POINT_EXP_DOUBLE_BONUS,		// 83
	POINT_GOLD_DOUBLE_BONUS,		// 84
	POINT_ITEM_DROP_BONUS,		// 85

	/// 회복 관련 ///
	POINT_POTION_BONUS,			// 86
	POINT_KILL_HP_RECOVERY,		// 87

	POINT_IMMUNE_STUN,			// 88
	POINT_IMMUNE_SLOW,			// 89
	POINT_IMMUNE_FALL,			// 90
	//////////////////

	POINT_PARTY_ATTACKER_BONUS,		// 91
	POINT_PARTY_TANKER_BONUS,		// 92

	POINT_ATT_BONUS,			// 93
	POINT_DEF_BONUS,			// 94

	POINT_ATT_GRADE_BONUS,		// 95
	POINT_DEF_GRADE_BONUS,		// 96
	POINT_MAGIC_ATT_GRADE_BONUS,	// 97
	POINT_MAGIC_DEF_GRADE_BONUS,	// 98

	POINT_RESIST_NORMAL_DAMAGE,		// 99

	POINT_HIT_HP_RECOVERY,		// 100
	POINT_HIT_SP_RECOVERY, 		// 101
	POINT_MANASHIELD,			// 102 흑신수호 스킬에 의한 마나쉴드 효과 정도

	POINT_PARTY_BUFFER_BONUS,		// 103
	POINT_PARTY_SKILL_MASTER_BONUS,	// 104

	POINT_HP_RECOVER_CONTINUE,		// 105
	POINT_SP_RECOVER_CONTINUE,		// 106

	POINT_STEAL_GOLD,			// 107
	POINT_POLYMORPH,			// 108 변신한 몬스터 번호
	POINT_MOUNT,			// 109 타고있는 몬스터 번호

	POINT_PARTY_HASTE_BONUS,		// 110
	POINT_PARTY_DEFENDER_BONUS,		// 111
	POINT_STAT_RESET_COUNT,		// 112 피의 단약 사용을 통한 스텟 리셋 포인트 (1당 1포인트 리셋가능)

	POINT_HORSE_SKILL,			// 113

	POINT_MALL_ATTBONUS,		// 114 공격력 +x%
	POINT_MALL_DEFBONUS,		// 115 방어력 +x%
	POINT_MALL_EXPBONUS,		// 116 경험치 +x%
	POINT_MALL_ITEMBONUS,		// 117 아이템 드롭율 x/10배
	POINT_MALL_GOLDBONUS,		// 118 돈 드롭율 x/10배

	POINT_MAX_HP_PCT,			// 119 최대생명력 +x%
	POINT_MAX_SP_PCT,			// 120 최대정신력 +x%

	POINT_SKILL_DAMAGE_BONUS,		// 121 스킬 데미지 *(100+x)%
	POINT_NORMAL_HIT_DAMAGE_BONUS,	// 122 평타 데미지 *(100+x)%

	// DEFEND_BONUS_ATTRIBUTES
	POINT_SKILL_DEFEND_BONUS,		// 123 스킬 방어 데미지
	POINT_NORMAL_HIT_DEFEND_BONUS,	// 124 평타 방어 데미지
	// END_OF_DEFEND_BONUS_ATTRIBUTES

	// PC_BANG_ITEM_ADD
	POINT_PC_BANG_EXP_BONUS,		// 125 PC방 전용 경험치 보너스
	POINT_PC_BANG_DROP_BONUS,		// 126 PC방 전용 드롭률 보너스
	// END_PC_BANG_ITEM_ADD
	POINT_RAMADAN_CANDY_BONUS_EXP,			// 라마단 사탕 경험치 증가용

	POINT_ENERGY = 128,					// 128 기력

	// 기력 ui 용.
	// 서버에서 쓰지 않기만, 클라이언트에서 기력의 끝 시간을 POINT로 관리하기 때문에 이렇게 한다.
	// 아 부끄럽다
	POINT_ENERGY_END_TIME = 129,					// 129 기력 종료 시간

	POINT_COSTUME_ATTR_BONUS = 130,
	POINT_MAGIC_ATT_BONUS_PER = 131,
	POINT_MELEE_MAGIC_ATT_BONUS_PER = 132,

	// 추가 속성 저항
	POINT_RESIST_ICE = 133,          //   냉기 저항   : 얼음공격에 대한 대미지 감소
	POINT_RESIST_EARTH = 134,        //   대지 저항   : 얼음공격에 대한 대미지 감소
	POINT_RESIST_DARK = 135,         //   어둠 저항   : 얼음공격에 대한 대미지 감소

	POINT_RESIST_CRITICAL = 136,		// 크리티컬 저항	: 상대의 크리티컬 확률을 감소
	POINT_RESIST_PENETRATE = 137,		// 관통타격 저항	: 상대의 관통타격 확률을 감소

#ifdef ENABLE_WOLFMAN_CHARACTER
	POINT_BLEEDING_REDUCE = 138,
	POINT_BLEEDING_PCT = 139,

	POINT_ATTBONUS_WOLFMAN = 140,				// 140 수인족에게 강함
	POINT_RESIST_WOLFMAN = 141,				// 141 수인족에게 저항
	POINT_RESIST_CLAW = 142,					// 142 CLAW에 저항
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	POINT_ACCEDRAIN_RATE = 143,
#endif
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	POINT_RESIST_MAGIC_REDUCTION = 144,
#endif

#ifdef ENABLE_NEW_BONUS_WORLDARD
	POINT_RESIST_HUMAN = 220,
#endif

#ifdef ELEMENT_NEW_BONUSES
	POINT_ATTBONUS_ELEC = 176,
	POINT_ATTBONUS_WIND = 177,
	POINT_ATTBONUS_EARTH  = 178,
	POINT_ATTBONUS_DARK  = 179,
#endif

#ifdef ELEMENT_SPELL_WORLDARD
	POINT_ATT_ELEMENTS = 180,
#endif

#ifdef BATTLEPASS_WORLDARD
	POINT_BATTLE_PASS_ID	= 181,
#endif
#ifdef ELEMENT_NEW_BONUSES
	POINT_ENCHANT_ELEC,
	POINT_ENCHANT_FIRE,
	POINT_ENCHANT_ICE,
	POINT_ENCHANT_WIND,
	POINT_ENCHANT_EARTH,
	POINT_ENCHANT_DARK,

	POINT_ATTBONUS_SWORD,		
	POINT_ATTBONUS_TWOHAND,	
	POINT_ATTBONUS_DAGGER,	
	POINT_ATTBONUS_BELL,		
	POINT_ATTBONUS_FAN,		
	POINT_ATTBONUS_BOW,

	POINT_ATTBONUS_ZODIAC,
#ifdef ENABLE_WOLFMAN_CHARACTER
	POINT_ATTBONUS_CLAW,
#endif
#endif
#ifdef NEW_BONUS
	POINT_ATTBONUS_STONE,
	POINT_ATTBONUS_BOSS,
#endif

#ifdef ENABLE_CONQUEROR_LEVEL
	POINT_CONQUEROR_LEVEL = 205,
	POINT_CONQUEROR_LEVEL_STEP = 206,
	POINT_SUNGMA_STR = 207,
	POINT_SUNGMA_HP = 208,
	POINT_SUNGMA_MOVE = 209,
	POINT_SUNGMA_INMUNE = 210,
	POINT_CONQUEROR_POINT = 211,
	POINT_CONQUEROR_EXP = 212,
	POINT_CONQUEROR_NEXT_EXP = 213,
#endif


#ifdef __GEM_SYSTEM__
	POINT_GEM = 240,
#endif


	//POINT_MAX_NUM = 129	common/length.h
};

enum EPKModes
{
	PK_MODE_PEACE,
	PK_MODE_REVENGE,
	PK_MODE_FREE,
	PK_MODE_PROTECT,
	PK_MODE_GUILD,
#ifdef TOURNAMENT_PVP_SYSTEM
	PK_MODE_TEAM_A,
	PK_MODE_TEAM_B,
#endif
	PK_MODE_MAX_NUM
};

enum EPositions
{
	POS_DEAD,
	POS_SLEEPING,
	POS_RESTING,
	POS_SITTING,
	POS_FISHING,
	POS_FIGHTING,
	POS_MOUNTING,
	POS_STANDING
};

enum EBlockAction
{
	BLOCK_EXCHANGE		= (1 << 0),
	BLOCK_PARTY_INVITE		= (1 << 1),
	BLOCK_GUILD_INVITE		= (1 << 2),
	BLOCK_WHISPER		= (1 << 3),
	BLOCK_MESSENGER_INVITE	= (1 << 4),
	BLOCK_PARTY_REQUEST		= (1 << 5),
};

// <Factor> Dynamically evaluated CHARACTER* equivalent.
// Referring to SCharDeadEventInfo.
struct DynamicCharacterPtr {
	DynamicCharacterPtr() : is_pc(false), id(0) {}
	DynamicCharacterPtr(const DynamicCharacterPtr& o)
		: is_pc(o.is_pc), id(o.id) {}

	// Returns the LPCHARACTER found in CHARACTER_MANAGER.
	LPCHARACTER Get() const;
	// Clears the current settings.
	void Reset() {
		is_pc = false;
		id = 0;
	}

	// Basic assignment operator.
	DynamicCharacterPtr& operator=(const DynamicCharacterPtr& rhs) {
		is_pc = rhs.is_pc;
		id = rhs.id;
		return *this;
	}
	// Supports assignment with LPCHARACTER type.
	DynamicCharacterPtr& operator=(LPCHARACTER character);
	// Supports type casting to LPCHARACTER.
	operator LPCHARACTER() const {
		return Get();
	}

	bool is_pc;
	uint32_t id;
};

/* 저장하는 데이터 */
typedef struct character_point
{
	long long		points[POINT_MAX_NUM];

	BYTE			job;
	BYTE			voice;

	BYTE			level;

#ifdef ENABLE_CONQUEROR_LEVEL
	BYTE	conquerorlevel;
	DWORD	conqueror_exp;
#endif

	DWORD			exp;
	long long		gold;
#ifdef __GEM_SYSTEM__
	int			gem;
#endif

	int				hp;
	int				sp;

	int				iRandomHP;
	int				iRandomSP;

	int				stamina;

	BYTE			skill_group;
#ifdef ENABLE_NEW_DETAILS_GUI
	long			kill_log[KILL_MAX_NUM];
#endif
} CHARACTER_POINT;

/* 저장되지 않는 캐릭터 데이터 */
typedef struct character_point_instant
{
	long long		points[POINT_MAX_NUM];

	float			fRot;

	int				iMaxHP;
	int				iMaxSP;

	long			position;

	long			instant_flag;
	DWORD			dwAIFlag;
	DWORD			dwImmuneFlag;
	DWORD			dwLastShoutPulse;

	WORD			parts[PART_MAX_NUM];

// 아... 진짜 욕을 안 할래야 안 할 수가 없다.
// char는 인벤을 BYTE array로 grid를 관리하고, exchange나 cube는 CGrid로 grid를 관리하고 뭐냐 이거...
// grid를 만들어 놨으면 grid를 쓰란 말이야!!!
// ㅅㅂ 용혼석 인벤을 똑같이 따라서 만든 나도 잘못했다 ㅠㅠ
	LPITEM			pItems[INVENTORY_AND_EQUIP_SLOT_MAX];
	BYTE			bItemGrid[INVENTORY_AND_EQUIP_SLOT_MAX];

	// 용혼석 인벤토리.
	LPITEM			pDSItems[DRAGON_SOUL_INVENTORY_MAX_NUM];
	WORD			wDSItemGrid[DRAGON_SOUL_INVENTORY_MAX_NUM];
#ifdef ENABLE_SWITCHBOT
	LPITEM			pSwitchbotItems[SWITCHBOT_SLOT_COUNT];
#endif
#ifdef ENABLE_SPECIAL_STORAGE
	LPITEM			pSSUItems[SPECIAL_INVENTORY_MAX_NUM];
	WORD			wSSUItemGrid[SPECIAL_INVENTORY_MAX_NUM];
	LPITEM			pSSBItems[SPECIAL_INVENTORY_MAX_NUM];
	WORD			wSSBItemGrid[SPECIAL_INVENTORY_MAX_NUM];
	LPITEM			pSSSItems[SPECIAL_INVENTORY_MAX_NUM];
	WORD			wSSSItemGrid[SPECIAL_INVENTORY_MAX_NUM];

	LPITEM			pSSCItems[SPECIAL_INVENTORY_MAX_NUM];
	WORD			wSSCItemGrid[SPECIAL_INVENTORY_MAX_NUM];
	
	LPITEM			pCostumeItems[SPECIAL_INVENTORY_MAX_NUM];
	WORD			wCostumeItemGrid[SPECIAL_INVENTORY_MAX_NUM];

#endif

#ifdef CHANGE_EQUIP_WORLDARD
	LPITEM 			pChangeEquipItem[CHANGE_EQUIP_SLOT_COUNT];
#endif

	// by mhh
	LPITEM			pCubeItems[CUBE_MAX_NUM];
	LPCHARACTER		pCubeNpc;
	#ifdef __SASH_SYSTEM__
	LPITEM	pSashMaterials[SASH_WINDOW_MAX_MATERIALS];
	#endif
	LPCHARACTER			battle_victim;

	BYTE			gm_level;

	BYTE			bBasePart;	// 평상복 번호

	int				iMaxStamina;

	BYTE			bBlockMode;

	int				iDragonSoulActiveDeck;
	LPENTITY		m_pDragonSoulRefineWindowOpener;
#ifdef ENABLE_ANTI_EXP
	bool			anti_exp;
#endif
} CHARACTER_POINT_INSTANT;

#define TRIGGERPARAM		LPCHARACTER ch, LPCHARACTER causer

typedef struct trigger
{
	BYTE	type;
	int		(*func) (TRIGGERPARAM);
	long	value;
} TRIGGER;

class CTrigger
{
	public:
		CTrigger() : bType(0), pFunc(NULL)
		{
		}

		BYTE	bType;
		int	(*pFunc) (TRIGGERPARAM);
};

EVENTINFO(char_event_info)
{
	DynamicCharacterPtr ch;
};

typedef std::map<VID, size_t> target_map;
struct TSkillUseInfo
{
	int	    iHitCount;
	int	    iMaxHitCount;
	int	    iSplashCount;
	DWORD   dwNextSkillUsableTime;
	int	    iRange;
	bool    bSkillCD;
	DWORD   dwHitCount;
	bool    bUsed;
	DWORD   dwVID;
	bool    isGrandMaster;

	target_map TargetVIDMap;

	TSkillUseInfo()
		: iHitCount(0), iMaxHitCount(0), iSplashCount(0), dwNextSkillUsableTime(0), iRange(0), bSkillCD(false), dwHitCount(0), bUsed(false),
		dwVID(0), isGrandMaster(false)
   	{}

	bool    HitOnce(DWORD dwVnum = 0);
	bool    IsSkillCooldown(DWORD dwVnum, float fSkillPower);

	bool    UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount = 1, int hitcount = -1, int range = -1);
	DWORD   GetMainTargetVID() const	{ return dwVID; }
	void    SetMainTargetVID(DWORD vid) { dwVID=vid; }
	void    ResetHitCount() { if (iSplashCount) { iHitCount = iMaxHitCount; iSplashCount--; } }
};

typedef struct packet_party_update TPacketGCPartyUpdate;
class CExchange;
class CSkillProto;
class CParty;
class CDungeon;
class CWarMap;
class CAffect;
class CGuild;
class CSafebox;
class CArena;
class CShop;
typedef class CShop * LPSHOP;

class CMob;
class CMobInstance;
typedef struct SMobSkillInfo TMobSkillInfo;

//SKILL_POWER_BY_LEVEL
extern int GetSkillPowerByLevelFromType(int job, int skillgroup, int skilllevel);
//END_SKILL_POWER_BY_LEVEL

namespace marriage
{
	class WeddingMap;
}
enum e_overtime
{
	OT_NONE,
	OT_3HOUR,
	OT_5HOUR,
};

#define NEW_ICEDAMAGE_SYSTEM
class CHARACTER : public CEntity, public CFSM, public CHorseRider
{
	protected:
		//////////////////////////////////////////////////////////////////////////////////
		// Entity 관련
		virtual void	EncodeInsertPacket(LPENTITY entity);
		virtual void	EncodeRemovePacket(LPENTITY entity);
		//////////////////////////////////////////////////////////////////////////////////

	public:
		LPCHARACTER			FindCharacterInView(const char * name, bool bFindPCOnly);
		void				UpdatePacket();

		//////////////////////////////////////////////////////////////////////////////////
		// FSM (Finite State Machine) 관련
	protected:
		CStateTemplate<CHARACTER>	m_stateMove;
		CStateTemplate<CHARACTER>	m_stateBattle;
		CStateTemplate<CHARACTER>	m_stateIdle;

	public:
		virtual void		StateMove();
		virtual void		StateBattle();
		virtual void		StateIdle();
		virtual void		StateFlag();
		virtual void		StateFlagBase();
		void				StateHorse();

	protected:
		// STATE_IDLE_REFACTORING
		void				__StateIdle_Monster();
		void				__StateIdle_Stone();
		void				__StateIdle_NPC();
		// END_OF_STATE_IDLE_REFACTORING

	public:
		DWORD GetAIFlag() const	{ return m_pointsInstant.dwAIFlag; }

		void				SetAggressive();
		bool				IsAggressive() const;

		void				SetCoward();
		bool				IsCoward() const;
		void				CowardEscape();

		void				SetNoAttackShinsu();
		bool				IsNoAttackShinsu() const;

		void				SetNoAttackChunjo();
		bool				IsNoAttackChunjo() const;

		void				SetNoAttackJinno();
		bool				IsNoAttackJinno() const;

		void				SetAttackMob();
		bool				IsAttackMob() const;

		virtual void			BeginStateEmpty();
		virtual void			EndStateEmpty() {}

		void				RestartAtSamePos();

	protected:
		DWORD				m_dwStateDuration;
		//////////////////////////////////////////////////////////////////////////////////

	public:
		CHARACTER();
		virtual ~CHARACTER();

		void			Create(const char * c_pszName, DWORD vid, bool isPC);
		void			Destroy();

		void			Disconnect(const char * c_pszReason);

	protected:
		void			Initialize();

		//////////////////////////////////////////////////////////////////////////////////
		// Basic Points
#ifdef __SEND_TARGET_INFO__
	private:
		DWORD			dwLastTargetInfoPulse;

	public:
		DWORD			GetLastTargetInfoPulse() const	{ return dwLastTargetInfoPulse; }
		void			SetLastTargetInfoPulse(DWORD pulse) { dwLastTargetInfoPulse = pulse; }
#endif
	public:
		DWORD			GetPlayerID() const	{ return m_dwPlayerID; }

		void			SetPlayerProto(const TPlayerTable * table);
		void			CreatePlayerProto(TPlayerTable & tab);	// 저장 시 사용

		void			SetProto(const CMob * c_pkMob);
		WORD			GetRaceNum() const;

		void			Save();		// DelayedSave
		void			SaveReal();	// 실제 저장
		void			FlushDelayedSaveItem();
#ifdef ENABLE_NEW_AFFECT_POTION	
		bool			SetAffectPotion(LPITEM item);
#endif	
		const char *	GetName() const;
		const VID &		GetVID() const		{ return m_vid;		}

		void			SetName(const std::string& name) { m_stName = name; }

		void			SetRace(BYTE race);
		bool			ChangeSex();

		DWORD			GetAID() const;
		int				GetChangeEmpireCount() const;
		void			SetChangeEmpireCount();
		int				ChangeEmpire(BYTE empire);

		BYTE			GetJob() const;
		BYTE			GetCharType() const;

		bool			IsPC() const		{ return GetDesc() ? true : false; }
		bool			IsNPC()	const		{ return m_bCharType != CHAR_TYPE_PC; }
		bool			IsMonster()	const	{ return m_bCharType == CHAR_TYPE_MONSTER; }
		bool			IsStone() const		{ return m_bCharType == CHAR_TYPE_STONE; }
		bool			IsDoor() const		{ return m_bCharType == CHAR_TYPE_DOOR; }
		bool			IsBuilding() const	{ return m_bCharType == CHAR_TYPE_BUILDING;  }
		bool			IsWarp() const		{ return m_bCharType == CHAR_TYPE_WARP; }
		bool			IsGoto() const		{ return m_bCharType == CHAR_TYPE_GOTO; }
//		bool			IsPet() const		{ return m_bCharType == CHAR_TYPE_PET; }

		DWORD			GetLastShoutPulse() const	{ return m_pointsInstant.dwLastShoutPulse; }
		void			SetLastShoutPulse(DWORD pulse) { m_pointsInstant.dwLastShoutPulse = pulse; }
		int				GetLevel() const		{ return m_points.level;	}
		void			SetLevel(BYTE level);
#ifdef ENABLE_CONQUEROR_LEVEL
		int			GetConquerorLevel() const		{ return m_points.conquerorlevel;	}
		void			SetConquerorLevel(BYTE level);
		DWORD			GetConquerorExp() const		{ return m_points.conqueror_exp;	}
		void			SetConquerorExp(DWORD exp)	{ m_points.conqueror_exp = exp;	}
		DWORD			GetConquerorNextExp() const;
#endif


		BYTE			GetGMLevel() const;
		BOOL 			IsGM() const;
		void			SetGMLevel();

		DWORD			GetExp() const		{ return m_points.exp;	}
		void			SetExp(DWORD exp)	{ m_points.exp = exp;	}
		DWORD			GetNextExp() const;
#ifdef __ANTI_EXP__
		bool            block_exp;  
#endif

#ifdef __NEWPET_SYSTEM__
		DWORD			PetGetNextExp() const;
#endif
		LPCHARACTER		DistributeExp();	// 제일 많이 때린 사람을 리턴한다.
		void			DistributeHP(LPCHARACTER pkKiller);
		void			DistributeSP(LPCHARACTER pkKiller, int iMethod=0);

		void			SetPosition(int pos);
		bool			IsPosition(int pos) const	{ return m_pointsInstant.position == pos ? true : false; }
		int				GetPosition() const		{ return m_pointsInstant.position; }

		void			SetPart(BYTE bPartPos, WORD wVal);
		WORD			GetPart(BYTE bPartPos) const;
		WORD			GetOriginalPart(BYTE bPartPos) const;

		void			SetHP(int hp)		{ m_points.hp = hp; }
		int				GetHP() const		{ return m_points.hp; }

		void			SetSP(int sp)		{ m_points.sp = sp; }
		int				GetSP() const		{ return m_points.sp; }

		void			SetStamina(int stamina)	{ m_points.stamina = stamina; }
		int				GetStamina() const		{ return m_points.stamina; }

		void			SetMaxHP(int iVal)	{ m_pointsInstant.iMaxHP = iVal; }
		int				GetMaxHP() const	{ return m_pointsInstant.iMaxHP; }

		void			SetMaxSP(int iVal)	{ m_pointsInstant.iMaxSP = iVal; }
		int				GetMaxSP() const	{ return m_pointsInstant.iMaxSP; }

		void			SetMaxStamina(int iVal)	{ m_pointsInstant.iMaxStamina = iVal; }
		int				GetMaxStamina() const	{ return m_pointsInstant.iMaxStamina; }

		void			SetRandomHP(int v)	{ m_points.iRandomHP = v; }
		void			SetRandomSP(int v)	{ m_points.iRandomSP = v; }

		int				GetRandomHP() const	{ return m_points.iRandomHP; }
		int				GetRandomSP() const	{ return m_points.iRandomSP; }

		int				GetHPPct() const;

		void			SetRealPoint(BYTE idx, int val);
		int				GetRealPoint(BYTE idx) const;

		void			SetPoint(BYTE idx, long long val);

		long long		GetPoint(BYTE idx) const;
		int				GetLimitPoint(BYTE idx) const;
		int				GetPolymorphPoint(BYTE idx) const;

		const TMobTable &	GetMobTable() const;
		BYTE				GetMobRank() const;
		BYTE				GetMobBattleType() const;
		BYTE				GetMobSize() const;
		DWORD				GetMobDamageMin() const;
		DWORD				GetMobDamageMax() const;
		WORD				GetMobAttackRange() const;
		DWORD				GetMobDropItemVnum() const;
		float				GetMobDamageMultiply() const;

		// NEWAI
		bool			IsBerserker() const;
		bool			IsBerserk() const;
		void			SetBerserk(bool mode);

		bool			IsStoneSkinner() const;

		bool			IsGodSpeeder() const;
		bool			IsGodSpeed() const;
		void			SetGodSpeed(bool mode);

		bool			IsDeathBlower() const;
		bool			IsDeathBlow() const;

		bool			IsReviver() const;
		bool			HasReviverInParty() const;
		bool			IsRevive() const;
		void			SetRevive(bool mode);
		// NEWAI END

		bool			IsRaceFlag(DWORD dwBit) const;
		bool			IsSummonMonster() const;
		DWORD			GetSummonVnum() const;

		DWORD			GetPolymorphItemVnum() const;
		DWORD			GetMonsterDrainSPPoint() const;

		void			MainCharacterPacket();	// 내가 메인캐릭터라고 보내준다.

		void			ComputePoints();
		void			ComputeBattlePoints();
		void			PointChange(BYTE type, long long amount, bool bAmount = false, bool bBroadcast = false);
		void			PointsPacket();
		void			ApplyPoint(BYTE bApplyType, int iVal);
		void			CheckMaximumPoints();	// HP, SP 등의 현재 값이 최대값 보다 높은지 검사하고 높다면 낮춘다.
#ifdef __NEWPET_SYSTEM__
		void			SendPetLevelUpEffect(int vid, int type, int value, int amount);
#endif
		bool			Show(long lMapIndex, long x, long y, long z = LONG_MAX, bool bShowSpawnMotion = false);

		void			Sitdown(int is_ground);
		void			Standup();

		void			SetRotation(float fRot);
		void			SetRotationToXY(long x, long y);
		float			GetRotation() const	{ return m_pointsInstant.fRot; }

		void			MotionPacketEncode(BYTE motion, LPCHARACTER victim, struct packet_motion * packet);
		void			Motion(BYTE motion, LPCHARACTER victim = NULL);

		void			ChatPacket(BYTE type, const char *format, ...);
		void			MonsterChat(BYTE bMonsterChatType);
		void			SendGreetMessage();

		void			ResetPoint(int iLv);

		void			SetBlockMode(BYTE bFlag);
		void			SetBlockModeForce(BYTE bFlag);
		bool			IsBlockMode(BYTE bFlag) const	{ return (m_pointsInstant.bBlockMode & bFlag)?true:false; }

		bool			IsPolymorphed() const		{ return m_dwPolymorphRace>0; }
		bool			IsPolyMaintainStat() const	{ return m_bPolyMaintainStat; } // 이전 스텟을 유지하는 폴리모프.
		void			SetPolymorph(DWORD dwRaceNum, bool bMaintainStat = false);
		DWORD			GetPolymorphVnum() const	{ return m_dwPolymorphRace; }
		int				GetPolymorphPower() const;

		void			StartAnticheatCommunication();


		// FISING
#ifdef ENABLE_CSHIELD
		bool			IsNearWater() const;
#endif
		void			fishing();
		void			fishing_take();
		// END_OF_FISHING

		// MINING
		void			mining(LPCHARACTER chLoad);
		void			mining_cancel();
		void			mining_take();
		// END_OF_MINING

		void			ResetPlayTime(DWORD dwTimeRemain = 0);

		void			CreateFly(BYTE bType, LPCHARACTER pkVictim);

		void			ResetChatCounter();
		BYTE			IncreaseChatCounter();
		BYTE			GetChatCounter() const;

	protected:
		DWORD			m_dwPolymorphRace;
		bool			m_bPolyMaintainStat;
		DWORD			m_dwLoginPlayTime;
		DWORD			m_dwPlayerID;
		VID				m_vid;
		std::string		m_stName;
#ifdef __NEWPET_SYSTEM__
		BYTE			m_stImmortalSt;
#endif
		BYTE			m_bCharType;
#ifdef __NEWPET_SYSTEM__
		DWORD			m_newpetskillcd[3];
#endif
		CHARACTER_POINT		m_points;
		CHARACTER_POINT_INSTANT	m_pointsInstant;

		LPEVENT			m_pkAnticheatEvent;
		
		int				m_iMoveCount;
		DWORD			m_dwPlayStartTime;
		BYTE			m_bAddChrState;
		bool			m_bSkipSave;
		std::string		m_stMobile;
		char			m_szMobileAuth[5];
		BYTE			m_bChatCounter;
#ifdef UNLOCK_INVENTORY
		DWORD			m_dwUnlockSlots;
#endif
		// End of Basic Points

		//////////////////////////////////////////////////////////////////////////////////
		// Move & Synchronize Positions
		//////////////////////////////////////////////////////////////////////////////////
	public:
		bool			IsStateMove() const			{ return IsState((CState&)m_stateMove); }
		bool			IsStateIdle() const			{ return IsState((CState&)m_stateIdle); }
		bool			IsWalking() const			{ return m_bNowWalking || GetStamina()<=0; }
		void			SetWalking(bool bWalkFlag)	{ m_bWalking=bWalkFlag; }
		void			SetNowWalking(bool bWalkFlag);
		void			ResetWalking()			{ SetNowWalking(m_bWalking); }

		bool			Goto(long x, long y);	// 바로 이동 시키지 않고 목표 위치로 BLENDING 시킨다.
		void			Stop();

		bool			CanMove() const;		// 이동할 수 있는가?

		void			SyncPacket();
		bool			Sync(long x, long y);	// 실제 이 메소드로 이동 한다 (각 종 조건에 의한 이동 불가가 없음)
		bool			Move(long x, long y);	// 조건을 검사하고 Sync 메소드를 통해 이동 한다.
		void			OnMove(bool bIsAttack = false);	// 움직일때 불린다. Move() 메소드 이외에서도 불릴 수 있다.
		DWORD			GetMotionMode() const;
		float			GetMoveMotionSpeed() const;
		float			GetMoveSpeed() const;
		void			CalculateMoveDuration();
		void			SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime=0, int iRot=-1);
#ifdef ENABLE_SUPPORT_SYSTEM
		void			SendSupportSkillPacket(DWORD skill_vnum, DWORD skill_level);
		DWORD			SupportGetNextExp() const;
#endif
		DWORD			GetCurrentMoveDuration() const	{ return m_dwMoveDuration; }
		DWORD			GetWalkStartTime() const	{ return m_dwWalkStartTime; }
		DWORD			GetLastMoveTime() const		{ return m_dwLastMoveTime; }
		DWORD			GetLastAttackTime() const	{ return m_dwLastAttackTime; }
		DWORD 			GetLastSkillTime() const {return m_dwLastSkillTime;}

		void			SetLastAttacked(DWORD time);	// 마지막으로 공격받은 시간 및 위치를 저장함

		bool			SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList = true);
		bool			IsSyncOwner(LPCHARACTER ch) const;

		bool			WarpSet(long x, long y, long lRealMapIndex = 0);
		void			SetWarpLocation(long lMapIndex, long x, long y);
		void			WarpEnd();

		const PIXEL_POSITION & GetWarpPosition() const { return m_posWarp; }
		bool			WarpToPID(DWORD dwPID);

		void			SaveExitLocation();
		void			ExitToSavedLocation();

		void			StartStaminaConsume();
		void			StopStaminaConsume();
		bool			IsStaminaConsume() const;
		bool			IsStaminaHalfConsume() const;

		void			ResetStopTime();
		DWORD			GetStopTime() const;

	protected:
		void			ClearSync();

		float			m_fSyncTime;
		LPCHARACTER		m_pkChrSyncOwner;
		CHARACTER_LIST	m_kLst_pkChrSyncOwned;	// 내가 SyncOwner인 자들

		PIXEL_POSITION	m_posDest;
		PIXEL_POSITION	m_posStart;
		PIXEL_POSITION	m_posWarp;
		long			m_lWarpMapIndex;

		PIXEL_POSITION	m_posExit;
		long			m_lExitMapIndex;

		DWORD			m_dwMoveStartTime;
		DWORD			m_dwMoveDuration;

		DWORD			m_dwLastMoveTime;
		DWORD			m_dwLastAttackTime;
		DWORD			m_dwWalkStartTime;
		DWORD			m_dwStopTime;

		bool			m_bWalking;
		bool			m_bNowWalking;
		bool			m_bStaminaConsume;
		// End

		// Quickslot 관련
	public:
		void			SyncQuickslot(BYTE bType, BYTE bOldPos, BYTE bNewPos);
		bool			GetQuickslot(BYTE pos, TQuickslot ** ppSlot);
		bool			SetQuickslot(BYTE pos, TQuickslot & rSlot);
		bool			DelQuickslot(BYTE pos);
		bool			SwapQuickslot(BYTE a, BYTE b);
		void			ChainQuickslotItem(LPITEM pItem, BYTE bType, BYTE bOldPos);

	protected:
		TQuickslot		m_quickslot[QUICKSLOT_MAX_NUM];

		////////////////////////////////////////////////////////////////////////////////////////
		// Affect
	public:
		void			StartAffectEvent();
		void			ClearAffect(bool bSave=false);
		void			ComputeAffect(CAffect * pkAff, bool bAdd);
		bool			AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false);
		void			RefreshAffect();
		bool			RemoveAffect(DWORD dwType);
		bool			IsAffectFlag(DWORD dwAff) const;

		bool			UpdateAffect();	// called from EVENT
		int				ProcessAffect();

		void			LoadAffect(DWORD dwCount, TPacketAffectElement * pElements);
		void			SaveAffect();

		// Affect loading이 끝난 상태인가?
		bool			IsLoadedAffect() const	{ return m_bIsLoadedAffect; }

		bool			IsGoodAffect(BYTE bAffectType) const;

		void			RemoveGoodAffect();
		void			RemoveBadAffect();

		CAffect *		FindAffect(DWORD dwType, BYTE bApply=APPLY_NONE) const;
		const std::list<CAffect *> & GetAffectContainer() const	{ return m_list_pkAffect; }
		bool			RemoveAffect(CAffect * pkAff);

	protected:
		bool			m_bIsLoadedAffect;
		TAffectFlag		m_afAffectFlag;
		std::list<CAffect *>	m_list_pkAffect;

	public:
		// PARTY_JOIN_BUG_FIX
		void			SetParty(LPPARTY pkParty);
		LPPARTY			GetParty() const	{ return m_pkParty; }

		bool			RequestToParty(LPCHARACTER leader);
		void			DenyToParty(LPCHARACTER member);
		void			AcceptToParty(LPCHARACTER member);

		/// 자신의 파티에 다른 character 를 초대한다.
		/**
		 * @param	pchInvitee 초대할 대상 character. 파티에 참여 가능한 상태이어야 한다.
		 *
		 * 양측 character 의 상태가 파티에 초대하고 초대받을 수 있는 상태가 아니라면 초대하는 캐릭터에게 해당하는 채팅 메세지를 전송한다.
		 */
		void			PartyInvite(LPCHARACTER pchInvitee);

		/// 초대했던 character 의 수락을 처리한다.
		/**
		 * @param	pchInvitee 파티에 참여할 character. 파티에 참여가능한 상태이어야 한다.
		 *
		 * pchInvitee 가 파티에 가입할 수 있는 상황이 아니라면 해당하는 채팅 메세지를 전송한다.
		 */
		void			PartyInviteAccept(LPCHARACTER pchInvitee);

		/// 초대했던 character 의 초대 거부를 처리한다.
		/**
		 * @param [in]	dwPID 초대 했던 character 의 PID
		 */
		void			PartyInviteDeny(DWORD dwPID);

		bool			BuildUpdatePartyPacket(TPacketGCPartyUpdate & out);
		int				GetLeadershipSkillLevel() const;

		bool			CanSummon(int iLeaderShip);

		void			SetPartyRequestEvent(LPEVENT pkEvent) { m_pkPartyRequestEvent = pkEvent; }

	protected:

		/// 파티에 가입한다.
		/**
		 * @param	pkLeader 가입할 파티의 리더
		 */
		void			PartyJoin(LPCHARACTER pkLeader);

		/**
		 * 파티 가입을 할 수 없을 경우의 에러코드.
		 * Error code 는 시간에 의존적인가에 따라 변경가능한(mutable) type 과 정적(static) type 으로 나뉜다.
		 * Error code 의 값이 PERR_SEPARATOR 보다 낮으면 변경가능한 type 이고 높으면 정적 type 이다.
		 */
		enum PartyJoinErrCode {
			PERR_NONE		= 0,	///< 처리성공
			PERR_SERVER,			///< 서버문제로 파티관련 처리 불가
			PERR_DUNGEON,			///< 캐릭터가 던전에 있음
			PERR_OBSERVER,			///< 관전모드임
			PERR_LVBOUNDARY,		///< 상대 캐릭터와 레벨차이가 남
			PERR_LOWLEVEL,			///< 상대파티의 최고레벨보다 30레벨 낮음
			PERR_HILEVEL,			///< 상대파티의 최저레벨보다 30레벨 높음
			PERR_ALREADYJOIN,		///< 파티가입 대상 캐릭터가 이미 파티중
			PERR_PARTYISFULL,		///< 파티인원 제한 초과
			PERR_SEPARATOR,			///< Error type separator.
			PERR_DIFFEMPIRE,		///< 상대 캐릭터와 다른 제국임
			PERR_MAX				///< Error code 최고치. 이 앞에 Error code 를 추가한다.
		};

		/// 파티 가입이나 결성 가능한 조건을 검사한다.
		/**
		 * @param 	pchLeader 파티의 leader 이거나 초대한 character
		 * @param	pchGuest 초대받는 character
		 * @return	모든 PartyJoinErrCode 가 반환될 수 있다.
		 */
		static PartyJoinErrCode	IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		/// 파티 가입이나 결성 가능한 동적인 조건을 검사한다.
		/**
		 * @param 	pchLeader 파티의 leader 이거나 초대한 character
		 * @param	pchGuest 초대받는 character
		 * @return	mutable type 의 code 만 반환한다.
		 */
		static PartyJoinErrCode	IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		LPPARTY			m_pkParty;
		DWORD			m_dwLastDeadTime;
		LPEVENT			m_pkPartyRequestEvent;

		/**
		 * 파티초청 Event map.
		 * key: 초대받은 캐릭터의 PID
		 * value: event의 pointer
		 *
		 * 초대한 캐릭터들에 대한 event map.
		 */
		typedef std::map< DWORD, LPEVENT >	EventMap;
		EventMap		m_PartyInviteEventMap;

		// END_OF_PARTY_JOIN_BUG_FIX

		////////////////////////////////////////////////////////////////////////////////////////
		// Dungeon
	public:
		void			SetDungeon(LPDUNGEON pkDungeon);
		LPDUNGEON		GetDungeon() const	{ return m_pkDungeon; }
		LPDUNGEON		GetDungeonForce() const;
	protected:
		LPDUNGEON	m_pkDungeon;
		int			m_iEventAttr;

		////////////////////////////////////////////////////////////////////////////////////////
		// Guild
	public:
		void			SetGuild(CGuild * pGuild);
		CGuild*			GetGuild() const	{ return m_pGuild; }

		void			SetWarMap(CWarMap* pWarMap);
		CWarMap*		GetWarMap() const	{ return m_pWarMap; }

	protected:
		CGuild *		m_pGuild;
		DWORD			m_dwUnderGuildWarInfoMessageTime;
		CWarMap *		m_pWarMap;

		////////////////////////////////////////////////////////////////////////////////////////
		// Item related
	public:
		bool			CanHandleItem(bool bSkipRefineCheck = false, bool bSkipObserver = false); // 아이템 관련 행위를 할 수 있는가?

		bool			IsItemLoaded() const	{ return m_bItemLoaded; }
		void			SetItemLoaded()	{ m_bItemLoaded = true; }

		void			ClearItem();
		
		void			SetItem(TItemPos Cell, LPITEM item, bool bWereMine = false);

		LPITEM			GetItem(TItemPos Cell) const;
		LPITEM			GetInventoryItem(WORD wCell) const;
		bool			IsEmptyItemGrid(TItemPos Cell, BYTE size, int iExceptionCell = -1) const;
#ifdef ENABLE_SPECIAL_STORAGE
		LPITEM			GetUpgradeInventoryItem(WORD wCell) const;
		LPITEM			GetBookInventoryItem(WORD wCell) const;
		LPITEM			GetStoneInventoryItem(WORD wCell) const;
		LPITEM			GetChangeInventoryItem(WORD wCell) const;
		LPITEM			GetCostumeInventoryItem(WORD wCell) const;
#endif
		void			SetWear(BYTE bCell, LPITEM item);
		LPITEM			GetWear(BYTE bCell) const;


		bool			UseItemEx(LPITEM item, TItemPos DestCell);
		bool			UseItem(TItemPos Cell, TItemPos DestCell = NPOS);

#ifdef ENABLE_SHOW_CHEST_DROP
		void			SendChestData(DWORD chestIndex);
#endif

		// ADD_REFINE_BUILDING
		bool			IsRefineThroughGuild() const;
		CGuild *		GetRefineGuild() const;
		int				ComputeRefineFee(int iCost, int iMultiply = 5) const;
		void			PayRefineFee(int iTotalMoney);
		void			SetRefineNPC(LPCHARACTER ch);
		// END_OF_ADD_REFINE_BUILDING

		bool			RefineItem(LPITEM pkItem, LPITEM pkTarget);
		bool			DropItem(TItemPos Cell,  DWORD bCount=0);
		bool			GiveRecallItem(LPITEM item);
		void			ProcessRecallItem(LPITEM item);

		//	void			PotionPacket(int iPotionType);
		void			EffectPacket(int enumEffectType);
		void			SpecificEffectPacket(const char filename[128]);

#ifdef ENABLE_MULTI_REFINE_WORLDARD
		bool			DoRefine(LPITEM item, bool bMoneyOnly = false, BYTE index = -1);
		bool			DoRefineWithScroll(LPITEM item, BYTE index = -1);
		void 			SendMultiRefinePackets(BYTE bSubType, LPITEM item = NULL, BYTE bCell = -1, BYTE bType = -1, BYTE bSpecialStorage = INVENTORY);

#else
		bool			DoRefine(LPITEM item, bool bMoneyOnly = false);
		bool			DoRefineWithScroll(LPITEM item);
#endif

#ifdef ENABLE_SPECIAL_STORAGE

		bool			RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell = -1, BYTE bSpecialStorage = INVENTORY);
#else
		bool			RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell = -1);
#endif
		void			SetRefineMode(int iAdditionalCell = -1);
		void			ClearRefineMode();

		bool			GiveItem(LPCHARACTER victim, TItemPos Cell);
		bool			CanReceiveItem(LPCHARACTER from, LPITEM item) const;
		void			ReceiveItem(LPCHARACTER from, LPITEM item);
		bool			GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector <DWORD> &dwItemVnums,
							std::vector <DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count);
		
#ifdef ENABLE_SHOW_CHEST_DROP
		void			OpenChest(LPITEM item, WORD openCount);
		bool			GiveItemFromSpecialItemGroupNew(DWORD dwGroupNum, WORD& loopCount);
		bool			AutoGiveItemChest(LPITEM item, DWORD& itemCount, std::map<LPITEM, WORD>& vecUpdateItems);
#endif

		bool			MoveItem(TItemPos pos, TItemPos change_pos, DWORD num);
		bool			CanGetGroundItem(LPITEM item, LPCHARACTER partyOwner);
		bool			PickupItem(DWORD vid);
		bool			EquipItem(LPITEM item, int iCandidateCell = -1);
		bool			UnequipItem(LPITEM item);

		// 현재 item을 착용할 수 있는 지 확인하고, 불가능 하다면 캐릭터에게 이유를 알려주는 함수
		bool			CanEquipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

		// 착용중인 item을 벗을 수 있는 지 확인하고, 불가능 하다면 캐릭터에게 이유를 알려주는 함수
		bool			CanUnequipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

		bool			SwapItem(BYTE bWindow, BYTE bCell, BYTE bDestWindow, BYTE bDestCell);
		LPITEM			AutoGiveItem(DWORD dwItemVnum, DWORD bCount=1, int iRarePct = -1, bool bMsg = true);
		void			AutoGiveItem(LPITEM item, bool longOwnerShip = false);

		int				GetEmptyInventory(BYTE size) const;
		int				GetEmptyDragonSoulInventory(LPITEM pItem) const;
		void			CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const;
#ifdef ENABLE_SPECIAL_STORAGE
		int				GetEmptyUpgradeInventory(LPITEM pItem) const;
		int				GetEmptyBookInventory(LPITEM pItem) const;
		int				GetEmptyStoneInventory(LPITEM pItem) const;
		int				GetEmptyChangeInventory(LPITEM pItem) const;
		int				GetEmptyCostumeInventory(LPITEM pItem) const;
#endif
		int				CountEmptyInventory() const;
#ifdef ENABLE_BUY_ITEMS_WORLDARD
		int				CountSpecifyItemBuy(DWORD vnum) const;	
#endif
		int				CountSpecifyItem(DWORD vnum) const;
		void			RemoveSpecifyItem(DWORD vnum, DWORD count = 1);
		LPITEM			FindSpecifyItem(DWORD vnum) const;
		LPITEM			FindItemByID(DWORD id) const;

		int				CountSpecifyTypeItem(BYTE type) const;
		void			RemoveSpecifyTypeItem(BYTE type, DWORD count = 1);

		bool			IsEquipUniqueItem(DWORD dwItemVnum) const;

		// CHECK_UNIQUE_GROUP
		bool			IsEquipUniqueGroup(DWORD dwGroupVnum) const;
		// END_OF_CHECK_UNIQUE_GROUP

		void			SendEquipment(LPCHARACTER ch);
		// End of Item

	protected:

		bool			m_bItemLoaded;
		int				m_iRefineAdditionalCell;
		bool			m_bUnderRefine;
		DWORD			m_dwRefineNPCVID;

	public:
		////////////////////////////////////////////////////////////////////////////////////////
		// Money related
		/*
		INT				GetGold() const		{ return m_points.gold;	}
		void			SetGold(INT gold)	{ m_points.gold = gold;	}
		bool			DropGold(INT gold);
		INT				GetAllowedGold() const;
		void			GiveGold(INT iAmount);	// 파티가 있으면 파티 분배, 로그 등의 처리
		// End of Money*/

		long long		GetGold() const		{ return m_points.gold;	}
		void			SetGold(long long gold)	{ m_points.gold = gold;	}
		bool			DropGold(INT gold);
		long long		GetAllowedGold() const;
		void			GiveGold(long long iAmount);	// ??? ??? ?? ??, ?? ?? ??

#ifdef ENABLE_SORT_INVENTORY
        void            SetNextSortInventoryPulse(int pulse) { m_sortInventoryPulse = pulse; }
        int             GetSortInventoryPulse() { return m_sortInventoryPulse; }
       
        void            SetNextSortSpecialStoragePulse(int pulse) { m_sortSpecialStoragePulse = pulse; }
        int             GetSortSpecialStoragePulse() { return m_sortSpecialStoragePulse; }
#endif
#ifdef UNLOCK_INVENTORY
		DWORD 			GetUnlockSlots() const { return m_dwUnlockSlots;}
		void			SetUnlockSlots(DWORD slots) {m_dwUnlockSlots = slots;}
#endif
#ifdef ENABLE_MOUNT_SYSTEM
		////////////////////////////////
		// Mount system
	public:
		void SetMountingM(bool bValue) { bMounting = bValue; }
		bool GetMountVnumM() { return bMounting; }
		DWORD GetMountingVnumM() const { return bMountVnumM; }
		void SetMountVnumM(DWORD bVnum) { bMountVnumM = bVnum; }
		void SetSummonMounting(bool bValue) {bMountingS = bValue;}
		bool GetSummonMounting() {return bMountingS;}
		
	private:
		bool bMounting;
		bool bMountingS;
		DWORD bMountVnumM;
	
#endif
		////////////////////////////////////////////////////////////////////////////////////////
		// Shop related
	public:
		void			SetShop(LPSHOP pkShop);
		LPSHOP			GetShop() const { return m_pkShop; }

	protected:
		LPSHOP			m_pkShop;

		// End of shop
#ifdef ENABLE_SORT_INVENTORY
        int     m_sortInventoryPulse;
        int     m_sortSpecialStoragePulse;
#endif
		////////////////////////////////////////////////////////////////////////////////////////
		// Exchange related
	public:
		bool			ExchangeStart(LPCHARACTER victim);
		void			SetExchange(CExchange * pkExchange);
		CExchange *		GetExchange() const	{ return m_pkExchange;	}

	protected:
		CExchange *		m_pkExchange;
		// End of Exchange

		////////////////////////////////////////////////////////////////////////////////////////
		// Battle
	public:
		struct TBattleInfo
		{
			int iTotalDamage;
			int iAggro;

			TBattleInfo(int iTot, int iAggr)
				: iTotalDamage(iTot), iAggro(iAggr)
				{}
		};
		typedef std::map<VID, TBattleInfo>	TDamageMap;

		typedef struct SAttackLog
		{
			DWORD	dwVID;
			DWORD	dwTime;
		} AttackLog;

		bool				Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		void 				SetDamageMonsterHp(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		bool 				GetDamageMonsterHp(LPCHARACTER pAttacker);

		bool				__Profile__Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		void				DeathPenalty(BYTE bExpLossPercent);
		void				ReviveInvisible(int iDur);

		bool				Attack(LPCHARACTER pkVictim, BYTE bType = 0);
		bool				IsAlive() const		{ return m_pointsInstant.position == POS_DEAD ? false : true; }
		bool				CanFight() const;

		bool				CanBeginFight() const;
		void				BeginFight(LPCHARACTER pkVictim); // pkVictimr과 싸우기 시작한다. (강제적임, 시작할 수 있나 체크하려면 CanBeginFight을 사용)

		bool				CounterAttack(LPCHARACTER pkChr); // 반격하기 (몬스터만 사용)

		bool				IsStun() const;
		void				Stun();
		bool				IsDead() const;
		void				Dead(LPCHARACTER pkKiller = NULL, bool bImmediateDead=false);
#ifdef __NEWPET_SYSTEM__
		void				SetImmortal(int st) { m_stImmortalSt = st; };
		bool				IsImmortal() { return 1 == m_stImmortalSt; };
		void				SetNewPetSkillCD(int s, DWORD time) { m_newpetskillcd[s] = time; };
		DWORD				GetNewPetSkillCD(int s) { return m_newpetskillcd[s]; };
#endif
		void				Reward(bool bItemDrop);
#if defined(ENABLE_2LOOT)
		void				RewardGold(LPCHARACTER pkAttacker, DWORD dwMultiple);
#else
		void				RewardGold(LPCHARACTER pkAttacker);
#endif

		bool				Shoot(BYTE bType);
		void				FlyTarget(DWORD dwTargetVID, long x, long y, BYTE bHeader);

		void				ForgetMyAttacker(bool invisible = true);
		void				AggregateMonster();
		void				AttractRanger();
		void				PullMonster();

		int					GetArrowAndBow(LPITEM * ppkBow, LPITEM * ppkArrow, int iArrowCount = 1);
		void				UseArrow(LPITEM pkArrow, DWORD dwArrowCount);

		void				AttackedByPoison(LPCHARACTER pkAttacker);
		void				RemovePoison();
#ifdef ENABLE_WOLFMAN_CHARACTER
		void				AttackedByBleeding(LPCHARACTER pkAttacker);
		void				RemoveBleeding();
#endif
		void				AttackedByFire(LPCHARACTER pkAttacker, int amount, int count);
		void				RemoveFire();

		void				UpdateAlignment(int iAmount);
		int					GetAlignment() const;

		//선악치 얻기
		int					GetRealAlignment() const;
		void				ShowAlignment(bool bShow);

		void				SetKillerMode(bool bOn);
		bool				IsKillerMode() const;
		void				UpdateKillerMode();

		BYTE				GetPKMode() const;
		void				SetPKMode(BYTE bPKMode);

		void				ItemDropPenalty(LPCHARACTER pkKiller);

		void				UpdateAggrPoint(LPCHARACTER ch, EDamageType type, int dam);

		//
		// HACK
		//
	public:
		void SetComboSequence(BYTE seq);
		BYTE GetComboSequence() const;

		void SetLastComboTime(DWORD time);
		DWORD GetLastComboTime() const;

		int GetValidComboInterval() const;
		void SetValidComboInterval(int interval);

		BYTE GetComboIndex() const;

		void IncreaseComboHackCount(int k = 1);
		void ResetComboHackCount();
		void SkipComboAttackByTime(int interval);
		DWORD GetSkipComboAttackByTime() const;

	protected:
		BYTE m_bComboSequence;
		DWORD m_dwLastComboTime;
		int m_iValidComboInterval;
		BYTE m_bComboIndex;
		int m_iComboHackCount;
		DWORD m_dwSkipComboAttackByTime;

	protected:
		void				UpdateAggrPointEx(LPCHARACTER ch, EDamageType type, int dam, TBattleInfo & info);
		void				ChangeVictimByAggro(int iNewAggro, LPCHARACTER pNewVictim);

		DWORD				m_dwFlyTargetID;
		std::vector<DWORD>	m_vec_dwFlyTargets;
		TDamageMap			m_map_kDamage;	// 어떤 캐릭터가 나에게 얼마만큼의 데미지를 주었는가?
//		AttackLog			m_kAttackLog;
		DWORD				m_dwKillerPID;

		int					m_iAlignment;		// Lawful/Chaotic value -200000 ~ 200000
		int					m_iRealAlignment;
		int					m_iKillerModePulse;
		BYTE				m_bPKMode;

		// Aggro
		DWORD				m_dwLastVictimSetTime;
		int					m_iMaxAggro;
		// End of Battle

		// Stone
	public:
		void				SetStone(LPCHARACTER pkChrStone);
		void				ClearStone();
		void				DetermineDropMetinStone();
		DWORD				GetDropMetinStoneVnum() const { return m_dwDropMetinStone; }
		BYTE				GetDropMetinStonePct() const { return m_bDropMetinStonePct; }

	protected:
		LPCHARACTER			m_pkChrStone;		// 나를 스폰한 돌
		CHARACTER_SET		m_set_pkChrSpawnedBy;	// 내가 스폰한 놈들
		DWORD				m_dwDropMetinStone;
		BYTE				m_bDropMetinStonePct;
		// End of Stone

	public:
		enum
		{
			SKILL_UP_BY_POINT,
			SKILL_UP_BY_BOOK,
			SKILL_UP_BY_TRAIN,

			// ADD_GRANDMASTER_SKILL
			SKILL_UP_BY_QUEST,
			// END_OF_ADD_GRANDMASTER_SKILL
		};

		void				SkillLevelPacket();
		void				SkillLevelUp(DWORD dwVnum, BYTE bMethod = SKILL_UP_BY_POINT);
		bool				SkillLevelDown(DWORD dwVnum);
		// ADD_GRANDMASTER_SKILL
		bool				UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster = true);
		bool				IsSkillCooldown(DWORD dwVnum, float fSkillPower) { return m_SkillUseInfo[dwVnum].IsSkillCooldown(dwVnum, fSkillPower) ? true : false; }
		void				ResetSkill();
		void				SetSkillLevel(DWORD dwVnum, BYTE bLev);
		int					GetUsedSkillMasterType(DWORD dwVnum);

		bool				IsLearnableSkill(DWORD dwSkillVnum) const;
		// END_OF_ADD_GRANDMASTER_SKILL

		bool				CheckSkillHitCount(const BYTE SkillID, const VID dwTargetVID);
		bool				CanUseSkill(DWORD dwSkillVnum) const;
		bool				IsUsableSkillMotion(DWORD dwMotionIndex) const;
		int					GetSkillLevel(DWORD dwVnum) const;
		int					GetSkillMasterType(DWORD dwVnum) const;
		int					GetSkillPower(DWORD dwVnum, BYTE bLevel = 0) const;

		time_t				GetSkillNextReadTime(DWORD dwVnum) const;
		void				SetSkillNextReadTime(DWORD dwVnum, time_t time);
		void				SkillLearnWaitMoreTimeMessage(DWORD dwVnum);

		void				ComputePassiveSkill(DWORD dwVnum);
		int					ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
#ifdef ENABLE_WOLFMAN_CHARACTER
		int					ComputeSkillParty(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
#endif
		int					ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel = 0);
		void				ComputeSkillPoints();

		void				SetSkillGroup(BYTE bSkillGroup);
		BYTE				GetSkillGroup() const		{ return m_points.skill_group; }

		int					ComputeCooltime(int time);

		void				GiveRandomSkillBook();

		void				DisableCooltime();
		bool				LearnSkillByBook(DWORD dwSkillVnum, BYTE bProb = 0);
		bool				LearnGrandMasterSkill(DWORD dwSkillVnum);

	private:
		bool				m_bDisableCooltime;
		DWORD				m_dwLastSkillTime;	///< 마지막으로 skill 을 쓴 시간(millisecond).
		// End of Skill

		// MOB_SKILL
	public:
		bool				HasMobSkill() const;
		size_t				CountMobSkill() const;
		const TMobSkillInfo * GetMobSkill(unsigned int idx) const;
		bool				CanUseMobSkill(unsigned int idx) const;
		bool				UseMobSkill(unsigned int idx);
		void				ResetMobSkillCooltime();
	protected:
		DWORD				m_adwMobSkillCooltime[MOB_SKILL_MAX_NUM];
		// END_OF_MOB_SKILL

		// for SKILL_MUYEONG
	public:
		void				StartMuyeongEvent();
		void				StopMuyeongEvent();
#ifdef ENABLE_CONQUEROR_LEVEL
		void				StartCheonunEvent(long lApplyValue);
		void				StopCheonunEvent();
#endif

	private:
		LPEVENT				m_pkMuyeongEvent;
#ifdef ENABLE_CONQUEROR_LEVEL
		LPEVENT				m_pkCheonunEvent;
#endif
		// for SKILL_CHAIN lighting
	public:
		int					GetChainLightningIndex() const { return m_iChainLightingIndex; }
		void				IncChainLightningIndex() { ++m_iChainLightingIndex; }
		void				AddChainLightningExcept(LPCHARACTER ch) { m_setExceptChainLighting.insert(ch); }
		void				ResetChainLightningIndex() { m_iChainLightingIndex = 0; m_setExceptChainLighting.clear(); }
		int					GetChainLightningMaxCount() const;
		const CHARACTER_SET& GetChainLightingExcept() const { return m_setExceptChainLighting; }

	private:
		int					m_iChainLightingIndex;
		CHARACTER_SET m_setExceptChainLighting;

		// for SKILL_EUNHYUNG
	public:
		void				SetAffectedEunhyung();
		void				ClearAffectedEunhyung() { m_dwAffectedEunhyungLevel = 0; }
		bool				GetAffectedEunhyung() const { return m_dwAffectedEunhyungLevel; }

	private:
		DWORD				m_dwAffectedEunhyungLevel;

		//
		// Skill levels
		//
	protected:
		TPlayerSkill*					m_pSkillLevels;
		boost::unordered_map<BYTE, int>		m_SkillDamageBonus;
		std::map<int, TSkillUseInfo>	m_SkillUseInfo;

		////////////////////////////////////////////////////////////////////////////////////////
		// AI related
	public:
		void			AssignTriggers(const TMobTable * table);
		LPCHARACTER		GetVictim() const;	// 공격할 대상 리턴
		void			SetVictim(LPCHARACTER pkVictim);
		LPCHARACTER		GetNearestVictim(LPCHARACTER pkChr);
		LPCHARACTER		GetProtege() const;	// 보호해야 할 대상 리턴
		
#ifdef ENABLE_ITEMSHOP
		long long			GetDragonCoin();
		void				SetDragonCoin(long long amount);
#endif

		bool			Follow(LPCHARACTER pkChr, float fMinimumDistance = 150.0f);
		bool			Return();
		bool			IsGuardNPC() const;
		bool			IsChangeAttackPosition(LPCHARACTER target) const;
		void			ResetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time() - AI_CHANGE_ATTACK_POISITION_TIME_NEAR;}
		void			SetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time();}
#ifdef ENABLE_DEFENSAWE_SHIP
		bool			IsHydraMob() const;
		bool			IsHydraMobLP(LPCHARACTER ch) const;
		bool			IsHydraNPC() const;
		bool			IsHydra() const;
#endif

		bool			OnIdle();

		void			OnAttack(LPCHARACTER pkChrAttacker);
		void			OnClick(LPCHARACTER pkChrCauser);

		VID				m_kVIDVictim;

	protected:
		DWORD			m_dwLastChangeAttackPositionTime;
		CTrigger		m_triggerOnClick;
		// End of AI

		////////////////////////////////////////////////////////////////////////////////////////
		// Target
	protected:
		LPCHARACTER				m_pkChrTarget;		// 내 타겟
		CHARACTER_SET	m_set_pkChrTargetedBy;	// 나를 타겟으로 가지고 있는 사람들

	public:
		void				SetTarget(LPCHARACTER pkChrTarget);
		void				BroadcastTargetPacket();
		void				ClearTarget();
		void				CheckTarget();
		LPCHARACTER			GetTarget() const { return m_pkChrTarget; }

		////////////////////////////////////////////////////////////////////////////////////////
		// Safebox
	public:
		int					GetSafeboxSize() const;
		void				QuerySafeboxSize();
		void				SetSafeboxSize(int size);

		CSafebox *			GetSafebox() const;
		void				LoadSafebox(int iSize, DWORD dwGold, int iItemCount, TPlayerItem * pItems);
		void				ChangeSafeboxSize(BYTE bSize);
		void				CloseSafebox();

		/// 창고 열기 요청
		/**
		 * @param [in]	pszPassword 1자 이상 6자 이하의 창고 비밀번호
		 *
		 * DB 에 창고열기를 요청한다.
		 * 창고는 중복으로 열지 못하며, 최근 창고를 닫은 시간으로 부터 10초 이내에는 열 지 못한다.
		 */
		void				ReqSafeboxLoad(const char* pszPassword);

		/// 창고 열기 요청의 취소
		/**
		 * ReqSafeboxLoad 를 호출하고 CloseSafebox 하지 않았을 때 이 함수를 호출하면 창고를 열 수 있다.
		 * 창고열기의 요청이 DB 서버에서 실패응답을 받았을 경우 이 함수를 사용해서 요청을 할 수 있게 해준다.
		 */
		void				CancelSafeboxLoad( void ) { m_bOpeningSafebox = false; }

		void				SetMallLoadTime(int t) { m_iMallLoadTime = t; }
		int					GetMallLoadTime() const { return m_iMallLoadTime; }

		CSafebox *			GetMall() const;
		void				LoadMall(int iItemCount, TPlayerItem * pItems);
		void				CloseMall();

		void				SetSafeboxOpenPosition();
		float				GetDistanceFromSafeboxOpen() const;

	protected:
		CSafebox *			m_pkSafebox;
		int					m_iSafeboxSize;
		int					m_iSafeboxLoadTime;
		bool				m_bOpeningSafebox;	///< 창고가 열기 요청 중이거나 열려있는가 여부, true 일 경우 열기요청이거나 열려있음.

		CSafebox *			m_pkMall;
		int					m_iMallLoadTime;

		PIXEL_POSITION		m_posSafeboxOpen;

		////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////////////
		// Mounting
	public:
		void				MountVnum(DWORD vnum);
		DWORD				GetMountVnum() const { return m_dwMountVnum; }
		DWORD				GetLastMountTime() const { return m_dwMountTime; }

		bool				CanUseHorseSkill();

		// Horse
		virtual	void		SetHorseLevel(int iLevel);

		virtual	bool		StartRiding();
		virtual	bool		StopRiding();

		virtual	DWORD		GetMyHorseVnum() const;

		virtual	void		HorseDie();
		virtual bool		ReviveHorse();

		virtual void		SendHorseInfo();
		virtual	void		ClearHorseInfo();

		void				HorseSummon(bool bSummon, bool bFromFar = false, DWORD dwVnum = 0, const char* name = 0);

		LPCHARACTER			GetHorse() const			{ return m_chHorse; }	 // 현재 소환중인 말
		LPCHARACTER			GetRider() const; // rider on horse
		void				SetRider(LPCHARACTER ch);

		bool				IsRiding() const;
#ifdef ENABLE_SUPPORT_SYSTEM
	public:
		CSupportSystem*			GetSupportSystem()				{ return m_supportSystem; }

	protected:
		CSupportSystem*			m_supportSystem;

	public:
#endif 
#ifdef __PET_SYSTEM__
	public:
		CPetSystem*			GetPetSystem()				{ return m_petSystem; }

#ifdef ENABLE_NEW_PET_SYSTEM
		bool IsPet() const { return m_bCharType == CHAR_TYPE_PET; }
		void SetNewPet() { m_bIsNewPet = true; }
		bool IsNewPet() const { return m_bIsNewPet; }
		void StatePet();
		bool PetBlockMap();
#endif

	protected:
		CPetSystem*			m_petSystem;
#ifdef ENABLE_NEW_PET_SYSTEM
		bool m_bIsNewPet;
#endif
	public:
#endif
#ifdef __NEWPET_SYSTEM__
	public:
		CNewPetSystem*			GetNewPetSystem() { return m_newpetSystem; }

	protected:
		CNewPetSystem*			m_newpetSystem;

	public:
#endif
#ifdef ENABLE_MOUNT_SYSTEM
	public:
		CMountSystem*			GetMountSystem() { return m_MountSystem; }

	protected:
		CMountSystem*			m_MountSystem;

	public:
#endif
	protected:
		LPCHARACTER			m_chHorse;
		LPCHARACTER			m_chRider;

		DWORD				m_dwMountVnum;
		DWORD				m_dwMountTime;

		BYTE				m_bSendHorseLevel;
		BYTE				m_bSendHorseHealthGrade;
		BYTE				m_bSendHorseStaminaGrade;

		////////////////////////////////////////////////////////////////////////////////////////
		// Detailed Log
	public:
		void				DetailLog() { m_bDetailLog = !m_bDetailLog; }
		void				ToggleMonsterLog();
		void				MonsterLog(const char* format, ...);
	private:
		bool				m_bDetailLog;
		bool				m_bMonsterLog;

		////////////////////////////////////////////////////////////////////////////////////////
		// Empire

	public:
		void 				SetEmpire(BYTE bEmpire);
		BYTE				GetEmpire() const { return m_bEmpire; }

	protected:
		BYTE				m_bEmpire;

		////////////////////////////////////////////////////////////////////////////////////////
		// Regen
	public:
		void				SetRegen(LPREGEN pkRegen);

	protected:
		PIXEL_POSITION			m_posRegen;
		float				m_fRegenAngle;
		LPREGEN				m_pkRegen;
		size_t				regen_id_; // to help dungeon regen identification
		// End of Regen

		////////////////////////////////////////////////////////////////////////////////////////
		// Resists & Proofs
	public:
		bool				CannotMoveByAffect() const;	// 특정 효과에 의해 움직일 수 없는 상태인가?
		bool				IsImmune(DWORD dwImmuneFlag);
		void				SetImmuneFlag(DWORD dw) { m_pointsInstant.dwImmuneFlag = dw; }

	protected:
		void				ApplyMobAttribute(const TMobTable* table);
		// End of Resists & Proofs

		////////////////////////////////////////////////////////////////////////////////////////
		// QUEST
		//
	public:
		void				SetQuestNPCID(DWORD vid);
		DWORD				GetQuestNPCID() const { return m_dwQuestNPCVID; }
		LPCHARACTER			GetQuestNPC() const;

		void				SetQuestItemPtr(LPITEM item);
		void				ClearQuestItemPtr();
		LPITEM				GetQuestItemPtr() const;
#ifdef ENABLE_BIOLOG_SYSTEM
		int					GetBiologState(const std::string& state) const;
		void				SetBiologState(const std::string& state, int szValue);
#endif

		void				SetQuestBy(DWORD dwQuestVnum)	{ m_dwQuestByVnum = dwQuestVnum; }
		DWORD				GetQuestBy() const			{ return m_dwQuestByVnum; }

		int					GetQuestFlag(const std::string& flag) const;
		void				SetQuestFlag(const std::string& flag, int value);

		void				ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID);

	private:
		DWORD				m_dwQuestNPCVID;
		DWORD				m_dwQuestByVnum;
		LPITEM				m_pQuestItem;

		// Events
	public:
		bool				StartStateMachine(int iPulse = 1);
		void				StopStateMachine();
		void				UpdateStateMachine(DWORD dwPulse);
		void				SetNextStatePulse(int iPulseNext);

		// 캐릭터 인스턴스 업데이트 함수. 기존엔 이상한 상속구조로 CFSM::Update 함수를 호출하거나 UpdateStateMachine 함수를 사용했는데, 별개의 업데이트 함수 추가함.
		void				UpdateCharacter(DWORD dwPulse);

	protected:
		DWORD				m_dwNextStatePulse;

		// Marriage
	public:
		LPCHARACTER			GetMarryPartner() const;
		void				SetMarryPartner(LPCHARACTER ch);
		int					GetMarriageBonus(DWORD dwItemVnum, bool bSum = true);

		void				SetWeddingMap(marriage::WeddingMap* pMap);
		marriage::WeddingMap* GetWeddingMap() const { return m_pWeddingMap; }

	private:
		marriage::WeddingMap* m_pWeddingMap;
		LPCHARACTER			m_pkChrMarried;

		// Warp Character
	public:
		void				StartWarpNPCEvent();
#ifdef CHANGE_CHANNEL
		void				ChannelSwitch(int new_ch);
#endif

	public:
		void				StartSaveEvent();
		void				StartRecoveryEvent();
		void				StartCheckSpeedHackEvent();
		void				StartDestroyWhenIdleEvent();

		LPEVENT				m_pkDeadEvent;
		LPEVENT				m_pkStunEvent;
		LPEVENT				m_pkSaveEvent;
		LPEVENT				m_pkRecoveryEvent;
		LPEVENT				m_pkTimedEvent;
		LPEVENT				m_pkFishingEvent;
		LPEVENT				m_pkAffectEvent;
		LPEVENT				m_pkPoisonEvent;
#ifdef ENABLE_WOLFMAN_CHARACTER
		LPEVENT				m_pkBleedingEvent;
#endif
		LPEVENT				m_pkFireEvent;
		LPEVENT				m_pkWarpNPCEvent;
		//DELAYED_WARP
		//END_DELAYED_WARP

		// MINING
		LPEVENT				m_pkMiningEvent;
		// END_OF_MINING
		LPEVENT				m_pkWarpEvent;
		LPEVENT				m_pkCheckSpeedHackEvent;
		LPEVENT				m_pkDestroyWhenIdleEvent;
		LPEVENT				m_pkPetSystemUpdateEvent;
#ifdef __NEWPET_SYSTEM__
		LPEVENT				m_pkNewPetSystemUpdateEvent;
		LPEVENT				m_pkNewPetSystemExpireEvent;
#endif

#ifdef ENABLE_MOUNT_SYSTEM
		LPEVENT				m_pkMountSystemUpdateEvent;
#endif
		bool IsWarping() const { return m_pkWarpEvent ? true : false; }

		bool				m_bHasPoisoned;
#ifdef ENABLE_WOLFMAN_CHARACTER
		bool				m_bHasBled;
#endif

		const CMob *		m_pkMobData;
		CMobInstance *		m_pkMobInst;

		std::map<int, LPEVENT> m_mapMobSkillEvent;

		friend struct FuncSplashDamage;
		friend struct FuncSplashAffect;
		friend class CFuncShoot;

	public:
		int				GetPremiumRemainSeconds(BYTE bType) const;

	private:
		int				m_aiPremiumTimes[PREMIUM_MAX_NUM];

		// CHANGE_ITEM_ATTRIBUTES
		// static const DWORD		msc_dwDefaultChangeItemAttrCycle;	///< 디폴트 아이템 속성변경 가능 주기
		static const char		msc_szLastChangeItemAttrFlag[];		///< 최근 아이템 속성을 변경한 시간의 Quest Flag 이름
		// static const char		msc_szChangeItemAttrCycleFlag[];		///< 아이템 속성병경 가능 주기의 Quest Flag 이름
		// END_OF_CHANGE_ITEM_ATTRIBUTES

		// PC_BANG_ITEM_ADD
	private :
		bool m_isinPCBang;

	public :
		bool SetPCBang(bool flag) { m_isinPCBang = flag; return m_isinPCBang; }
		bool IsPCBang() const { return m_isinPCBang; }
		// END_PC_BANG_ITEM_ADD

		// NEW_HAIR_STYLE_ADD
	public :
		bool ItemProcess_Hair(LPITEM item, int iDestCell);
		// END_NEW_HAIR_STYLE_ADD

	public :
		void ClearSkill();
		void ClearSubSkill();

		// RESET_ONE_SKILL
		bool ResetOneSkill(DWORD dwVnum);
		// END_RESET_ONE_SKILL

	private :
		void SendDamagePacket(LPCHARACTER pAttacker, int Damage, BYTE DamageFlag);

	// ARENA
	private :
		CArena *m_pArena;
		bool m_ArenaObserver;
		int m_nPotionLimit;

	public :
		void 	SetArena(CArena* pArena) { m_pArena = pArena; }
		void	SetArenaObserverMode(bool flag) { m_ArenaObserver = flag; }

		CArena* GetArena() const { return m_pArena; }
		bool	GetArenaObserverMode() const { return m_ArenaObserver; }

		void	SetPotionLimit(int count) { m_nPotionLimit = count; }
		int		GetPotionLimit() const { return m_nPotionLimit; }
	// END_ARENA

		//PREVENT_TRADE_WINDOW
	public:
		bool	IsOpenSafebox() const { return m_isOpenSafebox ? true : false; }
		void 	SetOpenSafebox(bool b) { m_isOpenSafebox = b; }

		int		GetSafeboxLoadTime() const { return m_iSafeboxLoadTime; }
		void	SetSafeboxLoadTime() { m_iSafeboxLoadTime = thecore_pulse(); }
		//END_PREVENT_TRADE_WINDOW
	private:
		bool	m_isOpenSafebox;

	public:
		int		GetSkillPowerByLevel(int level, bool bMob = false) const;

		//PREVENT_REFINE_HACK
		int		GetRefineTime() const { return m_iRefineTime; }
		void	SetRefineTime() { m_iRefineTime = thecore_pulse(); }
		int		m_iRefineTime;
		//END_PREVENT_REFINE_HACK

		//RESTRICT_USE_SEED_OR_MOONBOTTLE
		int 	GetUseSeedOrMoonBottleTime() const { return m_iSeedTime; }
		void  	SetUseSeedOrMoonBottleTime() { m_iSeedTime = thecore_pulse(); }
		int 	m_iSeedTime;
		//END_RESTRICT_USE_SEED_OR_MOONBOTTLE

		//PREVENT_PORTAL_AFTER_EXCHANGE
		int		GetExchangeTime() const { return m_iExchangeTime; }
		void	SetExchangeTime() { m_iExchangeTime = thecore_pulse(); }
		int		m_iExchangeTime;
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		int 	m_iMyShopTime;
		int		GetMyShopTime() const	{ return m_iMyShopTime; }
		void	SetMyShopTime() { m_iMyShopTime = thecore_pulse(); }

		// Hack 방지를 위한 체크.
		bool	IsHack(bool bSendMsg = true, bool bCheckShopOwner = true, int limittime = g_nPortalLimitTime);

		// MONARCH
		BOOL	IsMonarch() const;
		// END_MONARCH
		void Say(const std::string & s);

		enum MONARCH_COOLTIME
		{
			MC_HEAL = 10,
			MC_WARP	= 60,
			MC_TRANSFER = 60,
			MC_TAX = (60 * 60 * 24 * 7),
			MC_SUMMON = (60 * 60),
		};

		enum MONARCH_INDEX
		{
			MI_HEAL = 0,
			MI_WARP,
			MI_TRANSFER,
			MI_TAX,
			MI_SUMMON,
			MI_MAX
		};

		DWORD m_dwMonarchCooltime[MI_MAX];
		DWORD m_dwMonarchCooltimelimit[MI_MAX];

		void  InitMC();
		DWORD GetMC(enum MONARCH_INDEX e) const;
		void SetMC(enum MONARCH_INDEX e);
		bool IsMCOK(enum MONARCH_INDEX e) const;
		DWORD GetMCL(enum MONARCH_INDEX e) const;
		DWORD GetMCLTime(enum MONARCH_INDEX e) const;

	public:
		bool ItemProcess_Polymorph(LPITEM item);

		// by mhh
		LPITEM*	GetCubeItem() { return m_pointsInstant.pCubeItems; }
		bool IsCubeOpen () const	{ return (m_pointsInstant.pCubeNpc?true:false); }
		void SetCubeNpc(LPCHARACTER npc)	{ m_pointsInstant.pCubeNpc = npc; }
		bool CanDoCube() const;

	public:
		bool IsSiegeNPC() const;

	private:
		//중국 전용
		//18세 미만 전용
		//3시간 : 50 % 5 시간 0%
		e_overtime m_eOverTime;

	public:
		bool IsOverTime(e_overtime e) const { return (e == m_eOverTime); }
		void SetOverTime(e_overtime e) { m_eOverTime = e; }

	private:
		int		m_deposit_pulse;

	public:
		void	UpdateDepositPulse();
		bool	CanDeposit() const;

	public:
		struct AttackedLog
		{
			DWORD 	dwPID;
			DWORD	dwAttackedTime;

			AttackedLog() : dwPID(0), dwAttackedTime(0)
			{
			}
		};

		AttackLog	m_kAttackLog;
		AttackedLog m_AttackedLog;
		int			m_speed_hack_count;

	private :
		std::string m_strNewName;

	public :
		const std::string GetNewName() const { return this->m_strNewName; }
		void SetNewName(const std::string name) { this->m_strNewName = name; }

	public :
		void GoHome();

	private :
		std::set<DWORD>	m_known_guild;

	public :
		void SendGuildName(CGuild* pGuild);
		void SendGuildName(DWORD dwGuildID);

	private :
		DWORD m_dwLogOffInterval;

	public :
		DWORD GetLogOffInterval() const { return m_dwLogOffInterval; }

	public:
		bool UnEquipSpecialRideUniqueItem ();

		bool CanWarp () const;

	private:
		DWORD m_dwLastGoldDropTime;
#ifdef ENABLE_NEWSTUFF
		DWORD m_dwLastItemDropTime;
		DWORD m_dwLastBoxUseTime;
		DWORD m_dwLastBuySellTime;
	public:
		DWORD GetLastBuySellTime() const { return m_dwLastBuySellTime; }
		void SetLastBuySellTime(DWORD dwLastBuySellTime) { m_dwLastBuySellTime = dwLastBuySellTime; }
#endif
	public:
		void StartHackShieldCheckCycle(int seconds);
		void StopHackShieldCheckCycle();

		bool GetHackShieldCheckMode() const { return m_HackShieldCheckMode; }
		void SetHackShieldCheckMode(bool m) { m_HackShieldCheckMode = m; }

		LPEVENT m_HackShieldCheckEvent;

	private:
		bool	m_HackShieldCheckMode;

	public:
		void AutoRecoveryItemProcess (const EAffectTypes);

	public:
		void BuffOnAttr_AddBuffsFromItem(LPITEM pItem);
		void BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem);

	private:
		void BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue);
		void BuffOnAttr_ClearAll();

		typedef std::map <BYTE, CBuffOnAttributes*> TMapBuffOnAttrs;
		TMapBuffOnAttrs m_map_buff_on_attrs;
		// 무적 : 원활한 테스트를 위하여.
	public:
		void SetArmada() { cannot_dead = true; }
		void ResetArmada() { cannot_dead = false; }
	private:
		bool cannot_dead;
#ifdef ENABLE_SUPPORT_SYSTEM
	private:
		bool m_bIsSupport;
		LPCHARACTER m_Owner;
		DWORD m_dwSupportCostume;
		DWORD m_dwSupportHair;
		DWORD m_dwSupportWeapon;
		DWORD m_dwSupportInt;
	public:
		void SetSupport() { m_bIsSupport = true; }
		bool IsSupport() { return m_bIsSupport; }
		void SetSupportArmor(int vnum) { m_dwSupportCostume = vnum; }
		void SetSupportHair(int vnum) { m_dwSupportHair = vnum; }
		void SetSupportWeapon(int vnum) { m_dwSupportWeapon = vnum; }
		DWORD GetSupportArmor() { return m_dwSupportCostume; }
		DWORD GetSupportHair() { return m_dwSupportHair; }
		DWORD GetSupportWeapon() { return m_dwSupportWeapon; }
		void SetOwner(LPCHARACTER owner) { m_Owner = owner; }
		LPCHARACTER GetOwner() { return m_Owner; }
		void SetSupportInt(int count) { m_dwSupportInt = count; }
		DWORD GetSupportInt() { return m_dwSupportInt; }
#endif
#if defined(__BL_67_ATTR__)
	public:
		void Open67Attr();
		void Set67Attr(bool b) { b67Attr = b; }
		bool Is67AttrOpen() const { return b67Attr; }
	private:
		bool b67Attr;
#endif
#ifdef __PET_SYSTEM__
	private:
		bool m_bIsPet;
	public:
		void SetPet() { m_bIsPet = true; }
#ifdef ENABLE_NEW_PET_SYSTEM
		bool IsPet() { return m_bCharType == CHAR_TYPE_PET; }
#else
		bool IsPet() { return m_bIsPet; }
#endif
#endif
#ifdef __NEWPET_SYSTEM__
	private:
		// Nuovo PetSystem
		bool m_bIsNewPet;
		int m_eggvid;
	public:
		// Nuovo PetSystem
		void SetNewPet() { m_bIsNewPet = true; }
		bool IsNewPet() const { return m_bIsNewPet ? true : false; }
		void SetEggVid(int vid) { m_eggvid = vid; }
		int GetEggVid() { return m_eggvid; }

#endif
#ifdef ENABLE_MOUNT_SYSTEM
	private:
		bool m_bIsMountSystem;
	public:
		void SetMount() { m_bIsMountSystem = true; }
		bool IsMountSystem() const { return m_bIsMountSystem;}
#endif
#ifdef NEW_ICEDAMAGE_SYSTEM
	private:
		DWORD m_dwNDRFlag;
		std::set<DWORD> m_setNDAFlag;
	public:
		const DWORD GetNoDamageRaceFlag();
		void SetNoDamageRaceFlag(DWORD dwRaceFlag);
		void UnsetNoDamageRaceFlag(DWORD dwRaceFlag);
		void ResetNoDamageRaceFlag();
		const std::set<DWORD> & GetNoDamageAffectFlag();
		void SetNoDamageAffectFlag(DWORD dwAffectFlag);
		void UnsetNoDamageAffectFlag(DWORD dwAffectFlag);
		void ResetNoDamageAffectFlag();
#endif

	//최종 데미지 보정.
	private:
		float m_fAttMul;
		float m_fDamMul;
	public:
		float GetAttMul() { return this->m_fAttMul; }
		void SetAttMul(float newAttMul) {this->m_fAttMul = newAttMul; }
		float GetDamMul() { return this->m_fDamMul; }
		void SetDamMul(float newDamMul) {this->m_fDamMul = newDamMul; }

	private:
		bool IsValidItemPosition(TItemPos Pos) const;

	public:
		//용혼석

		// 캐릭터의 affect, quest가 load 되기 전에 DragonSoul_Initialize를 호출하면 안된다.
		// affect가 가장 마지막에 로드되어 LoadAffect에서 호출함.
		void	DragonSoul_Initialize();

		bool	DragonSoul_IsQualified() const;
		void	DragonSoul_GiveQualification();

		int		DragonSoul_GetActiveDeck() const;
		bool	DragonSoul_IsDeckActivated() const;
		bool	DragonSoul_ActivateDeck(int deck_idx);

		void	DragonSoul_DeactivateAll();
		// 반드시 ClearItem 전에 불러야 한다.
		// 왜냐하면....
		// 용혼석 하나 하나를 deactivate할 때마다 덱에 active인 용혼석이 있는지 확인하고,
		// active인 용혼석이 하나도 없다면, 캐릭터의 용혼석 affect와, 활성 상태를 제거한다.
		//
		// 하지만 ClearItem 시, 캐릭터가 착용하고 있는 모든 아이템을 unequip하는 바람에,
		// 용혼석 Affect가 제거되고, 결국 로그인 시, 용혼석이 활성화되지 않는다.
		// (Unequip할 때에는 로그아웃 상태인지, 아닌지 알 수 없다.)
		// 용혼석만 deactivate시키고 캐릭터의 용혼석 덱 활성 상태는 건드리지 않는다.
		void	DragonSoul_CleanUp();
		// 용혼석 강화창
		
#ifdef ENABLE_NEW_DETAILS_GUI
		void	SendKillLog();
#endif
	public:
		bool		DragonSoul_RefineWindow_Open(LPENTITY pEntity);
		bool		DragonSoul_RefineWindow_Close();
		LPENTITY	DragonSoul_RefineWindow_GetOpener() { return  m_pointsInstant.m_pDragonSoulRefineWindowOpener; }
		bool		DragonSoul_RefineWindow_CanRefine();
#if defined(WJ_COMBAT_ZONE)
	private:
		DWORD m_iCombatZonePoints;
		DWORD m_iCombatZoneDeaths;

	public:
		LPEVENT m_pkCombatZoneLeaveEvent;
		LPEVENT m_pkCombatZoneWarpEvent;

		BYTE	GetCombatZoneRank();
	
		DWORD	GetRealCombatZonePoints();
		void	SetRealCombatZonePoints(DWORD dwValue);
		
		void	UpdateCombatZoneRankings(const char* memberName, DWORD memberEmpire, DWORD memberPoints);
	
		DWORD	GetCombatZonePoints() { return m_iCombatZonePoints; }
		void	SetCombatZonePoints(DWORD dwValue) { m_iCombatZonePoints = dwValue; }
		DWORD	GetCombatZoneDeaths() { return m_iCombatZoneDeaths; }
		void	SetCombatZoneDeaths(DWORD dwValue) { m_iCombatZoneDeaths = dwValue; }
#endif
		//독일 선물 기능 패킷 임시 저장
	private:
		unsigned int itemAward_vnum;
		char		 itemAward_cmd[20];
		//bool		 itemAward_flag;
#ifdef ENABLE_CONQUEROR_LEVEL
	public:
		bool			IsConquerorMap(int iMapIndex);
#endif
	public:
		unsigned int GetItemAward_vnum() { return itemAward_vnum; }
		char*		 GetItemAward_cmd() { return itemAward_cmd;	  }
		//bool		 GetItemAward_flag() { return itemAward_flag; }
		void		 SetItemAward_vnum(unsigned int vnum) { itemAward_vnum = vnum; }
		void		 SetItemAward_cmd(char* cmd) { strcpy(itemAward_cmd,cmd); }
		//void		 SetItemAward_flag(bool flag) { itemAward_flag = flag; }
#ifdef ENABLE_ANTI_CMD_FLOOD
	private:
		int m_dwCmdAntiFloodPulse;
		DWORD m_dwCmdAntiFloodCount;
	public:
		int GetCmdAntiFloodPulse(){return m_dwCmdAntiFloodPulse;}
		DWORD GetCmdAntiFloodCount(){return m_dwCmdAntiFloodCount;}
		DWORD IncreaseCmdAntiFloodCount(){return ++m_dwCmdAntiFloodCount;}
		void SetCmdAntiFloodPulse(int dwPulse){m_dwCmdAntiFloodPulse=dwPulse;}
		void SetCmdAntiFloodCount(DWORD dwCount){m_dwCmdAntiFloodCount=dwCount;}
#endif
	private:
		// SyncPosition을 악용하여 타유저를 이상한 곳으로 보내는 핵 방어하기 위하여,
		// SyncPosition이 일어날 때를 기록.
		timeval		m_tvLastSyncTime;
		int			m_iSyncHackCount;
	public:
		void			SetLastSyncTime(const timeval &tv) { memcpy(&m_tvLastSyncTime, &tv, sizeof(timeval)); }
		const timeval&	GetLastSyncTime() { return m_tvLastSyncTime; }
		void			SetSyncHackCount(int iCount) { m_iSyncHackCount = iCount;}
		int				GetSyncHackCount() { return m_iSyncHackCount; }

#ifdef ENABLE_AUTOMATIC_PICK_UP_SYSTEM
	public:
		DWORD			GetPickUPMode() const { return dwPickUPMode; }
		void			SetPickUPMode(DWORD dwMode) { dwPickUPMode = dwMode; }
		DWORD			GetRarityMode() const { return dwRarityMode; }
		void			SetRarityMode(DWORD dwMode) { dwRarityMode = dwMode; }

		void			ChangePickUPMode(DWORD dwMode, DWORD dwRMode);
		bool			CheckItemCanGet(const LPITEM item);

	private:
		DWORD dwPickUPMode;
		DWORD dwRarityMode;
#endif

#ifdef ENABLE_MULTILANGUAGE_SYSTEM
	private:
		BYTE m_language;
	public:
		BYTE GetCharacterLanguage(){return m_language;}
		void SetCharacterLanguage(BYTE newLanguage){m_language = newLanguage;}
#endif

#ifdef __SASH_SYSTEM__
	protected:
		bool	m_bSashCombination, m_bSashAbsorption;
	
	public:
		bool	isSashOpened(bool bCombination) {return bCombination ? m_bSashCombination : m_bSashAbsorption;}
		void	OpenSash(bool bCombination);
		void	CloseSash();
		void	ClearSashMaterials();
		bool	CleanSashAttr(LPITEM pkItem, LPITEM pkTarget);
		LPITEM*	GetSashMaterials() {return m_pointsInstant.pSashMaterials;}
		bool	SashIsSameGrade(long lGrade);
		DWORD	GetSashCombinePrice(long lGrade);
		void	GetSashCombineResult(DWORD & dwItemVnum, DWORD & dwMinAbs, DWORD & dwMaxAbs);
		BYTE	CheckEmptyMaterialSlot();
		void	AddSashMaterial(TItemPos tPos, BYTE bPos);
		void	RemoveSashMaterial(BYTE bPos);
		BYTE	CanRefineSashMaterials();
		void	RefineSashMaterials();
#endif

#ifdef ENABLE_CALENDAR_SYSTEM
	public:

		bool	IsOpenCalendar() const { return m_isOpenCalendar ? true : false; }
		void 	SetOpenCalendarSystem(bool b) { m_isOpenCalendar = b; }
		
		void 	OpenCalendarSystem();
		bool 	GetEventsInfo(int day);
		void 	SendEvents(int day, int index_event, const char * hora_inicio, const char * hora_final);
		void 	LoadEvents();

		void 	OpenCalendarADM();
		void 	LoadEventsADM();

		void 	SendCalendarPackets(BYTE subheader, BYTE bDay = 0,  const char* name = NULL, const char* image = NULL, const char* szStartAt = NULL, const char* szEndAt = NULL, DWORD dwDuration = 0);
		//void 	SendCalendarPackets(BYTE subheader, int day=0 , char event_escape[100]="", char event_descrip[250] ="", char banner[100] ="", char hora_inicio[6]="", char hora_final[6]="", char hora_inicio_2[6]="", char hora_final_2[6]="");
		void 	SendCalendarPacketsADM(BYTE subheader, int index_event=0 , const char* event_escape = NULL, const char* event_descrip = NULL);
	private:
		bool	m_isOpenCalendar;
		DWORD   m_dwLastOpenCalendar;

#endif

#ifdef ENABLE_HIDE_COSTUME_SYSTEM
	public:
		void 	FuncHideCostume(int slot);
		void 	FuncHideCostumeLoad();
		int 	GetHideCostume(int wear) const ;
		void 	SetHideCostumeUpdate();
		int		GetSlotsCostume();
		int 	GetSlotDefault(int slot);
		DWORD 	SetHideCostumeUpdateShining(DWORD id);
#endif

#ifdef ENABLE_VOTE_COINS_WHILE_PLAYING
	private:
		LPEVENT			m_pkVoteCoinsWhilePlaying;
	public:
		void			StartVoteCoinsEvent();
		void			SetVoteCoinsEventLastTime();
#endif

#ifdef ELEMENT_SPELL_WORLDARD
	public:
		bool 	ElementsSpellItem(LPITEM pkItem, LPITEM pkTarget);
		bool	IsOpenElementsSpell() const { return m_OpenElementsSpell ? true : false; }
		void 	SetOpenElementsSpell(bool b, int iAdditionalCell = -1);
		void 	ElementsSpellItemFunc(int pos, BYTE type_select = -1);
		BYTE 	GetElementsEffect();

	private:
		bool m_OpenElementsSpell;
		int	 m_iElementsAdditionalCell;
#endif

#ifdef ENABLE_DUNGEON_INFO
	public:
		void 	SetDamageDoneDungeonInfo(int dam){damage_done_dungeon_info = dam;}
		int 	GetDamageDoneDungeonInfo(){return damage_done_dungeon_info;}


		void 	SetDamageReceivedDungeonInfo(int dam){damage_received_dungeon_info = dam;}
		int 	GetDamageReceivedDungeonInfo(){return damage_received_dungeon_info;}

	private:
		int damage_done_dungeon_info;
		int damage_received_dungeon_info;
#endif

#ifdef UNLOCK_INVENTORY_WORLDARD
	public:
		DWORD 			GetUnlockSlotsW(DWORD type = 0) const { return inventory_unlock[type];}
		void			SetUnlockSlotsW(DWORD slots,DWORD type) {inventory_unlock[type] = slots;}
	private:
		DWORD 			inventory_unlock[UNLOCK_INVENTORY_MAX];
#endif


	public:
		void 			StartNoticeMapCombatZone();
	private:
		LPEVENT			m_pkNoticeCombatZoneEvent;

#ifdef RENEWAL_DEAD_PACKET
		DWORD			CalculateDeadTime(BYTE type);
#endif


#ifdef BATTLEPASS_WORLDARD
	public:
		BYTE 			GetBattlePassId();
		int 			GetSecondsTillNextMonth();
		int 			GetBattlePassEndTime() { return (m_dwBattlePassEndTime - get_global_time()); };
	protected:
		DWORD			m_dwBattlePassEndTime;
#endif

#ifdef CHANGE_EQUIP_WORLDARD
	public:
		bool 			IsValidItemChangeEquip(BYTE cell, LPITEM item);
		LPITEM			GetChangeEquipItem(WORD wCell) const;
#endif

#ifdef ENABLE_OFFLINESHOP_SYSTEM
	public:
		void 				SetOfflineShop(LPOFFLINESHOP pkOfflineShop){m_pkOfflineShop=pkOfflineShop;}
		LPOFFLINESHOP		GetOfflineShop() { return m_pkOfflineShop; }

		bool				IsOfflineShopNPC() { return (IsNPC() && GetOfflineShop()); }

		bool				GetOfflineShopPanel() {return isOfflineShopPanelOpen;}
		void				SetOfflineShopPanel(bool flag) {isOfflineShopPanelOpen= flag;}

		unsigned long long	GetOfflineShopFlag() { return m_pkOfflineShopFlag; }
		void				SetOfflineShopFlag(unsigned long long ptr) { m_pkOfflineShopFlag = ptr; }

		void				SetProtectTime(const std::string& flagname, int value);
		int					GetProtectTime(const std::string& flagname) const;

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		bool				IsLookingSearchItem(DWORD itemID);
		void				SetLookingSearch(const std::vector<const OFFLINE_SHOP_ITEM*>& dataVector);
#endif

		bool				CheckWindows(bool bChat);
		bool				CanAddItemShop();
		bool				CanDestroyShop();
		bool				CanOpenOfflineShop();
		bool				CanOpenShopPanel();
		bool				CanRemoveItemShop();
		bool				CanCreateShop();
		bool				CanRemoveLogShop();
		bool				CanWithdrawMoney();
		bool				CanChangeTitle();
		bool				CanChangeDecoration();
		bool				CanBuyItemOfflineShop();
		bool				CanGetBackItems();
		bool				CanAddTimeShop();

#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		bool				CanSearch();
#endif

	protected:
		LPOFFLINESHOP		m_pkOfflineShop;
		bool				isOfflineShopPanelOpen;
		unsigned long long			m_pkOfflineShopFlag;
		std::map<std::string, int>  m_protection_Time;
#ifdef ENABLE_SHOP_SEARCH_SYSTEM
		std::vector<DWORD>	m_vecSearchLooking;
#endif
#endif


#ifdef ENABLE_BIYOLOG
	public:
		void			CheckBio();
#endif

#ifdef ENABLE_CHANNEL_CHANGE
		void			MoveChannel(DWORD channelIndex);
#endif

#ifdef ENABLE_BATTLE_PASS
public:
	int		GetBattlePassStatus();
	bool	IsBattlePassActive();

	void	ClearBattlePassData();
	void	SetBattlePassMissonValue(BYTE missionIndex, long long missionValue);
	long long	GetBattlePassMissonValue(BYTE missionIndex);
protected:
	std::map<BYTE, long long> m_battlePassData;
	DWORD m_dwMountStartTime;
#endif

#ifdef ENABLE_RARITY
public:
	void				SetRarityValue(long decreaseValue, bool setWeapon, bool setArmor);
#endif
#ifdef ENABLE_ANTI_EXP
	bool			GetAntiExp() { return m_pointsInstant.anti_exp; }
	void			SetAntiExp(bool flag) { m_pointsInstant.anti_exp = flag; }
#endif
#ifdef ENABLE_MULTI_FARM_BLOCK
public:
	bool				GetRewardStatus() { return m_bmultiFarmStatus; }
	void				SetRewardStatus(bool bValue) { m_bmultiFarmStatus = bValue; }
protected:
	bool				m_bmultiFarmStatus;
#endif

#ifdef ENABLE_CHECK_PICKUP_HACK
	private:
		DWORD m_dwLastPickupTime;
	public:
		void SetLastPickupTime() { m_dwLastPickupTime = get_dword_time(); }
		DWORD GetLastPickupTime() { return m_dwLastPickupTime; }
#endif
#ifdef ENABLE_CHECK_GHOSTMODE
	private:
		DWORD m_dwCountGhostmodePoint;
	public:
		DWORD GetGhostmodeCount() { return m_dwCountGhostmodePoint; }
		void AddGhostmodeCount() { m_dwCountGhostmodePoint += 1; }
		void ResetGhostmodeCount() { m_dwCountGhostmodePoint = 0; }
#endif
#ifdef ENABLE_CHECK_WALLHACK
	private:
		DWORD m_dwCountWallhackPoint;
	public:
		DWORD GetWallhackCount() { return m_dwCountWallhackPoint; }
		void AddWallhackCount() { m_dwCountWallhackPoint += 1; }
		void ResetWallhackCount() { m_dwCountWallhackPoint = 0; }
#endif
#ifdef ENABLE_CSHIELD
	private:
		LPEVENT m_pkCShieldEvent;
		LPEVENT m_pkCShieldDataEvent;
		spCShield m_cshield;
	public:
		void StartCShieldEvent();
		void StartCShieldDataEvent();
		void SendCShieldPacket(bool start);
		spCShield GetCShield() const { return m_cshield; }
#endif


#ifdef ENABLE_ANTI_PACKET_FLOOD
private:
	int m_dwPacketAntiFloodPulse;
	uint32_t m_dwPacketAntiFloodCount;
public:
    int GetPacketAntiFloodPulse() noexcept { return m_dwPacketAntiFloodPulse; }
    DWORD GetPacketAntiFloodCount() noexcept { return m_dwPacketAntiFloodCount; }
    DWORD IncreasePacketAntiFloodCount() noexcept { return ++m_dwPacketAntiFloodCount; }
    void SetPacketAntiFloodPulse(int dwPulse) noexcept { m_dwPacketAntiFloodPulse = dwPulse; }
    void SetPacketAntiFloodCount(DWORD dwCount) noexcept { m_dwPacketAntiFloodCount = dwCount; }
#endif
#ifdef ENABLE_FLOOD_PRETECTION
public:
	int analyze_protect = 0;
	int analyze_protect_count = 0;
#endif

#ifdef ENABLE_PREVENT_FLOOD_PACKETS
public:
    int GetLastExchangeTime() const noexcept { return m_dwLastExchangeTime; }
    void SetLastExchangeTime() noexcept { m_dwLastExchangeTime = thecore_pulse(); }

    int GetLastBuyTime() const noexcept { return m_dwLastBuyTime; }
    void SetLastBuyTime() noexcept { m_dwLastBuyTime = thecore_pulse(); }

    int GetLastSafeboxCheckOutTime() const noexcept { return m_dwLastSafeBoxCheckOutTime; }
    void SetLastSafeboxCheckOutTime() noexcept { m_dwLastSafeBoxCheckOutTime = thecore_pulse(); }

    int GetLastItemSplitTime() const noexcept { return m_dwLastItemSplitTime; }
    void SetLastItemSplitTime() noexcept { m_dwLastItemSplitTime = thecore_pulse(); }

    int GetLastOfferExpTime() const noexcept { return m_dwLastOfferExpime; }
    void SetLastOfferExpTime() noexcept { m_dwLastOfferExpime = thecore_pulse(); }

    uint32_t GetLastBraveCapePulse() const noexcept { return dwLastBraveryCapePulse; }
    void SetLastBraveCapePulse(uint32_t time) noexcept { dwLastBraveryCapePulse = time; }

#ifdef ENABLE_GUILD_DONATE_ATTENDANCE
    int GetLastOfferNewExpTime() const noexcept { return m_dwLastOfferNewExpTime; }
    void SetLastOfferNewExpTime() noexcept { m_dwLastOfferNewExpTime = thecore_pulse(); }
#endif

#ifdef ENABLE_SEARCH_ITEM_DROP_ON_MOB
    int GetLastDropItemSearchTime() const noexcept { return m_dwLastDropItemSearchTime; }
    void SetLastDropItemSearchTime() noexcept { m_dwLastDropItemSearchTime = thecore_pulse(); }
#endif

private:
    int m_dwLastExchangeTime;
    int m_dwLastBuyTime;
    int m_dwLastSafeBoxCheckOutTime;
    int m_dwLastItemSplitTime;
    int m_dwLastOfferExpime;
    uint32_t dwLastBraveryCapePulse;
#ifdef ENABLE_GUILD_DONATE_ATTENDANCE
    int m_dwLastOfferNewExpTime;
#endif
#ifdef ENABLE_SEARCH_ITEM_DROP_ON_MOB
    int m_dwLastDropItemSearchTime;
#endif
#endif
public:
	void StopAttacksOnLogout(); 

#ifdef ENABLE_ZODIAC_MISSION
public:
	void				BeadCount(LPCHARACTER ch);
#endif


#ifdef ENABLE_MELEY_LAIR_DUNGEON
public:
	bool			IsMeley();
	void			SetQuestNPCIDAttack(DWORD vid){ m_dwQuestNPCVIDAttack = vid;}
	DWORD			GetQuestNPCIDAttack() const { return m_dwQuestNPCVIDAttack; }
	LPCHARACTER		GetQuestNPCAttack() const;
private:
	DWORD			m_dwQuestNPCVIDAttack;
#endif

#ifdef ENABLE_TRACK_WINDOW
public:
	void	GetDungeonCooldown(WORD bossIndex);
	void	GetDungeonCooldownTest(WORD bossIndex, int value, bool isCooldown);
#endif

#ifdef ENABLE_EXCHANGE_LOG
public:
	void	LoadExchangeLog();
	bool	LoadExchangeLogItem(DWORD logID);
	void	DeleteExchangeLog(DWORD logID);
	void	SendExchangeLogPacket(BYTE subHeader, DWORD id = 0, const TExchangeLog* exchangeLog = NULL);

protected:
	std::map<DWORD, std::pair<TExchangeLog, std::vector<TExchangeLogItem>>> m_mapExchangeLog;
#endif

#ifdef __GEM_SYSTEM__
public:
	int				GetGem() const { return m_points.gem; }
	void			SetGem(int gem) { m_points.gem = gem; }

	void			LoadGemItems();
	void			SaveGemItems();
	void			OpenGemSlot();
	void			BuyGemItem(BYTE slotIndex);
	void			RefreshGemItems(bool withItem = false);
	void			RefreshGemPlayer();
	void			RefreshGemALL(bool withRealTime = false);

protected:
	std::map<BYTE, TGemItem> m_mapGemItems;
#endif

# ifdef ENABLE_MINI_GAME_CATCH_KING
public:
	void MiniGameCatchKingSetFieldCards(std::vector<TCatchKingCard> vec) { m_vecCatchKingFieldCards = vec; }

	uint32_t MiniGameCatchKingGetScore() const noexcept { return dwCatchKingTotalScore; }
	void MiniGameCatchKingSetScore(uint32_t dwScore) noexcept { dwCatchKingTotalScore = dwScore; }

	uint32_t MiniGameCatchKingGetBetNumber() const noexcept { return bCatchKingBetSetNumber; }
	void MiniGameCatchKingSetBetNumber(uint8_t bSetNr) noexcept { bCatchKingBetSetNumber = bSetNr; }

	uint8_t MiniGameCatchKingGetHandCard() const noexcept { return bCatchKingHandCard; }
	void MiniGameCatchKingSetHandCard(uint8_t bKingCard) noexcept { bCatchKingHandCard = bKingCard; }

	uint8_t MiniGameCatchKingGetHandCardLeft() const noexcept { return bCatchKingHandCardLeft; }
	void MiniGameCatchKingSetHandCardLeft(uint8_t bHandCard) noexcept { bCatchKingHandCardLeft = bHandCard; }

	bool MiniGameCatchKingGetGameStatus() const noexcept { return dwCatchKingGameStatus; }
	void MiniGameCatchKingSetGameStatus(bool bStatus) noexcept { dwCatchKingGameStatus = bStatus; }

	std::vector<TCatchKingCard> m_vecCatchKingFieldCards;

protected:
	uint8_t bCatchKingHandCard;
	uint8_t bCatchKingHandCardLeft;
	bool dwCatchKingGameStatus;
	uint8_t bCatchKingBetSetNumber;
	uint32_t dwCatchKingTotalScore;
# endif // ENABLE_MINI_GAME_CATCH_KING

#ifdef ENABLE_VOTE4BUFF
public:
	long long	GetVoteCoin();
	void		SetVoteCoin(long long amount);
#endif
	public:
		struct S_CARD
		{
			DWORD	type;
			DWORD	value;
		};

		struct CARDS_INFO
		{
			S_CARD cards_in_hand[MAX_CARDS_IN_HAND];
			S_CARD cards_in_field[MAX_CARDS_IN_FIELD];
			DWORD	cards_left;
			DWORD	field_points;
			DWORD	points;
		};
		
		void			Cards_open(DWORD safemode);
		void			Cards_clean_list();
		DWORD			GetEmptySpaceInHand();
		void			Cards_pullout();
		void			RandomizeCards();
		bool			CardWasRandomized(DWORD type, DWORD value);
		void			SendUpdatedInformations();
		void			SendReward();
		void			CardsDestroy(DWORD reject_index);
		void			CardsAccept(DWORD accept_index);
		void			CardsRestore(DWORD restore_index);
		DWORD			GetEmptySpaceInField();
		DWORD			GetAllCardsCount();
		bool			TypesAreSame();
		bool			ValuesAreSame();
		bool			CardsMatch();
		DWORD			GetLowestCard();
		bool			CheckReward();
		void			CheckCards();
		void			RestoreField();
		void			ResetField();
		void			CardsEnd();
		void			GetGlobalRank(char * buffer, size_t buflen);
		void			GetRundRank(char * buffer, size_t buflen);
	protected:
		CARDS_INFO	character_cards;
		S_CARD	randomized_cards[24];
#ifdef RENEWAL_MISSION_BOOKS
public:
	void		SetMissionBook(BYTE missionType, BYTE value, DWORD arg, WORD level);
	void		RewardMissionBook(WORD missionID);
	void		DeleteBookMission(WORD missionID);
	void		LoadMissionData();
	void		SendMissionData();
	void		SaveMissionData();
	void		GiveNewMission(WORD missionID);
	bool		IsMissionHas(WORD missionID);
	BYTE		MissionCount();
	void		ModifySetMissionCMD(WORD missionID, std::string& cmdText);
protected:
	std::map<WORD, TMissionBook> m_mapMissionData;
#endif

};

ESex GET_SEX(LPCHARACTER ch);

#endif
