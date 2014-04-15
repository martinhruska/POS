#! /bin/bash

for i in {1..1000}
do
    res=`./proj02 $1 $i | wc -l`
    if [[ $res -eq $i ]]
    then
        echo "$i [OK]"
    else
        echo "$i [FAIL]"
    fi
done
