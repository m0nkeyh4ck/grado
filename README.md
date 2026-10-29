# LicheeRV Nano (SG2002) — Buildroot & Edge AI

**Undergraduate Thesis | Universidad Nacional de Colombia — Bogotá Campus**

*Implementation and Optimization of a YOLO Model on the LicheeRV Nano (SG2002) Embedded Platform for Object Detection with Replicable Documentation in Educational Environments.*

| Academic Information | Details |
|---|---|
| **Author** | Jesús Antonio López Trigozo |
| **Institution** | Universidad Nacional de Colombia |
| **Campus** | Bogotá |
| **Faculty** | Faculty of Engineering |
| **Advisor** | Carlos Iván Camargo Bareño |
| **Status** | 🟡 In Progress |

---

## 01. Project Overview

This project focuses on preparing a customized embedded Linux environment for **YOLO-based object detection** on the LicheeRV Nano, powered by the SG2002 SoC.

The current work covers system configuration, image compilation, development tools, network connectivity, and the software environment required for AI deployment on resource-constrained hardware.

**Project objectives**

- Customize and compile a Buildroot-based Linux image.
- Reduce unnecessary software and system overhead.
- Configure network connectivity and remote access.
- Prepare the CVI TDL SDK for application development.
- Integrate CivetWeb for HTTP streaming.
- Document the workflow for reproducibility in educational environments.

## 02. Hardware Platform

### LicheeRV Nano — SG2002

| Component | Specification |
|---|---|
| **SoC** | SOPHGO SG2002 |
| **Main core** | 1 GHz RISC-V C906 / ARM Cortex-A53 (selectable) |
| **Secondary core** | 700 MHz RISC-V C906 |
| **Low-power core** | 8051, 25–300 MHz |
| **Memory** | 2 Gbit (256 MB) DDR3 |
| **AI accelerator** | Integrated NPU — 1 TOPS INT8, BF16 support |
| **Storage** | TF card / SD NAND |
| **Video output** | 2-lane MIPI DSI, 31-pin interface |
| **Camera input** | 4-lane MIPI CSI, 22-pin interface |
| **Audio** | Onboard amplifier and analog microphone |
| **Wired network** | 100 Mbps Ethernet on E and WE variants |
| **Wireless network** | Dual-band Wi-Fi 6 and Bluetooth 5.4 on W and WE variants |
| **USB** | 1 × USB 2.0 OTG Type-C |
| **Expansion** | 2 × 14-pin, 2.54 mm headers |
| **Other interfaces** | RST and BOOT buttons, power and user LEDs |
| **Operating system** | Buildroot Linux / Debian |
| **Dimensions** | 22.86 × 35.56 mm |

### Board Variants

| Variant | Ethernet | Wi-Fi 6 | Bluetooth |
|---|---|---|---|
| **LicheeRV-Nano-B** | No | No | No |
| **LicheeRV-Nano-E** | Yes | No | No |
| **LicheeRV-Nano-W** | No | Yes | Yes |
| **LicheeRV-Nano-WE** | Yes | Yes | Yes |

---

## 03. Development Environment

The build environment uses Docker to isolate dependencies and provide a consistent workspace for compiling the system.

### 3.1. Clone the Build Environment

Run these commands on the **host machine** to download the source code and build the Docker image:

```bash
git clone https://github.com/sipeed/LicheeRV-Nano-Build --depth=1
cd LicheeRV-Nano-Build
git clone https://github.com/sophgo/host-tools --depth=1
cd host/ubuntu
docker build -t licheervnano-build-ubuntu .
```

### 3.2. Start the Docker Container

Run this command on the host machine, replacing the source path with the absolute path of your local repository:

```bash
docker run -v /path/to/LicheeRV-Nano-Build/:/workspace -it licheervnano-build-ubuntu /bin/bash
```

Once inside the container, use `/workspace` as the working directory for the following steps.

### 3.3. Configure and Compile Buildroot

Execute the following commands **inside the container**:

```bash
cd /workspace
source build/cvisetup.sh
defconfig sg2002_licheervnano_sd
git config --global --add safe.directory /workspace
build_all
```

---

## 04. CVI TDL SDK

The CVI TDL SDK provides sample applications and software components used during preparation of the AI deployment environment.

### 4.1. Clone the Repository

From the container's `/workspace` directory:

```bash
cd /workspace
git clone https://github.com/milkv-duo/cvitek-tdl-sdk-sg200x.git --depth=1
```

### 4.2. Compile the Samples

Add the cross-compiler to `PATH` and compile the samples:

```bash
export PATH="$PWD/host-tools/gcc/riscv64-linux-musl-x86_64/bin:$PATH"
cd cvitek-tdl-sdk-sg200x/sample
./compile_sample.sh
```

**Directory requirement:** The CVI TDL repository must be located at `/workspace/cvitek-tdl-sdk-sg200x` inside the container.

---

## 05. CivetWeb — HTTP Streaming

