#!/usr/bin/env bash

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

echo -n "[Info] Encrypted splits (${#array[@]}): "

for element in "${array[@]}"
do
    echo -n "$element "
done

echo

base=$(basename "$program" ".ll")
marked_checked=${base}\_wc.ll
assembly=${base}\_wc.s
binary=${base}\_wc

opt -load ../cmake-build-debug/water/libSplitWMPass.so -splitWM -S ${program} -o ${marked_checked} -splits ${array[@]} #-debug

check_fns=""
check_bbs=""

while IFS=":" read fn bb
do
#echo Read: ${fn}:${bb}
check_fns="${check_fns} ${fn}"
check_bbs="${check_bbs} ${bb}"
done < watermark.dat

#echo ${check_fns}
#echo ${check_bbs}

opt -load ../cmake-build-debug/water/libWMCheckerTPass.so -checkerWMT -S ${marked_checked} -o ${marked_checked} -checkfns $check_fns -checkbbs $check_bbs #-debug

llc ${marked_checked} -o ${assembly}
clang ${assembly} -o ${binary}

while IFS=":" read cstart cend cslot check_type
do
#echo Read: ${cstart} ${cend} ${cslot} ${check_type}
objdump -dz ${binary} | ./cval_wm.py ${cstart} ${cend} ${check_type}
cval0=$(echo $?)

#echo "Corrector value for basic block: ${cval0}"

objdump -dF ${binary} | ./cslot_wm.py $binary ${cslot} $cval0
done < corrector.dat





