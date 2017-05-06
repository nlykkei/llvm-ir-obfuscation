#!/bin/bash

usage()
{
    echo "Usage ./test.sh"
}

error()
{
    opt -view-cfg ${checked}
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

./checker.sh ${program} ${fn} ${basic_block} 1

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


# fac: if.then (3 checkers)

printf "[Testing] fac: if.then @ fac (3 checkers)\n"

basic_block="if.then"
fn="fac"

./checker.sh ${program} ${fn} ${basic_block} 3

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


./checker.sh ${program} ${fn} ${basic_block} 1

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

# fac: if.else (4 checkers)

printf "[Testing] fac: if.else @ fac (4 checkers)\n"

basic_block="if.else"
fn="fac"


./checker.sh ${program} ${fn} ${basic_block} 4

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

# pow

pow_t1_5="./pow_c 1 5"
pow_t2_7="./pow_c 2 7"
pow_t3_3="./pow_c 3 3"
pow_t4_3="./pow_c 4 3"
pow_t5_2="./pow_c 5 2"
pow_t2_5="./pow_c 2 5"
pow_t3_4="./pow_c 3 4"

program="../programs/ll/pow.ll"
base=$(basename "$program" ".ll")
checked=${base}\_c.ll
assembly=${base}\_c.s
binary=${base}\_c

# pow: if.else

printf "[Testing] pow: if.else @ power\n"

basic_block="if.else"
fn="power"

./checker.sh ${program} ${fn} ${basic_block} 1

res=$($pow_t1_5; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $pow_t1_5 ($res)"
    error
fi

res=$($pow_t2_7; echo $?)

if [ $res != 128 ]; then
    echo "Fail: $pow_t2_7 ($res)"
    error
fi

res=$($pow_t4_3; echo $?)

if [ $res != 64 ]; then
    echo "Fail: $pow_t4_3 ($res)"
    error
fi

# pow: if.else (2 checkers)

printf "[Testing] pow: if.else @ power (2 checkers)\n"

basic_block="if.else"
fn="power"

./checker.sh ${program} ${fn} ${basic_block} 2

res=$($pow_t1_5; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $pow_t1_5 ($res)"
    error
fi

res=$($pow_t2_7; echo $?)

if [ $res != 128 ]; then
    echo "Fail: $pow_t2_7 ($res)"
    error
fi

res=$($pow_t4_3; echo $?)

if [ $res != 64 ]; then
    echo "Fail: $pow_t4_3 ($res)"
    error
fi

# pow: if.then2

printf "[Testing] pow: if.then2 @ power\n"

basic_block="if.then2"
fn="power"

./checker.sh ${program} ${fn} ${basic_block} 1

res=$($pow_t1_5; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $pow_t1_5 ($res)"
    error
fi

res=$($pow_t2_7; echo $?)

if [ $res != 128 ]; then
    echo "Fail: $pow_t2_7 ($res)"
    error
fi

res=$($pow_t4_3; echo $?)

if [ $res != 64 ]; then
    echo "Fail: $pow_t4_3 ($res)"
    error
fi

# pow: if.then2 (5 checkers)

printf "[Testing] pow: if.then2 @ power (5 checkers)\n"

basic_block="if.then2"
fn="power"

./checker.sh ${program} ${fn} ${basic_block} 5

res=$($pow_t1_5; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $pow_t1_5 ($res)"
    error
fi

res=$($pow_t2_7; echo $?)

if [ $res != 128 ]; then
    echo "Fail: $pow_t2_7 ($res)"
    error
fi

res=$($pow_t4_3; echo $?)

if [ $res != 64 ]; then
    echo "Fail: $pow_t4_3 ($res)"
    error
fi

res=$($pow_t2_5; echo $?)

if [ ${res} != 32 ]; then
    echo "Fail: $pow_t2_5 ($res)"
    error
fi


res=$($pow_t3_4; echo $?)

if [ ${res} != 81 ]; then
    echo "Fail: $pow_t3_4 ($res)"
    error
fi

# fib

fib_t1="./fib_c 1"
fib_t2="./fib_c 2"
fib_t3="./fib_c 3"
fib_t5="./fib_c 5"
fib_t10="./fib_c 10"

program="../programs/ll/fib.ll"
base=$(basename "$program" ".ll")
checked=${base}\_c.ll
assembly=${base}\_c.s
binary=${base}\_c

# fib: if.else

printf "[Testing] fib: if.else @ fib\n"

basic_block="if.else"
fn="fib"

./checker.sh ${program} ${fn} ${basic_block} 1

res=$($fib_t1; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $fib_t1 ($res)"
    error
fi

res=$($fib_t5; echo $?)

if [ $res != 5 ]; then
    echo "Fail: $fib_t5 ($res)"
    error
fi

res=$($fib_t10; echo $?)

if [ $res != 55 ]; then
    echo "Fail: $fib_t10 ($res)"
    error
fi

# fib: if.else (2 checkers)

printf "[Testing] fib: if.else @ fib (2 checkers)\n"

basic_block="if.else"
fn="fib"

./checker.sh ${program} ${fn} ${basic_block} 2

res=$($fib_t1; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $fib_t1 ($res)"
    error
fi

res=$($fib_t5; echo $?)

if [ $res != 5 ]; then
    echo "Fail: $fib_t5 ($res)"
    error
fi

res=$($fib_t10; echo $?)

if [ $res != 55 ]; then
    echo "Fail: $fib_t10 ($res)"
    error
fi


# fib: if.else3

printf "[Testing] fib: if.else3 @ fib\n"

basic_block="if.else3"
fn="fib"

./checker.sh ${program} ${fn} ${basic_block} 1

res=$($fib_t1; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $fib_t1 ($res)"
    error
fi

res=$($fib_t5; echo $?)

if [ $res != 5 ]; then
    echo "Fail: $fib_t5 ($res)"
    error
fi

res=$($fib_t10; echo $?)

if [ $res != 55 ]; then
    echo "Fail: $fib_t10 ($res)"
    error
fi

# fib: if.else3 (7 checkers)

printf "[Testing] fib: if.else3 @ fib (7 checkers)\n"

basic_block="if.else3"
fn="fib"

./checker.sh ${program} ${fn} ${basic_block} 7

res=$($fib_t1; echo $?)

if [ $res != 1 ]; then
    echo "Fail: $fib_t1 ($res)"
    error
fi

res=$($fib_t5; echo $?)

if [ $res != 5 ]; then
    echo "Fail: $fib_t5 ($res)"
    error
fi

res=$($fib_t10; echo $?)

if [ $res != 55 ]; then
    echo "Fail: $fib_t10 ($res)"
    error
fi


echo "[Cleanup] Removing files..."
rm fac*
rm fib*
rm pow*

printf ".%.0s" {1..75}

echo

echo "[Success] All tests passed..."
exit 0


















    

