#ifndef _HAMSTERCORE_H
#define _HAMSTERCORE_H

void hamster_core_final();
void hamster_core_init();

extern struct HAMSTER_FUNCTIONS  *hamster_funcs;
extern struct HAMSTER_ATHENA	 *ea_funcs;

#ifdef HAMSTERGUARD
	#define HAMSTER_VERSION_MAYOR 4
	#define HAMSTER_VERSION_MENOR 1
	#define HAMSTER_VERSION_PATCH 1
	#define HAMSTER_VERSION (HAMSTER_VERSION_MAYOR * 10000 + HAMSTER_VERSION_MENOR * 100 + HAMSTER_VERSION_PATCH)
#endif

#ifdef HAMSTER_OLD_SERVER
	#define _athena_add_timer             iTimer->add_timer
	#define _athena_add_timer_interval    iTimer->add_timer_interval
	#define _athena_delete_timer          iTimer->delete_timer
	#define _athena_gettick               iTimer->gettick
#else
	#define _athena_add_timer             add_timer
	#define _athena_add_timer_interval    add_timer_interval
	#define _athena_delete_timer          delete_timer
	#define _athena_gettick               gettick
#endif

#if defined(__64BIT__) && defined(_MSC_VER)
	#define _FASTCALL
	#define _STDCALL
	#define _TIMERCALL
#elif defined(__64BIT__)
	#if !defined(__GNUC__)
		#error Invalid compiler
	#elif __GNUC__  < 4 || (__GNUC__  == 4 && __GNUC_MINOR__ < 4)
		#error GCC 4.4 required!
		#define _FASTCALL
		#define _STDCALL
		#define _TIMERCALL
	#else
		#define _FASTCALL __attribute__((ms_abi))
		#define _STDCALL __attribute__((ms_abi))
		#define _TIMERCALL __attribute__((ms_abi))
	#endif
#elif defined(_MSC_VER)
	#define _FASTCALL __fastcall
	#define _STDCALL __stdcall
	#define _TIMERCALL __cdecl
#else
	#define _FASTCALL __attribute__((fastcall))
	#define _STDCALL __attribute__((stdcall))
	#define _TIMERCALL __attribute__((cdecl))
#endif

#define HAMSTER_CALL(name, rettype, args) rettype (_FASTCALL * name)args

