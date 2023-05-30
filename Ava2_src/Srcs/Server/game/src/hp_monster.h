#ifndef __HP_MONSTER_SYSTEM__
#define __HP_MONSTER_SYSTEM__

#include <vector>

extern void LoadInfoHpMonster();
extern bool CheckMonsterHp(DWORD vnum_mob);
extern bool GetMapIndexMonster(DWORD vnum_mob, int map_index);
extern int GetValueIndexMonster(DWORD vnum_mob);
#endif
