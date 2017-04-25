#!/bin/bash

usage()
{
    echo "Usage ./test.sh"
}

error()
{
    opt -view-cfg ${checked}
    exit -1
}

case  $1 in
    -h | --help )
	echo "Testing checker.sh"
	usage
	exit 0
	;;
    *)
esac

# sum100

program="../programs/ll/sum100.ll" # program
watermark=17 # watermark
key=0xFFFFFFFF # encryption key
output_file="splits" # output file
primes=(2 3 5) # primes
label="wm_split" # label

printf "[Testing] Inserting watermark 17 into sum100\n"

IFS=$' '
read -d $'\n' -a array < <(python3 ./split_wm.py ${watermark} ${key} ${output_file} ${primes[@]})

echo -n "Encrypted splits (${#array[@]}): "

for element in "${array[@]}"
do
    echo -n "$element "
done

echo

base=$(basename "$program" ".ll")
marked=${base}\_w.ll
assembly=${base}\_w.s
binary=${base}\_w

opt -load ../cmake-build-debug/water/libSplitWMPass.so -splitWM -S ${program} -o ${marked} -debug -splits "${array[@]}"
llc ${marked} -o ${assembly}
clang ${assembly} -o ${binary}

watermark=$(objdump -dF ${binary} | python3 ./extract_wm.py ${binary} ${label} ${key} ${primes[@]})

if [ ${watermark} -ne 17 ]; then
    echo "Test failed: sum100"
    exit 1
fi

echo "[Success] All tests passed..."
exit 0

















    

