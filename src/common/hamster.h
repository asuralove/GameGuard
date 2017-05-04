#ifndef _HAMSTERCORE_H
#define _HAMSTERCORE_H
void do_hamster_final();
void do_hamster_init();

extern struct HAMSTER_CORE *hamster;

#ifdef HAMSTERGUARD
	#define HAMVER_A 4
	#define HAMVER_B 1
	#define HAMVER_C 1
	#define HAMSTER_VERSION (HAMSTER_VERSION_MAYOR * 10000 HAMSTER_VERSION_MENOR * 100 HAMSTER_VERSION_PATCH)
#endif

#if defined(__64BIT__) && defined(_MSC_VER)
	#define _FASTCALL
#elif defined(__64BIT__)
	#if !defined(__GNUC__)
		#error Invalid compiler
	#elif __GNUC__  < 4 || (__GNUC__  == 4 && __GNUC_MINOR__ < 4)
		#error GCC 4.4 required!
		#define _FASTCALL
	#else
		#define _FASTCALL __attribute__((ms_abi))
	#endif
#elif defined(_MSC_VER)
	#define _FASTCALL __fastcall
#else
	#define _FASTCALL __attribute__((fastcall))
#endif

#define HAMSTER_CALL(name, rettype, args) rettype (_FASTCALL * name)args

#ifdef __cplusplus
extern "C" {
#endif

enum {
	HAMSTER_UNKNOWN = 0,
	HAMSTER_JAIL,
	HAMSTER_BAN,
	HAMSTER_ATCMD,
	HAMSTER_MSG,
	HAMSTER_GET_ID,
	HAMSTER_SCRIPT,

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

#pragma pack(push, 1)
struct HAMSTER_CORE {
	HAMSTER_CALL(init, void, (void));
	HAMSTER_CALL(final, void, (void));
	HAMSTER_CALL(reload, void, (void));
	HAMSTER_CALL(is_mac_banned, bool, (const char *mac));
	HAMSTER_CALL(action_request, void, (int fd, int task, int id, intptr data));
	HAMSTER_CALL(socket_disconnect, void, (int fd));
	HAMSTER_CALL(socket_send, void, (int fd, const unsigned char* buf, int length));
	HAMSTER_CALL(player_log, void, (int fd, const char *msg));
	HAMSTER_CALL(hamster_msg, void, (const char*));
	HAMSTER_CALL(get_mac_address, void, (int acc_id, const char *mac));
};
#pragma pack(pop)

#ifdef __cplusplus	
}
#endif

#endif