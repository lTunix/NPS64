/* English/Spanish application text. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_I18N_H
#define NPS64_I18N_H
enum { NPS64_LANG_ENGLISH, NPS64_LANG_SPANISH, NPS64_LANGUAGE_COUNT };
unsigned nps64_language(void);
void nps64_language_set(unsigned language);
const char *nps64_tr_language(const char *spanish,unsigned language);
const char *nps64_tr(const char *spanish);
#endif
