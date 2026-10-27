# Implementation and Customization of Buildroot on the LicheeRV Nano (SG2002)

Official repository for the undergraduate thesis: *"Implementation and optimization of a YOLO model on the LicheeRV Nano (SG2002) embedded platform for object detection with replicable documentation in educational environments"*, presented by **Jesús Antonio López Trigozo** to the Faculty of Engineering at the **Universidad Nacional de Colombia (Bogotá Campus)**, under the direction of professor Carlos Iván Camargo Bareño.

> **Project Status:** 🟢 In Progress — Buildroot environment configured, CVI TDL SDK cloned, and official TDL samples compiled successfully. Application integration and deployment are ongoing.

---

## 📌 1. Introduction and Justification

Deploying custom Linux environments on resource-constrained hardware like the LicheeRV Nano (SoC SG2002) often encounters a lack of clear, reproducible technical guides. Official pre-compiled images may include unnecessary packages, drivers, and services for a particular application.

This project provides a step-by-step methodology to compile Buildroot from source, customize the operating system, configure wireless connectivity, and prepare the development environment for AI inference on the LicheeRV Nano.

The workflow separates three components:

1. **Buildroot and the board SDK:** Build and customize the Linux operating system.
2. **CVI TDL SDK:** Obtain the headers, libraries, and sample applications needed for computer vision and AI inference.
3. **Application development:** Cross-compile custom applications, such as `http_streamer_tdl.c`, using the appropriate headers and libraries.

These components have different dependencies. Successfully compiling the operating system does not automatically mean that the CVI TDL development libraries or application-specific dependencies are installed.

---

## 🛠️ 2. Technical Specifications and Versions

### General Specifications

| Element | Specification |
| :--- | :--- |
| **SoC** | SOPHGO SG2002 |
| **Main CPU** | RISC-V C906 / ARM Cortex-A53, depending on configuration |
| **Additional CPU cores** | RISC-V C906 and 8051 |
| **Memory** | 2 Gbit (256 MB) DDR3 |
| **Storage** | TF card / SD NAND |
| **AI Acceleration** | Integrated NPU, up to 1 TOPS INT8 |
| **Video Interface** | MIPI DSI display output and MIPI CSI camera input |
| **Audio** | Onboard microphone and speaker amplifier |
| **Ethernet** | Available on E and WE variants |
| **Wireless** | Wi-Fi and Bluetooth on W and WE variants |
| **Operating System** | Buildroot Linux / Debian |
| **Development Architecture** | RISC-V 64-bit Linux, depending on the selected SDK configuration |

### Board Variant Comparison

| Version | Base System | Mic / Speaker | Ethernet | Wi-Fi / Bluetooth |
| :--- | :---: | :---: | :---: | :---: |
| **LicheeRV-Nano-B** | Yes | Yes | No | No |
| **LicheeRV-Nano-E** | Yes | Yes | Yes | No |
| **LicheeRV-Nano-W** | Yes | Yes | No | Yes |
| **LicheeRV-Nano-WE** | Yes | Yes | Yes | Yes |

### Software Components

