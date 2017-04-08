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

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block}
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}
cval="$(objdump -d ${binary} | ./cval.py .cstart .cend; echo $?)"

echo Corrector value: ${cval}

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -cval=${cval}
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}














    

