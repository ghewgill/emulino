import gd

img = gd.image("lcd.png")
#red = img.colorAllocate((255, 0, 0))
chars = []
for c in range(16):
    for r in range(16):
        bytes = []
        for x in range(5):
            b = 0
            for y in range(8):
                #img.setPixel((63+121*c/4+4*x, 22+77*r/2+4*y), red)
                b <<= 1
                if img.getPixel((63+121*c/4+4*x, 22+77*r/2+4*y)) == 0:
                    b |= 1
            bytes.append(b)
        chars.append(bytes)
#img.writePng("test.png")

f = open("lcd.inc", "w")
print >>f, "u8 Chars[256][5] = {"
for c in chars:
    print >>f, "  {" + (",".join("0x%02x" % x for x in c)) + "},"
print >>f, "};"
