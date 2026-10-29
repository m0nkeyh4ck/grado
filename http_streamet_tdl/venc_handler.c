#include "venc_handler.h"
#include "vb_pool.h"
#include "vpss_init.h"

// Lista estática con las 80 clases COCO en inglés
static const char *COCO_CLASSES[80] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};

// Detección de objetos
cvtdl_object_t g_stObjMeta = {0};
pthread_mutex_t g_objMutex = PTHREAD_MUTEX_INITIALIZER;
MUTEXAUTOLOCK_INIT(ResultMutex);

// Buffer circular optimizado
frame_buffer_t g_frame_ring[BUFFER_SIZE];
int g_frame_write_idx = 0;
pthread_mutex_t g_frame_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_frame_cond = PTHREAD_COND_INITIALIZER;

// SPS/PPS para fMP4
uint8_t *g_last_sps = NULL;
size_t g_last_sps_len = 0;
uint8_t *g_last_pps = NULL;
size_t g_last_pps_len = 0;

// Tracking
track_t g_tracks[MAX_TRACKS];
int g_track_count = 0;
int g_next_track_id = 1;
pthread_mutex_t g_track_lock = PTHREAD_MUTEX_INITIALIZER;

// Handles CVI TDL
cvitdl_handle_t g_stTDLHandle = NULL;
cvitdl_service_handle_t g_stServiceHandle = NULL;

CVI_S32 init_venc(void)
{
    printf("🎬 Iniciando VENC H.264...\n");
    
    VENC_CHN_ATTR_S stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    
    stVencChnAttr.stVencAttr.enType = PT_H264;
    stVencChnAttr.stVencAttr.u32MaxPicWidth = VENC_WIDTH;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = VENC_HEIGHT;
    stVencChnAttr.stVencAttr.u32PicWidth = VENC_WIDTH;
    stVencChnAttr.stVencAttr.u32PicHeight = VENC_HEIGHT;
    stVencChnAttr.stVencAttr.u32BufSize = VENC_WIDTH * VENC_HEIGHT * 3 / 2;
    stVencChnAttr.stVencAttr.u32Profile = H264E_PROFILE_HIGH;
    stVencChnAttr.stVencAttr.bByFrame = CVI_TRUE;
    
    stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    stVencChnAttr.stRcAttr.stH264Cbr.u32Gop = VENC_FPS * 2;
    stVencChnAttr.stRcAttr.stH264Cbr.u32StatTime = 2;
    stVencChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRate = VENC_FPS;
    stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRate = VENC_FPS;
    stVencChnAttr.stRcAttr.stH264Cbr.u32BitRate = 4000;
    
    stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;
    
    CVI_S32 s32Ret = CVI_VENC_CreateChn(VENC_CHN, &stVencChnAttr);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VENC_CreateChn failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = -1;
    s32Ret = CVI_VENC_StartRecvFrame(VENC_CHN, &stRecvParam);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ CVI_VENC_StartRecvFrame failed: 0x%x\n", s32Ret);
        CVI_VENC_DestroyChn(VENC_CHN);
        return s32Ret;
    }
    
    CVI_VENC_RequestIDR(VENC_CHN, CVI_TRUE);
    
    printf("✅ VENC H.264 iniciado (%dx%d @ %d FPS)\n", VENC_WIDTH, VENC_HEIGHT, VENC_FPS);
    return CVI_SUCCESS;
}

CVI_S32 init_tdl(const char *model_path)
{
    printf("🤖 Iniciando TDL SDK...\n");
    CVI_S32 s32Ret;
    
    s32Ret = CVI_TDL_CreateHandle2(&g_stTDLHandle, 1, 0);
    if (s32Ret != CVI_SUCCESS) return s32Ret;
    
    CVI_TDL_SetVBPool(g_stTDLHandle, 0, VB_POOL_PREPROC);
    CVI_TDL_SetVpssTimeout(g_stTDLHandle, 1000);
    
    s32Ret = CVI_TDL_Service_CreateHandle(&g_stServiceHandle, g_stTDLHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_TDL_DestroyHandle(g_stTDLHandle);
        g_stTDLHandle = NULL;
        return s32Ret;
    }
    
    if (model_path && strlen(model_path) > 0) {
        s32Ret = CVI_TDL_OpenModel(g_stTDLHandle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, model_path);
        if (s32Ret == CVI_SUCCESS) {
            CVI_TDL_SetModelThreshold(g_stTDLHandle, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION, 0.5);
            printf("   ✅ Modelo YOLOv8 cargado con éxito\n");
        }
    }
    
    return CVI_SUCCESS;
}

