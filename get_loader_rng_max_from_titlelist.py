#!/usr/bin/python3
import sys

with open(sys.argv[1]) as fp:
    procinfo_list = []
    for arg in range(len(sys.argv)-2):
        with open(sys.argv[arg+2]) as fp2:
            procinfo_list = procinfo_list + [line for line in fp2.readlines()]
    boot_list = [line for line in fp.readlines()]
    for bootline in boot_list:
        boot_programid = bootline[:bootline.index('\n')]
        for line in procinfo_list:
            programid = line[:line.index('/')]
            size = line[line.rindex(' ')+1:]
            if boot_programid != programid:
                continue
            maxval = (0x7ff8000000 - int(size, 16))>>21
            print("%s 0x%x" % (boot_programid, maxval))
            break
