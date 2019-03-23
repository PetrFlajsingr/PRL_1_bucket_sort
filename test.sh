#!/bin/bash

function log2 {
    local x=0
    for (( y=$1-1 ; $y > 0; y >>= 1 )) ; do
        let x=$x+1
    done
    echo ${x}
}

if [[ $# -lt 1 ]];then
    numbers=10;
else
    numbers=$1;
fi;

dd if=/dev/random bs=1 count=${numbers} of=numbers

if [[ ${numbers} -eq 1 ]];then
    proc_count=1
else
    pw=$[$(log2 ${numbers})]
    proc_count=$[2**pw - 1]
fi;

mpic++ -std=c++17 --prefix /usr/local/share/openmpi -o PRL_1 bks.cpp

mpirun --prefix /usr/local/share/openmpi -np ${proc_count} ./PRL_1

rm -f numbers