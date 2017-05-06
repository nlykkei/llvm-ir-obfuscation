#!/bin/bash

usage()
{
    echo "Usage ./test.sh"
}

error()
{
    printf "[Error] Test failed: %s\n" "$1"
    opt -view-cfg ${binary}\.ll
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

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 17: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# sum100: 41

watermark=41 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5 7) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 41: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# sum100: 257

watermark=257 # watermark
key=0xFFFFFFFF # encryption key
primes=(7 11 13 19 3) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 257: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# sum100 (Checked watermarking)

program="../programs/ll/sum100.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_wc

# sum100: 301

watermark=301 # watermark
key=0xFFFFFFFF # encryption key
primes=(11 13 19 2) # primes

echo "[Testing] Inserting (checked) watermark ${watermark} into ${program}"

./checker_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 301: ${res}"
fi

echo "[Testing] Unit tests of ${binary}"

res=$(./${binary}; echo $?)

if [ ${res} == 100 ]; then
    echo "[Success] ${binary} computes expected result: ${res}"
else
    error "${binary} = ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# fac (watermarking)

program="../programs/ll/fac.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_w

# fac: 29

watermark=29 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 29: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# fac: 59

watermark=59 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5 7) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 59: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# fac: 141

watermark=141 # watermark
key=0xFFFFFFFF # encryption key
primes=(7 11 13 19 3) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 141: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# fac (checked watermarking)

program="../programs/ll/fac.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_wc

# fac: 289

watermark=289 # watermark
key=0xFFFFFFFF # encryption key
primes=(11 13 19 2) # primes

echo "[Testing] Inserting (checked) watermark ${watermark} into ${program}"

./checker_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 289: ${res}"
fi

echo "[Testing] Unit tests of ${binary}"

res=$(./${binary} 5; echo $?)

if [ ${res} == 120 ]; then
    echo "[Success] ${binary} 5 computes expected result: ${res}"
else
    error "${binary} 5 = ${res}"
fi

res=$(./${binary} 3; echo $?)

if [ ${res} == 6 ]; then
    echo "[Success] ${binary} 3 computes expected result: ${res}"
else
    error "${binary} 3 = ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# pow (watermarking)

program="../programs/ll/pow.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_w

# pow: 14

watermark=14 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 14: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# pow: 61

watermark=61 # watermark
key=0xFFFFFFFF # encryption key
primes=(2 3 5 7) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 61: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# pow: 311

watermark=311 # watermark
key=0xFFFFFFFF # encryption key
primes=(7 11 13 19 3) # primes

echo "[Testing] Inserting watermark ${watermark} into ${program}"

./insert_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 331: ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

echo

# fac (checked watermarking)

program="../programs/ll/pow.ll" # program
base=$(basename "$program" ".ll")
binary=${base}\_wc

# pow: 891

watermark=891 # watermark
key=0xFFFFFFFF # encryption key
primes=(11 13 19 2) # primes

echo "[Testing] Inserting (checked) watermark ${watermark} into ${program}"

./checker_wm.sh $program $watermark $key "${primes[@]}"
res=$(./extract_wm.sh ${binary} ${key} "${primes[@]}")

if [ ${res} == ${watermark} ]; then
    echo "[Success] Extracted watermark ${res} from ${binary}"
else
    error "${binary} 891: ${res}"
fi

echo "[Testing] Unit tests of ${binary}"

res=$(./${binary} 2 5; echo $?)

if [ ${res} == 32 ]; then
    echo "[Success] ${binary} 2 5 computes expected result: ${res}"
else
    error "${binary} 2 5 = ${res}"
fi

res=$(./${binary} 3 4; echo $?)

if [ ${res} == 81 ]; then
    echo "[Success] ${binary} 3 4 computes expected result: ${res}"
else
    error "${binary} 3 4 = ${res}"
fi

echo "[Cleaning] Removing files..."
rm ${base}*
rm splits
rm watermark.dat

printf ".%.0s" {1..75}

echo

echo "[Success] All tests passed..."
exit 0

















    

