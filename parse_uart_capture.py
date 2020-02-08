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


def exc_type_str(t):
    return { 
        0 : "Synchronous", 
        1 : "IRQ",         
        2 : "FIQ",         
        3 : "SError",      
    }.setdefault(t, 'Undefined')

class CoreException:
    def __init__(self, esr, spsr, far, typ, regs):
        self.esr = esr
        self.spsr = spsr
        self.far = far
        self.type = typ
        self.regs = regs

    def print_self(self, symbols):
        print('exception: type: {}, esr: {:016x}, spsr: {:016x}, far: {:016x}'.format(
            exc_type_str(self.type),
            self.esr,
            self.spsr,
            self.far))
        for reg in self.regs:
            reg.print_self()
  
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
    
def hex_be_to_uint32_t(be32):
    return struct.unpack('I', binascii.unhexlify(be32))[0]

def hex_be_to_uint64_t(be64):
    return struct.unpack('Q', binascii.unhexlify(be64))[0]

def unpack_be64(buf, offset):
    # print('Unpack be64 at {}'.format(offset))
    x = struct.unpack_from('<16s', buf, offset)[0].decode('ascii')
    # print(x)
    return offset + 16, hex_be_to_uint64_t(x)

def unpack_name(buf, offset, maxlen):
    # print('Unpack name at {}'.format(offset))
    name = struct.unpack_from('16s', buf, offset)[0].decode('ascii')
    name = binascii.unhexlify(name)
    name = struct.unpack('8s', name)[0]
    if name.find(0) != -1:
        name = name[:name.find(0)]
    name = name.decode('ascii')
    # print('Name :' + name)
    return offset + maxlen * 2, name

class Reg:
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def print_self(self):
        print('reg: {:>4}: {:016x}'.format(self.name, self.value))

def parse_reg(buf, offset):
    offset, name = unpack_name(buf, offset, 8)
    offset, value = unpack_be64(buf, offset)
    return offset, Reg(name, value)

def parse_regs(buf, offset):
    # print("Parsing regs at {}".format(offset))
    regs = []
    while True:
        offset, reg = parse_reg(buf, offset)
        # print(reg.name, reg.value)
        if not reg.name:
            break
        regs.append(reg)
    return offset, regs

def parse_binblock(buf, offset, symbols):
    print("-----------------")
    print("Exception block")
    # BINBLOCK
    h_magic, h_id, h_crc, h_len = struct.unpack_from('<8s8s8s16s', buf, offset)
    h_crc = hex_be_to_uint32_t(h_crc.decode('ascii'))
    h_len = hex_be_to_uint64_t(h_len.decode('ascii'))
    print(h_magic, h_id, h_crc, h_len)
    offset += 40

    # exception info
    offset, exception_type = unpack_be64(buf, offset) 
    offset, esr            = unpack_be64(buf, offset) 
    offset, spsr           = unpack_be64(buf, offset) 
    offset, far            = unpack_be64(buf, offset) 

    # regs
    offset, regs = parse_regs(buf, offset)

    ex_hdr = CoreException(esr, spsr, far, exception_type, regs)
    ex_hdr.print_self(symbols)

    offset += stack_hdr_sz

    # stk_hdr = StackHdr(stk_base_addr, sh_len, sh_crc) 
    # print(binstartpos, h_magic, h_crc, h_len, sh_magic, sh_crc, sh_len, hex(stk_base_addr))
    # print_stack(buf, offset, stk_hdr, symbols)
    print("---------------")
    binstart = 'BINBLOCK'.encode('ascii')
    binstartpos = buf.find(binstart, offset)
    return binstartpos

def main(filename):
    symbols = SymbolMap(kernel_map_filename)
    binstart = 'BINBLOCK'.encode('ascii')
    f = open(filename, 'rb')
    buf = f.read()
    binstartpos = buf.find(binstart)
    print(binstartpos)
    while (binstartpos != -1):
        binstartpos = parse_binblock(buf, binstartpos, symbols)



if __name__ == '__main__':
    filename = sys.argv[1]
    main(filename)
