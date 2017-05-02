#include "../common/cbasetypes.h"
#include "../common/showmsg.h"
#include "../common/db.h"
#include "../common/strlib.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../config/core.h"
#include "../common/hamster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

void *hamster_module = NULL;

static DBMap *mod_exports = NULL;
static DBMap *hamster_timer = NULL;

struct HAMSTER_FUNCTIONS	*hamster_funcs	= NULL;
struct HAMSTER_ATHENA		*ea_funcs	= NULL;

#ifdef __64BIT__
	#define HAMSTER_CODE "./3rdparty/hamsterguard/hamsterguard64.bin"
	typedef uint64 hsysint;
#else
	ShowFatalError("HamsterGuard isn't compatible with 32bit architecture.\n");
	exit(EXIT_FAILURE);
#endif

static void* hamster_get_symbol(const char *name);
static bool hamster_load_module(const char *path);

void _FASTCALL hamster_abnormal_start(int code);
void** _FASTCALL ea_fd2hamstersession(int fd);
unsigned int _FASTCALL ea_tick(void);
int _FASTCALL ea_timer_add(unsigned int tick, HamsterTimerProc func, int id, intptr data);
int _FASTCALL ea_timer_del(int tid, HamsterTimerProc func);
void _FASTCALL ea_socket_disconnect(int fd);
void _FASTCALL ea_socket_send(int fd, const unsigned char* buf, int length);
void _FASTCALL hamster_msg(const char *format);
void * _FASTCALL crt_alloc(size_t size);
void   _FASTCALL crt_free(void * ptr);
void  _FASTCALL crt_exit(int code);
void* _FASTCALL crt_fopen(const char* file, const char *mode);
int   _FASTCALL crt_fclose(void* file);
char* _FASTCALL crt_fgets(char* buf, int max_count, void* file);
size_t _FASTCALL crt_fread(void* ptr, size_t size, size_t count, void* file);
time_t _FASTCALL crt_time(time_t* timer);
int _FASTCALL crt_rand(void);
void _FASTCALL crt_srand(unsigned int seed);

void hamster_core_final() {
	db_destroy(mod_exports);
	db_destroy(hamster_timer);
	hamster_funcs->final();
}

void hamster_core_init() {
	int *module_version;
	void (*module_init)();
	
	if(!hamster_load_module(HAMSTER_CODE)) {
		ShowFatalError("Unable to load HamsterGuard modules.\n");
		exit(EXIT_FAILURE);
	}
	
	module_version = (int*)hamster_get_symbol("version");
	if (!module_version) {
		ShowFatalError("Unable to determine HamsterGuard module.\n");
		exit(EXIT_FAILURE);
	}
#ifndef HAMSTER_VERSION
	ShowStatus("HamsterGuard no se encuentra activado.\n");
#else
	ShowStatus("HamsterGuard Version: %d.%d.%d\n", HAMSTER_VERSION_MAYOR, HAMSTER_VERSION_MENOR, HAMSTER_VERSION_PATCH);
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": Module loaded succefully (v%d)\n", *module_version);
#endif
	hamster_funcs = (struct HAMSTER_FUNCTIONS*)hamster_get_symbol("hamsterfnc");
	ea_funcs = (struct HAMSTER_ATHENA*)hamster_get_symbol("ea_funcs");
	module_init = (void(*)())hamster_get_symbol("Init");
	if (!module_init) {
		ShowFatalError("Invalid HamsterGuard module exports.\n");
		exit(EXIT_FAILURE);
	}
	if(!hamster_funcs) {
		ShowFatalError("Can't load HamsterGuard Source Functions.\n");
		exit(EXIT_FAILURE);
	}
	if(!ea_funcs) {
		ShowFatalError("Can't load HamsterGuard Athena Source Functions.\n");
		exit(EXIT_FAILURE);
	}
	ea_funcs->alloc = crt_alloc;
	ea_funcs->free = crt_free;
	ea_funcs->exit = crt_exit;
	ea_funcs->fopen = crt_fopen;
	ea_funcs->fclose = crt_fclose;
	ea_funcs->fread = crt_fread;
	ea_funcs->fgets = crt_fgets;
#if HAMSTER_VERSION >= 40000
	ea_funcs->time = crt_time;
	ea_funcs->rand = crt_rand;
	ea_funcs->srand = crt_srand;
#endif
	ea_funcs->hamster_msg = hamster_msg;
	ea_funcs->hamstersrv_abnormal_error = hamster_abnormal_start;
	ea_funcs->ea_fd2hamstersession = ea_fd2hamstersession;
	ea_funcs->ea_tick = ea_tick;
	ea_funcs->timer_add = ea_timer_add;
	ea_funcs->timer_del = ea_timer_del;
	ea_funcs->socket_disconnect = ea_socket_disconnect;
	ea_funcs->socket_send = ea_socket_send;

	hamster_timer = idb_alloc(DB_OPT_BASE);

	module_init();
	hamster_funcs->init();
}

