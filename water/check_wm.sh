#!/bin/bash

usage()
{
    echo "Usage ./insert_wm.sh <program> <watermark> <key> <prime> <prime>..."
}

case  $1 in
    -h | --help )
	echo "Insert a CRT-based watermark into a module"
	usage
	exit 0
	;;
    *)
esac

if [ "$1" == "" ]; then
    echo "Missing parameter: <program>"
    usage
    exit 1
fi

if [ "$2" == "" ]; then
    echo "Missing parameter: <watermark>"
    usage
    exit 1
fi

if [ "$3" == "" ]; then
    echo "Missing parameter: <key>"
    usage
    exit 1
fi

if [ $(wc -w <<< "${@:4}") -lt 2 ]; then
    echo "Missing paramater: <prime>"
    usage
    exit 1
fi

program=$1 # llvm bytecode program
watermark=$2 # watermark
key=$3 # encryption key
output_file="splits"
primes="${@:4}" # primes

IFS=$' '
read -d $'\n' -a array < <(python3 ./split_wm.py ${watermark} ${key} ${output_file} ${primes})

echo -n "Encrypted splits (${#array[@]}): "

for element in "${array[@]}"
do
    echo -n "$element "
done

echo

base=$(basename "$program" ".ll")
marked_checked=${base}\_wc.ll
assembly=${base}\_w.s
binary=${base}\_w

opt -load ../cmake-build-debug/water/libSplitWMPass.so -splitWM -S ${program} -o ${marked_checked} -debug -splits "${array[@]}"

while IFS=":" read fn bb
do
echo Checking ${fn}:{$bb}
../checker/checker.sh ${marked_checked} ${bb} ${fn} --overwrite
done < watermark.dat

llc ${marked_checked} -o ${assembly}
clang ${assembly} -o ${binary}




