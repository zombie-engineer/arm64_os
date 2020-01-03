#!/usr/bin/python3
import os
import sys
import binascii
import struct

excp_hdr_sz = 48 
bin_hdr_sz = 16
stack_hdr_sz = 24
regs_hdr_sz = 20


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


class StackHdr:
    def __init__(self, stack_base_addr, bytelen, crc):
        self.stk_base_addr = stack_base_addr
        self.bytelen = bytelen
        self.crc = crc


def get_symbol_from_addr(addr, symbols, prefix):
    saddr, stype, sname = symbols.get_by_addr(addr)
    if saddr:
        return '{}<{} +0x{:x}>'.format(prefix, sname, addr - saddr)
    return ''

def print_stack(buf, offset, stk_hdr, symbols):
    print('stack:')
    stack_depth = stk_hdr.bytelen >> 3
    for i in range(stack_depth):
        stk_addr = stk_hdr.stk_base_addr + i * 8
        stk_value = struct.unpack_from('<Q', buf, offset + i * 8)[0]
        symbol = get_symbol_from_addr(stk_value, symbols, '')
        print('{:016x}: {:016x}: {}'.format(stk_addr, stk_value, symbol))

class RegsHdr:
    def __init__(self, numregs, bytelen, crc):
        self.numregs = numregs
        self.bytelen = bytelen
        self.crc = crc

class ExcpHdr:
    def __init__(self, esr, spsr, far, typ):
        self.esr = esr
        self.spsr = spsr
        self.far = far
        self.type = typ

def exc_type_str(t):
    return { 
        0 : "Synchronous", 
        1 : "IRQ",         
        2 : "FIQ",         
        3 : "SError",      
    }.setdefault(t, 'Undefined')

def print_exception(ex_hdr, symbols):
    print('exception: type: {}, esr: {:016x}, spsr: {:016x}, far: {:016x}'.format(
        exc_type_str(ex_hdr.type),
        ex_hdr.esr,
        ex_hdr.spsr,
        ex_hdr.far))

def print_regs(buf, offset, regs_hdr, symbols):
    print('regs:')
    reg_sz = 16
    for i in range(regs_hdr.numregs):
        reg_off = offset + i * reg_sz
        name, value = struct.unpack_from('<8sQ', buf, reg_off)
        for i in range(len(name)):
            if name[i] == 0:
                name = name[:i]
                break
        symbol = get_symbol_from_addr(value, symbols, ' <-- ')
        print('reg: {:>4}: {:016x}{}'.format(name.decode('ascii'), value, symbol))
    

def main(filename):
    symbols = SymbolMap(kernel_map_filename)
    binstart = 'BINBLOCK'.encode('ascii')
    f = open(filename, 'rb')
    buf = f.read()
    binstartpos = buf.find(binstart)
    while (binstartpos != -1):
        print("-----------------")
        print("Exception block")
        # BINBLOCK
        offset = binstartpos
        h_magic, h_crc, h_len = struct.unpack_from('<8sII', buf, offset)
        offset += bin_hdr_sz

        # exception info
        h_magic, h_crc, h_len, esr, spsr, far, tp = struct.unpack_from('<8sIIQQQQ', buf, offset)
        offset += excp_hdr_sz
        ex_hdr = ExcpHdr(esr, spsr, far, tp)
        print_exception(ex_hdr, symbols)

        # regs
        print(offset)
        h_magic, h_crc, h_len, h_numregs = struct.unpack_from('<8sIII', buf, offset)
        offset += regs_hdr_sz
        regs_hdr = RegsHdr(h_numregs, h_len, h_crc)
        print_regs(buf, offset, regs_hdr, symbols)
        offset += h_len


        sh_magic, sh_crc, sh_len, stk_base_addr = struct.unpack_from('<8sIIQ', buf, offset)
        offset += stack_hdr_sz

        stk_hdr = StackHdr(stk_base_addr, sh_len, sh_crc) 
        # print(binstartpos, h_magic, h_crc, h_len, sh_magic, sh_crc, sh_len, hex(stk_base_addr))
        print_stack(buf, offset, stk_hdr, symbols)
        print("---------------")
        binstartpos = buf.find(binstart, offset)



if __name__ == '__main__':
    filename = sys.argv[1]
    main(filename)
