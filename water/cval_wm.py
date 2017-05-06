#!/usr/bin/python3

import sys


def main():
    if len(sys.argv) < 4:
        print("Usage: ./{program} <start_label> <end_label> <check_type>".format(program=sys.argv[0]))
        exit(1)

    #    for arg in sys.argv:
    #        print(arg)

    start_label = "<" + sys.argv[1] + ">:"
    end_label = "<" + sys.argv[2] + ">:"
    check_type = int(sys.argv[3])
    cmul = [3, 5, 7, 11]

    it = iter(sys.stdin)

    for token_string in it:
        tokens = token_string.split()
        if start_label in tokens:
            break

    cval = 0x0

    if check_type == 0:
        for token_string in it:
            tokens = token_string.split()[1:] # skip address
            if end_label in tokens:
                break
            for token in tokens:
                if len(token) != 2: # bytes in hex
                    break
                try:
                    val = int(token, 16)
                    cval = cval ^ val
                #                   print(token, end=' ')
                except ValueError as e:
                    break # x86 mnemonic
                #           print()
    else:
        n = 0
        k = 3
        c = cmul[check_type - 1]
        for token_string in it:
            tokens = token_string.split()[1:] # skip address
            if end_label in tokens:
                break
            for token in tokens:
                if len(token) != 2: # bytes in hex
                    break
                try:
                    val = int(token, 16)
                    cval += val
                    cval *= c
                    cval = cval % 256
                    n += 1
                #                    print(token, end=' ')
                except ValueError as e:
                    break # x86 mnemonic
                #            print()
        z = cval

        cpow = 1
        for _ in range(n - k + 1):
            cpow *= c
            cpow = cpow % 256

        (g, x, y) = xgcd(cpow, 256)
        cval = (x * (-z)) % 256

    #        print(z)
    #        print("cpow * xk + z = 0 % mod 256:", (cpow * cval + z) % 256, cpow * cval + z)
    #        print("c=", c, "cpow", cpow,  "x=", x, "cpow * x % 256", (cpow * x) % 256, "cval=", cval)

    sys.exit(cval)

def xgcd(b, n):
    x0, x1, y0, y1 = 1, 0, 0, 1
    while n != 0:
        q, b, n = b // n, n, b % n
        x0, x1 = x1, x0 - q * x1
        y0, y1 = y1, y0 - q * y1
    return  b, x0, y0


if __name__ == '__main__':
    main()
