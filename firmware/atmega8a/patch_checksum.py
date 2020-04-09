import sys
import zlib 
import struct

def write_checksum(filepath, checksum_off, checksum):
    crc_bytes = struct.pack('>I', checksum)
    with open(filepath, 'rb+') as f:
        f.seek(checksum_off)
        f.write(crc_bytes)

def calc_checksum(filepath):
    with open(filepath, 'rb') as f:
        return zlib.crc32(f.read()) & 0xffffffff

def do_checksum(filepath, checksum_off):
    write_checksum(filepath, checksum_off, 0)
    crc = calc_checksum(filepath)
    print('Patching {} with crc {:x}'.format(filepath, crc))
    write_checksum(filepath, checksum_off, crc)

if __name__ == '__main__':
    do_checksum(sys.argv[1], int(sys.argv[2], base=0))
