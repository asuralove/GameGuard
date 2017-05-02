#include "../common/cbasetypes.h"
#include "../common/showmsg.h"
#include "../common/db.h"
#include "../common/strlib.h"
#include "../common/mmo.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/sql.h"
#include "../common/hamster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pc.h"
#include "clif.h"
#include "atcommand.h"
#include "chrif.h"
#include "pc_groups.h"
#include "npc.h"
#include "hamster.h"

void _FASTCALL hamster_action_request(int fd, int task, int id, intptr data);
void _FASTCALL hamster_log(int fd, const char *msg);
static int hamster_group_register_timer(int tid, unsigned int tick, int id, intptr data);

static SqlStmt* log_stmt = NULL;
static SqlStmt* ban_stmt = NULL;
static SqlStmt* admin_stmt = NULL;

enum {
	LOG_SQL = 1,
	LOG_TXT,
	LOG_TRYSQL,
};

static int log_method = LOG_TRYSQL;
static int tid_group_register = INVALID_TIMER;
static int current_groupscan_minlevel = 0;

#ifdef HAMSTER_OLD_SERVER
	#define SqlStmt_Malloc                SQL->StmtMalloc
	#define SqlStmt_Prepare               SQL->StmtPrepare
	#define SqlStmt_BindParam             SQL->StmtBindParam
	#define SqlStmt_BindColumn            SQL->StmtBindColumn
	#define SqlStmt_Execute               SQL->StmtExecute
	#define SqlStmt_Free                  SQL->StmtFree
	#define Sql_EscapeStringLen           SQL->EscapeStringLen
	#define SqlStmt_NextRow               SQL->StmtNextRow
	#define Sql_QueryStr                  SQL->QueryStr
	#define Sql_Query                     SQL->Query
	#define Sql_NumRows                   SQL->NumRows
	
	#define chrif_isconnected             chrif->isconnected
	#define map_id2bl                     iMap->id2bl
	#define map_mapname2mapid             iMap->mapname2mapid
	
	#define clif_send                     clif->send
	#define clif_authfail_fd              clif->authfail_fd
	#define clif_displaymessage           clif->message
	#define is_atcommand                  atcommand->parse
	#define run_script                    script->run
	
#endif

void hamster_init() {
	if (logmysql_handle != NULL) {
		log_stmt = SqlStmt_Malloc(logmysql_handle);
		if (SQL_SUCCESS != SqlStmt_Prepare(log_stmt, "INSERT DELAYED INTO hamster_log (`account_id`, `char_name`, `IP`, `data`) VALUES (?, ?, ?, ?)")) {
			ShowFatalError("HamsterGuard: Preparing statement 1 failed.\n");
			Sql_ShowDebug(logmysql_handle);
			exit(EXIT_FAILURE);
		}
	}

	if (mmysql_handle == NULL) {
		ShowFatalError("HamsterGuard: SQL is not yet initialized. Please report this!\n");
		exit(EXIT_FAILURE);
	}

	ban_stmt = SqlStmt_Malloc(mmysql_handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(ban_stmt, "UPDATE login SET state = ? WHERE account_id = ?")) {
		ShowFatalError("HamsterGuard: Preparing statement 2 failed.\n");
		Sql_ShowDebug(mmysql_handle);
		exit(EXIT_FAILURE);
	}

	admin_stmt = SqlStmt_Malloc(mmysql_handle);
#ifndef HAMSTER_OLD_SERVER
	if (SQL_SUCCESS != SqlStmt_Prepare(admin_stmt, "SELECT account_id FROM `login` WHERE `group_id` = ?")) {
#else
	if (SQL_SUCCESS != SqlStmt_Prepare(admin_stmt, "SELECT account_id FROM `login` WHERE `level` >= ?")) {
#endif
		ea_funcs->action_request = hamster_action_request;
		ea_funcs->player_log = hamster_log;
	}
	hamster_funcs->zone_init();
}

void hamster_final() {
	hamster_funcs->zone_final();
	if (log_stmt) {
		SqlStmt_Free(log_stmt);
		log_stmt = NULL;
	}
	SqlStmt_Free(ban_stmt);
	SqlStmt_Free(admin_stmt);
	if (tid_group_register != INVALID_TIMER) {
		_athena_delete_timer(tid_group_register, hamster_group_register_timer);
		tid_group_register = INVALID_TIMER;
	}
}

void hamster_parse(int fd,struct map_session_data *sd) {
}

void hamster_ban(int32 account_id, int state) {
	if (!ban_stmt)
		return;
	if (SQL_SUCCESS != SqlStmt_BindParam(ban_stmt, 0, SQLDT_INT, (void*)&state, sizeof(state)) ||
		SQL_SUCCESS != SqlStmt_BindParam(ban_stmt, 1, SQLDT_INT, (void*)&account_id, sizeof(account_id)) ||
		SQL_SUCCESS != SqlStmt_Execute(ban_stmt))
	{
		Sql_ShowDebug(mmysql_handle);
	}
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Banned account #%d (state %d)\n", account_id, state);
}

int hamster_log_sql(TBL_PC* sd, const char* ip, const char* msg) {
	if (!logmysql_handle || !log_stmt)
		return 0;
	if (SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 0, SQLDT_INT, (void*)&sd->status.account_id, sizeof(sd->status.account_id)) ||
		SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 1, SQLDT_STRING, (void*)&sd->status.name, strlen(sd->status.name)) ||
		SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 2, SQLDT_STRING, (void*)ip, strlen(ip)) ||
		SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 3, SQLDT_STRING, (void*)msg, strlen(msg)) ||
		SQL_SUCCESS != SqlStmt_Execute(log_stmt))
	{
		Sql_ShowDebug(logmysql_handle);
		return 0;
	}
	return 1;
}

