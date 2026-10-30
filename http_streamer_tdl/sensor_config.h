#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cvi_sys.h"
#include "cvi_comm.h"
#include "sample_comm.h"

// Variables globales exportadas del sensor
extern CVI_U32 g_sensor_w;
extern CVI_U32 g_sensor_h;
extern SAMPLE_VI_CONFIG_S g_stViConfig;

// Prototipos de función
CVI_S32 read_sensor_config(SIZE_S *pstSensorSize);

#endif // SENSOR_CONFIG_H