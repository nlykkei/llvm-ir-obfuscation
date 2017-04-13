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

# fac

fac_t1="./fac_c 1"
fac_t2="./fac_c 2"
fac_t3="./fac_c 3"
fac_t4="./fac_c 4"
fac_t5="./fac_c 5"
fac_t6="./fac_c 6"
fac_t7="./fac_c 7"
fac_t8="./fac_c 8"
fac_t9="./fac_c 9"
fac_t10="./fac_c 10"

program="../programs/ll/fac.ll"
base=$(basename "$program" ".ll")
checked=${base}\_c.ll
assembly=${base}\_c.s
binary=${base}\_c

# fac: if.then

printf "[Testing] fac: if.then @ fac\n"

basic_block="if.then"
fn="fac"
checkpid="c$(($RANDOM % 100))"
seed=$RANDOM

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -checkfn=${fn} -checkpid=${checkpid} -seed=${seed}
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

cval0="$(objdump -d ${binary} | ./cval.py .cstart_$checkpid\0 .cend_$checkpid\0; echo $?)"
cval1="$(objdump -d ${binary} | ./cval.py .cstart_$checkpid\1 .cend_$checkpid\1; echo $?)"

echo "Corrector value for basic block: ${cval0}"
echo "Corrector value for checker: ${cval1}"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -checkfn=${fn} -cval0=${cval0} -cval1=${cval1} -checkpid=${checkpid} -seed=${seed}
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

res=$($fac_t1; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $fac_t1 ($res)"
    error
fi

res=$($fac_t4; echo $?)

if [ $res != 24 ]; then
    echo "Fail: $fac_t1 ($res)"
    error
fi

res=$($fac_t5; echo $?)

if [ $res != 120 ]; then
    echo "Fail: $fac_t1 ($res)"
    error
fi

# fac: if.else

printf "[Testing] fac: if.else @ fac\n"

basic_block="if.else"
fn="fac"
checkpid="c$(($RANDOM % 100))"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -checkfn=${fn} -checkpid=${checkpid}
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

cval0="$(objdump -d ${binary} | ./cval.py .cstart_$checkpid\0 .cend_$checkpid\0; echo $?)"
cval1="$(objdump -d ${binary} | ./cval.py .cstart_$checkpid\1 .cend_$checkpid\1; echo $?)"

echo "Corrector value for basic block: ${cval0}"
echo "Corrector value for checker: ${cval1}"

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked} -checkbb=${basic_block} -checkfn=${fn} -cval0=${cval0} -cval1=${cval1} -checkpid=${checkpid} #-debug
llc ${checked} -o ${assembly}
clang ${assembly} -o ${binary}

res=$($fac_t1; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $fac_t1 ($res)"
    error
fi

res=$($fac_t4; echo $?)

if [ $res != 24 ]; then
    echo "Fail: $fac_t1 ($res)"
    error
fi

res=$($fac_t5; echo $?)

if [ $res != 120 ]; then
    echo "Fail: $fac_t1 ($res)"
    error
fi

echo "[Success] All tests passed..."
exit 0

















    

