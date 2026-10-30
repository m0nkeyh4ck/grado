#include "mse_stream.h"
#include <time.h>

int send_chunk(struct mg_connection *conn, const uint8_t *data, size_t len)
{
    char header[32];
    int n = snprintf(header, sizeof(header), "%zx\r\n", len);
    if (n <= 0) return -1;
    if (mg_write(conn, header, n) <= 0) return -1;
    if (len > 0 && mg_write(conn, data, len) <= 0) return -1;
    if (mg_write(conn, "\r\n", 2) <= 0) return -1;
    return 0;
}

int handler_stream_mse(struct mg_connection* conn, void* cbdata)
{
    (void)cbdata;
    printf("📺 Cliente MSE conectado: %s\n", mg_get_request_info(conn)->remote_addr);

    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: video/mp4\r\n"
        "Cache-Control: no-cache, no-store\r\n"
        "Pragma: no-cache\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n");

    int start_idx = -1;
    int wait_count = 0;

    while (!g_bExit && start_idx == -1) {
        int have_headers = 0;
        pthread_mutex_lock(&g_frame_lock);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int idx = (g_frame_write_idx - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
            if (g_frame_ring[idx].data && g_frame_ring[idx].is_keyframe) {
                start_idx = idx;
                break;
            }
        }
        have_headers = (g_last_sps && g_last_pps);

        // Espera pasiva eficiente con pthread_cond_timedwait
        if (start_idx == -1 || !have_headers) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 50000000; // 50ms timeout
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&g_frame_cond, &g_frame_lock, &ts);
        }
        pthread_mutex_unlock(&g_frame_lock);

        if (start_idx == -1 || !have_headers) {
            wait_count++;
            if (wait_count % 20 == 1) {
                printf("   ⏳ Esperando keyframe/SPS/PPS (key=%s, sps=%s, pps=%s)\n",
                       start_idx != -1 ? "✅" : "❌",
                       have_headers ? "✅" : "❌",
                       have_headers ? "✅" : "❌");
            }
        }
    }

    if (start_idx == -1) {
        printf("   ❌ No se encontró keyframe, cerrando conexión\n");
        mg_printf(conn, "0\r\n\r\n");
        return 200;
    }

    printf("   ✅ Keyframe en idx=%d, iniciando stream MSE\n", start_idx);

    uint32_t seq_num = 1;
    uint64_t base_dts = 0;
    uint32_t sample_dur = (uint32_t)(90000 / (VENC_FPS ? VENC_FPS : 30));
    int last_sent_idx = (start_idx - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    int init_enviado = 0;
    int frag_count = 0;

    // Buffers dinámicos persistentes (evitan reallocations por frame)
    uint8_t *frame_copy = NULL;
    size_t frame_cap = 0;

    dynbuf_t avcc; dbuf_init(&avcc);
    dynbuf_t frag; dbuf_init(&frag);

    while (!g_bExit) {
        pthread_mutex_lock(&g_frame_lock);
        int next = (last_sent_idx + 1) % BUFFER_SIZE;
        while (!g_bExit && (!g_frame_ring[next].data || next == g_frame_write_idx)) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 50000000;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&g_frame_cond, &g_frame_lock, &ts);
            next = (last_sent_idx + 1) % BUFFER_SIZE;
        }

        if (g_bExit) {
            pthread_mutex_unlock(&g_frame_lock);
            break;
        }

        size_t frame_size = g_frame_ring[next].size;
        int frame_is_key = g_frame_ring[next].is_keyframe;

        if (frame_cap < frame_size) {
            uint8_t *tmp = realloc(frame_copy, frame_size + 4096);
            if (!tmp) {
                pthread_mutex_unlock(&g_frame_lock);
                break;
            }
            frame_copy = tmp;
            frame_cap = frame_size + 4096;
        }

        memcpy(frame_copy, g_frame_ring[next].data, frame_size);
        pthread_mutex_unlock(&g_frame_lock);

        parse_annexb(frame_copy, frame_size, capture_sps_pps, NULL);

        if (!init_enviado) {
            uint8_t *sps_copy = NULL;
            uint8_t *pps_copy = NULL;
            size_t sps_len = 0, pps_len = 0;

            pthread_mutex_lock(&g_frame_lock);
            if (g_last_sps && g_last_pps) {
                sps_copy = malloc(g_last_sps_len);
                pps_copy = malloc(g_last_pps_len);
                if (sps_copy && pps_copy) {
                    memcpy(sps_copy, g_last_sps, g_last_sps_len);
                    memcpy(pps_copy, g_last_pps, g_last_pps_len);
                    sps_len = g_last_sps_len;
                    pps_len = g_last_pps_len;
                } else {
                    free(sps_copy); free(pps_copy);
                    sps_copy = NULL; pps_copy = NULL;
                }
            }
            pthread_mutex_unlock(&g_frame_lock);

            if (sps_copy && pps_copy) {
                dynbuf_t init; dbuf_init(&init);
                if (build_init_segment(&init, VENC_WIDTH, VENC_HEIGHT, 90000,
                                       sps_copy, sps_len, pps_copy, pps_len) == 0) {
                    printf("   📦 Enviando init segment (%zu bytes)\n", init.size);
                    if (send_chunk(conn, init.data, init.size) == 0) {
                        init_enviado = 1;
                    } else {
                        dbuf_free(&init); free(sps_copy); free(pps_copy);
                        break;
                    }
                }
                dbuf_free(&init);
            }
            free(sps_copy);
            free(pps_copy);
        }

        if (!init_enviado) continue;

        avcc.size = 0;
        if (annexb_to_avcc(frame_copy, frame_size, &avcc) == 0) {
            frag.size = 0;
            if (build_fragment(&frag, avcc.data, avcc.size, base_dts,
                               sample_dur, frame_is_key, &seq_num) == 0) {
                frag_count++;
                if (send_chunk(conn, frag.data, frag.size) != 0) {
                    break;
                }
                base_dts += sample_dur;
                last_sent_idx = next;
            }
        }
    }

    dbuf_free(&avcc);
    dbuf_free(&frag);
    free(frame_copy);
    mg_printf(conn, "0\r\n\r\n");
    printf("👋 Cliente MSE desconectado\n");
    return 200;
}