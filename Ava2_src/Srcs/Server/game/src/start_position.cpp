#include "stdafx.h"
#include "start_position.h"


char g_nation_name[4][32] =
{
	"",
	"Shinsoo",
	"Chunjo",
	"Jinno",
};

//	"934"
//	"935"
//	"936"

long g_start_map[4] =
{
	0,	// reserved
	90,	// 신수국
	21,	// 천조국
	91	// 진노국
};

DWORD g_start_position[4][2] =
{
	{      0,      0 },	// reserved
	{ 469300, 964200 },	// 신수국
	{  55700, 157900 },	// 천조국
	{ 969600, 278400 }	// 진노국
};


DWORD arena_return_position[4][2] =
{
	{       0,  0       },
	{   347600, 882700  }, // 자양현
	{   138600, 236600  }, // 복정현
	{   857200, 251800  }  // 박라현
};


DWORD g_create_position[4][2] =
{
	{      0,      0 },	// reserved
	{ 469300, 964200 },	// 신수국
	{  55700, 157900 },	// 천조국
	{ 969600, 278400 }	// 진노국
};

DWORD g_create_position_canada[4][2] =
{
	{      0,      0 },	// reserved
	{ 469300, 964200 },	// 신수국
	{  55700, 157900 },	// 천조국
	{ 969600, 278400 }	// 진노국
};

