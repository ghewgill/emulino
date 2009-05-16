# Script to make the instruction jump table for emulino
# Copyright 2009 Greg Hewgill
#
# This file is part of Emulino.
#
# Emulino is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Emulino is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Emulino.  If not, see <http://www.gnu.org/licenses/>.

def bitmask(a):
    return sum(2**x for x in range(len(a)) if a[x])

def countbits(x):
    n = 0
    while x != 0:
        if x & 1:
            n += 1
        x >>= 1
    return n

def spread(template, x):
    r = 0
    i = 0
    while x != 0:
        if template & (1 << i):
            r |= (x & 1) << i
            x >>= 1
        i += 1
    return r

def uniq(a):
    if len(a) == 0:
        return []
    else:
        return [a[0]] + uniq([x for x in a if x != a[0]])

def pieces(x):
    r = []
    i = 0
    while i < len(x):
        while i < len(x) and not x[-1-i]:
            i += 1
        if i >= len(x):
            break
        j = i
        while x[-1-i]:
            i += 1
        r.append((j, i-j))
    return r

f = open("instructions.txt")
instr = [("".join(x[i] for i in range(4)), x[4]) for x in (x.split() for x in f.readlines() if not x.startswith("#"))]
f.close()

table = [None] * 0x10000
for i in instr:
    mask1 = bitmask(list(reversed([x == '1' for x in i[0]])))
    maskx = bitmask(list(reversed([x not in "01" for x in i[0]])))
    print hex(mask1), hex(maskx), i[1]
    for x in range(2**countbits(maskx)):
        sx = spread(maskx, x)
        if table[mask1 | sx] is not None:
            print table[mask1 | sx]
            assert table[mask1 | sx] is None
        table[mask1 | sx] = i[1]

f = open("avr.inc", "w")
print >>f, "Handler Instr[0x10000] = {"
f.write("".join(["  do_%s,\n" % (x if x else "BREAK") for x in table]))
print >>f, "};"
for t in sorted(uniq([x[0].replace("0", "-").replace("1", "-") for x in instr])):
    print >>f, "//", t
    for v in uniq(t):
        if v == "-": continue
        p = pieces([c == v for c in t])
        print >>f, "// u16 %c = %s;" % (v, " | ".join("(%s & 0x%x)" % ("instr" if x[0] == 0 else "(instr >> %d)" % (x[0] - sum(x[1] for x in p[:i])), (2**x[1]-1) << sum(x[1] for x in p[:i])) for i, x in enumerate(p)))
f.close()
