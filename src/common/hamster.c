#include "cbasetypes.h"
#include "showmsg.h"
#include "db.h"
#include "strlib.h"
#include "socket.h"
#include "sql.h"
#include "timer.h"
#include "malloc.h"
#include "mmo.h"
#include "core.h"
#include "hamster.h"
#include "../login/macban.h"
#include "../config/secure.h"
#include "../map/hamster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

static Sql* sql_handle = NULL;
int hamster_inited = NULL;

struct HAMSTER_CORE *HamsterData;
struct Hamster_Config hamster_config;

void do_hamster_final() {
	hamster_inited = NULL;
}

void do_hamster_init() {
#ifndef HAMSTERGUARD
	ShowFatalError("HamsterGuard no se encuentra activado, por favor revisa tu configuracion 'secure.h'.\n");
	exit(EXIT_FAILURE);
#else
#ifndef HAMSTER_VERSION
	ShowFatalError("No se puede obtener la version de HamsterGuard.\n");
	exit(EXIT_FAILURE);
#endif
	ShowStatus("HamsterGuard Version: %d.%d.%d\n", HAMVER_A, HAMVER_B, HAMVER_C);
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Los modulos de HamsterGuard se han cargado exitosamente.\n");
#endif
        hamster_msg("Iniciado. Esperando conexion con el map server.\n");
	hamster_inited = true;
}

void hamster_reload() {
	do_hamster_final();
	do_hamster_final();
	hamster_msg("Reiniciado correctamente.");
	return;
}

void hamster_msg(const char *msg) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": %s \n", msg);
}

void hamster_mac_banned(const char *mac) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": La MAC '%s' ha sido baneada.\n", mac);
}

void hamster_mac_unbanned(const char *mac) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": La MAC '%s' ha sido desbaneada.\n", mac);
}

int hamster_macban(const char *mac) {
	if( !hamster_config.macban )
		return 0;// macban disabled

	if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `mac_bans` WHERE `mac` = '%s'", mac) )
		Sql_ShowDebug(sql_handle);

	return 0;
}

void hamster_validate_connection(const char *mac) {
#ifdef HAMSTERGUARD
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Conexion autorizada a la MAC '"CL_WHITE"%s"CL_RESET"'\n",mac);
#else
	ShowFatalError("HamsterGuard no se encuentra activado, por favor revisa tu configuracion 'secure.h'.\n");
	exit(EXIT_FAILURE);
#endif
}
