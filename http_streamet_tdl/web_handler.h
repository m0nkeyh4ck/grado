#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "civetweb.h"
#include "venc_handler.h"

int handler_index(struct mg_connection* conn, void* cbdata);
int handler_stream(struct mg_connection* conn, void* cbdata);
int handler_stats(struct mg_connection* conn, void* cbdata);

#endif // WEB_HANDLER_H