| Component | Repository / Location | Purpose |
| :--- | :--- | :--- |
| LicheeRV Nano SDK | [sipeed/LicheeRV-Nano-Build](https://github.com/sipeed/LicheeRV-Nano-Build) | Linux system and board support |
| Host tools | [sophgo/host-tools](https://github.com/sophgo/host-tools) | Cross-compilation toolchain and host utilities |
| CVI TDL SDK | [milkv-duo/cvitek-tdl-sdk-sg200x](https://github.com/milkv-duo/cvitek-tdl-sdk-sg200x) | Computer vision headers, libraries, and examples |
| CivetWeb | [civetweb/civetweb](https://github.com/civetweb/civetweb) | Optional HTTP server dependency for streaming applications |

**Important compatibility note:** The Build SDK revision and CVI TDL SDK revision are separate. The successful compilation of the official TDL samples confirms that the sample build completed with the available cross-compiler. It does not, by itself, prove that every library or model is compatible with every firmware configuration.

---

## 🚀 3. Step-by-Step Replication Guide

### 3.1. Download the LicheeRV Nano SDK

Clone the official Sipeed repository:

```bash
git clone https://github.com/sipeed/LicheeRV-Nano-Build.git --depth=1
cd LicheeRV-Nano-Build
```

The `--depth=1` option downloads a shallow Git history to reduce download size and time.

The SDK directory should contain components such as:

```text
LicheeRV-Nano-Build/
├── build/
├── buildroot/
├── demo/
├── freertos/
├── host-tools/
├── install/
├── middleware/
├── osdrv/
├── u-boot-2021.10/
└── README.md
```

The exact directory contents may vary by SDK revision.

### 3.2. Prepare the Build Environment

The project uses the SDK's build environment and host tools. If the official Docker workflow is required, build the development container using the instructions provided by the SDK.

For the workflow used in this project, the SDK is located at:

```text
~/Documents/clases/embebidos/TRABAJODEGRADO/LicheeRV-Nano-Build
```

Enter the SDK directory:

```bash
cd ~/Documents/clases/embebidos/TRABAJODEGRADO/LicheeRV-Nano-Build
```

Verify the available cross-compiler:

```bash
find host-tools -type f \
  -name 'riscv64-unknown-linux-musl-gcc'
```

For the toolchain used during the sample build, the executable was located at:

```text
host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc
```

Add the toolchain directory to the current shell's `PATH`:

```bash
export PATH="$PWD/host-tools/gcc/riscv64-linux-musl-x86_64/bin:$PATH"
```

Verify the compiler:

```bash
riscv64-unknown-linux-musl-gcc --version
```

This `PATH` change applies to the current terminal session. Run the export again in a new terminal before compiling.

### 3.3. Configure and Build Buildroot

From the SDK root, configure the board:

```bash
source build/cvisetup.sh
defconfig sg2002_licheervnano_sd
```

If Git reports a safe-directory error in a container or mounted workspace, configure the directory used by that environment:

```bash
git config --global --add safe.directory /workspace
```

Build the SDK image:

```bash
build_all
```

This operation can take a considerable amount of time and disk space. Do not delete the build directory while compilation is in progress.

The generated image path depends on the SDK configuration and build output. Locate the actual image before flashing it.

### 3.4. Download the CVI TDL SDK

**This is a separate step from building Buildroot.** The base SDK does not necessarily include the CVI TDL headers and libraries required by custom computer vision applications.

From the root of `LicheeRV-Nano-Build`, clone the Milk-V CVI TDL repository:

```bash
cd ~/Documents/clases/embebidos/TRABAJODEGRADO/LicheeRV-Nano-Build

git clone https://github.com/milkv-duo/cvitek-tdl-sdk-sg200x.git --depth=1
```

This creates the directory:

```text
LicheeRV-Nano-Build/
└── cvitek-tdl-sdk-sg200x/
```

If the directory already exists, do not clone it a second time.

The repository should contain the following components:

```text
cvitek-tdl-sdk-sg200x/
├── bin/
├── cvimodel/
├── doc/
├── include/
├── lib/
├── regression/
├── sample/
└── README.md
```

The relevant directories are:

- `include/`: CVI TDL and related API headers.
- `lib/`: CVI TDL libraries.
- `sample/`: official sample applications and their build script.
- `sample/3rd/`: third-party libraries and middleware supplied with the sample project.
- `cvimodel/`: model files, when provided by the repository.

The CVI TDL repository used here includes the headers and libraries needed to begin developing applications. Additional runtime dependencies may still be required by a specific application.

### 3.5. Compile the Official CVI TDL Samples

Before compiling a custom application, verify that the SDK's sample build works with the available cross-compiler.

First, return to the SDK root:

```bash
cd ~/Documents/clases/embebidos/TRABAJODEGRADO/LicheeRV-Nano-Build
```

Set the compiler path:

```bash
export PATH="$PWD/host-tools/gcc/riscv64-linux-musl-x86_64/bin:$PATH"
```

Enter the TDL sample directory:

```bash
cd cvitek-tdl-sdk-sg200x/sample
```

Make the script executable if necessary:

```bash
chmod +x compile_sample.sh
```

Run the sample build:

```bash
./compile_sample.sh
```

The script compiles the sample applications using the build configuration and dependencies supplied with the TDL repository.

A successful build should finish with messages similar to:

```text
sample_lib_draw_rect done
finish build sample!!!
```

Compiler warnings, such as `_GNU_SOURCE` being redefined, are not necessarily fatal. Check the final build status and look for actual `error` messages if compilation fails.

**Verified result in this project:** The sample build completed and produced `sample_lib_draw_rect`. The output included compiler warnings but no fatal error at the end of the build.

### 3.6. Verify the CVI TDL Libraries

From the TDL repository root:

```bash
cd ~/Documents/clases/embebidos/TRABAJODEGRADO/LicheeRV-Nano-Build/cvitek-tdl-sdk-sg200x
```

Check the main libraries:

```bash
find lib -maxdepth 1 -type f \
  \( -name 'libcvi_tdl*' -o -name 'libcvi_tpu*' \)
```

The following files were found during development:

```text
lib/libcvi_tdl_app.a
lib/libcvi_tdl_app.so
lib/libcvi_tdl.a
lib/libcvi_tdl.so
```

Check the main API header:

```bash
find include -name 'cvi_tdl.h'
```

The headers and libraries are used when compiling applications that call CVI TDL functions. The correct include paths, library paths, linker flags, and target runtime must be selected for each application.

### 
