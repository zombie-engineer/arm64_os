#!/usr/bin/python3
import re
import time
import subprocess
import sys

from telnetlib import Telnet
ENCODING = 'utf-8'
HOST = 'localhost'
PORT = 4444
PI_NUM_CORES = 4

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

def wait_ready(t):
    for i in range(PI_NUM_CORES):
        tn_wait_for_regex(t, '.*hardware has.*breakpoints,.*watchpoints')

def cmd_update(t):
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
    wait_ready(t)
    t_reboot_time_end = time.time()

    image_to_mem_time = t_write_end - t_write_start
    sd_write_time = t_sdcard_write_end - t_sdcard_write_start
    reboot_time = t_reboot_time_end - t_reboot_time_start
    total_time = image_to_mem_time + sd_write_time + reboot_time
    print('Image to mem time: {} sec'.format(image_to_mem_time))
    print('SDCard write time: {} sec'.format(sd_write_time))
    print('PI reboot time   : {} sec'.format(reboot_time))
    print('Total update time: {} sec'.format(total_time))

def cmd_reboot(t):
    t.write('pi_reboot')
    wait_ready(t)
    t.write('reset init')

class Pi:
    def __init__(self, t):
        self.__t = t

    def update(self):
        return cmd_update(self.__t)
    def reboot(self):
        return cmd_reboot(self.__t)

    def cmd(self, cmd):
        return {
                'up' : self.update,
                're' : self.reboot
                }[cmd]()


def main(cmd):
    t = telnet_client(Telnet(HOST, PORT))
    Pi(t).cmd(cmd)

if __name__ == '__main__':
    cmd = sys.argv[1]
    main(cmd)
