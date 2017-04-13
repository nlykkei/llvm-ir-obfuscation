#!/usr/bin/python3

import sys

def main():
    if len(sys.argv) < 3:
        print("Usage: ./{program} start_label end_label".format(program=sys.argv[0])) 
        exit(1)

#    for arg in sys.argv:
#        print(arg)

    start_label = "<" + sys.argv[1] + ">:"
    end_label = "<" + sys.argv[2] + ">:"
    found = False

    it = iter(sys.stdin)

    for token_string in it:
        tokens = token_string.split()
        if start_label in tokens:
            found = True
            break

    if not found:
        print("Could not find start label: ", start_label)
        sys.exit(-1)

    cval = 0x0
    found = False

    for token_string in it:
        tokens = token_string.split()[1:] # skip address
        if end_label in tokens:
            found = True
            break
        for token in tokens:
            if len(token) != 2: # bytes in hex
                break
            try:
                val = int(token, 16)
                cval = cval ^ val
#                print(token, end=' ')
            except ValueError as e:
                break # x86 mnemonic
#        print()

    if not found:
        print("Could not find end label: ", end_label)
        sys.exit(-1)

    sys.exit(cval) 

if __name__ == '__main__':
    main()
