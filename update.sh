#!/bin/bash
SD_DIR=/mnt/sdcard
make && \
mount /dev/sdc1 $SD_DIR && \
ls $SD_DIR && cp -fv kernel8.img $SD_DIR/u-boot.bin && umount $SD_DIR
