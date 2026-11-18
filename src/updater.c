/* Background release checks and verified downloads. SPDX-License-Identifier: GPL-2.0-or-later */
#include "updater.h"
#include "i18n.h"
#include "update_feed.h"
#include "update_http.h"
#include "sha256.h"
#include "version.h"
#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#ifndef NPS64_UPDATE_DIR
#define NPS64_UPDATE_DIR "/data/pkg"
#endif
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_t thread;
static bool started,busy,confirm_mode,dismissed;
static int stopping,cancel_download;
static Nps64UpdaterState current_state=NPS64_UPDATER_IDLE;
static Nps64Release pending_release;
static uint64_t received_bytes;
static unsigned progress_percent;
static char status_text[2][160]={"Updates: not checked","Actualizaciones: sin consultar"};
static bool cancelled(void){return __atomic_load_n(&stopping,__ATOMIC_ACQUIRE)!=0||__atomic_load_n(&cancel_download,__ATOMIC_ACQUIRE)!=0;}
static void status(const char *format,...) {
    va_list args;va_start(args,format);pthread_mutex_lock(&mutex);
    for(unsigned i=0;i<NPS64_LANGUAGE_COUNT;i++){va_list copy;va_copy(copy,args);vsnprintf(status_text[i],sizeof(status_text[i]),nps64_tr_language(format,i),copy);va_end(copy);}
    pthread_mutex_unlock(&mutex);va_end(args);
}
static void set_state(Nps64UpdaterState st){pthread_mutex_lock(&mutex);current_state=st;pthread_mutex_unlock(&mutex);}
void nps64_updater_status(char *text,size_t capacity,bool *active) {
    pthread_mutex_lock(&mutex);if(text&&capacity)snprintf(text,capacity,"%s",status_text[nps64_language()]);if(active)*active=busy;pthread_mutex_unlock(&mutex);
}
void nps64_updater_info(Nps64UpdaterInfo *info) {
    if(!info)return;
    pthread_mutex_lock(&mutex);
    memset(info,0,sizeof(*info));
    info->state=current_state;
    info->busy=busy;
    info->dismissed=dismissed;
    snprintf(info->tag,sizeof(info->tag),"%s",pending_release.tag);
    snprintf(info->name,sizeof(info->name),"%s",pending_release.name);
    info->size=pending_release.size;
    info->received=received_bytes;
    info->percent=progress_percent;
    snprintf(info->status,sizeof(info->status),"%s",status_text[nps64_language()]);
    pthread_mutex_unlock(&mutex);
}
void nps64_updater_set_confirm_mode(bool enabled){pthread_mutex_lock(&mutex);confirm_mode=enabled;pthread_mutex_unlock(&mutex);}
void nps64_updater_dismiss(void){pthread_mutex_lock(&mutex);dismissed=true;pthread_mutex_unlock(&mutex);}
static bool existing(const char *path,const Nps64Release *release) {
    FILE *file=fopen(path,"rb");if(!file)return false;
    Nps64Sha256 hash;nps64_sha256_init(&hash);unsigned char buffer[32768],digest[32];size_t n;uint64_t bytes=0;
    while(!cancelled()&&(n=fread(buffer,1,sizeof(buffer),file))) {
        if((uint64_t)n>release->size-bytes){fclose(file);return false;}
        bytes+=n;nps64_sha256_update(&hash,buffer,n);
    }
    bool ok=!ferror(file)&&!cancelled()&&bytes==release->size;fclose(file);nps64_sha256_final(&hash,digest);return ok&&!memcmp(digest,release->digest,32);
}
static bool download(Nps64Http *http,const Nps64Release *release) {
    char path[256],partial[264];snprintf(path,sizeof(path),NPS64_UPDATE_DIR "/%s",release->name);snprintf(partial,sizeof(partial),"%s.part",path);
    if(mkdir(NPS64_UPDATE_DIR,0777)&&errno!=EEXIST){set_state(NPS64_UPDATER_ERROR);status("No se puede crear %s",NPS64_UPDATE_DIR);return false;}
    if(existing(path,release)){set_state(NPS64_UPDATER_READY);status("%s ya esta en %s. Lista para instalar.",release->tag,NPS64_UPDATE_DIR);return true;}
    if(cancelled())return false;
    set_state(NPS64_UPDATER_DOWNLOADING);
    int code=0;if(nps64_http_open(http,release->url,&code)<0||code!=200){set_state(NPS64_UPDATER_ERROR);status("No se pudo descargar el PKG (HTTP %d). Reintenta.",code);return false;}
    FILE *file=fopen(partial,"wb");if(!file){set_state(NPS64_UPDATER_ERROR);status("No se puede guardar en %s. Revisa espacio y permisos.",NPS64_UPDATE_DIR);return false;}
    Nps64Sha256 hash;nps64_sha256_init(&hash);unsigned char buffer[32768],digest[32];uint64_t received=0;bool ok=true;
    for(;;) {
        if(cancelled()){ok=false;break;}
        int n=nps64_http_read(http,buffer,sizeof(buffer));if(!n)break;
        if(n<0||(size_t)n>sizeof(buffer)||(uint64_t)n>release->size-received){ok=false;break;}
        if(fwrite(buffer,1,(size_t)n,file)!=(size_t)n){ok=false;break;}
        nps64_sha256_update(&hash,buffer,(size_t)n);received+=(uint64_t)n;
        unsigned pct=release->size?(unsigned)(100*received/release->size):0;
        pthread_mutex_lock(&mutex);received_bytes=received;progress_percent=pct;pthread_mutex_unlock(&mutex);
        status("Descargando %s: %u%% a %s",release->tag,pct,NPS64_UPDATE_DIR);
    }
    if(fflush(file)||fsync(fileno(file)))ok=false;
    if(fclose(file))ok=false;
    nps64_http_close(http);nps64_sha256_final(&hash,digest);
    if(!ok||received!=release->size||memcmp(digest,release->digest,32)||cancelled()) {
        unlink(partial);
        if(cancelled()){
            pthread_mutex_lock(&mutex);
            if(pending_release.size){current_state=NPS64_UPDATER_AVAILABLE;dismissed=true;}
            else current_state=NPS64_UPDATER_IDLE;
            received_bytes=0;progress_percent=0;
            pthread_mutex_unlock(&mutex);
            status("Descarga cancelada.");
        }else{
            set_state(NPS64_UPDATER_ERROR);
            status("Descarga incompleta o incorrecta. Busca actualizaciones para reintentar.");
        }
        nps64_log("[update] rejected download bytes=%llu expected=%llu cancelled=%d\n",(unsigned long long)received,(unsigned long long)release->size,cancelled());return false;
    }
    /* Only a complete verified file becomes visible to the package installer. */
    if(rename(partial,path)){unlink(partial);set_state(NPS64_UPDATER_ERROR);status("No se pudo guardar el PKG en %s",NPS64_UPDATE_DIR);return false;}
    pthread_mutex_lock(&mutex);received_bytes=received;progress_percent=100;current_state=NPS64_UPDATER_READY;pthread_mutex_unlock(&mutex);
    status("%s descargada en %s. Lista para instalar.",release->tag,NPS64_UPDATE_DIR);
    nps64_log("[update] verified package=%s bytes=%llu\n",path,(unsigned long long)received);return true;
}
static void *worker(void *unused) {
    (void)unused;Nps64Http http;char *json=NULL;Nps64Release release={0};
    nps64_log("[update] check repository=lTunix/NPS64 installed=%s\n",NPS64_CURRENT_TAG);
    if(nps64_http_init(&http)<0){set_state(NPS64_UPDATER_ERROR);status("No se pudo conectar a GitHub. Busca actualizaciones para reintentar.");goto done;}
    int code=0;if(cancelled()||nps64_http_open(&http,NPS64_RELEASES_URL,&code)<0||code!=200){set_state(NPS64_UPDATER_ERROR);status(code==403||code==429?"GitHub limita las consultas. Reintenta mas tarde.":"No se pudo consultar GitHub (HTTP %d). Reintenta.",code);goto cleanup;}
    size_t capacity=512*1024,used=0;json=malloc(capacity+1);if(!json){set_state(NPS64_UPDATER_ERROR);status("Sin memoria para consultar actualizaciones.");goto cleanup;}
    bool ok=true;for(;;){if(cancelled()){ok=false;break;}unsigned char block[32768];int n=nps64_http_read(&http,block,sizeof(block));if(!n)break;if(n<0||(size_t)n>sizeof(block)||(size_t)n>capacity-used){ok=false;break;}memcpy(json+used,block,(size_t)n);used+=(size_t)n;}
    nps64_http_close(&http);json[used]=0;
    int result=ok?nps64_release_find(json,used,NPS64_CURRENT_TAG,&release):-1;free(json);json=NULL;
    if(result<0){set_state(NPS64_UPDATER_ERROR);status("La respuesta de GitHub esta incompleta. Reintenta mas tarde.");goto cleanup;}
    if(!result){set_state(NPS64_UPDATER_UP_TO_DATE);status("Ya tienes la ultima version: %s",NPS64_VERSION);nps64_log("[update] up to date\n");goto cleanup;}
    nps64_log("[update] new release=%s package=%s bytes=%llu\n",release.tag,release.name,(unsigned long long)release.size);
    pthread_mutex_lock(&mutex);
    pending_release=release;
    bool ask_confirm=confirm_mode;
    pthread_mutex_unlock(&mutex);
    if(ask_confirm){
        char path[256];snprintf(path,sizeof(path),NPS64_UPDATE_DIR "/%s",release.name);
        if(existing(path,&release)){set_state(NPS64_UPDATER_READY);status("%s ya esta en %s. Lista para instalar.",release.tag,NPS64_UPDATE_DIR);}
        else{
            pthread_mutex_lock(&mutex);current_state=NPS64_UPDATER_AVAILABLE;dismissed=false;received_bytes=0;progress_percent=0;pthread_mutex_unlock(&mutex);
            status("Nueva version %s disponible.",release.tag);
        }
        goto cleanup;
    }
    download(&http,&release);
cleanup:
    free(json);nps64_http_term(&http);
done:
    pthread_mutex_lock(&mutex);busy=false;pthread_mutex_unlock(&mutex);return NULL;
}
static void *download_worker(void *unused) {
    (void)unused;Nps64Http http;Nps64Release release;
    pthread_mutex_lock(&mutex);release=pending_release;pthread_mutex_unlock(&mutex);
    if(nps64_http_init(&http)<0){set_state(NPS64_UPDATER_ERROR);status("No se pudo conectar a GitHub. Busca actualizaciones para reintentar.");goto done;}
    download(&http,&release);
    nps64_http_term(&http);
done:
    pthread_mutex_lock(&mutex);busy=false;pthread_mutex_unlock(&mutex);return NULL;
}
void nps64_updater_request(void) {
    pthread_mutex_lock(&mutex);bool active=busy;pthread_mutex_unlock(&mutex);if(active||__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))return;
    if(started){pthread_join(thread,NULL);started=false;}
    __atomic_store_n(&cancel_download,0,__ATOMIC_RELEASE);
    pthread_mutex_lock(&mutex);current_state=NPS64_UPDATER_CHECKING;dismissed=false;received_bytes=0;progress_percent=0;pthread_mutex_unlock(&mutex);
    status("Buscando actualizaciones en GitHub...");pthread_mutex_lock(&mutex);busy=true;pthread_mutex_unlock(&mutex);
    int result=pthread_create(&thread,NULL,worker,NULL);
    if(result){pthread_mutex_lock(&mutex);busy=false;current_state=NPS64_UPDATER_ERROR;pthread_mutex_unlock(&mutex);status("No se pudo iniciar la busqueda. Reintenta.");nps64_log("[update] thread error=%d\n",result);}else started=true;
}
void nps64_updater_confirm(void) {
    pthread_mutex_lock(&mutex);
    bool can_dl=!busy&&(current_state==NPS64_UPDATER_AVAILABLE||current_state==NPS64_UPDATER_ERROR)&&pending_release.size>0;
    pthread_mutex_unlock(&mutex);
    if(!can_dl||__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))return;
    if(started){pthread_join(thread,NULL);started=false;}
    __atomic_store_n(&cancel_download,0,__ATOMIC_RELEASE);
    pthread_mutex_lock(&mutex);current_state=NPS64_UPDATER_DOWNLOADING;dismissed=false;received_bytes=0;progress_percent=0;busy=true;pthread_mutex_unlock(&mutex);
    status("Iniciando descarga de %s...",pending_release.tag);
    int result=pthread_create(&thread,NULL,download_worker,NULL);
    if(result){pthread_mutex_lock(&mutex);busy=false;current_state=NPS64_UPDATER_ERROR;pthread_mutex_unlock(&mutex);status("No se pudo iniciar la descarga. Reintenta.");}else started=true;
}
void nps64_updater_cancel(void){__atomic_store_n(&cancel_download,1,__ATOMIC_RELEASE);}
void nps64_updater_init(void){__atomic_store_n(&stopping,0,__ATOMIC_RELEASE);__atomic_store_n(&cancel_download,0,__ATOMIC_RELEASE);nps64_updater_request();}
void nps64_updater_shutdown(void){__atomic_store_n(&stopping,1,__ATOMIC_RELEASE);if(started){pthread_join(thread,NULL);started=false;}}
