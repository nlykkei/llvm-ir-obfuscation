#!/usr/bin/python3

import sys


def main():
    if len(sys.argv) < 3:
        print("Usage: ./{program} <program> <cslot_label> <cval>".format(program=sys.argv[0]))
        exit(1)

    program = sys.argv[1]
    cslot_label = "<" + sys.argv[2] + ">"
    cval = int(sys.argv[3])
    found = False
    offset = None

    it = iter(sys.stdin)

    for token_string in it:
        tokens = token_string.split()
        if cslot_label in tokens:
            found = True
            try:
                offset = int(tokens[4][:-2], 16)
            except ValueError as e:
                print(e)
            #print("Found %s at offset 0x%x" % (cslot_label, offset))
            break

    if not found:
        #print("Could not find label: %s" % cslot_label)
        sys.exit(-1)

    f = open(program, "rb+")
    f.seek(offset, 0) # absolute
    #print(f.read(1))
    f.write(bytes([cval]))
    f.close()

    sys.exit(0)

if __name__ == '__main__':
    main()
