#ifndef _HAMSTER_H
#define _HAMSTER_H

void hamster_init();
void hamster_final();

void hamster_parse(int fd,struct map_session_data *sd);

void hamster_logout(TBL_PC* sd);

#endif

