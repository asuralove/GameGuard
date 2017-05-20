#include "../common/hamster.h"
#include "../common/cbasetypes.h"
#include "../common/showmsg.h"
#include "../common/sql.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "../common/hamster.h"
#include "login.h"
#include "macban.h"
#include "loginlog.h"
#include "account.h"
#include <stdlib.h>

static Sql* sql_handle = NULL;
static bool macban_inited = false;

static char   hamster_db_hostname[32] = "127.0.0.1";
static uint16 hamster_db_port = 3306;
static char   hamster_db_username[32] = "ragnarok";
static char   hamster_db_password[32] = "";
static char   hamster_db_database[32] = "ragnarok";

bool macban_check(const char *mac) {
	char* data = NULL;
	int matches;

	if( SQL_ERROR == Sql_Query(sql_handle, "SELECT count(*) FROM `mac_bans` WHERE `mac` = '%s'", mac) )
	{
		Sql_ShowDebug(sql_handle);
		return true;
	}

	if ( SQL_ERROR == Sql_NextRow(sql_handle) )
		return true;
	
	Sql_GetData(sql_handle, 0, &data, NULL);
	matches = atoi(data);
	Sql_FreeResult(sql_handle);
	
	return( matches > 0);
}

void macban_init(void) {
	const char* username = hamster_db_username;
	const char* password = hamster_db_password;
	const char* hostname = hamster_db_hostname;
	uint16      port     = hamster_db_port;
	const char* database = hamster_db_database;

	macban_inited = true;

	sql_handle = Sql_Malloc();
	if( SQL_ERROR == Sql_Connect(sql_handle, username, password, hostname, port, database) )
	{
                ShowError("HamsterGuard no se puede conectar al servidor de MAC uname='%s',passwd='%s',host='%s',port='%d',database='%s'\n",
                        username, password, hostname, port, database);
		Sql_ShowDebug(sql_handle);
		Sql_Free(sql_handle);
		exit(EXIT_FAILURE);
	}
        hamster_msg("Conexion a la DB de MAC realizada exitosamente.\n");
}

void macban_final(void) {
	Sql_Free(sql_handle);
	sql_handle = NULL;
}

bool hamster_config_read(const char* key, const char* value) {
	const char* signature;

	if( macban_inited )
		return false;// settings can only be changed before init

	signature = "hamster_db_";
	if( strncmpi(key, signature, strlen(signature)) == 0 )
	{
		key += strlen(signature);
		if( strcmpi(key, "ip") == 0 )
			safestrncpy(hamster_db_hostname, value, sizeof(hamster_db_hostname));
		else
		if( strcmpi(key, "port") == 0 )
			hamster_db_port = (uint16)strtoul(value, NULL, 10);
		else
		if( strcmpi(key, "id") == 0 )
			safestrncpy(hamster_db_username, value, sizeof(hamster_db_username));
		else
		if( strcmpi(key, "pw") == 0 )
			safestrncpy(hamster_db_password, value, sizeof(hamster_db_password));
		else
		if( strcmpi(key, "db") == 0 )
			safestrncpy(hamster_db_database, value, sizeof(hamster_db_database));
		else
			return false;// not found
		return true;
	}

	signature = "mac_";
	if( strncmpi(key, signature, strlen(signature)) == 0 )
	{
		key += strlen(signature);
		if( strcmpi(key, "enable") == 0 )
			login_config.macban = (bool)config_switch(value);
		else
			return false;// not found
		return true;
	}

	return false;// not found
}

