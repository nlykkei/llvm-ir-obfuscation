#!/usr/bin/python3

# objdump -dF program | python3 extract_wm.py program wm_label key primes...

import sys
import re

DEBUG = False

def main():
    if len(sys.argv) < 6:
        print('Usage: {program} <program> <key> <prime> <prime>...'.format(program=sys.argv[0]))
        sys.exit(1)

    program = sys.argv[1]
    key = int(sys.argv[2], 16)
    primes = [int(sys.argv[i]) for i in range(3, len(sys.argv))]

    log('program: %s' % program)
    log('key: %x' % key)
    log('primes: %s' % primes)

    # EXTRACT
    wm_splits = []
    prefix = bytes(iter([0x66] * 15))

    input = sys.stdin.read().split()
    text_offset = int(input[2], 16)
    text_size = int(input[5], 16)

    with open(program, 'rb+') as f:
        f.seek(text_offset)
        for i in range(text_size - 15 + 1):
            bytes_read = f.read(15)
            if (bytes_read == bytes(iter([0x66] * 15))):
                log("Found byte sequence at position: %d" % int(text_offset + i))
                split_bytes = f.read(4)
                wm_split = int.from_bytes(split_bytes, byteorder='little', signed=False)
                wm_splits.append(wm_split)
                log("Read split: %d" % wm_split)
            f.seek(text_offset + i + 1)


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
    moduli = [equations[i][1] * equations[i][2] for i in range(len(equations))]

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
            log('Solution: ', end='')
            print(i)
            sys.exit(0)

    print('Could not find a solution!')


class UGraph:
    def __init__(self, name, N, E):
        self.name = name
        self.N = N
        self.E = E

    def print(self):
        print('Graph: ' + self.name)
        for node in self.N:
            print(node)
        for edge in self.E:
            print(edge.start_node + ' -> ' + edge.end_node)

    def remove_node(self, node):
        try:
            self.N.remove(node)
        except KeyError as e:
            return False

        for edge in self.E:
            if edge.start_node == node or edge.end_node == node:
                E.remove(edge)

        return True

    def remove_edge(self, edge):
        try:
            self.E.remove(edge)
        except KeyError as e:
            return False

        return True

    class Node:
        def __init__(self, name, val):
            self.name = name
            self.val = val

        def __str__(self):
            return self.name + ' (' + repr(self.val) + ')'

        def __eq__(self, other):
            return self.name == other.name and self.val == other.val

    class Edge:
        def __init(self, start_node, end_node):
            self.start_node = start_node
            self.end_node = end_node

        def __eq__(self, other):
            return self.start_node == other.start_node and self.end_node == other.end_node

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

def log(s, end='\n'):
    if DEBUG:
        print(s, end)

if __name__ == '__main__':
    main()