void *run_tdl_thread(void *arg)
{
    (void)arg;
    printf("🧠 TDL Thread iniciado (Detección COCO 80 Clases)\n");
    
    VIDEO_FRAME_INFO_S stFrame;
    cvtdl_object_t stObjMeta = {0};
    CVI_S32 s32Ret;
    
    while (!g_bExit) {
        s32Ret = CVI_VPSS_GetChnFrame(VPSS_GRP_MAIN, VPSS_CHN_TDL, &stFrame, 1000);
        if (s32Ret != CVI_SUCCESS) {
            usleep(10000);
            continue;
        }
        
        s32Ret = CVI_TDL_YOLOV8_Detection(g_stTDLHandle, &stFrame, &stObjMeta);
        
        if (s32Ret == CVI_SUCCESS) {
            stObjMeta.width = VENC_WIDTH;
            stObjMeta.height = VENC_HEIGHT;
            stObjMeta.rescale_type = RESCALE_RB;
            
            float scale_x = (float)VENC_WIDTH / (float)TDL_INPUT_WIDTH;
            float scale_y = (float)VENC_HEIGHT / (float)TDL_INPUT_HEIGHT;
            
            for (uint32_t i = 0; i < stObjMeta.size; i++) {
                // Escalar coordenadas al tamaño de salida (1080p)
                stObjMeta.info[i].bbox.x1 *= scale_x;
                stObjMeta.info[i].bbox.y1 *= scale_y;
                stObjMeta.info[i].bbox.x2 *= scale_x;
                stObjMeta.info[i].bbox.y2 *= scale_y;

                // En la estructura CVI_TDL, la clase está en 'classes' y la certeza en 'bbox.score'
                int cls = stObjMeta.info[i].classes;
                if (cls >= 0 && cls < 80) {
                    snprintf(stObjMeta.info[i].name, sizeof(stObjMeta.info[i].name),
                             "%s %.0f%%", COCO_CLASSES[cls], stObjMeta.info[i].bbox.score * 100.0f);
                }
            }
            
            MutexAutoLock(ResultMutex, lock);
            CVI_TDL_Free(&g_stObjMeta);
            CVI_TDL_CopyObjectMeta(&stObjMeta, &g_stObjMeta);
        }
        
        CVI_TDL_Free(&stObjMeta);
        CVI_VPSS_ReleaseChnFrame(VPSS_GRP_MAIN, VPSS_CHN_TDL, &stFrame);
    }
    
    pthread_exit(NULL);
}

