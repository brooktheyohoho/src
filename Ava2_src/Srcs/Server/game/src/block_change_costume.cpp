#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "locale_service.h"


struct block_costume_values
{
	DWORD vnum_item;
};

std::vector<block_costume_values> load_block_costumes;

void LoadInfoBlockCostumeBonus()
{
	FILE *fp;
	char file_name[256+1];
	char one_line[256];
	const char	*delim = " \t\r\n";
	char	*v, *token_string;
	DWORD 	vnum_item;

	block_costume_values costume_values_block = {0};

	snprintf(file_name, sizeof(file_name), "%s/block_bonus_costume.txt", LocaleService_GetBasePath().c_str());
	fp = fopen(file_name, "r");
	
	while (fgets(one_line, 256, fp))
	{
		vnum_item = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);

		if (NULL == token_string)
			continue;
		
		if ((v = strtok(NULL, delim)))
			str_to_number(vnum_item, v);


		TOKEN("ITEM_VNUM")
		{
			costume_values_block.vnum_item = vnum_item;
			load_block_costumes.push_back(costume_values_block);
		}
	}
	fclose(fp);
}

bool CheckVnumCostumeBlock(DWORD vnum_item)
{
	for (int i = 0; i < load_block_costumes.size(); ++i)
	{
		if(load_block_costumes[i].vnum_item == vnum_item)
		{
			return true;
		}
	}
	return false;
}