#ifdef __cplusplus
extern "C" {	
#endif

enum {
	HAMSTER_UNKNOWN = 0,
	HAMSTER_KICK,
	HAMSTER_ATCMD,
	HAMSTER_MSG,
	HAMSTER_PACKET,
	HAMSTER_GET_ID,
	HAMSTER_DC,
	HAMSTER_LOGIN_ACTION,
	HAMSTER_ZONE_ACTION,
	HAMSTER_GET_FD,
	HAMSTER_IS_ACTIVE,
	HAMSTER_SET_LOG_METHOD,
	HAMSTER_INIT_GROUPS,
	HAMSTER_RESOLVE_GROUP,
	HAMSTER_BAN,
	HAMSTER_JAIL,
	HAMSTER_SCRIPT,
	HAMSTER_GET_ADMINS,
	HAMSTER_IS_CHAR_CONNECTED,

	HAMSTER_LAST
};

enum {
	HAMID_AID = 0,
	HAMID_GID,
	HAMID_GDID,
	HAMID_PID,
	HAMID_CLASS,
	HAMID_GM,
};

typedef int (_TIMERCALL *HamsterTimerProc)(int tid, unsigned int tick, int id, intptr data);
#pragma pack(push, 1)
struct HAMSTER_FUNCTIONS {
	// CORE
	HAMSTER_CALL(net_send, void, (int fd, uint8 *buf, int len));
	HAMSTER_CALL(net_recv, int, (int fd, uint8 *buf, int len, uint8 *base_buf, size_t base_len));
	HAMSTER_CALL(session_new, int, (int fd, uint32 client_addr));
	HAMSTER_CALL(session_del, void, (int fd));

	HAMSTER_CALL(is_secure_session, int, (int fd));

	HAMSTER_CALL(init, void, (void));
	HAMSTER_CALL(final, void, (void));

	// LOGIN
	HAMSTER_CALL(login_init, void, (void));
	HAMSTER_CALL(login_final, void, (void));
	HAMSTER_CALL(login_process_auth, int, (int fd, char* buf, size_t buf_len, char* username, char* password, uint32* version));
	HAMSTER_CALL(login_process_auth2, int, (int fd, int level));
	HAMSTER_CALL(login_process, void, (int fd, uint8* buf, size_t buf_len));
	HAMSTER_CALL(login_get_mac_address, void, (int fd, char *buf));

	// ZONE
	HAMSTER_CALL(zone_init, void, (void));
	HAMSTER_CALL(zone_final, void, (void));
	HAMSTER_CALL(zone_process, int, (int fd, uint16 cmd, uint8* buf, size_t buf_len));
	HAMSTER_CALL(zone_logout, void, (int fd));
	HAMSTER_CALL(zone_reload, void, (void));
	HAMSTER_CALL(zone_grf_reload, void, (void));
	HAMSTER_CALL(zone_autoban_show, void, (int fd));
	HAMSTER_CALL(zone_autoban_lift, void, (uint32 ip));
	HAMSTER_CALL(zone_login_pak, void, (uint8* buf, size_t buf_len));
	HAMSTER_CALL(zone_get_mac_address, void, (int fd, char *buf));
	HAMSTER_CALL(zone_register_group, void, (int group_id, int level));
	HAMSTER_CALL(zone_register_admin, bool, (int aid, bool exclude));
};

struct HAMSTER_ATHENA {
	HAMSTER_CALL(alloc, void*, (size_t size));
	HAMSTER_CALL(free, void, (void *ptr));

	HAMSTER_CALL(exit, void, (int code));
#if HAMSTER_VERSION >= 40000
	HAMSTER_CALL(rand, int, (void));
	HAMSTER_CALL(srand, void, (unsigned int));
	HAMSTER_CALL(time, time_t, (time_t* timer));
#endif

	HAMSTER_CALL(fopen, void*, (const char* file, const char *mode));
	HAMSTER_CALL(fclose, int, (void* file));
	HAMSTER_CALL(fgets, char*, (char* buf, int max_count, void* file));
	HAMSTER_CALL(fread, size_t, (void* ptr, size_t size, size_t count, void* file));

	HAMSTER_CALL(ea_tick, unsigned int, (void));
	HAMSTER_CALL(ea_fd2hamstersession, void**, (int fd));
	HAMSTER_CALL(ea_is_mac_banned, bool, (const int8 *mac));
	HAMSTER_CALL(hamstersrv_abnormal_error, void, (int code));

	HAMSTER_CALL(timer_add, int, (unsigned int tick, HamsterTimerProc func, int id, intptr data));
	HAMSTER_CALL(timer_del, int, (int tid, HamsterTimerProc func));

	HAMSTER_CALL(action_request, void, (int fd, int task, int id, intptr data));

	HAMSTER_CALL(socket_disconnect, void, (int fd));
	HAMSTER_CALL(socket_send, void, (int fd, const unsigned char* buf, int length));

	HAMSTER_CALL(player_log, void, (int fd, const char *msg));

	HAMSTER_CALL(hamster_msg, void, (const char*));
};
#pragma pack(pop)

#define HAMSTER_MOD_HEADER_SIGNATURE 0x56525348
struct HAMSTER_MOD_HEADER {
	uint32 signature;
	uint32 memory_size;
	uint32 reloc_count;
	uint32 export_count;
	uint32 mem_offset;
};
#define GMLIST_MAX_ADMINS 100

#ifdef __cplusplus
}	
#endif

#endif
