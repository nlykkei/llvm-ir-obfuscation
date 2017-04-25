

import sys
import re

DEBUG = False

def main():

    if len(sys.argv) < 6:
        print('Usage: ./{program} <watermark> <key> <output_file> <prime> <prime>...'.format(program=sys.argv[0]))
        exit(1)

    watermark = int(sys.argv[1])
    key = int(sys.argv[2], 16)
    output_file = sys.argv[3]
    primes = [int(sys.argv[i]) for i in range(4, len(sys.argv))]

    #EQUATIONS
    for i in range(len(primes) - 1):
        for j in range(i + 1, len(primes)):
            log('Equation: %d cong %d mod %d * %d' % (watermark, (watermark % (primes[i] * primes[j])), primes[i], primes[j]))

    # SPLIT
    sum = 0
    vks = []
    for i in range(len(primes) - 1):
        for j in range(i + 1, len(primes)):
            vk = watermark % (primes[i] * primes[j]) + sum
            vks.append(vk)
            sum = sum + primes[i] * primes[j]

    log('Pieces: %s' % repr(vks))

    # ENCRYPT
    for i in range(len(vks)):
        vks[i] = vks[i] ^ key

    log('Encryptions: %s' % repr(vks))

    with open(output_file, 'wb+') as f:
        for split in vks:
            log('Writing: %s' % repr(split.to_bytes(4, 'little')))
            f.write(split.to_bytes(4, 'little'))

#    with open(output_file, 'rb+') as f:
#        for _ in range(3):
#            split = int.from_bytes(f.read(4), 'little', signed = False)
#            print('Reading:', split)

    for ev in vks:
        print(ev, end=' ')
    print()


def log(s):
    if DEBUG:
        print(s)


if __name__ == '__main__':
    main()