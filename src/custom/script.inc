// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Script Commands
 * Place the body of custom script commands in this file.
 * Format:
 *	BUILDIN_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

//BUILDIN_FUNC(example)
//{
//	ShowInfo("buildin_example: It works!\n");
//	script_pushint(st,1);
//	return 0;
//}
/*===============================
 * citem <item id>;
 * citem <"item name">;
 *===============================*/
BUILDIN_FUNC(citem)
{
	unsigned short nameid;
	struct item item_tmp;
	TBL_PC *sd;
	struct script_data *data;

	sd = script_rid2sd(st);
	if (sd == NULL)
	{	// No player attached.
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	data = script_getdata(st, 2);
	get_val(st, data);
	if (data_isstring(data)) {
		int ep;
		const char *name = conv_str(st, data);
		struct item_data *item_data = itemdb_searchname(name);
		if (item_data == NULL)
		{	//Failed
			script_pushint(st, 0);
			return SCRIPT_CMD_SUCCESS;
		}
		ep = item_data->equip;
		if (!(ep&EQP_HEAD_LOW) && !(ep&EQP_HEAD_MID) && !(ep&EQP_HEAD_TOP) && !(ep&EQP_GARMENT)){
			ShowError("buildin_getcostumeitem: Attempted to convert non-cosmetic item to costume.");
			return SCRIPT_CMD_FAILURE;
		}
		nameid = item_data->nameid;
	}
	else
		nameid = conv_num(st, data);

	if (!itemdb_exists(nameid))
	{	// Item does not exist.
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;
	}

	memset(&item_tmp, 0, sizeof(item_tmp));
	item_tmp.nameid = nameid;
	item_tmp.amount = 1;
	item_tmp.identify = 1;
	item_tmp.card[0] = CARD0_CREATE;
	item_tmp.card[2] = GetWord(battle_config.costume_reserved_char_id, 0);
	item_tmp.card[3] = GetWord(battle_config.costume_reserved_char_id, 1);
	if (pc_additem(sd, &item_tmp, 1, LOG_TYPE_SCRIPT)) {
		script_pushint(st, 0);
		return SCRIPT_CMD_SUCCESS;	//Failed to add item, we will not drop if they don't fit
	}

	script_pushint(st, 1);
	return SCRIPT_CMD_SUCCESS;
}
