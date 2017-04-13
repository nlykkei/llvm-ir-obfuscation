#!/bin/bash

usage()
{
    echo "Usage ./checker.sh <program> <basic_block> [function]"
}

case  $1 in
    -h | --help )
	echo "Insert checker before basic block" 
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
    echo "Missing parameter: <basic_block>" 
    usage
    exit 1
fi

program=$1 # llvm bytecode program
basic_block=$2 # basic block
fn=$3 # function

base=$(basename "$program" ".ll")
checked=${base}\_c.ll
assembly=${base}\_c.s
binary=${base}\_c
#checkpid=`cat /proc/sys/kernel/random/uuid`
checkpid="c$(($RANDOM % 100))"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -checkfn=${fn} -checkpid=${checkpid}
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

objdump -d ${binary} | ./cval.py .cstart_$checkpid\0 .cend_$checkpid\0
cval0=$(echo $?)
objdump -d ${binary} | ./cval.py .cstart_$checkpid\1 .cend_$checkpid\1
cval1=$(echo $?)

echo "[INFO] Corrector value for basic block: ${cval0}"
echo "[INFO] Corrector value for checker: ${cval1}"

objdump -dF ${binary} | ./cslot.py $binary .cslot_$checkpid\0 $cval0
cslot0=$(echo $?)
objdump -dF ${binary} | ./cslot.py $binary .cslot_$checkpid\1 $cval1
cslot1=$(echo $?)

if [ $cslot0 != 0 ] || [ $cslot1 != 0 ]; then
    echo "Failed to initialize corrector slots"
    exit -1
fi

echo "[INFO] Success..."
exit 0
















    

