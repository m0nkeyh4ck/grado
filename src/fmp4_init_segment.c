#include "fmp4_init_segment.h"

void dbuf_init(dynbuf_t *b)
{
    b->data = NULL;
    b->size = 0;
    b->capacity = 0;
}

void dbuf_free(dynbuf_t *b)
{
    free(b->data);
    b->data = NULL;
    b->size = 0;
    b->capacity = 0;
}

int dbuf_reserve(dynbuf_t *b, size_t extra)
{
    if (b->size + extra <= b->capacity) return 0;
    size_t new_cap = b->capacity ? b->capacity * 2 : 4096;
    while (new_cap < b->size + extra) new_cap *= 2;
    uint8_t *n = realloc(b->data, new_cap);
    if (!n) return -1;
    b->data = n;
    b->capacity = new_cap;
    return 0;
}

int dbuf_put_u8(dynbuf_t *b, uint8_t v)
{
    if (dbuf_reserve(b, 1)) return -1;
    b->data[b->size++] = v;
    return 0;
}

int dbuf_put_u16(dynbuf_t *b, uint16_t v)
{
    if (dbuf_reserve(b, 2)) return -1;
    b->data[b->size++] = (v >> 8) & 0xFF;
    b->data[b->size++] = v & 0xFF;
    return 0;
}

int dbuf_put_u32(dynbuf_t *b, uint32_t v)
{
    if (dbuf_reserve(b, 4)) return -1;
    b->data[b->size++] = (v >> 24) & 0xFF;
    b->data[b->size++] = (v >> 16) & 0xFF;
    b->data[b->size++] = (v >> 8) & 0xFF;
    b->data[b->size++] = v & 0xFF;
    return 0;
}

void dbuf_patch_u32(dynbuf_t *b, size_t offset, uint32_t v)
{
    if (offset + 4 <= b->size) {
        b->data[offset] = (v >> 24) & 0xFF;
        b->data[offset + 1] = (v >> 16) & 0xFF;
        b->data[offset + 2] = (v >> 8) & 0xFF;
        b->data[offset + 3] = v & 0xFF;
    }
}

int dbuf_put_data(dynbuf_t *b, const void *data, size_t len)
{
    if (dbuf_reserve(b, len)) return -1;
    memcpy(b->data + b->size, data, len);
    b->size += len;
    return 0;
}

int dbuf_box_start(dynbuf_t *b, const char type[4], size_t *pos)
{
    if (dbuf_put_u32(b, 0)) return -1;
    if (dbuf_put_data(b, type, 4)) return -1;
    *pos = b->size;
    return 0;
}

void dbuf_box_end(dynbuf_t *b, size_t pos)
{
    size_t box_size = b->size - pos + 8;
    dbuf_patch_u32(b, pos - 8, (uint32_t)box_size);
}

// Copia Annex-B a AVCC (longitud de 4 bytes), omitiendo SPS/PPS
static void annexb_copy_cb(const uint8_t *nal, size_t len, void *u)
{
    dynbuf_t *b = (dynbuf_t *)u;
    uint8_t type = nal[0] & 0x1F;
    if (type == 7 || type == 8) return;
    dbuf_put_u32(b, (uint32_t)len);
    dbuf_put_data(b, nal, len);
}

int annexb_to_avcc(const uint8_t *data, size_t size, dynbuf_t *out)
{
    parse_annexb(data, size, annexb_copy_cb, out);
    return out->size ? 0 : -1;
}

