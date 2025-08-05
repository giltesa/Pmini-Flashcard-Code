#!/usr/bin/env python3

# A script to convert a C array file back to binary (.min)
# Author: ChatGPT

import sys
import os
import re

def usage():
    print("Usage: drag a .h file onto this script or run:")
    print("python codeToBin.py INPUT [OUTPUT]  (Example: python codeToBin.py rom.h rom.min)")

if len(sys.argv) == 2:
    srcFile = sys.argv[1]
    base, ext = os.path.splitext(srcFile)
    dstFile = base + ".min"
elif len(sys.argv) == 3:
    srcFile = sys.argv[1]
    dstFile = sys.argv[2]
else:
    usage()
    exit(1)

# Read the .h file and extract the array
with open(srcFile, "r") as f:
    content = f.read()

# Use regex to find all 0x?? hex values in the array
hex_bytes = re.findall(r'0x[0-9a-fA-F]{2}', content)

# Write them as binary to the output file
with open(dstFile, "wb") as f:
    for hx in hex_bytes:
        f.write(bytes([int(hx, 16)]))

print(f"Converted {len(hex_bytes)} bytes to '{dstFile}'")
