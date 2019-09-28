#!/usr/bin/python3

import sys


def mini_baudrate_get_reg_value(target_baudrate, system_clock):
    return int(system_clock / target_baudrate / 8) - 1


def print_usage():
    print("Usage: ")
    print("mini_reg_value TARGET_BAUDRATE [SYSTEMCLOCK]")


if __name__ == '__main__':
    if len(sys.argv) == 1:
        print_usage()
    elif sys.argv[1] == 'mini_reg_value':
        target_baudrate = int(sys.argv[2])
        system_clock = 250000000
        if len(sys.argv) > 4:
            system_clock = int(sys.argv[3])
        print(mini_baudrate_get_reg_value(target_baudrate, system_clock))
