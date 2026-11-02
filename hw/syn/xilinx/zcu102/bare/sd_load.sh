#!/usr/bin/env bash
set -euo pipefail

echo "==============================================="
echo "   SD LOADING SCRIPT for BARE METAL ZCU102     "
echo "==============================================="
echo
echo "REMEMBER: Always check for the right device with lsblk!"
echo

if [ $# -ne 2 ]; then
    echo "Usage: $0 <device> <products_dir>"
    echo "Example: $0 /dev/sdb bare"
    exit 1
fi

DEV="$1"
PRODUCTS="$(realpath "$2")"

echo "[INFO] Device      : $DEV"
echo "[INFO] Products dir: $PRODUCTS"
echo

echo "*** Unmounting existing partitions..."
sudo umount "${DEV}"* 2>/dev/null || true
echo "Done"
echo

echo "*** Creating partition table..."
sudo fdisk --wipe-partitions always "$DEV" <<EOF
o
n
p
1

+512M
t
c
w
EOF
echo "Partition table written."
echo 


echo "*** Reloading partition table..."
sudo partprobe "$DEV"
sleep 2
echo "Done"
echo

echo "*** Formatting partition..."
sudo mkfs.vfat -F 32 -n BOOT "${DEV}1"
echo "Done"
echo


echo "*** Mounting partition..."
sudo mkdir -p /mnt/boot
sudo mount "${DEV}1" /mnt/boot
echo "Done"
echo

echo "*** Copying boot file..."
sudo cp "${PRODUCTS}/BOOT.BIN" /mnt/boot/

echo "*** Syncing and unmounting..."
sync

sudo umount /mnt/boot

echo
echo "========================================"
echo " SD card successfully programmed!"
echo "========================================"


