#ifndef __INC_METIN_II_MINI_GAME_H__
#define __INC_METIN_II_MINI_GAME_H__

#ifdef ENABLE_MINI_GAME_CATCH_KING
#include <unordered_map>

#include "../../common/stl.h"
#include "../../common/length.h"
#include "../../common/tables.h"
#include "packet.h"
#include "questmanager.h"

class CMiniGameManager : public singleton<CMiniGameManager>
{
public:
	CMiniGameManager();
	virtual ~CMiniGameManager();

	bool Initialize();
	void Destroy();

	void SpawnEventNPC(uint32_t dwVnum);

public:
	int MiniGameCatchKing(LPCHARACTER ch, const char* data, size_t uiBytes);

	void MiniGameCatchKingEventInfo(LPCHARACTER pkChar);
	void InitializeMiniGameCatchKing(int iEnable);
	void InitializeMiniGameCatchKingEndTime(int iEndTime) noexcept { iCatchKingEndTime = iEndTime; }
	void MiniGameCatchKingCheckEnd();

	void MiniGameCatchKingStartGame(LPCHARACTER pkChar, uint8_t bSetCount);
	void MiniGameCatchKingDeckCardClick(LPCHARACTER pkChar);
	void MiniGameCatchKingFieldCardClick(LPCHARACTER pkChar, uint8_t bFieldPos);
	void MiniGameCatchKingGetReward(LPCHARACTER pkChar);

	void MiniGameCatchKingRegisterScore(LPCHARACTER pkChar, uint32_t dwScore);
	int MiniGameCatchKingGetScore(lua_State* L, bool isTotal);
	int MiniGameCatchKingGetMyScore(LPCHARACTER pkChar, bool isTotal = false);

protected:
	int iCatchKingEndTime;
};

#endif
#endif
