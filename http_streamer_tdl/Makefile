# ============================================================================
# Makefile para HTTP H.264 Streamer con Detección YOLOv8
# ============================================================================

TOP_DIR ?= $(abspath ../../../../)
BUILD_PATH ?= $(TOP_DIR)/build

# SDK Paths
SDK_VER ?= musl_riscv64
CHIP ?= CV181X

# Cross compiler
ifeq ($(SDK_VER), musl_riscv64)
CROSS_COMPILE ?= $(TOP_DIR)/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-
ARCH = riscv64
CFLAGS += -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d
else ifeq ($(SDK_VER), glibc_riscv64)
CROSS_COMPILE ?= $(TOP_DIR)/host-tools/gcc/riscv64-linux-gnu-x86_64/bin/riscv64-unknown-linux-gnu-
ARCH = riscv64
CFLAGS += -march=rv64imafdcv0p7xthead -mabi=lp64d
else ifeq ($(SDK_VER), 64bit)
CROSS_COMPILE ?= aarch64-linux-gnu-
ARCH = arm64
CFLAGS += -march=armv8-a
endif

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

# ============================================================================
# PATHS
# ============================================================================

# Middleware
MW_PATH ?= $(TOP_DIR)/middleware/v2
MW_LIB_PATH = $(MW_PATH)/lib
MW_INC_PATH = $(MW_PATH)/include
MW_3RD_LIB_PATH = $(MW_LIB_PATH)/3rd
MW_SAMPLE_PATH = $(MW_PATH)/sample/common

# Kernel headers
KERNEL_BUILD_PATH ?= $(TOP_DIR)/linux_5.10/build/sg2002_licheervnano_sd

# ISP Include paths
ifeq ($(CHIP), CV180X)
MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv180x
CFLAGS += -DCV180X
else
MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv181x
CFLAGS += -DCV181X
endif

# TDL SDK
TDL_SDK_PATH ?= $(TOP_DIR)/cvitek-tdl-sdk-sg200x
TDL_LIB_PATH = $(TDL_SDK_PATH)/lib
TDL_INC_PATH = $(TDL_SDK_PATH)/include
TDL_SAMPLE_PATH = $(TDL_SDK_PATH)/sample
TDL_OPENCV_LIB_PATH = $(TDL_SAMPLE_PATH)/3rd/opencv/lib

# TPU
TPU_LIB_PATH ?= $(TDL_SDK_PATH)/sample/3rd/tpu/lib

# Sysroot
BR_OUTPUT = $(TOP_DIR)/buildroot/output
SYSROOT = $(BR_OUTPUT)/host/riscv64-buildroot-linux-musl/sysroot

# ============================================================================
# TARGET & FUENTES MODULARIZADAS
# ============================================================================

TARGET = http_streamer_tdl

SOURCES = http_streamer_tdl.c \
          web_handler.c \
          venc_handler.c \
          mse_stream.c \
          fmp4_init_segment.c \
          vpss_init.c \
          vi_init.c \
          vb_pool.c \
          sensor_config.c

OBJECTS = $(SOURCES:.c=.o)

# ============================================================================
# FLAGS
# ============================================================================

CFLAGS += -Wall -Wextra -O2 -g
CFLAGS += -std=gnu11 -Wno-pointer-to-int-cast -fsigned-char
CFLAGS += -Wno-format-truncation -fdiagnostics-color=always
CFLAGS += -Wno-unused-function

# Sysroot includes FIRST
CFLAGS += -I$(SYSROOT)/usr/include

# MW and SDK includes
CFLAGS += -I.
CFLAGS += -I$(MW_INC_PATH)
CFLAGS += -I$(MW_ISP_INC_PATH)
CFLAGS += -I$(MW_SAMPLE_PATH)
CFLAGS += -I$(TDL_INC_PATH)
CFLAGS += -I$(TDL_INC_PATH)/cvi_tdl
CFLAGS += -I$(TDL_INC_PATH)/cvi_tdl_app
CFLAGS += -I$(TDL_SAMPLE_PATH)/3rd/middleware/v2/include

