#!/usr/bin/python
import sys

def clz(value):
    i = 0
    while (i < 32 and value & (1<<(31 - i)) == 0):
        i += 1
    return i

def main(value):
    value = int(value, 0)

    print(hex(value), clz(value), 31 - clz(value ^ (value - 1)))

if __name__ == '__main__':
    main(sys.argv[1])
