#ifndef __ITEM_BLOCK__H
#define __ITEM_BLOCK__H

#include <vector>

extern void LoadBlockItemList();
extern bool CanUseItem(long lMapIndex,DWORD dwItemVnum);
extern bool CanDestroyItem(DWORD dwItemVNum);

#endif
