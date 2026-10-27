# Implementation and Customization of Buildroot on the LicheeRV Nano (SG2002)

Official repository for the undergraduate thesis: *"Implementation and optimization of a YOLO model on the LicheeRV Nano (SG2002) embedded platform for object detection with replicable documentation in educational environments"*, presented by **Jesús Antonio López Trigozo** to the Faculty of Engineering at the **Universidad Nacional de Colombia (Bogotá Campus)**, under the direction of professor Carlos Iván Camargo Bareño.

> **Project Status:** 🟢 In Progress (Custom build environment and OS configuration stage).

---

## 📌 1. Introduction and Justification

Deploying custom Linux environments on resource-constrained hardware like the LicheeRV Nano (SoC SG2002) often encounters a lack of clear, reproducible technical guides. Official pre-compiled images tend to be bloated (~1.2 GB) with unnecessary packages and unused network drivers. This repository provides a clean, step-by-step methodology to compile Buildroot from source using Docker, strip out unnecessary software, flash the optimized image, and configure wireless connectivity from scratch as part of an open academic framework for Edge AI.

---

## 🛠️ 2. Technical Specifications and Versions

### General Specifications Table

| **Element** | **Specification** |
|---|---|
| **CPU** | SOPHGO SG2002<br>• Major core: 1GHz RISC-V C906 / ARM A53 (selectable)<br>• Minor core: 700MHz RISC-V C906<br>• Low-power core: 25–300M 8051 |
| **Memory** | Integrated 2Gbit (256MByte) DDR3 |
| **Storage** | Bootable from TF card / SD NAND (SD NAND pads located under the TF card slot) |
| **AI Acceleration** | Integrated NPU delivering **1 TOPS INT8** (supports BF16) |
| **Video Interface** | • Output: 2-lane MIPI DSI (31-pin standard interface, supports 6-pin capacitive touchscreen)<br>• Input: 4-lane MIPI CSI (22-pin interface, supports dual CSI split) |
| **Audio Interface** | • Output: Onboard PA amplifier (direct speaker connection <1W)<br>• Input: Onboard analog silicon microphone |
| **Wired Connection** | **E** suffix version includes a 100M RJ45 connector |
| **Wireless Connection** | **W** suffix version includes dual-band 2.4G / 5.8G Wi-Fi 6 + BLE 5.4 |
| **USB / GPIO / Others** | 1 x USB 2.0 OTG Type-C \| 2 x 14-pin 2.54 mm headers \| RST & BOOT buttons \| Power & User LEDs |
| **Operating System** | Buildroot Linux / Debian |
| **Dimensions** | 22.86 x 35.56 mm |

### Variant Comparison

| **Version** | **Base System** | **Mic / Speaker** | **Ethernet** | **Wi-Fi 6 / BT5** |
|---|---|---|---|---|
| **LicheeRV-Nano-B** | Yes | Yes | No | No |
| **LicheeRV-Nano-E** | Yes | Yes | Yes | No |
| **LicheeRV-Nano-W** | Yes | Yes | No | Yes |
| **LicheeRV-Nano-WE** | Yes | Yes | Yes | Yes |

---

## 🚀 3. Step-by-Step Replication Guide

### Custom Buildroot Compilation and OS Preparation

Compiling from source allows you to build a lightweight system tailored specifically for your needs, avoiding host dependency conflicts by using a Docker container:

1. **Download the source code and build the Docker container:**

   ```bash
   git clone https://github.com/sipeed/LicheeRV-Nano-Build --depth=1
   cd LicheeRV-Nano-Build
   git clone https://github.com/sophgo/host-tools --depth=1
   cd host/ubuntu
   docker build -t licheervnano-build-ubuntu .
   ```

2. **Start the Docker container:**

   ```bash
   docker run -v /path/to/LicheeRV-Nano-Build/:/workspace -it licheervnano-build-ubuntu /bin/bash
   ```

   *(Note: Replace `/path/to/LicheeRV-Nano-Build/` with the absolute path on your host machine).*

