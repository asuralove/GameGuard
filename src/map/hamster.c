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

void _FASTCALL hamster_log(int fd, const char *msg);

static SqlStmt* log_stmt = NULL;
static SqlStmt* ban_stmt = NULL;

static int log_method = LOG_TRYSQL;

void hamster_init() {
	if (logmysql_handle != NULL) {
		log_stmt = SqlStmt_Malloc(logmysql_handle);
		if (SQL_SUCCESS != SqlStmt_Prepare(log_stmt, "INSERT DELAYED INTO hamster_log (`account_id`, `char_name`, `IP`, `data`) VALUES (?, ?, ?, ?)")) {
			ShowFatalError("HamsterGuard: No se ha podido preparar la SQL de log.\n");
			Sql_ShowDebug(logmysql_handle);
			exit(EXIT_FAILURE);
		}
	}

	if (mmysql_handle == NULL) {
		ShowFatalError("HamsterGuard: No se ha iniciado el SQL!\n");
		exit(EXIT_FAILURE);
	}

	ban_stmt = SqlStmt_Malloc(mmysql_handle);
	if (SQL_SUCCESS != SqlStmt_Prepare(ban_stmt, "UPDATE login SET state = ? WHERE account_id = ?")) {
		ShowFatalError("HamsterGuard: No se ha podido preparar la SQL de baneos.\n");
		Sql_ShowDebug(mmysql_handle);
		exit(EXIT_FAILURE);
	}

	hamster_msg("Conectado a la SQL de HamsterGuard correctamente.");
}

void hamster_final() {
	if (log_stmt) {
		SqlStmt_Free(log_stmt);
		log_stmt = NULL;
	}
	SqlStmt_Free(ban_stmt);
}

void hamster_parse(int fd, struct map_session_data *sd) {
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
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Se ha baneado a la cuenta #%d (estado %d)\n", account_id, state);
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

void _FASTCALL hamster_log(int fd, const char *msg) {
	char ip[20];
	TBL_PC *sd = (TBL_PC*)session[fd]->session_data;

	if (!sd)
		return;

	sprintf(ip, "%u.%u.%u.%u", CONVIP(session[fd]->client_addr));

	if (log_method == LOG_SQL && !hamster_log_sql(sd, ip, msg)) {
		ShowError("No se ha podido conectar al Log de HamsterGuard.\n");
	} else if (log_method == LOG_TRYSQL && log_stmt && hamster_log_sql(sd, ip, msg)) {
		;
	}

	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": %s (Player: %s, IP: %s)\n", msg, sd->status.name, ip);
}

void _FASTCALL hamster_action_request(int fd, int task, int id, intptr data) {
	TBL_PC *sd;

	if (fd == 0) {
		return;
	}

	sd = (TBL_PC *)session[fd]->session_data;
	if (!sd)
		return;

	switch (task) {
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
			default:
				ShowError("HamsterGuard ha hecho una peticion erronea (ID=%d)\n", id);
				ShowError("Estas usando una version incompatible del GameGuard.\n");
				break;
			}
			break;
		case HAMSTER_SCRIPT:
			{
				struct npc_data* nd = npc_name2id((const char*)data);
				if (nd) {
					run_script(nd->u.scr.script, 0, sd->bl.id, fake_nd->bl.id);
				} else {
					ShowError("Una accion de HamsterGuard esta intentando ejecutar un script inexistente '%s'\n", data);
				}
			}
			break;
		default:
			ShowError("HamsterGuard ha hecho una peticion erronea (ID=%d)\n", task);
			ShowError("Estas usando una version incompatible del GameGuard.\n");
			break;
	}
}



