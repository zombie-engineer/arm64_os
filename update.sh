#!/bin/bash
SD_DIR=/mnt/sdcard
make && \
sudo mount /dev/sdd1 $SD_DIR 2>/dev/zero || true && \
ls $SD_DIR && sudo cp -fv kernel8.img $SD_DIR/u-boot.bin && sudo umount $SD_DIR
