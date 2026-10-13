/* Bounded GitHub release reader. SPDX-License-Identifier: GPL-2.0-or-later */
#include "update_feed.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

typedef struct { size_t start,end,next; char type; } Token;
typedef struct { const char *s; size_t len,pos,count; Token *tokens; } Parser;
#define TOKEN_CAP 32768
static void space(Parser *p) { while(p->pos<p->len && strchr(" \r\n\t",p->s[p->pos]))p->pos++; }
static int hex(char c) { if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1; }
static bool value(Parser *p,unsigned depth) {
    space(p);if(depth>32 || p->pos>=p->len || p->count>=TOKEN_CAP)return false;
    size_t id=p->count++;Token *t=&p->tokens[id];t->start=p->pos;char c=p->s[p->pos++];t->type=c;
    if(c=='{'||c=='[') {
        space(p);char close=c=='{'?'}':']';
        if(p->pos<p->len && p->s[p->pos]==close)p->pos++;
        else for(;;) {
            if(c=='{') {space(p);if(p->pos>=p->len||p->s[p->pos]!='"'||!value(p,depth+1))return false;space(p);if(p->pos>=p->len||p->s[p->pos++]!=':')return false;}
            if(!value(p,depth+1))return false;
            space(p);if(p->pos>=p->len)return false;char end=p->s[p->pos++];if(end==close)break;if(end!=',')return false;
        }
    } else if(c=='"') {
        t->start=p->pos;
        for(;;) {
            if(p->pos>=p->len)return false;
            unsigned char ch=(unsigned char)p->s[p->pos++];if(ch=='"')break;if(ch<32)return false;
            if(ch=='\\') {
                if(p->pos>=p->len)return false;char esc=p->s[p->pos++];
                if(esc=='u') {for(unsigned n=0;n<4;n++)if(p->pos>=p->len||hex(p->s[p->pos++])<0)return false;}
                else if(!strchr("\"\\/bfnrt",esc))return false;
            }
        }
        t->end=p->pos-1;t->next=p->count;return true;
    } else {
        p->pos--;size_t begin=p->pos;
        if(c=='t'||c=='f'||c=='n') {
            const char *literal=c=='t'?"true":c=='f'?"false":"null";size_t n=strlen(literal);
            if(p->len-p->pos<n||memcmp(p->s+p->pos,literal,n))return false;p->pos+=n;
        } else {
            if(p->s[p->pos]=='-')p->pos++;
            if(p->pos>=p->len)return false;
            if(p->s[p->pos]=='0')p->pos++;
            else {if(p->s[p->pos]<'1'||p->s[p->pos]>'9')return false;while(p->pos<p->len&&p->s[p->pos]>='0'&&p->s[p->pos]<='9')p->pos++;}
            if(p->pos<p->len&&p->s[p->pos]=='.') {p->pos++;size_t start=p->pos;while(p->pos<p->len&&p->s[p->pos]>='0'&&p->s[p->pos]<='9')p->pos++;if(p->pos==start)return false;}
            if(p->pos<p->len&&(p->s[p->pos]=='e'||p->s[p->pos]=='E')) {p->pos++;if(p->pos<p->len&&(p->s[p->pos]=='+'||p->s[p->pos]=='-'))p->pos++;size_t start=p->pos;while(p->pos<p->len&&p->s[p->pos]>='0'&&p->s[p->pos]<='9')p->pos++;if(p->pos==start)return false;}
        }
        t->start=begin;t->type='p';
    }
    t->end=p->pos;t->next=p->count;return true;
}
static bool equal(Parser *p,size_t id,const char *text) { return id<p->count && p->tokens[id].end-p->tokens[id].start==strlen(text)&&!memcmp(p->s+p->tokens[id].start,text,strlen(text)); }
static size_t member(Parser *p,size_t object,const char *key) {
    if(object>=p->count||p->tokens[object].type!='{')return SIZE_MAX;
    for(size_t i=object+1;i<p->tokens[object].next;) {size_t v=p->tokens[i].next;if(equal(p,i,key))return v;i=p->tokens[v].next;}
    return SIZE_MAX;
}
static bool string(Parser *p,size_t id,char *out,size_t cap) {
    if(id>=p->count||p->tokens[id].type!='"')return false;size_t n=0;
    for(size_t i=p->tokens[id].start;i<p->tokens[id].end;i++) {
        unsigned char c=(unsigned char)p->s[i];
        if(c=='\\') {c=(unsigned char)p->s[++i];if(c=='u'){unsigned num=0;for(unsigned k=0;k<4;k++)num=num*16+(unsigned)hex(p->s[++i]);if(num<32||num>126)return false;c=(unsigned char)num;}else if(c!='/'&&c!='\\'&&c!='"')return false;}
        if(c<32||c>126||n+1>=cap)return false;out[n++]=(char)c;
    }
    out[n]=0;return true;
}
static bool version(const char *s,unsigned out[3]) {
    if(*s=='v')s++;
    for(unsigned i=0;i<3;i++) {if(*s<'0'||*s>'9')return false;unsigned n=0;do {if(n>99999)return false;n=n*10+(unsigned)(*s++-'0');}while(*s>='0'&&*s<='9');out[i]=n;if(i<2&&*s++!='.')return false;}
    return !*s;
}
static int compare(const unsigned a[3],const unsigned b[3]) {for(unsigned i=0;i<3;i++)if(a[i]!=b[i])return a[i]>b[i]?1:-1;return 0;}
static bool safe_name(const char *name) {
    size_t n=strlen(name);if(n<24||strncmp(name,"NPS64-by-lTunix-",15)||strcmp(name+n-4,".pkg"))return false;
    for(size_t i=0;i<n;i++)if(!((name[i]>='a'&&name[i]<='z')||(name[i]>='A'&&name[i]<='Z')||(name[i]>='0'&&name[i]<='9')||strchr("._-",name[i])))return false;
    return strstr(name,"..")==NULL;
}
int nps64_release_find(const char *json,size_t size,const char *current,Nps64Release *release) {
    unsigned installed[3],best[3];if(!json||!release||!version(current,installed)||!size||size>512*1024)return -1;
    memcpy(best,installed,sizeof(best));Parser p={.s=json,.len=size};p.tokens=calloc(TOKEN_CAP,sizeof(Token));if(!p.tokens)return -1;
    int result=-1;bool incomplete=false;
    if(!value(&p,0))goto done;space(&p);if(p.pos!=size||p.tokens[0].type!='[')goto done;
    result=0;
    for(size_t r=1;r<p.tokens[0].next;r=p.tokens[r].next) {
        unsigned candidate[3];char tag[64];
        if(!equal(&p,member(&p,r,"draft"),"false")||!string(&p,member(&p,r,"tag_name"),tag,sizeof(tag))||!version(tag,candidate)||compare(candidate,installed)<=0)continue;
        size_t assets=member(&p,r,"assets");bool usable=false;
        if(assets<p.count&&p.tokens[assets].type=='[')for(size_t a=assets+1;a<p.tokens[assets].next;a=p.tokens[a].next) {
            Nps64Release found={0};char digest[80],number[32],expected[512];
            if(!string(&p,member(&p,a,"name"),found.name,sizeof(found.name))||!safe_name(found.name))continue;
            size_t bytes=member(&p,a,"size");if(bytes>=p.count||p.tokens[bytes].type!='p'||p.tokens[bytes].end-p.tokens[bytes].start>=sizeof(number))continue;
            size_t n=p.tokens[bytes].end-p.tokens[bytes].start;memcpy(number,json+p.tokens[bytes].start,n);number[n]=0;bool digits=n>0;for(size_t i=0;i<n;i++)if(number[i]<'0'||number[i]>'9')digits=false;if(!digits||n>9)continue;
            found.size=strtoull(number,NULL,10);if(!found.size||found.size>NPS64_MAX_PACKAGE)continue;
            if(!string(&p,member(&p,a,"browser_download_url"),found.url,sizeof(found.url))||!string(&p,member(&p,a,"digest"),digest,sizeof(digest))||strlen(digest)!=71||strncmp(digest,"sha256:",7))continue;
            bool hash=true;for(unsigned i=0;i<32;i++){int hi=hex(digest[7+i*2]),lo=hex(digest[8+i*2]);if(hi<0||lo<0){hash=false;break;}found.digest[i]=(unsigned char)((hi<<4)|lo);}if(!hash)continue;
            snprintf(expected,sizeof(expected),"https://github.com/lTunix/NPS64/releases/download/%s/%s",tag,found.name);if(strcmp(found.url,expected))continue;
            usable=true;if(compare(candidate,best)>0){snprintf(found.tag,sizeof(found.tag),"%s",tag);*release=found;memcpy(best,candidate,sizeof(best));result=1;}break;
        }
        if(!usable)incomplete=true;
    }
    if(!result&&incomplete)result=-1;
done:
    free(p.tokens);return result;
}
