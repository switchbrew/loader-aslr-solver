#!/usr/bin/python3
import sys

with open(sys.argv[1]) as fp:
    samples = [line for line in fp.readlines()]
    for line in samples:
        entryid = line[:line.index(' ')]
        sample = int(line[line.rindex(' ')+1:], 16)
        print("%s 0x%x" % (entryid, (sample<<21)+0x8000000))
