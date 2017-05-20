#ifndef _HAMSTERCORE_H
#define _HAMSTERCORE_H

void do_hamster_final();
void do_hamster_init();

extern struct HAMSTER_CORE *HamsterData;
extern struct Hamster_Config hamster_config;

#ifdef HAMSTERGUARD
	#define HAMVER_A 4
	#define HAMVER_B 1
	#define HAMVER_C 1
	#define HAMSTER_VERSION (HAMVER_A * 10000 HAMVER_B * 100 HAMVER_C)
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

struct HAMSTER_CORE {
	char mac_address[20];
	int ip;
	int player_log;
};

struct Hamster_Config {
	int macban;
	const char macban_db_username[32];
	const char macban_db_password[32];
	const char macban_db_hostname[32];
	int macban_db_port;
	const char macban_db_database[32];
	const char macban_db_codepage[32];
};

void hamster_reload();
int hamster_macban(const char *mac);
void hamster_mac_banned(const char *mac);
void hamster_mac_unbanned(const char *mac);
void hamster_validate_connection(const char *mac);
void hamster_msg(const char*);
void do_hamster_final();
void do_hamster_init();

#endif