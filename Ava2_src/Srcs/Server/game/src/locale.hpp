#ifndef __INC_METIN2_GAME_LOCALE_H__
#define __INC_METIN2_GAME_LOCALE_H__

#include "../../common/CommonDefines.h"

extern "C"
{
	void locale_init(const char *filename);

	const char *locale_find(const char *string);
	#define LC_TEXT(str) locale_find(str)

	extern int g_iUseLocale;

};

#endif