int hamster_log_txt(TBL_PC* sd, const char* ip, const char* msg) {
	FILE* logfp;
	static char timestring[255];
	time_t curtime;

	if((logfp = fopen("./log/hamster.log", "a+")) == NULL)
		return 0;
	time(&curtime);
	strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
	fprintf(logfp,"%s - %s[%d:%d] - %s\t%s\n", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, ip, msg);
	fclose(logfp);
	return 1;
}

void _FASTCALL hamster_log(int fd, const char *msg) {
	char ip[20];
	TBL_PC *sd = (TBL_PC*)session[fd]->session_data;

	if (!sd)
		return;

	sprintf(ip, "%u.%u.%u.%u", CONVIP(session[fd]->client_addr));

	if (log_method == LOG_SQL && !hamster_log_sql(sd, ip, msg)) {
		ShowError("Logging to hamster_log failed. Please check your Hamster SQL setup.\n");
	} else if (log_method == LOG_TRYSQL && log_stmt && hamster_log_sql(sd, ip, msg)) {
		;
	} else if (log_method == LOG_TXT || log_method == LOG_TRYSQL) {
		hamster_log_txt(sd, ip, msg);
	}

	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": %s (Player: %s, IP: %s)\n", msg, sd->status.name, ip);
}

static bool hamster_iterate_groups(int group_id, int level, const char* name) {
	hamster_funcs->zone_register_group(group_id, level);
	return true;
}

static void hamster_register_groups() {
	hamster_funcs->zone_register_group(25311, 25311);
#ifndef HAMSTER_OLD_SERVER
	pc_group_iterate(hamster_iterate_groups);
#else
	hamster_funcs->zone_register_group(25312, 25312);
#endif
}

static int hamster_group_register_timer(int tid, unsigned int tick, int id, intptr data) {
	if (chrif_isconnected()) {
		hamster_register_groups();
		
		// We will re-send group associations every thirty seconds, otherwise the login server would never be able to recover from a restart
		_athena_delete_timer(tid, hamster_group_register_timer);
		tid_group_register = _athena_add_timer_interval(_athena_gettick()+30*1000, hamster_group_register_timer, 0, 0, 30*1000);
	}
	return 0;
}

static bool hamster_iterate_groups_adminlevel(int group_id, int level, const char* name) {
	int account_id;

	if (level < current_groupscan_minlevel)
		return true;

	ShowInfo("Registering group %d..\n", group_id);
	if (SQL_SUCCESS != SqlStmt_BindParam(admin_stmt, 0, SQLDT_INT, (void*)&group_id, sizeof(group_id)) ||
		SQL_SUCCESS != SqlStmt_Execute(admin_stmt))
	{
		ShowError("Fetching GM accounts from group %d failed.\n", group_id);
		Sql_ShowDebug(mmysql_handle);
		return true;
	}

	SqlStmt_BindColumn(admin_stmt, 0, SQLDT_INT, &account_id, 0, NULL, NULL);
	while (SQL_SUCCESS == SqlStmt_NextRow(admin_stmt)) {
		hamster_funcs->zone_register_admin(account_id, false);
	}
	return true;
}

