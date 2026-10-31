#!/bin/bash
# Script to collect and flatten dynamic shared libraries for hardware deployment

BASE="/root/LicheeRV-Nano-Build"
DEST="/root/LicheeRV-Nano-Build/libraries"

# Clean and create destination directory
rm -rf "$DEST"
mkdir -p "$DEST"

echo "Copying shared libraries and flattening directory structure..."

# 1. Copy from rootfs main usr/lib
find "$BASE/install/soc_sg2002_licheervnano_sd/rootfs/mnt/system/usr/lib" \
  -type f -name '*.so*' -exec cp -n -t "$DEST" {} +

# 2. Copy 3rd party libraries
find "$BASE/install/soc_sg2002_licheervnano_sd/rootfs/mnt/system/usr/lib/3rd" \
  -type f -name '*.so*' -exec cp -n -t "$DEST" {} +

# 3. Copy CVI TDL SDK libraries
find "$BASE/cvitek-tdl-sdk-sg200x/lib" \
  -type f -name '*.so*' -exec cp -n -t "$DEST" {} +

# 4. Copy TPU and OpenCV dependencies
find "$BASE/cvitek-tdl-sdk-sg200x/sample/3rd/tpu/lib" \
  -type f -name '*.so*' -exec cp -n -t "$DEST" {} +

find "$BASE/cvitek-tdl-sdk-sg200x/sample/3rd/opencv/lib" \
  -type f -name '*.so*' -exec cp -n -t "$DEST" {} +

# 5. Automatically create symbolic links for OpenCV if .3.2.0 exists
echo "Creating OpenCV symbolic links..."
cd "$DEST"
for lib in libopencv_core libopencv_imgproc libopencv_imgcodecs; do
    if [ -f "${lib}.so.3.2.0" ] && [ ! -e "${lib}.so.3.2" ]; then
        ln -s "${lib}.so.3.2.0" "${lib}.so.3.2"
    fi
done
cd - > /dev/null

echo "Done! All libraries are flattened in: $DEST"