#ifndef VENC_HANDLER_H
#define VENC_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// SDK Headers
#include "cvi_sys.h"
#include "cvi_vpss.h"
#include "cvi_venc.h"
#include "cvi_comm.h"
#include "sample_comm.h"

#include "cvi_tdl.h"
#include "cvi_tdl_app.h"

#ifndef RESCALE_RB
#define RESCALE_RB 2
#endif

// Parámetros VENC / TDL
#define VENC_WIDTH      1920
#define VENC_HEIGHT     1080
#define VENC_FPS        30

#define TDL_INPUT_WIDTH   640
#define TDL_INPUT_HEIGHT  480

#define TDL_PREPROC_WIDTH  768
#define TDL_PREPROC_HEIGHT 1024

#define VENC_CHN        0
#define VENC_MAX_PACKS  8

// Buffer circular optimizado con capacidad reutilizable
#define BUFFER_SIZE 60
typedef struct {
    unsigned char* data;
    size_t size;
    size_t capacity;
    int is_keyframe;
    uint64_t timestamp;
} frame_buffer_t;

// Tracking
#define MAX_TRACKS 64
typedef struct {
    int id;
    float x1, y1, x2, y2;
    float cx, cy;
    int class_id;
    int hits;
    int miss;
    uint64_t last_seen_us;
} track_t;

extern volatile bool g_bExit;

#define MUTEXAUTOLOCK_INIT(mutex) pthread_mutex_t AUTOLOCK_##mutex = PTHREAD_MUTEX_INITIALIZER;
#define MutexAutoLock(mutex, lock) \
    __attribute__((cleanup(AutoUnLock))) pthread_mutex_t *lock = &AUTOLOCK_##mutex; \
    pthread_mutex_lock(lock);

inline void AutoUnLock(void *mutex) {
    pthread_mutex_unlock(*(pthread_mutex_t **)mutex);
}

extern cvtdl_object_t g_stObjMeta;
extern pthread_mutex_t g_objMutex;
extern pthread_mutex_t AUTOLOCK_ResultMutex;

extern frame_buffer_t g_frame_ring[BUFFER_SIZE];
extern int g_frame_write_idx;
extern pthread_mutex_t g_frame_lock;
extern pthread_cond_t g_frame_cond;

extern uint8_t *g_last_sps;
extern size_t g_last_sps_len;
extern uint8_t *g_last_pps;
extern size_t g_last_pps_len;

extern track_t g_tracks[MAX_TRACKS];
extern int g_track_count;
extern int g_next_track_id;
extern pthread_mutex_t g_track_lock;

extern cvitdl_handle_t g_stTDLHandle;
extern cvitdl_service_handle_t g_stServiceHandle;

CVI_S32 init_venc(void);
CVI_S32 init_tdl(const char *model_path);
void *run_venc_thread(void *arg);
void *run_tdl_thread(void *arg);

void parse_annexb(const uint8_t *data, size_t size, 
                  void (*cb)(const uint8_t *nal, size_t len, void *ud), void *ud);
void capture_sps_pps(const uint8_t *nal, size_t len, void *ud);
void update_person_tracks(const cvtdl_object_t *meta, uint64_t now_us);
float bbox_iou(const cvtdl_bbox_t *a, const cvtdl_bbox_t *b);

#endif // VENC_HANDLER_H