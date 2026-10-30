#include "vpss_init.h"

CVI_S32 init_vpss_2ch(void)
{
    printf("📺 Configurando VPSS con 2 canales...\n");
    
    CVI_S32 s32Ret;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stChnAttr;
    
    // Crear VPSS Group
    memset(&stGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
    stGrpAttr.u32MaxW = g_sensor_w;
    stGrpAttr.u32MaxH = g_sensor_h;
    stGrpAttr.enPixelFormat = PIXEL_FORMAT_NV21;
    stGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stGrpAttr.u8VpssDev = 1;  // Device 1 para 3 outputs
    
    s32Ret = CVI_VPSS_CreateGrp(VPSS_GRP_MAIN, &stGrpAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VPSS_CreateGrp failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    printf("   ✅ VPSS Grp%d creado\n", VPSS_GRP_MAIN);
    
    // Canal 0: VENC (1080p)
    memset(&stChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    stChnAttr.u32Width = VENC_WIDTH;
    stChnAttr.u32Height = VENC_HEIGHT;
    stChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stChnAttr.enPixelFormat = PIXEL_FORMAT_NV21;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;
    stChnAttr.bMirror = CVI_FALSE;
    stChnAttr.bFlip = CVI_FALSE;
    stChnAttr.u32Depth = 1;
    stChnAttr.stAspectRatio.enMode = ASPECT_RATIO_AUTO;
    stChnAttr.stAspectRatio.bEnableBgColor = CVI_TRUE;
    stChnAttr.stNormalize.bEnable = CVI_FALSE;
    
    s32Ret = CVI_VPSS_SetChnAttr(VPSS_GRP_MAIN, VPSS_CHN_VENC, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VPSS_SetChnAttr CHN0 failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    // Attach VB Pool
    s32Ret = CVI_VPSS_AttachVbPool(VPSS_GRP_MAIN, VPSS_CHN_VENC, VB_POOL_VENC);
    if (s32Ret != CVI_SUCCESS) {
        printf("⚠️  AttachVbPool CHN0 failed: 0x%x\n", s32Ret);
    }
    
    s32Ret = CVI_VPSS_EnableChn(VPSS_GRP_MAIN, VPSS_CHN_VENC);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VPSS_EnableChn CHN0 failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    printf("   ✅ CHN%d: %dx%d (VENC)\n", VPSS_CHN_VENC, VENC_WIDTH, VENC_HEIGHT);
    
    // Canal 1: TDL (menor resolución)
    memset(&stChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    stChnAttr.u32Width = TDL_INPUT_WIDTH;
    stChnAttr.u32Height = TDL_INPUT_HEIGHT;
    stChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stChnAttr.enPixelFormat = PIXEL_FORMAT_NV21;
    stChnAttr.stFrameRate.s32SrcFrameRate = -1;
    stChnAttr.stFrameRate.s32DstFrameRate = -1;
    stChnAttr.bMirror = CVI_FALSE;
    stChnAttr.bFlip = CVI_FALSE;
    stChnAttr.u32Depth = 1;
    stChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;  // Stretch para rescaling lineal
    stChnAttr.stNormalize.bEnable = CVI_FALSE;
    
    s32Ret = CVI_VPSS_SetChnAttr(VPSS_GRP_MAIN, VPSS_CHN_TDL, &stChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VPSS_SetChnAttr CHN1 failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    s32Ret = CVI_VPSS_AttachVbPool(VPSS_GRP_MAIN, VPSS_CHN_TDL, VB_POOL_TDL);
    if (s32Ret != CVI_SUCCESS) {
        printf("⚠️  AttachVbPool CHN1 failed: 0x%x\n", s32Ret);
    }
    
    s32Ret = CVI_VPSS_EnableChn(VPSS_GRP_MAIN, VPSS_CHN_TDL);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VPSS_EnableChn CHN1 failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    printf("   ✅ CHN%d: %dx%d (TDL)\n", VPSS_CHN_TDL, TDL_INPUT_WIDTH, TDL_INPUT_HEIGHT);
    
    // Iniciar VPSS Group
    s32Ret = CVI_VPSS_StartGrp(VPSS_GRP_MAIN);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VPSS_StartGrp failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    printf("✅ VPSS configurado\n");
    return CVI_SUCCESS;
}

CVI_S32 bind_vi_vpss(void)
{
    printf("🔗 Conectando VI → VPSS...\n");
    
    MMF_CHN_S stSrcChn, stDestChn;
    
    stSrcChn.enModId = CVI_ID_VI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VI_CHN;
    
    stDestChn.enModId = CVI_ID_VPSS;
    stDestChn.s32DevId = VPSS_GRP_MAIN;
    stDestChn.s32ChnId = 0;
    
    CVI_S32 s32Ret = CVI_SYS_Bind(&stSrcChn, &stDestChn);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_SYS_Bind failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    printf("✅ VI → VPSS conectados\n");
    return CVI_SUCCESS;
}