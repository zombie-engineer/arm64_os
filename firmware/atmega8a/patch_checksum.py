import sys
import zlib 
import struct

def write_checksum_at(filepath, at, checksum):
    crc_bytes = struct.pack('>I', checksum)
    with open(filepath, 'rb+') as f:
        if at == 'end':
            f.seek(0, 2)
        else:
            f.seek(int(at, base=0), 0)
        f.write(b'ATMGBIN8')
        f.write(crc_bytes)
        f.write(b'ATMGEND0')

def calc_checksum(filepath):
    with open(filepath, 'rb') as f:
        return zlib.crc32(f.read()) & 0xffffffff

def do_checksum(filepath, at):
    # write_checksum_at(filepath, at, 0)
    crc = calc_checksum(filepath)
    print('Patching {} with crc {:x}'.format(filepath, crc))
    write_checksum_at(filepath, at, crc)

if __name__ == '__main__':
    do_checksum(sys.argv[1], sys.argv[2])