CivetWeb provides the embedded HTTP server dependency for the `http_streamer_tdl.c` application.

### 5.1. Clone CivetWeb

Clone version `v1.15` inside `/workspace`:

```bash
cd /workspace
git clone --branch v1.15 --depth=1 https://github.com/civetweb/civetweb.git
```

### 5.2. Directory Structure

The relevant project directories inside the container are organized as follows:

```text
/workspace/
├── build/
├── buildroot/
├── cvitek-tdl-sdk-sg200x/
└── civetweb/
    ├── include/
    │   └── civetweb.h
    └── src/
        └── civetweb.c
```

### 5.3. Configure the Application MakeFile

The application's `MakeFile` must reference CivetWeb using its path inside the container:

```makefile
CIVETWEB_DIR := /workspace/civetweb
CIVETWEB_SOURCE := $(CIVETWEB_DIR)/src/civetweb.c
CIVETWEB_INCLUDE_DIR := $(CIVETWEB_DIR)/include
```

**Note:** Cloning CivetWeb provides its source code; it does not, by itself, compile or deploy the HTTP streaming application to the board. The application must still be built with the appropriate RISC-V cross-compiler and SDK libraries.

---

## 06. System Image Deployment

### 6.1. Flash the Image to the MicroSD Card

Write the compiled image to the MicroSD card:

```bash
sudo dd if=/path/to/image.img of=/dev/sdX bs=16M status=progress conv=fdatasync
```

Replace `/path/to/image.img` with the actual image path and `/dev/sdX` with the MicroSD card identifier.

> **Important:** The image path and the MicroSD device path depend on where the flashing command is executed. Verify the target device before running `dd`, as selecting the wrong device can overwrite data on another disk.

### 6.2. Configure Wi-Fi

For the **W and WE variants**, mount the `rootfs` partition of the SD card and create or edit `/etc/wpa_supplicant.conf`:

```text
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
network={
    ssid="YOUR_WIFI_SSID"
    psk="YOUR_WIFI_PASSWORD"
}
```

Replace the placeholder values with the credentials of your wireless network.

### 6.3. Identify the Board on the Network

After inserting the MicroSD card and powering on the board, identify its IP address on the local network using `nmap`:

```bash
sudo nmap -sV -p 22 192.168.1.0/24
```

### 6.4. Connect Through SSH

Connect to the board using the default root account:

```bash
ssh root@<BOARD_IP_ADDRESS>
```

---

## 07. Current Development Scope

The repository documents the setup and configuration of the embedded Linux environment and the supporting software required for development on the SG2002 platform.

The current workflow covers:

- Buildroot configuration and system image compilation.
- Docker-based development environment.
- CVI TDL SDK setup and sample compilation.
- CivetWeb integration for HTTP streaming.
- MicroSD image deployment.
- Wireless networking and SSH access.

YOLO deployment, inference validation, and performance evaluation remain part of the ongoing thesis work.

---

## 08. Software Architecture & Thermal/Performance Optimization

The streaming application (`http_streamer_tdl`) is engineered specifically for resource-constrained SoC environments (SOPHGO SG2002 / CV181X). High performance, stable FPS, low CPU utilization, and minimal thermal dissipation are achieved through strict C-level design pattern optimizations.

### 8.1. Source File Responsibilities

| Module (`.c` / `.h`) | Primary Responsibility | Optimization Focus |
|---|---|---|
| `http_streamer_tdl` | Main entry point, signal handling, SDK hardware binding, thread lifecycle. | Clean subsystem tear-down and memory release. |
| `sensor_config` | Sensor initialization (`sensor_cfg.ini` parsing) and format negotiation. | Dynamic resolution setup matching hardware capability. |
| `vb_pool` | Allocation of hardware Video Buffer (VB) pools in DDR memory. | Zero-copy ISP/VPSS hardware buffer reuse across channels. |
| `vi_init` | Video Input (VI) pipeline setup for MIPI CSI capture. | Direct hardware DMA streaming from sensor to DDR. |
| `vpss_init` | Video Processing SubSystem configuration (scaling, pixel format conversion). | Dual-channel hardware scaling (1080p for stream, 640x480 for NPU). |
| `venc_handler` | H.264 encoding thread (`VENC`), NPU inference thread (`TDL`), and ring buffer. | Reusable memory ring buffer and NPU label overlay. |
| `fmp4_init_segment` | ISO/IEC 14496-12 MP4 container box builder (`ftyp`, `moov`, `moof`, `mdat`). | In-memory fast NAL unit Annex-B to AVCC conversion. |
| `mse_stream` | Media Source Extensions (MSE) HTTP chunked video streaming server handler. | Passive thread synchronization and static copy buffering. |
| `web_handler` | CivetWeb HTTP routes (`/`, `/stream`, `/stats`), HTML/JS web player interface. | Network socket I/O decoupled from mutex lock scopes. |

---

### 8.2. Core Optimization Strategies for Low CPU Usage & Heat Reduction

