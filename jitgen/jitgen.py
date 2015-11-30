#!/usr/bin/python

import sys
from os.path import basename


class Chunk(object):
    offset = 0
    length = 0
    name = ''
    data = ''
    abs_op = -1
    rel_op = -1

    def __init__(self, offset, name):
        self.offset = offset
        self.name = name

    def to_string(self):
        out = ''
        for i in self.data:
            h = hex(ord(i))
            out += '\\x{0}'.format(h[2:].zfill(2))
        return '{{ {:<12}, {:<2}, {:>2}, {:>2}, "{:}" }}'.format(
            self.name,
            self.length,
            self.abs_op,
            self.rel_op,
            out)

    def analyse(self):
        self.abs_op = self.data.find('\xDD\xCC\xBB\xAA')
        if self.abs_op >= 0:
            return
        self.rel_op = self.data.find('\xBB\xAA')
        if self.rel_op >= 2:
            self.rel_op -= 2
            return


def main(argv):
    if len(argv) != 4:
        print "usage: {0} map bin outpath".format(basename(argv[0]))
        exit(0)

    fd_map = open(argv[1], "rb")
    fd_bin = open(argv[2], "rb")
    if not (fd_map and fd_bin):
        print("unable to open input files")
        exit(1)

    lines = fd_map.readlines()

    total_size = 0
    table = []
    writing = False
    for line in lines:
        tokens = line.split()
        if len(tokens) == 0:
            continue
        if tokens == ['Real', 'Virtual', 'Name']:
            writing = True
            continue
        if tokens[0] == 'length:':
            total_size = int(tokens[1], 16)
            continue
        if writing:
            if len(tokens) == 3:
                table.append(
                    Chunk(int(tokens[0], 16), tokens[2]))

    for i in range(0, len(table)-1):
        i0 = table[i+0]
        i1 = table[i+1]
        size = i1.offset - i0.offset
        i0.length = size
        fd_bin.seek(i0.offset)
        i0.data = fd_bin.read(i0.length)

    last = table[-1]
    last.length = total_size - last.offset
    fd_bin.seek(last.offset)
    last.data = fd_bin.read(last.length)

    for item in table:
        item.analyse()
        assert not (item.rel_op != -1 and item.abs_op != -1)

    fd_out = open(argv[3], 'w')
    if fd_out:
        fd_out.write('#include "chunks.h"\n\n')
        fd_out.write('jit_chunk_t chunk_table[] = {\n')
        for item in table:
            out = item.to_string()
            fd_out.write('  {0},\n'.format(out))
        fd_out.write('};\n')
        fd_out.close()
    else:
        print("unable to open output file")
        exit(1)

main(sys.argv)
