/* Per-ROM GameShark library and saved opt-in selections. GPL-2.0-or-later */
#pragma once
#include "storage.h"
#define NPS64_CHEAT_MAX 4096
#define NPS64_CHEAT_ACTIVE_MAX 32
#define NPS64_CHEAT_CODE_MAX 1792
typedef struct {char id[17],group[17],name[192],code[NPS64_CHEAT_CODE_MAX];bool enabled,applied;} Nps64Cheat;
typedef struct {Nps64Cheat *items;unsigned count;char key[40],identity[21];bool truncated;} Nps64Cheats;
void nps64_cheats_free(Nps64Cheats*);
bool nps64_cheats_load(Nps64Cheats*,const Game*,const char *database,const char *selection_dir);
bool nps64_cheats_toggle(Nps64Cheats*,unsigned,const char*);
bool nps64_cheats_disable_all(Nps64Cheats*,const char*);
unsigned nps64_cheats_enabled(const Nps64Cheats*);
void nps64_cheats_apply(Nps64Cheats*,bool reset,void (*reset_fn)(void),void (*set_fn)(unsigned,bool,const char*));
bool nps64_cheat_code_valid(const char*);