# Kernel headers
CFLAGS += -I$(KERNEL_BUILD_PATH)/riscv/usr/include

# Library paths
LDFLAGS = -L$(MW_LIB_PATH)
LDFLAGS += -L$(MW_3RD_LIB_PATH)
LDFLAGS += -L$(TDL_LIB_PATH)
LDFLAGS += -L$(TDL_OPENCV_LIB_PATH)
LDFLAGS += -L$(TPU_LIB_PATH)
LDFLAGS += -L$(SYSROOT)/usr/lib

# Libraries
MW_LIBS = -lsample -lsys -lvpu -lvenc -lvdec
MW_LIBS += -lcvi_bin -lcvi_bin_isp -lisp -lisp_algo
MW_LIBS += -lae -laf -lawb -lsns_full -lini -latomic -lcvi_ive

TDL_LIBS = -lcvi_tdl -lcvi_tdl_app
TPU_LIBS = -lcviruntime -lcvikernel -lcvimath -lcnpy -lz
OTHER_LIBS = -lcivetweb -lssl -lcrypto -lpthread -ldl -lm -lrt -latomic -lz
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs

LDFLAGS += -Wl,--start-group $(MW_LIBS) $(TDL_LIBS) $(TPU_LIBS) $(OPENCV_LIBS) $(OTHER_LIBS) -Wl,--end-group

# ============================================================================
# REGLAS
# ============================================================================

.PHONY: all clean install info check-deps help

all: info check-deps $(TARGET)

info:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║  HTTP H.264 Streamer con YOLOv8 - Build                    ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║  CHIP:       $(CHIP)"
	@echo "║  SDK_VER:    $(SDK_VER)"
	@echo "║  CC:         $(CC)"
	@echo "║  MW_PATH:    $(MW_PATH)"
	@echo "║  TDL_PATH:   $(TDL_SDK_PATH)"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""

$(TARGET): $(OBJECTS)
	@echo "🔨 Enlazando $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo ""
	@echo "✅ Compilación exitosa: $(TARGET)"
	@echo ""
	@echo "Para ejecutar:"
	@echo "  ./$(TARGET) /path/to/yolov8n_cv181x_int8_sym.cvimodel"
	@echo ""

%.o: %.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "🧹 Limpiando..."
	rm -f $(TARGET) *.o
	@echo "✅ Limpieza completada"

install: $(TARGET)
	@if [ -z "$(DESTDIR)" ]; then \
		echo "❌ ERROR: DESTDIR no definido"; \
		echo "   Uso: make install DESTDIR=/path/to/rootfs"; \
		exit 1; \
	fi
	@echo "📦 Instalando en $(DESTDIR)..."
	mkdir -p $(DESTDIR)/usr/bin
	install -m 0755 $(TARGET) $(DESTDIR)/usr/bin/
	@echo "✅ Instalado en $(DESTDIR)/usr/bin/$(TARGET)"

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)
	@echo "✅ Build de debug completado"

check-deps:
	@echo "Verificando dependencias..."
	@test -d $(MW_PATH) || (echo "❌ MW_PATH no encontrado: $(MW_PATH)" && exit 1)
	@test -d $(TDL_SDK_PATH) || (echo "❌ TDL_SDK_PATH no encontrado: $(TDL_SDK_PATH)" && exit 1)
	@test -f $(SYSROOT)/usr/lib/libcivetweb.a || (echo "❌ libcivetweb no encontrada" && exit 1)
	@echo "✅ Todas las dependencias encontradas"

help:
	@echo "Targets disponibles:"
	@echo "  all               - Verificar deps y compilar $(TARGET)"
	@echo "  clean             - Limpiar archivos generados"
	@echo "  install           - Instalar (requiere DESTDIR)"
	@echo "  debug             - Build con símbolos de debug"
	@echo "  check-deps        - Verificar dependencias"
