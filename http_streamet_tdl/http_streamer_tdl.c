#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "civetweb.h"

#include "cvi_sys.h"
#include "cvi_vb.h"
#include "cvi_vi.h"
#include "cvi_vpss.h"
#include "cvi_venc.h"
#include "cvi_comm.h"
#include "sample_comm.h"

#include "sensor_config.h"
#include "vb_pool.h"
#include "vi_init.h"
#include "vpss_init.h"
#include "venc_handler.h"
#include "fmp4_init_segment.h"
#include "mse_stream.h"
#include "web_handler.h"

volatile bool g_bExit = false;
static struct mg_context* g_servidor_http = NULL;

static void signal_handler(int sig)
{
    (void)sig;
    printf("\n🛑 Cerrando...\n");
    g_bExit = true;
    if (g_servidor_http) mg_stop(g_servidor_http);
    pthread_cond_broadcast(&g_frame_cond);
}

static void cleanup(void)
{
    printf("🧹 Limpiando recursos...\n");
    
    if (g_stServiceHandle) CVI_TDL_Service_DestroyHandle(g_stServiceHandle);
    if (g_stTDLHandle) CVI_TDL_DestroyHandle(g_stTDLHandle);
    
    CVI_VENC_StopRecvFrame(VENC_CHN);
    CVI_VENC_DestroyChn(VENC_CHN);
    
    MMF_CHN_S src = {CVI_ID_VI, 0, VI_CHN};
    MMF_CHN_S dst = {CVI_ID_VPSS, VPSS_GRP_MAIN, 0};
    CVI_SYS_UnBind(&src, &dst);
    
    CVI_VPSS_DisableChn(VPSS_GRP_MAIN, VPSS_CHN_VENC);
    CVI_VPSS_DisableChn(VPSS_GRP_MAIN, VPSS_CHN_TDL);
    CVI_VPSS_StopGrp(VPSS_GRP_MAIN);
    CVI_VPSS_DestroyGrp(VPSS_GRP_MAIN);
    
    SAMPLE_COMM_VI_DestroyIsp(&g_stViConfig);
    SAMPLE_COMM_VI_DestroyVi(&g_stViConfig);
    
    CVI_VB_Exit();
    CVI_SYS_Exit();
    
    for (int i = 0; i < BUFFER_SIZE; i++) free(g_frame_ring[i].data);
    free(g_last_sps);
    free(g_last_pps);
    CVI_TDL_Free(&g_stObjMeta);
    
    pthread_cond_destroy(&g_frame_cond);
    pthread_mutex_destroy(&g_frame_lock);
    pthread_mutex_destroy(&g_objMutex);
    
    printf("✅ Limpieza completada\n");
}

int main(int argc, char *argv[])
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║   HTTP H.264 Streamer con Detección Multi-Objeto YOLOv8       ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");

    const char *model_path = (argc >= 2) ? argv[1] : NULL;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    CVI_S32 s32Ret;
    SIZE_S stSensorSize = {0};

    s32Ret = read_sensor_config(&stSensorSize);
    if (s32Ret != CVI_SUCCESS) return -1;

    s32Ret = CVI_SYS_Init();
    if (s32Ret != CVI_SUCCESS) return -1;

    s32Ret = init_vb_pool(stSensorSize.u32Width, stSensorSize.u32Height);
    if (s32Ret != CVI_SUCCESS) { cleanup(); return -1; }

    s32Ret = init_vi();
    if (s32Ret != CVI_SUCCESS) { cleanup(); return -1; }

    s32Ret = init_vpss_2ch();
    if (s32Ret != CVI_SUCCESS) { cleanup(); return -1; }

    s32Ret = bind_vi_vpss();
    if (s32Ret != CVI_SUCCESS) { cleanup(); return -1; }

    s32Ret = init_venc();
    if (s32Ret != CVI_SUCCESS) { cleanup(); return -1; }

    init_tdl(model_path);

    const char* opts[] = {"listening_ports", "8080", "num_threads", "4", NULL};
    struct mg_callbacks cb = {0};
    g_servidor_http = mg_start(&cb, NULL, opts);
    if (!g_servidor_http) {
        cleanup();
        return -1;
    }
    mg_set_request_handler(g_servidor_http, "/", handler_index, NULL);
    mg_set_request_handler(g_servidor_http, "/stream", handler_stream, NULL);
    mg_set_request_handler(g_servidor_http, "/mse_stream", handler_stream_mse, NULL);
    mg_set_request_handler(g_servidor_http, "/stats", handler_stats, NULL);

    pthread_t venc_thread, tdl_thread;
    pthread_create(&venc_thread, NULL, run_venc_thread, NULL);
    if (g_stTDLHandle) pthread_create(&tdl_thread, NULL, run_tdl_thread, NULL);

    printf("🚀 Servidor iniciado en http://IP:8080/\n");

    while (!g_bExit) sleep(1);

    pthread_join(venc_thread, NULL);
    if (g_stTDLHandle) pthread_join(tdl_thread, NULL);
    cleanup();

    return 0;
}