#!/usr/bin/env bash

make

i=0

for file in ./tests/cpu_instrs/individual/*; do
    i=$((i + 1))

    if [ $i -lt 9 ] && ! [ $i -eq 7 ]; then
        timeout 3 ./build/main "$file"
    else
        timeout 10 ./build/main "$file"
    fi
done
