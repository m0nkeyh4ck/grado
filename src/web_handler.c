#include "web_handler.h"
#include <time.h>

int handler_index(struct mg_connection* conn, void* cbdata)
{
    (void)cbdata;
    const char* html =
        "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>Detección de Objetos - YOLOv8</title>"
        "<style>"
        "* { box-sizing: border-box; margin: 0; padding: 0; }"
        "body { background: #0b0f19; color: #e0e0e0; font-family: system-ui, -apple-system, sans-serif; "
        "       display: flex; flex-direction: column; align-items: center; justify-content: center; "
        "       min-height: 100vh; padding: 20px; }"
        ".container { width: 100%; max-width: 1080px; display: flex; flex-direction: column; "
        "              align-items: center; gap: 16px; }"
        ".video-wrap { width: 100%; aspect-ratio: 16/9; background: #000; border-radius: 12px; "
        "              overflow: hidden; box-shadow: 0 8px 32px rgba(0,0,0,0.6); border: 1px solid #1e293b; }"
        "#player { width: 100%; height: 100%; object-fit: contain; display: block; }"
        ".controls { display: flex; gap: 12px; }"
        ".btn { background: #10b981; color: #ffffff; border: none; padding: 12px 28px; font-size: 16px; "
        "       font-weight: 600; border-radius: 8px; cursor: pointer; transition: all 0.2s ease; "
        "       box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3); }"
        ".btn:hover { background: #059669; transform: translateY(-1px); }"
        ".btn.stop { background: #ef4444; box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3); }"
        ".btn.stop:hover { background: #dc2626; }"
        "</style></head><body>"
        "<div class='container'>"
        "  <div class='video-wrap'>"
        "    <video id='player' autoplay muted playsinline></video>"
        "  </div>"
        "  <div class='controls'>"
        "    <button id='btnToggle' class='btn' onclick='toggleStream()'>Iniciar Stream</button>"
        "  </div>"
        "</div>"
        "<script>"
        "const video = document.getElementById('player');"
        "const btn = document.getElementById('btnToggle');"
        "let mediaSource = null;"
        "let reader = null;"
        "let isStreaming = false;"
        "const codecList = ['video/mp4; codecs=\"avc1.4d4032\"','video/mp4; codecs=\"avc1.640028\"','video/mp4; codecs=\"avc1.4d401f\"','video/mp4; codecs=\"avc1.42e01e\"'];"
        "function pickCodec() { return codecList.find(c => MediaSource.isTypeSupported(c)) || null; }"
        "function parseBoxes(data) {"
        "  const boxes = []; let o = 0;"
        "  while (o + 8 <= data.byteLength) {"
        "    const size = (data[o] << 24) | (data[o+1] << 16) | (data[o+2] << 8) | data[o+3];"
        "    if (size < 8 || o + size > data.byteLength) break;"
        "    boxes.push(data.slice(o, o + size));"
        "    o += size;"
        "  }"
        "  return { boxes, remaining: data.slice(o) };"
        "}"
        "async function startStream() {"
        "  try {"
        "    const mime = pickCodec();"
        "    if (!mime) { alert('Codec H.264 no soportado en este navegador'); return; }"
        "    mediaSource = new MediaSource();"
        "    video.src = URL.createObjectURL(mediaSource);"
        "    mediaSource.addEventListener('sourceopen', async () => {"
        "      const sb = mediaSource.addSourceBuffer(mime);"
        "      const resp = await fetch('/mse_stream');"
        "      reader = resp.body.getReader();"
        "      let queue = [], appending = false, buffer = new Uint8Array(0);"
        "      function pump() {"
        "        if (appending || sb.updating || queue.length === 0) return;"
        "        const chunk = queue.shift(); appending = true; sb.appendBuffer(chunk);"
        "      }"
        "      sb.addEventListener('updateend', () => { appending = false; pump(); });"
        "      isStreaming = true;"
        "      btn.textContent = 'Detener Stream';"
        "      btn.classList.add('stop');"
        "      while (isStreaming) {"
        "        const { done, value } = await reader.read();"
        "        if (done) break;"
        "        if (!value) continue;"
        "        const merged = new Uint8Array(buffer.byteLength + value.byteLength);"
        "        merged.set(buffer); merged.set(value, buffer.byteLength);"
        "        const parsed = parseBoxes(merged);"
        "        buffer = parsed.remaining;"
        "        parsed.boxes.forEach(b => queue.push(b));"
        "        pump();"
        "      }"
        "    }, { once: true });"
        "  } catch(e) { stopStream(); }"
        "}"
        "function stopStream() {"
        "  isStreaming = false;"
        "  if (reader) { reader.cancel(); reader = null; }"
        "  if (video) { video.pause(); video.src = ''; }"
        "  btn.textContent = 'Iniciar Stream';"
        "  btn.classList.remove('stop');"
        "}"
        "function toggleStream() { if (isStreaming) stopStream(); else startStream(); }"
        "window.addEventListener('DOMContentLoaded', startStream);"
        "</script></body></html>";

    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
              "Content-Length: %zu\r\nConnection: close\r\n\r\n%s", strlen(html), html);
    return 200;
}

int handler_stream(struct mg_connection* conn, void* cbdata)
{
    (void)cbdata;
    printf("📺 Cliente conectado: %s\n", mg_get_request_info(conn)->remote_addr);

    mg_printf(conn, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: video/h264\r\n"
        "Cache-Control: no-cache, no-store\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n"
        "\r\n");

    int start_idx = -1;
    int wait_count = 0;
    
    while (!g_bExit && start_idx == -1 && wait_count < 500) {
        pthread_mutex_lock(&g_frame_lock);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int idx = (g_frame_write_idx - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
            if (g_frame_ring[idx].data && g_frame_ring[idx].is_keyframe) {
                start_idx = idx;
                break;
            }
        }
        if (start_idx == -1) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 50000000;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&g_frame_cond, &g_frame_lock, &ts);
            wait_count++;
        }
        pthread_mutex_unlock(&g_frame_lock);
    }

    if (start_idx == -1) return 200;

    int last_idx = start_idx;
    uint8_t *local_buf = NULL;
    size_t local_cap = 0;

    while (!g_bExit) {
        pthread_mutex_lock(&g_frame_lock);
        int next = (last_idx + 1) % BUFFER_SIZE;
        while (!g_bExit && (!g_frame_ring[next].data || next == g_frame_write_idx)) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 50000000;
            if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
            pthread_cond_timedwait(&g_frame_cond, &g_frame_lock, &ts);
            next = (last_idx + 1) % BUFFER_SIZE;
        }

        if (g_bExit) {
            pthread_mutex_unlock(&g_frame_lock);
            break;
        }

        size_t size = g_frame_ring[next].size;
        if (local_cap < size) {
            uint8_t *tmp = realloc(local_buf, size + 4096);
            if (!tmp) {
                pthread_mutex_unlock(&g_frame_lock);
                break;
            }
            local_buf = tmp;
            local_cap = size + 4096;
        }

        memcpy(local_buf, g_frame_ring[next].data, size);
        last_idx = next;
        // Liberar el mutex ANTES de enviar por socket para evitar bloquear hilos de video
        pthread_mutex_unlock(&g_frame_lock);

        int written = mg_write(conn, local_buf, size);
        if (written <= 0) break;
    }

    free(local_buf);
    return 200;
}

int handler_stats(struct mg_connection* conn, void* cbdata)
{
    (void)cbdata;
    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n\r\n"
        "{\"status\":\"ok\"}");
    return 200;
}