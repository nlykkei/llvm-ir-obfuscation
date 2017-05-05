#!/bin/bash

usage()
{
    echo "Usage ./checker.sh <program> <function> <basic_block> <num_checkers>"
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

if [ "$3" == "" ]; then
    echo "Missing parameter: <fn>"
    usage
    exit 1
fi

if [ "$4" == "" ]; then
    echo "Missing parameter: <num_checkers>"
    usage
    exit 1
fi


program=$1 # llvm bytecode program
fn=$2 # function
basic_block=$3 # basic block
num_checkers=$4 # number of chained checkers

base=$(basename "$program" ".ll")
checked=${base}\_c.ll
assembly=${base}\_c.s
binary=${base}\_c
#checkpid=`cat /proc/sys/kernel/random/uuid`
checkpid="c$(($RANDOM % 100))"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkfn=${fn} -checkbb=${basic_block} -checkpid=${checkpid} -checknum=${num_checkers} -debug
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}


while IFS=":" read cstart cend cslot ctype
do
#echo Read: ${cstart} ${cend} ${cslot}
objdump -dz ${binary} | ./cval.py ${cstart} ${cend} ${ctype}
cval0=$(echo $?)
#echo "Corrector value for basic block: ${cval0}"
objdump -dF ${binary} | ./cslot.py $binary ${cslot} $cval0
done < corrector.dat

exit 0
















    

