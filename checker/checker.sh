#!/bin/bash

usage()
{
    echo "Usage ./checker.sh <program> <basic_block>"
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

base=$(basename "$program" ".ll")
checked=${base}\_c.ll
assembly=${base}\_c.s
binary=${base}\_c
#checkid=`cat /proc/sys/kernel/random/uuid`
checkid="c$(($RANDOM % 100))"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -checkid=${checkid} -debug
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

cval0="$(objdump -d ${binary} | ./cval.py .cstart_$checkid\0 .cend_$checkid\0; echo $?)"
cval1="$(objdump -d ${binary} | ./cval.py .cstart_$checkid\1 .cend_$checkid\1; echo $?)"
#cval1=$(($cval0 ^ $cval1))

echo "Corrector value for basic block: ${cval0}"
echo "Corrector value for checker: ${cval1}"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -cval0=${cval0} -cval1=${cval1} -checkid=${checkid} -debug
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

















    

