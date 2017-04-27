#!/bin/bash

usage()
{
    echo "Usage ./test.sh"
}

error()
{
    printf "[Error] Test failed\n" #%s $1
    exit 1
}

case  $1 in
    -h | --help )
	echo "Testing checker.sh"
	usage
	exit 0
	;;
    *)
esac

# sum100 (watermarking)

program="../programs/ll/sum100.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_w

# sum100: 17

watermark=17 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5) # primes

echo "[Testing] Inserting watermark ${watermark} into sum100"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from sum100"
else
    error "${res}"
fi

echo "[Cleaning] Removing files..."
rm sum*
rm splits
rm watermark.dat

# sum100: 41

watermark=41 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5 7) # primes

echo "[Testing] Inserting watermark ${watermark} into sum100"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from sum100"
else
    error "${res}"
fi

echo "[Cleaning] Removing files..."
rm sum*
rm splits
rm watermark.dat

# sum100: 257

watermark=257 # watermark
key=0xFFFFFFFF # encryption key
primes=(7 11 13 19 3) # primes

echo "[Testing] Inserting watermark ${watermark} into sum100"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from sum100"
else
    error "${res}"
fi

echo "[Cleaning] Removing files..."
rm sum*
rm splits
rm watermark.dat

# sum100 (checked watermarking)

program="../programs/ll/sum100.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_wc

# sum100: 301

watermark=301 # watermark
key=0xFFFFFFFF # encryption key
primes=(11 13 19 2) # primes

echo "[Testing] Inserting (checked) watermark ${watermark} into sum100"

./checker_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from sum100"
else
    error "${res}"
fi

res=$(./${binary}; echo $?)

if [ ${res} == 100 ]; then
    echo "[Success] Program computes expected result"
else
    error "${res}"
fi

echo "[Cleaning] Removing files..."
rm sum*
rm splits
rm watermark.dat


echo "[Success] All tests passed..."
exit 0

















    

