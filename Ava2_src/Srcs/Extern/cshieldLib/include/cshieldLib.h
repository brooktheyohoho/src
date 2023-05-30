#ifndef cshieldLib_h__
#define cshieldLib_h__

#include <memory>
#include <cstring>

typedef unsigned int DWORD;

class CShield
{
public:
	CShield();

// Movespeed
public:
	bool CheckMoveSpeedhack(long x, long y, DWORD time, bool isRiding, float moveSpeed);
	void ResetMoveSpeedhack(DWORD time) { m_mshIgnoreUntil = time + 1 * 1000; m_mshStartDetect = 0; }
private:
	DWORD m_mshNextReduction;
	bool isRidingLocal;
	int m_mshHackCount;
	long m_mshStartX;
	long m_mshStartY;
	DWORD m_mshStartDetect;
	DWORD m_mshLastDetect;
	DWORD m_mshIgnoreUntil;
	DWORD m_mshFirstDetect;
	
// Waithack
public:
	bool CheckWaithack(long x, long y, DWORD time, float moveSpeed);
private:
	DWORD m_dwCountWaithackPoint;
	long m_mshStartX2;
	long m_mshStartY2;
	DWORD m_mshStartDetect2;
	DWORD startDetect;
	int distWalked;
	int mobCount;
	
// Attackspeed Hack
public:
	DWORD CheckAttackspeedHack(bool isRiding, DWORD ani_speed, long long att_speed, DWORD victimVID, DWORD time);
private:
	DWORD m_dwCountAttackSpeedhackPoint;
	DWORD dwAttackVID;
	DWORD dwAttackTime;

// AttackspeedBow Hack
public:
	DWORD CheckAttackspeedBowHack(DWORD ani_speed, long long att_speed, DWORD time);
private:
	DWORD m_dwCountAttackSpeedhackBowPoint;
	DWORD dwAttackBowTime;

// Verification
public:
	DWORD VerifyCShield(const char * playerName);

	bool GetCShieldInit() { return cShieldInit; }
	void SetCShieldIinit() { cShieldInit = true; }
	char* GetKeyLocalStored() { return keyLocalStored; }
	void SetKeyLocalStored(const char* key) { strncpy(keyLocalStored, key, sizeof(keyLocalStored)); }
	char* GetKeyLocalStored2() { return keyLocalStored2; }
	void SetKeyLocalStored2(const char* key) { strncpy(keyLocalStored2, key, sizeof(keyLocalStored2)); }
	void SetCShieldNumber(DWORD num) { cShieldNumber = num; }
	DWORD GetCShieldNumber() { return cShieldNumber; }
	time_t GetCShieldTime() { return cShieldTime; }
	void SetCShieldTime() { cShieldTime = time(0); }
	void ResetCShieldTriesCount() { cShieldTriesCount = 0; }
	void IncreaseCShieldTriesCount() { cShieldTriesCount += 1; }
	DWORD GetCShieldTriesCount() { return cShieldTriesCount; }
	char* GetKeyStored() { return keyStored; }
	void SetKeyStored(const char* key) { strncpy(keyStored, key, sizeof(keyStored)); }
private:
	time_t cShieldTime;
	DWORD cShieldTriesCount;
	DWORD cShieldNumber;
	bool cShieldInit;
	char keyStored[128] = {0};
	char keyLocalStored[128] = {0};
	char keyLocalStored2[128] = {0};
};

typedef std::shared_ptr<CShield> spCShield;

extern void InitCShield();

#endif