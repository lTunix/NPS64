/* Persistent UI preferences and isolated per-ROM options. GPL-2.0-or-later */
#ifndef NPS64_SETTINGS_H
#define NPS64_SETTINGS_H
#include <stdbool.h>
enum {NPS64_ASPECT_NATIVE,NPS64_ASPECT_STRETCHED,NPS64_ASPECT_EXPERIMENTAL,NPS64_ASPECT_COUNT};
typedef struct {unsigned aspect;bool filtered_vi;unsigned slot,controller_pak,resolution;} Nps64GameSettings;
typedef struct {unsigned theme,hud_mode,language;Nps64GameSettings defaults;} Nps64AppSettings;
void nps64_settings_defaults(Nps64AppSettings*);
bool nps64_settings_load(const char*,Nps64AppSettings*);
bool nps64_settings_save(const char*,const Nps64AppSettings*);
bool nps64_game_settings_load(const char*,const char*,const Nps64GameSettings*,Nps64GameSettings*);
bool nps64_game_settings_save(const char*,const char*,const Nps64GameSettings*);
bool nps64_game_settings_reset(const char*,const char*);
#endif
