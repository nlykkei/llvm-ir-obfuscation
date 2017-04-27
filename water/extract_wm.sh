#!/bin/bash

usage()
{
    echo "Usage extract_wm.sh <program> <key> <prime> <prime>..."
}

case  $1 in
    -h | --help )
	echo "Extract a CRT-based watermark from a module"
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
    echo "Missing parameter: <key>"
    usage
    exit 1
fi

if [ $(wc -w <<< "${@:3}") -lt 2 ]; then
    echo "Missing paramater: <prime>"
    usage
    exit 1
fi

program=$1 # binary
key=$2 # encryption key
label="wm_split" # label
primes="${@:3}" # primes

watermark=$(objdump -dF ${program} | python3 ./extract_wm.py ${program} ${label} ${key} ${primes})

if [ $? -ne 0 ]; then
    echo "Error occurred: ${watermark}"
else
    echo ${watermark}
fi















    

