/* Native PS4 HTTPS backend. SPDX-License-Identifier: GPL-2.0-or-later */
#include "update_http.h"
#include "version.h"
#include "log.h"
#include <orbis/Sysmodule.h>
#include <orbis/Net.h>
#include <orbis/Ssl.h>
#include <orbis/Http.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
static pthread_mutex_t net_mutex=PTHREAD_MUTEX_INITIALIZER;
static unsigned net_users;
static bool net_owned;
extern int32_t update_module_loaded(enum OrbisSysModuleInternal) __asm__("sceSysmoduleIsLoadedInternal");
/* Correct signatures for SDK declarations that have unspecified arguments. */
extern int32_t update_redirect(int32_t,int32_t) __asm__("sceHttpSetAutoRedirect");
extern int32_t update_receive_timeout(int32_t,uint32_t) __asm__("sceHttpSetRecvTimeOut");
extern int32_t update_header_limit(int32_t,uint32_t) __asm__("sceHttpSetResponseHeaderMaxSize");
static int load(enum OrbisSysModuleInternal module) {
    int32_t result=update_module_loaded(module);
    if(result)result=(int32_t)sceSysmoduleLoadModuleInternal(module);
    if(result)nps64_log("[update] module 0x%08x error=0x%08x\n",(unsigned)module,(unsigned)result);
    return result;
}
int nps64_http_init(Nps64Http *c) {
    *c=(Nps64Http){.pool=-1,.ssl=-1,.http=-1,.template_id=-1,.connection=-1,.request=-1};
    if(load(ORBIS_SYSMODULE_INTERNAL_NET)||load(ORBIS_SYSMODULE_INTERNAL_SSL)||load(ORBIS_SYSMODULE_INTERNAL_HTTP))return -1;
    pthread_mutex_lock(&net_mutex);
    int result=net_users?0:sceNetInit();if(!net_users)net_owned=result==0;
    net_users++;c->own_net=1;pthread_mutex_unlock(&net_mutex);
    /* An existing network context may already be owned by another subsystem. */
    c->pool=sceNetPoolCreate("NPS64 updates",1024*1024,0);if(c->pool<0)goto fail;
    c->ssl=sceSslInit(1024*1024);if(c->ssl<0)goto fail;
    c->http=sceHttpInit(c->pool,c->ssl,1024*1024);if(c->http<0)goto fail;
    c->template_id=sceHttpCreateTemplate(c->http,"NPS64/" NPS64_VERSION " (lTunix/NPS64)",ORBIS_HTTP_VERSION_1_1,0);
    if(c->template_id<0)goto fail;
    if(update_redirect(c->template_id,1)<0 ||
       sceHttpSetResolveTimeOut(c->template_id,10000000)<0 ||
       sceHttpSetConnectTimeOut(c->template_id,10000000)<0 ||
       sceHttpSetSendTimeOut(c->template_id,10000000)<0 ||
       update_receive_timeout(c->template_id,10000000)<0 ||
       update_header_limit(c->template_id,32768)<0 ||
       /* Certificate, hostname, validity dates, known CA and SNI remain enabled. */
       sceHttpsEnableOption(c->template_id,0xbd)<0)goto fail;
    return 0;
fail:
    nps64_log("[update] HTTPS init failed pool=%d ssl=%d http=%d template=%d net=0x%08x\n",c->pool,c->ssl,c->http,c->template_id,(unsigned)result);
    nps64_http_term(c);return -1;
}
void nps64_http_close(Nps64Http *c) {
    if(c->request>=0){sceHttpDeleteRequest(c->request);c->request=-1;}
    if(c->connection>=0){sceHttpDeleteConnection(c->connection);c->connection=-1;}
}
int nps64_http_open(Nps64Http *c,const char *url,int *status) {
    nps64_http_close(c);if(strncmp(url,"https://",8))return -1;
    c->connection=sceHttpCreateConnectionWithURL(c->template_id,url,false);if(c->connection<0)goto fail;
    c->request=sceHttpCreateRequestWithURL(c->connection,ORBIS_METHOD_GET,url,0);if(c->request<0)goto fail;
    if(sceHttpAddRequestHeader(c->request,"Accept","application/vnd.github+json",0)<0 ||
       sceHttpAddRequestHeader(c->request,"Accept-Encoding","identity",0)<0 ||
       sceHttpSendRequest(c->request,NULL,0)<0 || sceHttpGetStatusCode(c->request,status)<0)goto fail;
    return 0;
fail:
    nps64_log("[update] HTTPS request failed connection=%d request=%d\n",c->connection,c->request);nps64_http_close(c);return -1;
}
int nps64_http_read(Nps64Http *c,void *data,size_t size) {return sceHttpReadData(c->request,data,(uint32_t)size);}
void nps64_http_term(Nps64Http *c) {
    nps64_http_close(c);
    if(c->template_id>=0){sceHttpDeleteTemplate(c->template_id);c->template_id=-1;}
    if(c->http>=0){sceHttpTerm(c->http);c->http=-1;}
    if(c->ssl>=0){sceSslTerm(c->ssl);c->ssl=-1;}
    if(c->pool>=0){sceNetPoolDestroy(c->pool);c->pool=-1;}
    if(c->own_net){pthread_mutex_lock(&net_mutex);if(--net_users==0&&net_owned){sceNetTerm();net_owned=false;}c->own_net=0;pthread_mutex_unlock(&net_mutex);}
}
