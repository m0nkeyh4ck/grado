#include "vb_pool.h"

CVI_S32 init_vb_pool(CVI_U32 u32SensorW, CVI_U32 u32SensorH)
{
    printf("📦 Configurando VB Pools...\n");
    
    VB_CONFIG_S stVbConfig;
    memset(&stVbConfig, 0, sizeof(VB_CONFIG_S));
    
    // Pool 0: VI - Resolución del sensor
    stVbConfig.astCommPool[VB_POOL_VI].u32BlkSize = 
        COMMON_GetPicBufferSize(u32SensorW, u32SensorH, PIXEL_FORMAT_NV21, 
                                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConfig.astCommPool[VB_POOL_VI].u32BlkCnt = 3;
    stVbConfig.astCommPool[VB_POOL_VI].enRemapMode = VB_REMAP_MODE_CACHED;
    printf("   Pool %d (VI):      %d x %u bytes (%ux%u)\n", VB_POOL_VI, 
           stVbConfig.astCommPool[VB_POOL_VI].u32BlkCnt,
           stVbConfig.astCommPool[VB_POOL_VI].u32BlkSize,
           u32SensorW, u32SensorH);
    
    // Pool 1: VENC output (1080p)
    stVbConfig.astCommPool[VB_POOL_VENC].u32BlkSize = 
        COMMON_GetPicBufferSize(VENC_WIDTH, VENC_HEIGHT, PIXEL_FORMAT_NV21, 
                                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConfig.astCommPool[VB_POOL_VENC].u32BlkCnt = 4;
    stVbConfig.astCommPool[VB_POOL_VENC].enRemapMode = VB_REMAP_MODE_CACHED;
    printf("   Pool %d (VENC):    %d x %u bytes (%dx%d)\n", VB_POOL_VENC, 
           stVbConfig.astCommPool[VB_POOL_VENC].u32BlkCnt,
           stVbConfig.astCommPool[VB_POOL_VENC].u32BlkSize,
           VENC_WIDTH, VENC_HEIGHT);
    
    // Pool 2: TDL input (menor resolución)
    stVbConfig.astCommPool[VB_POOL_TDL].u32BlkSize = 
        COMMON_GetPicBufferSize(TDL_INPUT_WIDTH, TDL_INPUT_HEIGHT, PIXEL_FORMAT_NV21,
                                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConfig.astCommPool[VB_POOL_TDL].u32BlkCnt = 3;
    stVbConfig.astCommPool[VB_POOL_TDL].enRemapMode = VB_REMAP_MODE_CACHED;
    printf("   Pool %d (TDL):     %d x %u bytes (%dx%d)\n", VB_POOL_TDL,
           stVbConfig.astCommPool[VB_POOL_TDL].u32BlkCnt,
           stVbConfig.astCommPool[VB_POOL_TDL].u32BlkSize,
           TDL_INPUT_WIDTH, TDL_INPUT_HEIGHT);
    
    // Pool 3: TDL preprocessing (RGB_888_PLANAR)
    stVbConfig.astCommPool[VB_POOL_PREPROC].u32BlkSize = 
        COMMON_GetPicBufferSize(TDL_PREPROC_WIDTH, TDL_PREPROC_HEIGHT, 
                                PIXEL_FORMAT_RGB_888_PLANAR,
                                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConfig.astCommPool[VB_POOL_PREPROC].u32BlkCnt = 1;
    stVbConfig.astCommPool[VB_POOL_PREPROC].enRemapMode = VB_REMAP_MODE_CACHED;
    printf("   Pool %d (PREPROC): %d x %u bytes (%dx%d RGB)\n", VB_POOL_PREPROC,
           stVbConfig.astCommPool[VB_POOL_PREPROC].u32BlkCnt,
           stVbConfig.astCommPool[VB_POOL_PREPROC].u32BlkSize,
           TDL_PREPROC_WIDTH, TDL_PREPROC_HEIGHT);
    
    stVbConfig.u32MaxPoolCnt = 4;
    
    CVI_S32 s32Ret = CVI_VB_SetConfig(&stVbConfig);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VB_SetConfig failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    s32Ret = CVI_VB_Init();
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VB_Init failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    printf("✅ VB Pools configurados (4 pools)\n");
    return CVI_SUCCESS;
}