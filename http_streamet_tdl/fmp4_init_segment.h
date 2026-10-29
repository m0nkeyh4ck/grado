#ifndef FMP4_INIT_SEGMENT_H
#define FMP4_INIT_SEGMENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "venc_handler.h"

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} dynbuf_t;

void dbuf_init(dynbuf_t *b);
void dbuf_free(dynbuf_t *b);
int dbuf_reserve(dynbuf_t *b, size_t extra);
int dbuf_put_u8(dynbuf_t *b, uint8_t v);
int dbuf_put_u16(dynbuf_t *b, uint16_t v);
int dbuf_put_u32(dynbuf_t *b, uint32_t v);
void dbuf_patch_u32(dynbuf_t *b, size_t offset, uint32_t v);
int dbuf_put_data(dynbuf_t *b, const void *data, size_t len);
int dbuf_box_start(dynbuf_t *b, const char type[4], size_t *pos);
void dbuf_box_end(dynbuf_t *b, size_t pos);

int annexb_to_avcc(const uint8_t *data, size_t size, dynbuf_t *out);
int build_init_segment(dynbuf_t *out, int width, int height, int timescale,
                       const uint8_t *sps, size_t sps_len,
                       const uint8_t *pps, size_t pps_len);
int build_fragment(dynbuf_t *out, const uint8_t *avcc_data, size_t avcc_len,
                   uint64_t base_dts, uint32_t sample_duration,
                   int is_keyframe, uint32_t *seq_num);

#endif // FMP4_INIT_SEGMENT_H