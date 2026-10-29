#ifndef VPSS_INIT_H
#define VPSS_INIT_H

#include <stdio.h>
#include <string.h>

#include "cvi_sys.h"
#include "cvi_vpss.h"
#include "sample_comm.h"

#include "sensor_config.h"
#include "vb_pool.h"
#include "vi_init.h"
#include "venc_handler.h"

#define VPSS_GRP_MAIN   0
#define VPSS_CHN_VENC   0
#define VPSS_CHN_TDL    1

CVI_S32 init_vpss_2ch(void);
CVI_S32 bind_vi_vpss(void);

#endif // VPSS_INIT_H