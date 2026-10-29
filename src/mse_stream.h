#ifndef MSE_STREAM_H
#define MSE_STREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "civetweb.h"
#include "venc_handler.h"
#include "fmp4_init_segment.h"

int send_chunk(struct mg_connection *conn, const uint8_t *data, size_t len);
int handler_stream_mse(struct mg_connection* conn, void* cbdata);

#endif // MSE_STREAM_H