// Construye init segment fMP4 (ftyp+moov)
int build_init_segment(dynbuf_t *out, int width, int height, int timescale,
                       const uint8_t *sps, size_t sps_len,
                       const uint8_t *pps, size_t pps_len)
{
    if (!sps || !pps || sps_len < 4 || pps_len < 4) return -1;
    uint8_t profile_idc = sps[1];
    uint8_t constraint = sps[2];
    uint8_t level_idc = sps[3];

    size_t pos;
    dbuf_box_start(out, "ftyp", &pos);
    dbuf_put_data(out, "iso6", 4);
    dbuf_put_u32(out, 1);
    dbuf_put_data(out, "iso6", 4);
    dbuf_put_data(out, "isom", 4);
    dbuf_put_data(out, "avc1", 4);
    dbuf_box_end(out, pos);

    dbuf_box_start(out, "moov", &pos);
    size_t mvhd_pos; dbuf_box_start(out, "mvhd", &mvhd_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, (uint32_t)timescale);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0x00010000);
    dbuf_put_u16(out, 0x0100);
    dbuf_put_u16(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0x00010000); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0x00010000); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0x40000000);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 2);
    dbuf_box_end(out, mvhd_pos);

    size_t trak_pos; dbuf_box_start(out, "trak", &trak_pos);
    size_t tkhd_pos; dbuf_box_start(out, "tkhd", &tkhd_pos);
    dbuf_put_u32(out, 0x00000007);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 1);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u16(out, 0);
    dbuf_put_u16(out, 0);
    dbuf_put_u16(out, 0);
    dbuf_put_u16(out, 0);
    dbuf_put_u32(out, 0x00010000); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0x00010000); dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0x40000000);
    dbuf_put_u32(out, width << 16);
    dbuf_put_u32(out, height << 16);
    dbuf_box_end(out, tkhd_pos);

    size_t mdia_pos; dbuf_box_start(out, "mdia", &mdia_pos);
    size_t mdhd_pos; dbuf_box_start(out, "mdhd", &mdhd_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, (uint32_t)timescale);
    dbuf_put_u32(out, 0);
    dbuf_put_u16(out, 0x55c4);
    dbuf_put_u16(out, 0);
    dbuf_box_end(out, mdhd_pos);

    size_t hdlr_pos; dbuf_box_start(out, "hdlr", &hdlr_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_data(out, "vide", 4);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u8(out, 0);
    dbuf_box_end(out, hdlr_pos);

    size_t minf_pos; dbuf_box_start(out, "minf", &minf_pos);
    size_t vmhd_pos; dbuf_box_start(out, "vmhd", &vmhd_pos);
    dbuf_put_u32(out, 0x00000001);
    dbuf_put_u16(out, 0); dbuf_put_u16(out, 0); dbuf_put_u16(out, 0); dbuf_put_u16(out, 0);
    dbuf_box_end(out, vmhd_pos);

    size_t dinf_pos; dbuf_box_start(out, "dinf", &dinf_pos);
    size_t dref_pos; dbuf_box_start(out, "dref", &dref_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 1);
    size_t url_pos; dbuf_box_start(out, "url ", &url_pos);
    dbuf_put_u32(out, 0x00000001);
    dbuf_box_end(out, url_pos);
    dbuf_box_end(out, dref_pos);
    dbuf_box_end(out, dinf_pos);

    size_t stbl_pos; dbuf_box_start(out, "stbl", &stbl_pos);
    size_t stsd_pos; dbuf_box_start(out, "stsd", &stsd_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 1);
    size_t avc1_pos; dbuf_box_start(out, "avc1", &avc1_pos);
    dbuf_put_u32(out, 0); dbuf_put_u16(out, 0);
    dbuf_put_u16(out, 1);
    dbuf_put_u16(out, 0);
    dbuf_put_u16(out, 0);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_put_u16(out, (uint16_t)width);
    dbuf_put_u16(out, (uint16_t)height);
    dbuf_put_u32(out, 0x00480000);
    dbuf_put_u32(out, 0x00480000);
    dbuf_put_u32(out, 0);
    dbuf_put_u16(out, 1);
    dbuf_put_data(out, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 32);
    dbuf_put_u16(out, 0x0018);
    dbuf_put_u16(out, 0xffff);

    size_t avcc_pos; dbuf_box_start(out, "avcC", &avcc_pos);
    dbuf_put_u8(out, 1);
    dbuf_put_u8(out, profile_idc);
    dbuf_put_u8(out, constraint);
    dbuf_put_u8(out, level_idc);
    dbuf_put_u8(out, 0xFF);
    dbuf_put_u8(out, 0xE1);
    dbuf_put_u16(out, (uint16_t)sps_len);
    dbuf_put_data(out, sps, sps_len);
    dbuf_put_u8(out, 1);
    dbuf_put_u16(out, (uint16_t)pps_len);
    dbuf_put_data(out, pps, pps_len);
    dbuf_box_end(out, avcc_pos);
    dbuf_box_end(out, avc1_pos);
    dbuf_box_end(out, stsd_pos);

    size_t stts_pos; dbuf_box_start(out, "stts", &stts_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_box_end(out, stts_pos);

    size_t stsc_pos; dbuf_box_start(out, "stsc", &stsc_pos);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_box_end(out, stsc_pos);

    size_t stsz_pos; dbuf_box_start(out, "stsz", &stsz_pos);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_box_end(out, stsz_pos);

    size_t stco_pos; dbuf_box_start(out, "stco", &stco_pos);
    dbuf_put_u32(out, 0); dbuf_put_u32(out, 0);
    dbuf_box_end(out, stco_pos);

    dbuf_box_end(out, stbl_pos);
    dbuf_box_end(out, minf_pos);
    dbuf_box_end(out, mdia_pos);
    dbuf_box_end(out, trak_pos);

    size_t mvex_pos; dbuf_box_start(out, "mvex", &mvex_pos);
    size_t trex_pos; dbuf_box_start(out, "trex", &trex_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 1);
    dbuf_put_u32(out, 1);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, 0);
    dbuf_box_end(out, trex_pos);
    dbuf_box_end(out, mvex_pos);

    dbuf_box_end(out, pos);
    return 0;
}

