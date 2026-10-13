/* Background release checks and verified downloads. SPDX-License-Identifier: GPL-2.0-or-later */
#include "updater.h"
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
static bool started,busy;
static int stopping;
static char status_text[160]="Actualizaciones: sin consultar";
static bool cancelled(void){return __atomic_load_n(&stopping,__ATOMIC_ACQUIRE)!=0;}
static void status(const char *format,...) {
    va_list args;va_start(args,format);pthread_mutex_lock(&mutex);vsnprintf(status_text,sizeof(status_text),format,args);pthread_mutex_unlock(&mutex);va_end(args);
}
void nps64_updater_status(char *text,size_t capacity,bool *active) {
    pthread_mutex_lock(&mutex);if(text&&capacity)snprintf(text,capacity,"%s",status_text);if(active)*active=busy;pthread_mutex_unlock(&mutex);
}
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
    if(mkdir(NPS64_UPDATE_DIR,0777)&&errno!=EEXIST){status("No se puede crear " NPS64_UPDATE_DIR);return false;}
    if(existing(path,release)){status("%s ya esta en " NPS64_UPDATE_DIR ". Lista para instalar.",release->tag);return true;}
    if(cancelled())return false;
    int code=0;if(nps64_http_open(http,release->url,&code)<0||code!=200){status("No se pudo descargar el PKG (HTTP %d). Reintenta.",code);return false;}
    FILE *file=fopen(partial,"wb");if(!file){status("No se puede guardar en " NPS64_UPDATE_DIR ". Revisa espacio y permisos.");return false;}
    Nps64Sha256 hash;nps64_sha256_init(&hash);unsigned char buffer[32768],digest[32];uint64_t received=0;bool ok=true;
    for(;;) {
        if(cancelled()){ok=false;break;}
        int n=nps64_http_read(http,buffer,sizeof(buffer));if(!n)break;
        if(n<0||(size_t)n>sizeof(buffer)||(uint64_t)n>release->size-received){ok=false;break;}
        if(fwrite(buffer,1,(size_t)n,file)!=(size_t)n){ok=false;break;}
        nps64_sha256_update(&hash,buffer,(size_t)n);received+=(uint64_t)n;
        status("Descargando %s: %u%% a " NPS64_UPDATE_DIR,release->tag,(unsigned)(100*received/release->size));
    }
    if(fflush(file)||fsync(fileno(file)))ok=false;
    if(fclose(file))ok=false;
    nps64_http_close(http);nps64_sha256_final(&hash,digest);
    if(!ok||received!=release->size||memcmp(digest,release->digest,32)||cancelled()) {
        unlink(partial);status("Descarga incompleta o incorrecta. Busca actualizaciones para reintentar.");
        nps64_log("[update] rejected download bytes=%llu expected=%llu cancelled=%d\n",(unsigned long long)received,(unsigned long long)release->size,cancelled());return false;
    }
    /* Only a complete verified file becomes visible to the package installer. */
    if(rename(partial,path)){unlink(partial);status("No se pudo guardar el PKG en " NPS64_UPDATE_DIR);return false;}
    status("%s descargada en " NPS64_UPDATE_DIR ". Lista para instalar.",release->tag);
    nps64_log("[update] verified package=%s bytes=%llu\n",path,(unsigned long long)received);return true;
}
static void *worker(void *unused) {
    (void)unused;Nps64Http http;char *json=NULL;Nps64Release release={0};
    nps64_log("[update] check repository=lTunix/NPS64 installed=%s\n",NPS64_CURRENT_TAG);
    if(nps64_http_init(&http)<0){status("No se pudo conectar a GitHub. Busca actualizaciones para reintentar.");goto done;}
    int code=0;if(cancelled()||nps64_http_open(&http,NPS64_RELEASES_URL,&code)<0||code!=200){status(code==403||code==429?"GitHub limita las consultas. Reintenta mas tarde.":"No se pudo consultar GitHub (HTTP %d). Reintenta.",code);goto cleanup;}
    size_t capacity=512*1024,used=0;json=malloc(capacity+1);if(!json){status("Sin memoria para consultar actualizaciones.");goto cleanup;}
    bool ok=true;for(;;){if(cancelled()){ok=false;break;}unsigned char block[32768];int n=nps64_http_read(&http,block,sizeof(block));if(!n)break;if(n<0||(size_t)n>sizeof(block)||(size_t)n>capacity-used){ok=false;break;}memcpy(json+used,block,(size_t)n);used+=(size_t)n;}
    nps64_http_close(&http);json[used]=0;
    int result=ok?nps64_release_find(json,used,NPS64_CURRENT_TAG,&release):-1;free(json);json=NULL;
    if(result<0){status("La respuesta de GitHub esta incompleta. Reintenta mas tarde.");goto cleanup;}
    if(!result){status("Ya tienes la ultima version: " NPS64_VERSION);nps64_log("[update] up to date\n");goto cleanup;}
    nps64_log("[update] new release=%s package=%s bytes=%llu\n",release.tag,release.name,(unsigned long long)release.size);
    download(&http,&release);
cleanup:
    free(json);nps64_http_term(&http);
done:
    pthread_mutex_lock(&mutex);busy=false;pthread_mutex_unlock(&mutex);return NULL;
}
void nps64_updater_request(void) {
    pthread_mutex_lock(&mutex);bool active=busy;pthread_mutex_unlock(&mutex);if(active||cancelled())return;
    if(started){pthread_join(thread,NULL);started=false;}
    status("Buscando actualizaciones en GitHub...");pthread_mutex_lock(&mutex);busy=true;pthread_mutex_unlock(&mutex);
    int result=pthread_create(&thread,NULL,worker,NULL);
    if(result){pthread_mutex_lock(&mutex);busy=false;pthread_mutex_unlock(&mutex);status("No se pudo iniciar la busqueda. Reintenta.");nps64_log("[update] thread error=%d\n",result);}else started=true;
}
void nps64_updater_init(void){__atomic_store_n(&stopping,0,__ATOMIC_RELEASE);nps64_updater_request();}
void nps64_updater_shutdown(void){__atomic_store_n(&stopping,1,__ATOMIC_RELEASE);if(started){pthread_join(thread,NULL);started=false;}}
