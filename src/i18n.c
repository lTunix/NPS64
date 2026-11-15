#include "i18n.h"
#include <string.h>
static unsigned language;
static const struct {const char *es,*en;} strings[]={
#include "i18n_strings.inc"
};
unsigned nps64_language(void){return __atomic_load_n(&language,__ATOMIC_RELAXED);}
void nps64_language_set(unsigned value){__atomic_store_n(&language,value<NPS64_LANGUAGE_COUNT?value:NPS64_LANG_ENGLISH,__ATOMIC_RELAXED);}
const char *nps64_tr_language(const char *spanish,unsigned value){
    if(value==NPS64_LANG_SPANISH)return spanish;
    for(unsigned i=0;i<sizeof strings/sizeof *strings;i++)if(!strcmp(strings[i].es,spanish))return strings[i].en;
    return spanish;
}
const char *nps64_tr(const char *spanish){return nps64_tr_language(spanish,nps64_language());}
