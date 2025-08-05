#!/usr/bin/env python3

# A script to convert binary files to a C array
# Author: zwenergy
# Updated by ChatGPT: Added drag-and-drop feature.

import sys
import math
import os

def usage():
    print("Usage: drag a .min file onto this script or run:")
    print("python binToCode.py INPUT [OUTPUT]  (Example: python binToCode.py rom.min rom.h)")

if len(sys.argv) == 2:
    srcFile = sys.argv[1]
    # Auto-generate output filename: replace extension with .h or add .h if no extension
    base, ext = os.path.splitext(srcFile)
    dstFile = base + ".h"
elif len(sys.argv) == 3:
    srcFile = sys.argv[1]
    dstFile = sys.argv[2]
else:
    usage()
    exit(1)

outStr = []

with open(srcFile, "rb") as f:
    curByte = f.read(1)
    while curByte != b"":
        tmpInt = int.from_bytes(curByte, "big")
        # Reverse the bit order if needed
        #tmpInt = int('{:08b}'.format(tmpInt)[::-1], 2)
        outStr.append(format(tmpInt, '#04x'))
        curByte = f.read(1)

with open(dstFile, "w") as f:
    addressBits = math.ceil(math.log2(len(outStr)))
    curLine = f"const uint8_t rom[ {len(outStr)} ] __attribute__ ((section(\".romStorage\"))) = {{\n"
    f.write(curLine)
    tmpCnt = 0
    firstLine = True
    for b in outStr:
        if not firstLine:
            f.write(", ")
        firstLine = False
        f.write(b)
        tmpCnt += 1
        if tmpCnt == 12:
            f.write("\n")
            tmpCnt = 0
    f.write("\n};\n")
