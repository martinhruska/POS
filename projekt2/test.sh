#! /bin/bash

lastRes=0
for i in {1..1000}
do
    res=`./proj02 $1 $i | wc -l`
    expRes=$(($lastRes+1))
    if [[ $res -eq $i && $res -ne $lastRes && $res -eq $expRes ]]
    then
        lastRes=$res
        echo "$i [OK]"
    else
        echo "$i [FAIL]"
    fi
done
