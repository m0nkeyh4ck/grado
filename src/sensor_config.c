#include "sensor_config.h"

// Configuración de video (se llena después de leer sensor)
CVI_U32 g_sensor_w = 1920;
CVI_U32 g_sensor_h = 1080;

// VI Config (global para cleanup e inicialización)
SAMPLE_VI_CONFIG_S g_stViConfig = {0};

CVI_S32 read_sensor_config(SIZE_S *pstSensorSize)
{
    printf("📋 Leyendo configuración del sensor...\n");
    
    SAMPLE_INI_CFG_S stIniCfg;
    CVI_S32 s32Ret;
    
    memset(&stIniCfg, 0, sizeof(SAMPLE_INI_CFG_S));
    memset(&g_stViConfig, 0, sizeof(SAMPLE_VI_CONFIG_S));

    // Buscar sensor_cfg.ini
    const char *ini_paths[] = {
        "/mnt/data/sensor_cfg.ini",
        "/mnt/system/usr/bin/sensor_cfg.ini",
        NULL,
    };
    
    const char *found_ini = NULL;
    for (int i = 0; ini_paths[i]; i++) {
        if (access(ini_paths[i], R_OK) == 0) {
            found_ini = ini_paths[i];
            SAMPLE_COMM_SNS_SetIniPath(ini_paths[i]);
            printf("   ✅ Encontrado: %s\n", ini_paths[i]);
            break;
        }
    }
    
    if (!found_ini) {
        printf("❌ No se encontró sensor_cfg.ini\n");
        return CVI_FAILURE;
    }
    
    // Parsear ini
    s32Ret = SAMPLE_COMM_SNS_ParseIni(&stIniCfg);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ SAMPLE_COMM_SNS_ParseIni failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    printf("   ✅ INI parseado\n");

    // Convertir a VI config
    s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &g_stViConfig);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ SAMPLE_COMM_VI_IniToViCfg failed: 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    if (g_stViConfig.s32WorkingViNum <= 0) {
        printf("❌ No hay VI configurado en el ini\n");
        return CVI_FAILURE;
    }
    printf("   ✅ VI Config: %d dispositivo(s)\n", g_stViConfig.s32WorkingViNum);
    
    // Obtener tamaño del sensor
    PIC_SIZE_E enPicSize;
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(
        g_stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ Cannot get sensor size\n");
        return s32Ret;
    }
    
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, pstSensorSize);
    if (s32Ret != CVI_SUCCESS) {
        printf("❌ SAMPLE_COMM_SYS_GetPicSize failed\n");
        return s32Ret;
    }
    
    g_sensor_w = pstSensorSize->u32Width;
    g_sensor_h = pstSensorSize->u32Height;
    
    printf("   📷 Sensor: %dx%d\n", g_sensor_w, g_sensor_h);
    
    return CVI_SUCCESS;
}