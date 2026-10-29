#include "vi_init.h"

CVI_S32 init_vi(void)
{
    printf("📷 Iniciando VI...\n");
    
    // Completar configuración VI
    g_stViConfig.astViInfo[0].stDevInfo.ViDev = 0;
    g_stViConfig.astViInfo[0].stDevInfo.enWDRMode = WDR_MODE_NONE;
    g_stViConfig.astViInfo[0].stPipeInfo.aPipe[0] = VI_PIPE;
    g_stViConfig.astViInfo[0].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    g_stViConfig.astViInfo[0].stChnInfo.ViChn = VI_CHN;
    g_stViConfig.astViInfo[0].stChnInfo.enPixFormat = PIXEL_FORMAT_NV21;
    g_stViConfig.astViInfo[0].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
    g_stViConfig.astViInfo[0].stChnInfo.enVideoFormat = VIDEO_FORMAT_LINEAR;
    g_stViConfig.astViInfo[0].stChnInfo.enCompressMode = COMPRESS_MODE_NONE;
    
    CVI_S32 s32Ret = SAMPLE_PLAT_VI_INIT(&g_stViConfig);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ SAMPLE_PLAT_VI_INIT failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    printf("✅ VI iniciado (%ux%u)\n", g_sensor_w, g_sensor_h);
    return CVI_SUCCESS;
}