#!/bin/bash

for i in $(ls /dev/ttyUSB*); do
  if udevadm info $i | grep -q 'USB-Serial_Controller'; then
    echo $i
  fi
done