void hamster_action_request_global(int task, int id, intptr data) {
	switch (task) {
	case HAMSTER_LOGIN_ACTION:
		chrif_hamster_request((uint8*)data, id);
		break;
	case HAMSTER_GET_FD:
		{
		TBL_PC *sd = BL_CAST(BL_PC, map_id2bl(id));
		*(int32*)data = (sd ? sd->fd : 0);
		}
		break;
	case HAMSTER_SET_LOG_METHOD:
		log_method = id;
		break;
	case HAMSTER_INIT_GROUPS:
		if (chrif_isconnected())
			hamster_register_groups();
		else {
			// Register groups as soon as the char server is available again
			if (tid_group_register != INVALID_TIMER)
				_athena_delete_timer(tid_group_register, hamster_group_register_timer);
			tid_group_register = _athena_add_timer_interval(_athena_gettick()+1000, hamster_group_register_timer, 0, 0, 500);
		}
		break;
	case HAMSTER_RESOLVE_GROUP:
#ifndef HAMSTER_OLD_SERVER
		*(int32*)data = pc_group_id2level(id);
#else
		*(int32*)data = id;
#endif
		break;
	case HAMSTER_PACKET:
		clif_send((const uint8*)data, id, NULL, ALL_CLIENT);
		break;
	case HAMSTER_GET_ADMINS:
	{
#ifndef HAMSTER_OLD_SERVER
		// Iterate groups and register each group individually
		current_groupscan_minlevel = id;
		pc_group_iterate(hamster_iterate_groups_adminlevel);
#else
		//
		int account_id;
		int level = id;
		if (SQL_SUCCESS != SqlStmt_BindParam(admin_stmt, 0, SQLDT_INT, (void*)&level, sizeof(level)) ||
			SQL_SUCCESS != SqlStmt_Execute(admin_stmt))
		{
			ShowError("Fetching GM accounts failed.\n");
			Sql_ShowDebug(mmysql_handle);
			break;
		}

		SqlStmt_BindColumn(admin_stmt, 0, SQLDT_INT, &account_id, 0, NULL, NULL);
		while (SQL_SUCCESS == SqlStmt_NextRow(admin_stmt)) {
			hamster_funcs->zone_register_admin(account_id, false);
		}
#endif
		break;
	}
	case HAMSTER_IS_CHAR_CONNECTED:
		*(int*)data = chrif_isconnected();
		break;
	default:
		ShowError("HamsterGuard requested unknown action! (Global; ID=%d)\n", task);
		ShowError("This indicates that you are running an incompatible version.\n");
		break;
	}
}

void _FASTCALL hamster_action_request(int fd, int task, int id, intptr data) {
	TBL_PC *sd;

	if (fd == 0) {
		hamster_action_request_global(task, id, data);
		return;
	}

	switch (task) {
	case HAMSTER_PACKET:
		memcpy(WFIFOP(fd, 0), (const void*)data, id);
		//ShowInfo("Sending %d bytes to session #%d (%x)\n", id, fd, WFIFOW(fd, 0));
		WFIFOSET(fd, id);
		return;
	}

	sd = (TBL_PC *)session[fd]->session_data;
	if (!sd)
		return;

	switch (task) {
	case HAMSTER_DC:
		ShowInfo("-- HamsterGuard requested disconnect.\n");
		set_eof(fd);
		break;
	case HAMSTER_KICK:
		ShowInfo("-- HamsterGuard requested kick.\n");
		if (id == 99)
			set_eof(fd);
		else
			clif_authfail_fd(fd, id);
		break;
	case HAMSTER_JAIL:
	{
		char msg[64];
		snprintf(msg, sizeof(msg)-1, "@jail %s", sd->status.name);
		is_atcommand(0, sd, msg, 0);
	}
		break;
	case HAMSTER_BAN:
		hamster_ban(sd->status.account_id, id);
		break;
	case HAMSTER_ATCMD:
		is_atcommand(fd, sd, (const char*)data, 0);
		break;
	case HAMSTER_MSG:
		clif_displaymessage(fd, (const char*)data);
		break;
	case HAMSTER_IS_ACTIVE:
		*(int32*)data = (sd->bl.prev == NULL || sd->invincible_timer != INVALID_TIMER) ? 0 : 1;
		break;
	case HAMSTER_GET_ID:
		switch (id) {
		case HAMID_AID:
			*(int*)data = sd->status.account_id;
			break;
		case HAMID_GID:
			*(int*)data = sd->status.char_id;
			break;
		case HAMID_GDID:
			*(int*)data = sd->status.guild_id;
			break;
		case HAMID_PID:
			*(int*)data = sd->status.party_id;
			break;
		case HAMID_CLASS:
			*(short*)data = sd->status.class_;
			break;
		case HAMID_GM:
#ifndef HAMSTER_OLD_SERVER
			*(int*)data = pc_group_id2level(sd->group_id);
#else
			*(int*)data = pc_isGM(sd);
#endif
			break;
		default:
			ShowError("HamsterGuard requested unknown ID! (ID=%d)\n", id);
			ShowError("This indicates that you are running an incompatible version.\n");
			break;
		}
		break;
	case HAMSTER_SCRIPT:
		{
			struct npc_data* nd = npc_name2id((const char*)data);
			if (nd) {
				run_script(nd->u.scr.script, 0, sd->bl.id, fake_nd->bl.id);
			} else {
				ShowError("A HamsterGuard action chain tried to execute non-existing script '%s'\n", data);
			}
		}
		break;
	default:
		ShowError("HamsterGuard requested unknown action! (ID=%d)\n", task);
		ShowError("This indicates that you are running an incompatible version.\n");
		break;
	}
}

void hamster_logout(TBL_PC* sd) {
	hamster_funcs->zone_logout(sd->fd);
}

