#!/bin/bash
# Script to flash SD card, stage binaries, libraries, YOLO model, and configure auto Wi-Fi

IMG="/root/LicheeRV-Nano-Build/install/soc_sg2002_licheervnano_sd/images/YOUR_IMAGE_NAME.img"
DEVICE="/dev/sda"
MOUNT_DIR="/mnt/sd_rootfs"
BUILD_ROOT="/root/LicheeRV-Nano-Build"

echo "=== 1. Unmounting target device partitions ==="
sudo umount ${DEVICE}* 2>/dev/null

echo "=== 2. Flashing image with dd ==="
sudo dd if="$IMG" of="$DEVICE" bs=4M status=progress conv=fsync
sync

echo "=== 3. Mounting rootfs partition ==="
sudo mkdir -p "$MOUNT_DIR"
sudo mount "${DEVICE}2" "$MOUNT_DIR"

echo "=== 4. Staging application binary, libraries, and YOLO model into /root ==="
sudo cp "$BUILD_ROOT/middleware/v2/sample/http_streamer_tdl/http_streamer_tdl" "$MOUNT_DIR/root/"
sudo chmod +x "$MOUNT_DIR/root/http_streamer_tdl"

sudo cp "$BUILD_ROOT/YOLOv8-Object-Detection/yolov8n_cv181x_int8_sym.cvimodel" "$MOUNT_DIR/root/"

sudo cp -r "$BUILD_ROOT/libraries" "$MOUNT_DIR/root/"

echo "=== 5. Setting LD_LIBRARY_PATH in system environment ==="
sudo bash -c "echo 'export LD_LIBRARY_PATH=\"/root/libraries:/usr/lib:/usr/local/lib:\$LD_LIBRARY_PATH\"' >> $MOUNT_DIR/etc/profile"

echo "=== 6. Writing Wi-Fi configuration ==="
sudo mkdir -p "$MOUNT_DIR/etc/wpa_supplicant"
sudo bash -c "cat > $MOUNT_DIR/etc/wpa_supplicant/wpa_supplicant.conf" << 'EOF'
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1
network={
  ssid="YOUR_WIFI_SSID"
  psk="YOUR_WIFI_PASSWORD"
  scan_ssid=1
}
EOF

echo "=== 7. Configuring clean automatic startup in rcS ==="
if [ -f "$MOUNT_DIR/etc/init.d/rcS" ]; then
    # Clean stale sockets, bring up wlan0, start wpa_supplicant, and request DHCP lease
    sudo bash -c "echo 'rm -rf /var/run/wpa_supplicant' >> $MOUNT_DIR/etc/init.d/rcS"
    sudo bash -c "echo 'ip link set wlan0 up' >> $MOUNT_DIR/etc/init.d/rcS"
    sudo bash -c "echo 'wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf' >> $MOUNT_DIR/etc/init.d/rcS"
    sudo bash -c "echo 'sleep 4' >> $MOUNT_DIR/etc/init.d/rcS"
    sudo bash -c "echo 'udhcpc -i wlan0 -b' >> $MOUNT_DIR/etc/init.d/rcS"
fi

echo "=== 8. Syncing data buffers and unmounting ==="
sync
sudo umount "$MOUNT_DIR"
sudo rm -rf "$MOUNT_DIR"

echo "=== DONE! Board image flashed and pre-configured with binary, model, libraries, and Wi-Fi. ==="