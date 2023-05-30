#ifndef __INC_METIN_II_GAME_SHOP_MANAGER_H__
#define __INC_METIN_II_GAME_SHOP_MANAGER_H__

class CShop;
typedef class CShop * LPSHOP;

class CShopManager : public singleton<CShopManager>
{
public:
	typedef std::map<DWORD, CShop *> TShopMap;

public:
	CShopManager();
	virtual ~CShopManager();

	bool	Initialize(TShopTable * table, int size);
	void	Destroy();

	LPSHOP	Get(DWORD dwVnum);
	LPSHOP	GetByNPCVnum(DWORD dwVnum);

	bool	StartShopping(LPCHARACTER pkChr, LPCHARACTER pkShopKeeper, int iShopVnum = 0);

	bool	StartShoppingNew(LPCHARACTER pkChr, int iShopVnum = 0);

	void	StopShopping(LPCHARACTER ch);

	void	Buy(LPCHARACTER ch, BYTE pos);

#ifdef ENABLE_SPECIAL_STORAGE
	void	Sell(LPCHARACTER ch, BYTE bInv, BYTE bCell, DWORD bCount = 0);
#else
	void	Sell(LPCHARACTER ch, BYTE bCell, DWORD bCount = 0);
#endif

private:
	TShopMap	m_map_pkShop;
	TShopMap	m_map_pkShopByNPCVnum;
	bool	ReadShopTableEx(const char* stFileName);
};

#endif
