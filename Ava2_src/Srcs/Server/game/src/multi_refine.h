#ifndef __INC_MULTI_REFINE_H
#define __INC_MULTI_REFINE_H
#include "constants.h"

class CMultiRefineManager : public singleton<CMultiRefineManager>
{
	public:
	CMultiRefineManager();
	virtual ~CMultiRefineManager();

	bool	Initialize(TMultiRefineTable * table, int size);
	const TMultiRefineTable* GetRefineRecipeNew(int vnum);

	int 	GetMultiRefineSize(int vnum);
	int 	GetMultiRefineVnum(int vnum);

	int 	GetMultiRefineSet(int vnum);

	void 	SetInformationRefineMulti(LPCHARACTER ch , BYTE bSubType);

	std::map<int, TMultiRefineTable> GetMultiRefineRecipe(){return m_map_MultiRefineRecipe; }

	private:
		std::map<int, TMultiRefineTable>   m_map_MultiRefineRecipe;
};
#endif