void *run_venc_thread(void *arg)
{
    (void)arg;
    printf("🎬 VENC Thread iniciado\n");
    
    VIDEO_FRAME_INFO_S stFrame;
    VENC_STREAM_S stStream;
    VENC_PACK_S stPacks[VENC_MAX_PACKS];
    cvtdl_object_t stObjDraw = {0};
    CVI_S32 s32Ret;
    
    memset(&stStream, 0, sizeof(VENC_STREAM_S));
    stStream.pstPack = stPacks;
    
    while (!g_bExit) {
        s32Ret = CVI_VPSS_GetChnFrame(VPSS_GRP_MAIN, VPSS_CHN_VENC, &stFrame, 1000);
        if (s32Ret != CVI_SUCCESS) {
            usleep(10000);
            continue;
        }
        
        {
            MutexAutoLock(ResultMutex, lock);
            CVI_TDL_Free(&stObjDraw);
            CVI_TDL_CopyObjectMeta(&g_stObjMeta, &stObjDraw);
        }
        
        // Dibujar cajas y etiquetas de texto antes de codificar H.264
        if (stObjDraw.size > 0 && g_stServiceHandle) {
            CVI_TDL_Service_ObjectDrawRect(g_stServiceHandle, &stObjDraw, &stFrame, 
                                           CVI_TRUE, CVI_TDL_Service_GetDefaultBrush());
        }
        
        s32Ret = CVI_VENC_SendFrame(VENC_CHN, &stFrame, 1000);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VPSS_ReleaseChnFrame(VPSS_GRP_MAIN, VPSS_CHN_VENC, &stFrame);
            continue;
        }
        
        memset(stPacks, 0, sizeof(stPacks));
        s32Ret = CVI_VENC_GetStream(VENC_CHN, &stStream, 1000);
        
        if (s32Ret == CVI_SUCCESS && stStream.u32PackCount > 0) {
            size_t total_size = 0;
            for (CVI_U32 i = 0; i < stStream.u32PackCount; i++) {
                total_size += stStream.pstPack[i].u32Len;
            }
            
            int is_keyframe = 0;
            for (CVI_U32 i = 0; i < stStream.u32PackCount; i++) {
                if (stStream.pstPack[i].DataType.enH264EType == H264E_NALU_ISLICE ||
                    stStream.pstPack[i].DataType.enH264EType == H264E_NALU_IDRSLICE ||
                    stStream.pstPack[i].DataType.enH264EType == H264E_NALU_SPS ||
                    stStream.pstPack[i].DataType.enH264EType == H264E_NALU_PPS) {
                    is_keyframe = 1;
                    break;
                }
            }
            
            pthread_mutex_lock(&g_frame_lock);
            
            int idx = g_frame_write_idx;
            if (g_frame_ring[idx].capacity < total_size) {
                uint8_t *new_ptr = realloc(g_frame_ring[idx].data, total_size + 16384);
                if (new_ptr) {
                    g_frame_ring[idx].data = new_ptr;
                    g_frame_ring[idx].capacity = total_size + 16384;
                }
            }
            
            if (g_frame_ring[idx].data) {
                size_t offset = 0;
                for (CVI_U32 i = 0; i < stStream.u32PackCount; i++) {
                    memcpy(g_frame_ring[idx].data + offset, 
                           stStream.pstPack[i].pu8Addr, 
                           stStream.pstPack[i].u32Len);
                    offset += stStream.pstPack[i].u32Len;
                }
                g_frame_ring[idx].size = total_size;
                g_frame_ring[idx].is_keyframe = is_keyframe;
                g_frame_ring[idx].timestamp = (uint64_t)time(NULL) * 1000000;
                
                parse_annexb(g_frame_ring[idx].data, total_size, capture_sps_pps, NULL);
                g_frame_write_idx = (g_frame_write_idx + 1) % BUFFER_SIZE;
                
                pthread_cond_broadcast(&g_frame_cond);
            }
            
            pthread_mutex_unlock(&g_frame_lock);
            CVI_VENC_ReleaseStream(VENC_CHN, &stStream);
        }
        
        CVI_VPSS_ReleaseChnFrame(VPSS_GRP_MAIN, VPSS_CHN_VENC, &stFrame);
    }
    
    CVI_TDL_Free(&stObjDraw);
    pthread_exit(NULL);
}

void parse_annexb(const uint8_t *data, size_t size, 
                  void (*cb)(const uint8_t *nal, size_t len, void *ud), void *ud)
{
    size_t i = 0;
    while (i + 3 < size) {
        size_t start = SIZE_MAX;
        if (data[i] == 0 && data[i+1] == 0 && data[i+2] == 1) {
            start = i + 3;
        } else if (i + 4 < size && data[i]==0 && data[i+1]==0 && data[i+2]==0 && data[i+3]==1) {
            start = i + 4;
        }
        if (start == SIZE_MAX) { i++; continue; }
        
        size_t next = start;
        int found = 0;
        while (next + 3 <= size) {
            if (data[next]==0 && data[next+1]==0 && 
                (data[next+2]==1 || (next + 3 < size && data[next+2]==0 && data[next+3]==1))) {
                found = 1;
                break;
            }
            next++;
        }
        size_t nal_len = found ? (next - start) : (size - start);
        if (nal_len > 0) cb(data + start, nal_len, ud);
        i = found ? next : size;
    }
}

void capture_sps_pps(const uint8_t *nal, size_t len, void *ud)
{
    (void)ud;
    uint8_t nal_type = nal[0] & 0x1F;
    if (nal_type == 7) {
        if (g_last_sps_len != len || !g_last_sps || memcmp(g_last_sps, nal, len) != 0) {
            free(g_last_sps);
            g_last_sps = malloc(len);
            if (g_last_sps) { memcpy(g_last_sps, nal, len); g_last_sps_len = len; }
        }
    } else if (nal_type == 8) {
        if (g_last_pps_len != len || !g_last_pps || memcmp(g_last_pps, nal, len) != 0) {
            free(g_last_pps);
            g_last_pps = malloc(len);
            if (g_last_pps) { memcpy(g_last_pps, nal, len); g_last_pps_len = len; }
        }
    }
}