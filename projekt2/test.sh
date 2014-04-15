#! /bin/bash

for i in {1..1000}
do
    if [[ `./proj02 1024 $i | wc -l` -eq $i ]]
    then
        echo "$i [OK]"
    else
        echo "$i [FAIL]"
    fi
done
