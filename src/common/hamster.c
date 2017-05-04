#include "cbasetypes.h"
#include "showmsg.h"
#include "db.h"
#include "strlib.h"
#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "mmo.h"
#include "core.h"
#include "hamster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

struct HAMSTER_CORE *hamster;
static void* hamster_get(const char *name);
void _FASTCALL hamster_msg(const char *format);
void _FASTCALL hamster_abnormal_start(int code);
void _FASTCALL hamster_socket_disconnect(int fd);
void _FASTCALL hamster_socket_send(int fd, const insigned char* buf, int lenght);

void do_hamster_final() {
	db_destroy(hamster_timer);
	hamster->final();
}

void do_hamster_init() {
	int *version;
	
	version = (int*)hamster_get("version");
	if(!version) {
		ShowFatalError("No se puede obtener la versión de HamsterGuard.");
		exit(EXIT_FAILURE);
	}
#ifndef HAMSTERGUARD
	ShowFatalError("HamsterGuard no se encuentra activado, por favor revisa tu configuración 'core'.\n");
	exit(EXIT_FAILURE);
#else
	ShowStatus("HamsterGuard Version: %d.%d.%d\n", HAMVER_A, HAMVER_B, HAMVER_C);
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Los módulos de HamsterGuard se han cargado exitosamente.");
#endif
	hamster->abnormal = hamster_abnormal_start;
	hamster->msg = hamster_msg;
	hamster->socket_dc = hamster_socket_disconnect;
	hamster->socket_c = hamster_socket_send;
	
	hamster_timer = idb_alloc(DB_OPT_BASE);
	
	hamster->init();
}

static void* hamster_get(const char *name) {
	switch (name) {
		case "version":
			return HAMSTER_VERSION;
	}
}

void reload() {
	//TODO: Recarga de database.
	hamster->msg("La configuración se ha recargado correctamente.");
	return 0;
}
void get_mac_address(int acc_id, const char *mac) {
	sprintf(mac, "SELECT `last_mac` FROM `login` WHERE `account_id` = '%d'", acc_id);
	return mac;
}

void _FASTCALL hamster_socket_disconnect(int fd) {
	session[fd]->flag.eof = 1;
}

void _FASTCALL hamster_socket_send(int fd, const unsigned char* buf, int lenght) {
	WFIFOHEAD(fd, lenght);
	memcpy(WFIFOP(fd, 0), buf, lenght);
	WFIFOSET(fd, lenght);
}

void _FASTCALL hamster_abnormal_start(int code) {
	ShowFatalError("HamsterGuard ha reportado un inicio anormal. código: %d\n", code);
	exit(EXIT_FAILURE);
}

void _FASTCALL hamster_msg(const char *msg) {
	ShowMessage(""CL_MAGENTA"[Hamster_Guard]"CL_RESET": %s", msg);
}