// Construye un fragmento moof+mdat para una muestra
int build_fragment(dynbuf_t *out, const uint8_t *avcc_data, size_t avcc_len,
                   uint64_t base_dts, uint32_t sample_duration,
                   int is_keyframe, uint32_t *seq_num)
{
    size_t moof_pos; dbuf_box_start(out, "moof", &moof_pos);
    size_t mfhd_pos; dbuf_box_start(out, "mfhd", &mfhd_pos);
    dbuf_put_u32(out, 0);
    dbuf_put_u32(out, *seq_num);
    dbuf_box_end(out, mfhd_pos);

    size_t traf_pos; dbuf_box_start(out, "traf", &traf_pos);
    size_t tfhd_pos; dbuf_box_start(out, "tfhd", &tfhd_pos);
    dbuf_put_u32(out, 0x020000 | 0x000008 | 0x000010);
    dbuf_put_u32(out, 1);
    dbuf_put_u32(out, sample_duration);
    dbuf_put_u32(out, (uint32_t)avcc_len);
    dbuf_box_end(out, tfhd_pos);

    size_t tfdt_pos; dbuf_box_start(out, "tfdt", &tfdt_pos);
    dbuf_put_u32(out, 1 << 24);
    dbuf_put_u32(out, (uint32_t)(base_dts >> 32));
    dbuf_put_u32(out, (uint32_t)(base_dts & 0xffffffff));
    dbuf_box_end(out, tfdt_pos);

    size_t trun_pos; dbuf_box_start(out, "trun", &trun_pos);
    dbuf_put_u32(out, 0x000701);
    dbuf_put_u32(out, 1);
    size_t data_offset_patch = out->size;
    dbuf_put_u32(out, 0);
    uint32_t sample_flags = is_keyframe ? 0x02000000 : 0x01010000;
    dbuf_put_u32(out, sample_duration);
    dbuf_put_u32(out, (uint32_t)avcc_len);
    dbuf_put_u32(out, sample_flags);
    dbuf_box_end(out, trun_pos);

    dbuf_box_end(out, traf_pos);
    dbuf_box_end(out, moof_pos);

    uint32_t moof_size = (uint32_t)(out->size - moof_pos + 8);
    uint32_t data_offset = moof_size + 8;
    dbuf_patch_u32(out, data_offset_patch, data_offset);

    size_t mdat_pos; dbuf_box_start(out, "mdat", &mdat_pos);
    dbuf_put_data(out, avcc_data, avcc_len);
    dbuf_box_end(out, mdat_pos);

    (*seq_num)++;
    return 0;
}