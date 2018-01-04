#!/bin/bash
./faultsim tests/c17.bench tests/c17.refinput myc17.out
DIFF= $(diff myc17.out tests/c17.refout)
if [ "$DIFF" == "" ];then
    echo "c17 success"
fi
echo "$DIFF"


./faultsim tests/c432.bench tests/c432.refinput myc432.out
DIFF= $(diff myc432.out tests/c432.refout)

if [ "$DIFF" == "" ];then
    echo "c432 success"
fi
echo "$DIFF"

./faultsim tests/c5315.bench tests/c5315.refinput myc5315.out
DIFF= $(diff myc5315.out tests/c5315.refout)
if [ "$DIFF" == "" ];then
    echo "c5315 success"
fi
echo "$DIFF"

./faultsim tests/hw.bench tests/hw.refinput myhw.out
DIFF= $(diff myhw.out tests/hw.refout)
if [ "$DIFF" == "" ];then
    echo "hw success"
fi
echo "$DIFF"

./faultsim tests/nandtest.bench tests/nandtest.refinput mynandtest.out
DIFF= $(diff mynandtest.out tests/nandtest.refout)
if [ "$DIFF" == "" ];then
    echo "nandtest success"
fi
echo "$DIFF"

./faultsim tests/target2.bench tests/target2.refinput mytarget2.out
DIFF= $(diff mytarget2.out tests/target2.refout)
if [ "$DIFF" == "" ];then
    echo "target2 success"
fi
echo "$DIFF"

./faultsim tests/target.bench tests/target.refinput mytarget.out
DIFF= $(diff mytarget.out tests/target.refout)
if [ "$DIFF" == "" ];then
    echo "target success"
fi
echo "$DIFF"

./faultsim tests/xor.bench tests/xor.refinput myxor.out
DIFF= $(diff myxor.out tests/xor.refout)
if [ "$DIFF" == "" ];then
    echo "xor success"
fi
echo "$DIFF"
