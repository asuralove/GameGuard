#ifndef __MACBAN_H_INCLUDED__
#define __MACBAN_H_INCLUDED__

#include "../common/cbasetypes.h"

bool macban_check(const char *mac);
int macban_cleanup(const char *mac);
bool hamster_config_read(const char* key, const char* value);

void macban_init(void);
void macban_final(void);

#endif
