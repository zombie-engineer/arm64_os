#!/bin/bash
set -e
UUID=9F12-EAC7
SD_DIR=/mnt/sdcard
DEV_NAME=$(sudo blkid -U $UUID)
if make; then
  if (cat /proc/mounts | grep $DEV_NAME); then
    echo has mount, unmounting ...
    sudo umount $DEV_NAME
  fi
  echo mounting device: $DEV_NAME uuid: $UUID
  sudo mount --uuid $UUID $SD_DIR
  mount | grep sde
  if (ls $SD_DIR | grep 'not a '); then
    echo mount failed
  else
    sudo cp -fv kernel8.img $SD_DIR/u-boot.bin
    sudo umount --verbose $SD_DIR
  fi
fi