3. **Configure and assemble the image:** Inside the container terminal, run:

   ```bash
   cd /workspace
   source build/cvisetup.sh
   defconfig sg2002_licheervnano_sd
   git config --global --add safe.directory /workspace
   build_all
   ```

4. **Clone the CVI TDL repository and compile the samples:**

   From the `LicheeRV-Nano-Build` directory, clone the repository:

   ```bash
   git clone https://github.com/milkv-duo/cvitek-tdl-sdk-sg200x.git --depth=1
   ```

   Enter the sample directory and add the cross-compiler to `PATH`:

   ```bash
   export PATH="$PWD/host-tools/gcc/riscv64-linux-musl-x86_64/bin:$PATH"
   cd cvitek-tdl-sdk-sg200x/sample
   ./compile_sample.sh
   ```

   The CVI TDL repository must be located inside `LicheeRV-Nano-Build/`. Its absolute location on your computer does not matter, provided the repository is cloned into that directory.

5. **Install CivetWeb for the HTTP streaming application:**

   CivetWeb is used as the embedded HTTP server dependency for the `http_streamer_tdl.c` application. Clone version `v1.15` inside the `LicheeRV-Nano-Build/` directory so the source files remain organized alongside the SDK.

   ```bash
   cd /path/to/LicheeRV-Nano-Build
   git clone --branch v1.15 --depth=1 https://github.com/civetweb/civetweb.git
   ```

   The expected directory structure is:

   ```text
   LicheeRV-Nano-Build/
   ├── build/
   ├── buildroot/
   ├── cvitek-tdl-sdk-sg200x/
   └── civetweb/
       ├── include/
       │   └── civetweb.h
       └── src/
           └── civetweb.c
   ```

   The `civetweb` directory contains the source code and header required by the HTTP streaming application. In the development environment described here, the repository is cloned at:

   ```text
   LicheeRV-Nano-Build/civetweb
   ```

   The application's `MakeFile` must reference the actual CivetWeb location:

   ```makefile
   CIVETWEB_DIR := /path/to/LicheeRV-Nano-Build/civetweb
   CIVETWEB_SOURCE := $(CIVETWEB_DIR)/src/civetweb.c
   CIVETWEB_INCLUDE_DIR := $(CIVETWEB_DIR)/include
   ```

   Replace `/path/to/LicheeRV-Nano-Build` with the absolute path to your local SDK directory. For this project, the directory is:

   ```text
   /home/chucho/Documents/clases/embebidos/TRABAJODEGRADO/LicheeRV-Nano-Build
   ```

   **Note:** Cloning CivetWeb provides its source code; it does not, by itself, compile or deploy the HTTP streaming application to the board. The application must still be built with the appropriate RISC-V cross-compiler and SDK libraries.

6. **Flash the compiled image to the MicroSD card:**

   ```bash
   sudo dd if=/path/to/image.img of=/dev/sdX bs=16M status=progress conv=fdatasync
   ```

   *(Make sure to replace `/path/to/image.img` with your actual image path and `/dev/sdX` with your MicroSD card identifier).*

7. **Configure Wi-Fi Credentials (W / WE Versions):** Mount the `rootfs` partition of the SD card and configure your wireless network by creating or editing `/etc/wpa_supplicant.conf`:

   ```text
   ctrl_interface=/var/run/wpa_supplicant
   ap_scan=1
   network={
       ssid="YOUR_WIFI_SSID"
       psk="YOUR_WIFI_PASSWORD"
   }
   ```

8. **Connection and Initial Login:** Insert the MicroSD card into the board, power it on, and locate its IP address on your local network using `nmap`:

   ```bash
   sudo nmap -sV -p 22 192.168.1.0/24
   ```

   Connect to the board via SSH (Default root user):

   ```bash
   ssh root@<BOARD_IP_ADDRESS>
   ```
