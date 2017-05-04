#ifndef _HAMSTER_H
#define _HAMSTER_H

void hamster_init();
void hamster_final();

enum {
	LOG_SQL = 1,
	LOG_TXT,
	LOG_TRYSQL,
};

void hamster_parse(int fd, struct map_session_data *sd);

void hamster_ban(int32 account_id, int state);

#endif