static uint8 *hamster_map_file(const char *path, size_t *size) {
	uint8 *buf;
	struct stat statinf;
	if (stat(path, &statinf))
		return NULL;
	int fd = open(path, O_RDONLY);
	if (!fd)
		return NULL;
	buf = mmap(NULL, statinf.st_size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	*size = statinf.st_size;
	return buf;
}

static bool hamster_load_module(const char *path) {
	size_t i;
	struct HAMSTER_MOD_HEADER *head;
	size_t pos, size = 0;
	uint8 *buf = hamster_map_file(path, &size);
	if (!buf) {
		ShowFatalError("Failed to open %s.\n", path);
		return false;
	}

	if (size < sizeof(struct HAMSTER_MOD_HEADER))
		return false;

	head = (struct HAMSTER_MOD_HEADER *)buf;
	if (head->signature != HAMSTER_MOD_HEADER_SIGNATURE)
		return false;
	pos = sizeof(*head);
	// Relocations
	{
		hsysint delta = (hsysint)(buf + head->mem_offset);
		for (i = 0; i < head->reloc_count; i++) {
			*(hsysint*)(buf + head->mem_offset + *(uint32*)(buf + pos + i * sizeof(uint32))) += delta;
		}
		pos += head->reloc_count * sizeof(uint32);
	}
	// Exports
	{
		mod_exports = strdb_alloc(DB_OPT_BASE, 50);
		for (i = 0; i < head->export_count; i++) {
			uint32 offset = *(uint32*)(buf + pos);
			strdb_put(mod_exports, (int8*)buf+pos+5, buf + head->mem_offset + offset);
			pos += 4 + 1 + *(uint8*)(buf + pos + 4) + 1;
		}
	}
	return true;
}

void * _FASTCALL crt_alloc(size_t size) {
#ifdef HAMSTER_OLD_SERVER
	return aCalloc(size, 1);
#else
	return malloc(size);
#endif
}

void _FASTCALL crt_free(void *ptr) {
#ifdef HAMSTER_OLD_SERVER
	aFree(ptr);
#else
	free(ptr);
#endif
}

void  _FASTCALL crt_exit(int code) {
	exit(code);
}

void* _FASTCALL crt_fopen(const char* file, const char *mode) {
	return (void*)fopen(file, mode);
}

int   _FASTCALL crt_fclose(void* file) {
	return fclose((FILE*)file);
}

char* _FASTCALL crt_fgets(char* buf, int max_count, void* file) {
	return fgets(buf, max_count, (FILE*)file);
}

size_t _FASTCALL crt_fread(void* ptr, size_t size, size_t count, void* file) {
	return fread(ptr, size, count, (FILE*)file);
}

time_t _FASTCALL crt_time(time_t* timer) {
	return time(timer);
}

int _FASTCALL crt_rand(void) {
	return rand();
}

void _FASTCALL crt_srand(unsigned int seed) {
	srand(seed);
}

void _FASTCALL ea_socket_disconnect(int fd) {
	session[fd]->flag.eof = 1;
}

void _FASTCALL ea_socket_send(int fd, const unsigned char* buf, int length) {
	WFIFOHEAD(fd, length);
	memcpy(WFIFOP(fd, 0), buf, length);
	WFIFOSET(fd, length);
}

struct GccBinaryCompatibilityDoesNotSeemToExist {
	intptr data;
	int id;
	HamsterTimerProc func;
};

int ea_timer_wrap(int tid, unsigned int tick, int id, intptr data) {
	struct GccBinaryCompatibilityDoesNotSeemToExist *e = (struct GccBinaryCompatibilityDoesNotSeemToExist *)data;
	
	e->func(tid, tick, e->id, e->data);
	idb_remove(hamster_timer, tid);
	aFree(e);

	return 0;
}

int _FASTCALL ea_timer_add(unsigned int tick, HamsterTimerProc func, int id, intptr data) {
#if !defined(__64BIT__)
	return _athena_add_timer(tick, (TimerFunc)func, id, data);
#else
	struct GccBinaryCompatibilityDoesNotSeemToExist *e;
	int tid;

	CREATE(e, struct GccBinaryCompatibilityDoesNotSeemToExist, 1);

	tid = _athena_add_timer(tick, ea_timer_wrap, 0, (intptr)e);
	e->data = data;
	e->id = id;
	e->func = func;
	idb_put(hamster_timer, tid, e);

	return tid;
#endif
}

int _FASTCALL ea_timer_del(int tid, HamsterTimerProc func) {
#if !defined(__64BIT__)
	return _athena_delete_timer(tid, (TimerFunc)func);
#else
	struct GccBinaryCompatibilityDoesNotSeemToExist *e = (struct GccBinaryCompatibilityDoesNotSeemToExist *)idb_get(hamster_timer, tid);

	if (!e) {
		ShowWarning("Trying to remove non-existing timer %d\n", tid);
		return -1;
	}

	if (e->func != func) {
		ShowWarning("ea_timer_del: Function mismatch!\n");
		return -1;
	}

	idb_remove(hamster_timer, tid);
	aFree(e);

	return _athena_delete_timer(tid, ea_timer_wrap);
#endif
}

/* --- */

unsigned int _FASTCALL ea_tick(void) {
	return _athena_gettick();
}

void** _FASTCALL ea_fd2hamstersession(int fd) {
	return &session[fd]->hamster_sd;
}

void _FASTCALL hamster_abnormal_start(int code) {
	ShowFatalError("HamsterGuard module reported critical startup code: %d\n", code);
	exit(EXIT_FAILURE);
}

static void* hamster_get_symbol(const char *name) {
	return strdb_get(mod_exports, name);
}

void _FASTCALL hamster_msg(const char *msg) {
	ShowMessage(""CL_MAGENTA"[HamsterGuard]"CL_RESET": %s", msg);
}
