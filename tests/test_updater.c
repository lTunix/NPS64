/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "updater.h"
#include "update_feed.h"
#include "update_http.h"
#include "sha256.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#define DIR "/tmp/nps64-update-test"
#define NAME "NPS64-by-lTunix-00.69.pkg"
#define TARGET DIR "/" NAME
enum { NORMAL, OFFLINE, TRUNCATED, CORRUPT, OVERSIZE, LIMITED, MISSING, CANCEL, BAD_JSON };
static int mode,package_requests;
static unsigned char package[65537];
static char feed[4096];
static void digest_text(const unsigned char *digest,char out[65]){for(unsigned i=0;i<32;i++)sprintf(out+i*2,"%02x",digest[i]);}
static void make_feed(void) {
    Nps64Sha256 hash;unsigned char digest[32];char hex[65];nps64_sha256_init(&hash);nps64_sha256_update(&hash,package,sizeof(package));nps64_sha256_final(&hash,digest);digest_text(digest,hex);
    snprintf(feed,sizeof(feed),"[{\"tag_name\":\"v0.69.0\",\"draft\":false,\"prerelease\":true,\"body\":\"test \\u0061 \\\"text\\\"\",\"assets\":[{\"name\":\"%s\",\"size\":%zu,\"digest\":\"sha256:%s\",\"browser_download_url\":\"https://github.com/lTunix/NPS64/releases/download/v0.69.0/%s\"}]}]",NAME,sizeof(package),hex,NAME);
}
void nps64_log(const char *format,...){(void)format;}
int nps64_http_init(Nps64Http *client){memset(client,0,sizeof(*client));return mode==OFFLINE?-1:0;}
int nps64_http_open(Nps64Http *client,const char *url,int *status){client->request=!strcmp(url,NPS64_RELEASES_URL)?0:1;client->connection=0;if(client->request)package_requests++;*status=mode==LIMITED?429:mode==MISSING&&client->request?404:200;return 0;}
int nps64_http_read(Nps64Http *client,void *bytes,size_t capacity) {
    size_t offset=(size_t)client->connection;
    if(!client->request){const char *body=mode==BAD_JSON?"<html>error</html>":feed;size_t n=strlen(body)-offset;if(n>capacity)n=capacity;memcpy(bytes,body+offset,n);client->connection+=(int)n;return (int)n;}
    if(mode==CANCEL)usleep(50000);
    size_t total=sizeof(package)-(mode==TRUNCATED?1:0)+(mode==OVERSIZE?1:0);size_t n=total-offset;if(n>capacity)n=capacity;
    for(size_t i=0;i<n;i++)((unsigned char*)bytes)[i]=package[(offset+i)%sizeof(package)];
    if(mode==CORRUPT&&offset==0&&n)((unsigned char*)bytes)[0]^=1;
    client->connection+=(int)n;return (int)n;
}
void nps64_http_close(Nps64Http *client){(void)client;}
void nps64_http_term(Nps64Http *client){(void)client;}
static void wait_done(char text[160]) {
    for(unsigned i=0;i<5000;i++){bool active;nps64_updater_status(text,160,&active);if(!active)return;usleep(1000);}assert(!"updater timed out");
}
static void run(int test_mode,const char *expected,bool file_expected) {
    mode=test_mode;package_requests=0;nps64_updater_init();char text[160];wait_done(text);nps64_updater_shutdown();assert(strstr(text,expected));
    assert((access(TARGET,F_OK)==0)==file_expected);assert(access(TARGET ".part",F_OK)!=0);
}
static void sha_vectors(void) {
    static const char *answers[]={"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"};
    for(unsigned i=0;i<3;i++){Nps64Sha256 hash;unsigned char digest[32];char text[65];nps64_sha256_init(&hash);if(i==1){nps64_sha256_update(&hash,"a",1);nps64_sha256_update(&hash,"bc",2);}if(i==2)for(unsigned n=0;n<1000000;n++)nps64_sha256_update(&hash,"a",1);nps64_sha256_final(&hash,digest);digest_text(digest,text);assert(!strcmp(text,answers[i]));}
}
static void feed_cases(void) {
    Nps64Release release;assert(nps64_release_find(feed,strlen(feed),"v0.68.0",&release)==1);assert(!strcmp(release.name,NAME));assert(release.size==sizeof(package));
    assert(nps64_release_find(feed,strlen(feed),"v0.69.0",&release)==0);assert(nps64_release_find(feed,strlen(feed),"v0.100.0",&release)==0);
    assert(nps64_release_find(feed,strlen(feed)-1,"v0.68.0",&release)==-1);
    const char *bad[]={"", "{}", "[{}", "[true,]", "[01]", "[\"\\uZZZZ\"]", "[\"bad\ntext\"]", "[] trailing", "[1.]"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)assert(nps64_release_find(bad[i],strlen(bad[i]),"v0.68.0",&release)==-1);
    char modified[4096];snprintf(modified,sizeof(modified),"%s",feed);char *pos=strstr(modified,"github.com/lTunix");pos[0]='x';assert(nps64_release_find(modified,strlen(modified),"v0.68.0",&release)==-1);
    snprintf(modified,sizeof(modified),"%s",feed);pos=strstr(modified,"sha256:");pos[7]='x';assert(nps64_release_find(modified,strlen(modified),"v0.68.0",&release)==-1);
    snprintf(modified,sizeof(modified),"%s",feed);pos=strstr(modified,"NPS64-by-lTunix-");pos[15]='/';assert(nps64_release_find(modified,strlen(modified),"v0.68.0",&release)==-1);
    /* Drafts cannot cause downloads; public prereleases above deliberately do. */
    const char *draft="[{\"tag_name\":\"v9.0.0\",\"draft\":true,\"assets\":[]}]";assert(nps64_release_find(draft,strlen(draft),"v0.68.0",&release)==0);
    const char *invalid="[{\"tag_name\":\"v0.69.0\",\"draft\":false,\"assets\":[]}]";assert(nps64_release_find(invalid,strlen(invalid),"v0.68.0",&release)==-1);
}
int main(void) {
    sha_vectors();for(size_t i=0;i<sizeof(package);i++)package[i]=(unsigned char)(i*37);make_feed();feed_cases();mkdir(DIR,0777);unlink(TARGET);unlink(TARGET ".part");
    run(OFFLINE,"conectar",false);run(LIMITED,"limita",false);run(BAD_JSON,"incompleta",false);run(MISSING,"descargar",false);
    run(TRUNCATED,"incorrecta",false);run(CORRUPT,"incorrecta",false);run(OVERSIZE,"incorrecta",false);
    run(NORMAL,"Lista para instalar",true);assert(package_requests==1);run(NORMAL,"ya esta",true);assert(package_requests==0);
    FILE *file=fopen(TARGET,"r+b");assert(file);fputc(99,file);fclose(file);run(NORMAL,"descargada",true);assert(package_requests==1);unlink(TARGET);
    mkdir(TARGET,0777);mode=NORMAL;nps64_updater_init();char text[160];wait_done(text);nps64_updater_shutdown();assert(strstr(text,"guardar"));assert(access(TARGET ".part",F_OK)!=0);rmdir(TARGET);
    mode=CANCEL;nps64_updater_init();for(unsigned i=0;i<5000;i++){nps64_updater_status(text,sizeof(text),NULL);if(strstr(text,"Descargando"))break;usleep(1000);}assert(strstr(text,"Descargando"));nps64_updater_request();nps64_updater_shutdown();assert(access(TARGET,F_OK)!=0);assert(access(TARGET ".part",F_OK)!=0);
    /* Current versions report correctly, without creating an installable file. */
    snprintf(feed,sizeof(feed),"[]");run(NORMAL,"ultima version",false);
    rmdir(DIR);puts("PASS: versions, prereleases, hostile/truncated JSON, SHA-256 vectors, offline/rate limits, verified download, reuse, corruption, oversize, rename failure, cancellation");return 0;
}
