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

checked=$(basename "$program" ".ll")\-check.ll

opt -load ../cmake-build-debug/checker/libCheckerTPass.so -checkerT -S ${program} -o ${checked}









    