#### 1. Hardware Pipeline Offloading (Zero-CPU Video/AI Pipeline)
Instead of processing frames in software via CPU routines:
- **MIPI CSI Camera Capture (`VI`)** streams frames via DMA directly into hardware memory pools (`VB Pool`).
- **Video Process SubSystem (`VPSS`)** splits the capture into two hardware streams: Channel 0 scales frames to 1080p for encoding, while Channel 1 scales frames to 640x480 for NPU inference.
- **NPU Acceleration (`TPU / CVI_TDL`)** handles INT8 YOLOv8 tensor operations on dedicated silicon, leaving the RISC-V host CPU idling.
- **Hardware Encoder (`VENC`)** compresses raw NV21 frames directly into H.264 streams in real-time.

```text
[ Camera (MIPI CSI) ]
         │ (DMA)
         ▼
    [ VI Subsystem ]
         │ (Hardware Bind)
         ▼
    [ VPSS Group ] ───► Channel 1 (640x480)  ──► [ TPU / NPU ] (YOLOv8 INT8)
         │                                            │ (BBoxes & COCO Labels)
         ▼                                            ▼
    Channel 0 (1080p) ────────────────────────► [ VENC Engine ] (H.264 Encoder)
                                                      │ (Ring Buffer)
                                                      ▼
                                            [ CivetWeb HTTP Stream ]
```

#### 2. Zero Per-Frame Heap Allocation (Persistent Dynamic Buffering)
Executing `malloc()` and `free()` inside 30 FPS high-frequency loops causes kernel heap fragmentation, system calls overhead, thermal throttling, and CPU spikes.

To eliminate this, `venc_handler.c` and `mse_stream.c` utilize **persistent capacity-expanding buffers**. Buffers expand only when encountering larger frame sizes and reuse memory indefinitely:

```c
// Example from venc_handler.c: Ring buffer memory reuse
pthread_mutex_lock(&g_frame_lock);

int idx = g_frame_write_idx;
if (g_frame_ring[idx].capacity < total_size) {
    // Reallocate only when required by larger keyframe spikes
    uint8_t *new_ptr = realloc(g_frame_ring[idx].data, total_size + 16384);
    if (new_ptr) {
        g_frame_ring[idx].data = new_ptr;
        g_frame_ring[idx].capacity = total_size + 16384;
    }
}

// Memory is reused directly without freeing between frames
memcpy(g_frame_ring[idx].data, stStream.pstPack[i].pu8Addr, total_size);
g_frame_ring[idx].size = total_size;
```

#### 3. Passive Thread Synchronization (`pthread_cond_t` vs Polling)
Polling loops with `usleep()` consume CPU cycles during sleep-wake transitions and introduce jitter.

The streaming handlers (`mse_stream.c` and `web_handler.c`) utilize POSIX condition variables (`pthread_cond_timedwait`). When no new frame is ready, HTTP worker threads sleep passively with **0% CPU usage**, waking up instantly via `pthread_cond_broadcast()` when the `VENC` thread writes a new frame:

```c
// Example from mse_stream.c: Passive condition wait loop
pthread_mutex_lock(&g_frame_lock);
while (!g_bExit && (!g_frame_ring[next].data || next == g_frame_write_idx)) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += 50000000; // 50ms timeout guard
    if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
    
    // CPU thread yields completely until woken by the VENC encoder thread
    pthread_cond_timedwait(&g_frame_cond, &g_frame_lock, &ts);
    next = (last_sent_idx + 1) % BUFFER_SIZE;
}
```

#### 4. Mutex Decoupling from Network Socket I/O
Performing slow blocking socket writes (`mg_write`) inside critical section locks causes thread starvation and blocks video capture pipelines.

In `web_handler.c`, frame data is copied locally inside a fast mutex lock, and the lock is **released immediately prior to network transmission**:

```c
// Example from web_handler.c: Fast copy and lock release before I/O
pthread_mutex_lock(&g_frame_lock);
// ... wait for frame ...
memcpy(local_buf, g_frame_ring[next].data, size);
last_idx = next;

// Mutex lock is UNLOCKED before blocking TCP socket operations
pthread_mutex_unlock(&g_frame_lock);

// Network latency cannot block hardware VENC encoding thread
int written = mg_write(conn, local_buf, size);
```

#### 5. Efficient NPU Overlay & Label Mapping
In `venc_handler.c`, the NPU inference thread (`run_tdl_thread`) processes detection outputs and maps integer COCO class IDs (`stObjMeta.info[i].classes`) directly to an array of static class strings (`COCO_CLASSES[cls]`) along with confidence percentage scores (`stObjMeta.info[i].bbox.score`). Hardware vector drawing routines (`CVI_TDL_Service_ObjectDrawRect`) overlay bounding boxes and English labels on video frames prior to H.264 compression, requiring zero client-side browser rendering computation.

---

**Universidad Nacional de Colombia · Bogotá Campus**  
*Undergraduate Thesis — Embedded Systems and Edge AI*
