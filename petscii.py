import sys


class Petscii:
    def __init__(self):
        self.byte_arr = []

    def create(self):
        with open("petscii/chargen", "rb") as f:
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
            out.append("{")
            for x in range(0, 8):
                out.append(self.byte_arr[y * 8 + x] + ",")
            out.append("},")

        with open("petscii/petscii.h", "w") as f:
            for s in out:
                f.write("%s\n" % s)


p = Petscii()
p.create()
