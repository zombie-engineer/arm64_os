#!/usr/bin/python3
import os
import sys
import binascii
import struct

excp_hdr_sz = 48 
bin_hdr_sz = 16
stack_hdr_sz = 24
regs_hdr_sz = 20


kernel_map_filename = 'kernel8.sym'

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


class CoreStack:
    def __init__(self, stack_values):
        self.stack_values = stack_values

    def print_self(self, stack_addr, symbols):
        print('Stack:')
        for value in self.stack_values:
            stack_addr += 8
            symbol = get_symbol_from_addr(value, symbols, '')
            print('{:016x}: {:016x}: {}'.format(stack_addr, value, symbol))


def get_symbol_from_addr(addr, symbols, prefix):
    saddr, stype, sname = symbols.get_by_addr(addr)
    if saddr:
        return '{}<{} +0x{:x}>'.format(prefix, sname, addr - saddr)
    return ''

class RegsHdr:
    def __init__(self, numregs, bytelen, checksum):
        self.numregs = numregs
        self.bytelen = bytelen
        self.checksum = checksum


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

    def get_reg(self, name):
        for reg in self.regs:
            if reg.name == name:
                return reg
        return None

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
        print('{:>4}: {:016x}{}'.format(name.decode('ascii'), value, symbol))
    
def hex_be_to_uint32_t(be32):
    return struct.unpack('I', binascii.unhexlify(be32))[0]

def hex_be_to_uint64_t(be64):
    return struct.unpack('Q', binascii.unhexlify(be64))[0]

def unpack_be32(buf, offset):
    x = struct.unpack_from('<8s', buf, offset)[0].decode('ascii')
    return offset + 8, hex_be_to_uint32_t(x)

def ascii_unpack_be64(buf, offset):
    x = struct.unpack_from('<16s', buf, offset)[0].decode('ascii')
    return offset + 16, hex_be_to_uint64_t(x)


def fetch_ascii_string(buf, offset, namelen):
    bytename =  struct.unpack_from('{}s'.format(namelen), buf, offset)
    name = bytename[0].decode('ascii')
    return offset + namelen, name

def ascii_unpack_string(buf, offset, maxlen):
    # print('Unpack name at {}'.format(offset))
    name = struct.unpack_from('16s', buf, offset)[0].decode('ascii')
    # print(name)
    name = binascii.unhexlify(name)
    name = struct.unpack('8s', name)[0]
    if name.find(0) != -1:
        name = name[:name.find(0)]
    name = name.decode('ascii')
    # print('Name :' + name)
    return offset + maxlen * 2, name

class CoreReg:
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def print_self(self):
        print('reg: {:>4}: {:016x}'.format(self.name, self.value))

def parse_reg(buf, offset):
    # print('parse_reg: offset: {}'.format(offset))
    # print(buf[offset:offset+10])
    offset, name = ascii_unpack_string(buf, offset, 8)
    offset, value = ascii_unpack_be64(buf, offset)
    return offset, CoreReg(name, value)

def parse_regs(buf, offset):
    print("Parsing regs at {}".format(offset))
    regs = []
    while True:
        offset, reg = parse_reg(buf, offset)
        # print(reg.name, reg.value)
        if not reg.name:
            break
        regs.append(reg)
    return offset, regs

def parse_binblock_exception(buf, offset, size):
    # exception info
    offset, exception_type = ascii_unpack_be64(buf, offset) 
    offset, esr            = ascii_unpack_be64(buf, offset) 
    offset, spsr           = ascii_unpack_be64(buf, offset) 
    offset, far            = ascii_unpack_be64(buf, offset) 
    # regs
    offset, regs = parse_regs(buf, offset)
    return offset, CoreException(esr, spsr, far, exception_type, regs)

def parse_binblock_stack(buf, offset, size):
    assert size % 8 == 0
    stack_values = []
    while size:
        offset, value = ascii_unpack_be64(buf, offset)
        stack_values.append(value)
        size -= 8
    return offset, CoreStack(stack_values)


BINBLOCK_ID_EXCEPTION = '__EXCPTN'
BINBLOCK_ID_STACK     = '___STACK'

binblock_parser = {
        BINBLOCK_ID_EXCEPTION : parse_binblock_exception,
        BINBLOCK_ID_STACK     : parse_binblock_stack
}

def parse_binblock(buf, offset):
    print("-----------------")
    print("Exception block")
    # BINBLOCK
    offset, binblock_magic    = fetch_ascii_string(buf, offset, 8)
    offset, binblock_id       = fetch_ascii_string(buf, offset, 8)
    offset, binblock_checksum = unpack_be32(buf, offset)
    offset, binblock_len      = ascii_unpack_be64(buf, offset)

    print('Binblock: magic1: "{}", magic2: "{}", checksum: {}, len: {}'.format(
        binblock_magic, 
        binblock_id, 
        binblock_checksum, 
        binblock_len))

    offset, block = binblock_parser[binblock_id](buf, offset, binblock_len)
    return offset, binblock_id, block
    

def main(filename):
    symbols = SymbolMap(kernel_map_filename)
    binstart = 'BINBLOCK'.encode('ascii')
    blocks = {}

    f = open(filename, 'rb')
    buf = f.read()
    offset = buf.find(binstart)
    while (offset != -1):
        offset, block_id, block = parse_binblock(buf, offset)
        print(offset, block_id, block)
        blocks[block_id] = block
        offset = buf.find(binstart, offset)

    exception_block = blocks[BINBLOCK_ID_EXCEPTION]
    stack_block = blocks[BINBLOCK_ID_STACK]

    exception_block.print_self(symbols)
    sp = exception_block.get_reg('sp').value
    stack_block.print_self(sp, symbols)
        

if __name__ == '__main__':
    filename = sys.argv[1]
    main(filename)
