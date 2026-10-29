#ifndef VB_POOL_H
#define VB_POOL_H

#include <stdio.h>
#include <string.h>

#include "cvi_sys.h"
#include "cvi_vb.h"
#include "sample_comm.h"

#include "venc_handler.h"

// VB Pool IDs - 4 pools
#define VB_POOL_VI      0    // Pool para VI (resolución del sensor)
#define VB_POOL_VENC    1    // Pool para VENC output
#define VB_POOL_TDL     2    // Pool para TDL input
#define VB_POOL_PREPROC 3    // Pool para TDL preprocessing

CVI_S32 init_vb_pool(CVI_U32 u32SensorW, CVI_U32 u32SensorH);

#endif // VB_POOL_H