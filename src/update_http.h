/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NPS64_UPDATE_HTTP_H
#define NPS64_UPDATE_HTTP_H
#include <stddef.h>
typedef struct { int pool,ssl,http,template_id,connection,request; int own_net; } Nps64Http;
int nps64_http_init(Nps64Http *client);
int nps64_http_open(Nps64Http *client,const char *url,int *status);
int nps64_http_read(Nps64Http *client,void *data,size_t size);
void nps64_http_close(Nps64Http *client);
void nps64_http_term(Nps64Http *client);
#endif
