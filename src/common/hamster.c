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
#include "../config/secure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

struct HAMSTER_CORE *hamster;
void _FASTCALL hamster_msg(const char *format);
void _FASTCALL hamster_abnormal_start(int code);
void _FASTCALL hamster_socket_disconnect(int fd);
void _FASTCALL mac_banned(const char *mac);
void _FASTCALL mac_unbanned(const char *mac);
void _FASTCALL hamster_socket_send(int fd, const unsigned char* buf, int lenght);

void do_hamster_final() {
	hamster->final();
}

void do_hamster_init() {
#ifndef HAMSTERGUARD
	ShowFatalError("HamsterGuard no se encuentra activado, por favor revisa tu configuracion 'secure.h'.\n");
	exit(EXIT_FAILURE);
#else
#ifndef HAMSTER_VERSION
	ShowFatalError("No se puede obtener la version de HamsterGuard.");
	exit(EXIT_FAILURE);
#endif
	ShowStatus("HamsterGuard Version: %d.%d.%d\n", HAMVER_A, HAMVER_B, HAMVER_C);
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Los modulos de HamsterGuard se han cargado exitosamente.");
#endif
	hamster->abnormal = hamster_abnormal_start;
	hamster->socket_dc = hamster_socket_disconnect;
	hamster->socket_c = hamster_socket_send;
	hamster->msg = hamster_msg;
	
	hamster->init();
}

void reload() {
	//TODO: Recarga de database.
	hamster->msg("La configuracion se ha recargado correctamente.");
	return;
}
int get_mac_address(int acc_id) {
	char mac_address[20];
	sprintf(mac_address, "SELECT `last_mac` FROM `login` WHERE `account_id` = '%d'", acc_id);
	return mac_address;
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
	ShowFatalError("HamsterGuard ha reportado un inicio anormal. codigo: %d\n", code);
	exit(EXIT_FAILURE);
}

void _FASTCALL hamster_msg(const char *msg) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": %s", msg);
}

void _FASTCALL mac_banned(const char *mac) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": La MAC '%s' ha sido baneada.\n", mac);
}

void _FASTCALL mac_unbanned(const char *mac) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": La MAC '%s' ha sido desbaneada.\n", mac);
}

