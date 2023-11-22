# Font references:   http://www.6502.org/users/sjgray/computer/cbmchr/cbmchr.html

import sys

# Standard Commodore PETSCII
fontname = "chargen"


# Non-Commodore PETSCII
# fontname = "901447-10m.bin"

class Petscii:
    def __init__(self):
        self.byte_arr = []

    def create(self):
        with open(f"../fonts/petscii/{fontname}", "rb") as f:
            count = 0
            byte = f.read(1)
            while byte != b"" and count < 2048:
                count += 1
                b = bin(int.from_bytes(byte, byteorder=sys.byteorder))
                b = b.replace("0b", "")
                b = "0b" + ("00000000" + b)[-8:]
                self.byte_arr.append(b)
                byte = f.read(1)

        print(len(self.byte_arr))
        out = []
        for y in range(0, 256):
            out.append("{ // " + str(y))
            for x in range(0, 8):
                out.append(self.byte_arr[y * 8 + x] + ",")
            out.append("},")

        with open("../fonts/petscii/petscii.h", "w") as f:
            f.write("%s\n" % "#ifndef PETSCII_H")
            f.write("%s\n\n" % "#define PETSCII_H")
            f.write("%s\n\n" % "// THIS FILE IS GENERATED WITH THE petscii.py PROGRAM!");
            f.write("%s\n\n" % "// DO NOT HAND EDIT!");
            f.write("%s\n" % "unsigned char petscii[256][8] = {")

            for s in out:
                f.write("%s\n" % s)

            f.write("%s\n\n" % "};")
            f.write("%s\n\n" % "#endif")


p = Petscii()
p.create()
