#!/usr/bin/python3
import re
import time
import subprocess
import sys

from telnetlib import Telnet
ENCODING = 'utf-8'

class telnet_client:
    def __init__(self, t):
        self.__t = t
        self.__last = ''

    def readline(self):
        line = self.__t.read_until(b'\r\n')
        line = line.replace(b'\x08', b'')
        line = line.decode(ENCODING)
  #      if line.startswith(b'> \x08\x08  \x08\x08'):
   #         print('--')
        line = line.strip()
        return line

    def write(self, msg):
        self.__t.write(msg.encode(ENCODING) + b'\r\n')

def tn_wait_for_line(t, line, do_print=True):
    while True:
        l = t.readline()
        if do_print:
            print('|"{}"|'.format(l))
        if l == line:
            break
def tn_wait_for_regex(t, pattern, do_print=True):
    while True:
        l = t.readline()
        if do_print:
            print('|"{}"|'.format(l))
        m = re.match(pattern, l)
        if m:
            break


def run_telnet():
    num_cores = 4
    print('Running telnet session')
    t = telnet_client(Telnet('localhost', 4444))
    t.write('targets')
    tn_wait_for_line(t, '')
    t_write_start = time.time()
    t.write('www')
    tn_wait_for_line(t, 'Write completed')
    t_write_end = time.time()
    t_sdcard_write_start = t_write_end
    tn_wait_for_regex(t, '.*Invalid ACK.*in DAP response')
    t_sdcard_write_end = time.time()
    t_reboot_time_start = t_sdcard_write_end
    for i in range(num_cores):
        tn_wait_for_regex(t, '.*hardware has.*breakpoints,.*watchpoints')
        print('matching {}'.format(i))
    t_reboot_time_end = time.time()

    image_to_mem_time = t_write_end - t_write_start
    sd_write_time = t_sdcard_write_end - t_sdcard_write_start
    reboot_time = t_reboot_time_end - t_reboot_time_start
    total_time = image_to_mem_time + sd_write_time + reboot_time
    print('Image to mem time: {} sec'.format(image_to_mem_time))
    print('SDCard write time: {} sec'.format(sd_write_time))
    print('PI reboot time   : {} sec'.format(reboot_time))
    print('Total update time: {} sec'.format(total_time))

def main(cmd):
    print(cmd)
    run_telnet()


if __name__ == '__main__':
    cmd = sys.argv[1]
    main(cmd)
