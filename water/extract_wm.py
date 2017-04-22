#!/usr/bin/python3

# objdump -dF program | python3 extract_wm.py program wm_label key primes...

import sys
import re

DEBUG = False

def main():
    if len(sys.argv) < 6:
        print('Usage: ./{program} program wm_label key primes...'.format(program=sys.argv[0]))
        exit(1)

    program = sys.argv[1]
    wm_label = sys.argv[2]
    key = int(sys.argv[3], 16)
    primes = [int(sys.argv[i]) for i in range(4, len(sys.argv))]
    pattern = '<\.' + wm_label + '\d>'

    log('program: %s' % program)
    log('wm_label: %s' % wm_label)
    log('key: %x' % key)
    log('primes: %s' % primes)
    log('pattern: %s' % pattern)

    # EXTRACT
    wm_splits= []
    file_offset = None

    with open(program, 'rb+') as f:
        it = iter(sys.stdin)
        for token_string in it:
            tokens = token_string.split()
            for token in tokens:
                match = re.match(pattern, token)
                if match:
                    log('Found: %s' % repr(match.group()))
                    try:
                        file_offset = int(tokens[4][:-2], 16)
                    except ValueError as e:
                        print(e)
                        sys.exit(1)
                    f.seek(file_offset, 0)
                    split_bytes = f.read(4)
                    wm_split = int.from_bytes(split_bytes, byteorder='little', signed=False)
                    wm_splits.append(wm_split)
                    break

    log('Extracted encryptions: %s' % repr(wm_splits))

    # DECRYPT
    for i in range(len(wm_splits)):
        wm_splits[i] = wm_splits[i] ^ key

    log('Decrypted splits: %s' % repr(wm_splits))

    # COMBINE
    equations = []
    for i in range(len(wm_splits)):
        for j in range(len(primes) - 1):
            for k in range(j + 1, len(primes)):
                if wm_splits[i] >= primes[j] * primes[k]:
                    wm_splits[i] = wm_splits[i] - primes[j] * primes[k]
                else:
                    equations.append((wm_splits[i], primes[j], primes[k]))
                    break
            else: # no break in loop
                continue
            break

    log('Equations: %s' % repr(equations))

    # SOLUTION
    lcm = None
    moduli = [equations[i][1] * equations[i][2] for i in len(equations)]

    if len(moduli) == 1:
        lcm = moduli[0]
    else:
        lcm = lcm_gen(moduli[0], moduli[1], *moduli[2:])

    log('Least-common-multiple: %d' % lcm)

    for i in range(lcm):
        success = True
        for j in range(len(equations)):
            res = (i - equations[j][0]) % (equations[j][1]*equations[j][2])
            if res != 0:
                success = False
                break # try next solution
        if success:
            log('Solution: %d' % i)
            break

    print(wm_splits)

def gcd(a, b):
    if b == 0:
        return a
    else:
        return gcd(b, a % b)

def lcm(a, b):
    return (a * b) / gcd(a, b)

def lcm_gen(a, b, *args):
    res = lcm(a, b)
    for mod in args:
        res = lcm(res, mod)
    return int(res)

def log(s):
    if DEBUG:
        print(s)

if __name__ == '__main__':
    main()
