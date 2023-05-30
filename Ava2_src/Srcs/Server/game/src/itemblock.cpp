#include "stdafx.h"
#include "fstream"
#include "string"
#include "sstream"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string.hpp"

typedef std::vector<std::pair<long, DWORD> > BlockVectorType;
BlockVectorType ItemBlockList;

typedef std::vector<DWORD> BlockVectorType2;
BlockVectorType2 ItemBlockList2;

void LoadBlockItemList()
{
	{
		std::string strMapIndex;
		std::string strItemVnums;
		
		std::ifstream File("item_blocks.txt");
		
		if(!File.is_open())
		{
			sys_log(0,"WARNING: can not open item_blocks.txt");
			return;
		}
		
		ItemBlockList.clear();
		
		while(!File.eof())
		{
			File >> strMapIndex >> strItemVnums;
			
			std::vector<std::string> tokens;
			boost::split(tokens, strItemVnums, boost::is_any_of(","));
			
			for(size_t i = 0; i < tokens.size(); ++i)
			{
				long lMapIndex = 0;
				str_to_number(lMapIndex, strMapIndex.c_str());
				
				DWORD dwItemVnum = 0;
				str_to_number(dwItemVnum,tokens[i].c_str());
				
				ItemBlockList.push_back(std::make_pair(lMapIndex,dwItemVnum));
				sys_log(0,"ItemBlock MapIndex[%ld] - Blocked Item : %u",lMapIndex,dwItemVnum);
			}
		}
		
		File.close();
	}
	
	{
		std::string strItemVnums;
		
		std::ifstream File("item_blocks_remove.txt");
		
		if(!File.is_open())
		{
			sys_log(0,"WARNING: can not open item_blocks_remove.txt");
			return;
		}
		
		ItemBlockList2.clear();
		
		while(!File.eof())
		{
			File >> strItemVnums;
			
			std::vector<std::string> tokens;
			boost::split(tokens, strItemVnums, boost::is_any_of(","));
			
			for(size_t i = 0; i < tokens.size(); ++i)
			{
				DWORD dwItemVnum = 0;
				str_to_number(dwItemVnum,tokens[i].c_str());
				
				ItemBlockList2.push_back(dwItemVnum);
				sys_log(0,"ItemBlock2 Blocked Item : %u",dwItemVnum);
			}
		}
		
		File.close();
	}
}

bool CanUseItem(long lMapIndex,DWORD dwItemVnum)
{
	for(BlockVectorType::iterator it = ItemBlockList.begin(); it != ItemBlockList.end(); ++it)
	{
		if(it->first == lMapIndex && it->second == dwItemVnum)
			return false;
	}
	
	return true;
}

bool CanDestroyItem(DWORD dwItemVnum)
{
	for(auto itemVnum : ItemBlockList2)
	{
		if(itemVnum == dwItemVnum)
			return true;
	}

	return false;
}
