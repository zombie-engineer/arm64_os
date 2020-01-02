#!/usr/bin/python3
import os
import sys
import binascii
import struct

bin_hdr_sz = 16
stack_hdr_sz = 24


kernel_map_filename = 'all_symbols.map'

class SymbolMap:
    def __init__(self, map_filename):
        # Assume symbols are sorted in file

        # List of symbols as output by nm
        self.__s = []

        with open(map_filename, 'r') as f:
            for line in f.readlines():
                addr, tp, name = line[:-1].split()
                addr = int('0x' + addr, base=16)
                if len(self.__s) and addr < self.__s[-1][0]:
                    raise Exception('Symbols not sorted')
                self.__s.append((addr, tp, name))

    def get_by_addr(self, addr):
        # print('get_by_addr: {:016x}'.format(addr))
        # Binary search in sorted array
        pos = len(self.__s)
        d = pos >> 1
        pos -= d

        last_lower = None

        while True:
            d >>= 1
            # print(pos, d)
            saddr, tp, name = self.__s[pos]
            saddr_last = pos == (len(self.__s) - 1)
            if not saddr_last:
                naddr, _, _ = self.__s[pos + 1]
            # print(saddr_last, hex(naddr))
            # print('{:06d}: {:016x} {}, {}'.format(pos, saddr, tp, name))
            if addr >= saddr and (saddr_last or addr < naddr):
                return saddr, tp, name

            if not d:
                return None, None, None

            if addr > saddr:
                # print('{:016x} > {:016x}({}->{})'.format(addr, saddr, pos, pos + d))
                pos += d
            else:
                # print('{:016x} < {:016x}({}->{})'.format(addr, saddr, pos, pos - d))
                pos -= d
            if d == 0:
                return 0, 0, 0



def main(filename):
    symbols = SymbolMap(kernel_map_filename)
    binstart = 'BINBLOCK'.encode('ascii')
    f = open(filename, 'rb')
    buf = f.read()
    binstartpos = buf.find(binstart)
    h_magic, h_crc, h_len = struct.unpack_from('<8sII', buf, binstartpos)
    sh_magic, sh_crc, sh_len, stk_base_addr = struct.unpack_from('<8sIIQ', buf, binstartpos + bin_hdr_sz)
    print(binstartpos, h_magic, h_crc, h_len, sh_magic, sh_crc, sh_len, hex(stk_base_addr))
    stackpos = binstartpos + bin_hdr_sz + stack_hdr_sz
    stack_depth = sh_len >> 3
    for i in range(stack_depth):
        stk_addr = stk_base_addr + i * 8
        stk_value = struct.unpack_from('<Q', buf, stackpos + i * 8)[0]
        saddr, stype, sname = symbols.get_by_addr(stk_value)
        symbol = ''
        if saddr:
            symbol = '<{} +0x{:x}>'.format(sname, stk_value - saddr)
        print('{:016x}: {:016x}: {}'.format(stk_addr, stk_value, symbol))



if __name__ == '__main__':
    filename = sys.argv[1]
    main(filename)
