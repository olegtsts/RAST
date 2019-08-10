#!/bin/bash

for bn in {1..2}; do
    make timers_benchmark_$bn
    echo > timers_benchmark_$bn.txt && for i in {1..100}; do (time ./timers_benchmark_$bn) 1>> /dev/null 2>> timers_benchmark_$bn.txt ;done
    echo "benchmark $bn"
    for tt in real user sys; do
        echo $tt
        grep $tt timers_benchmark_$bn.txt | awk '{print $2}' |sed -e 's/.*m//g' | sed -e 's/s//g' | awk '{SUM += $1} END {print SUM / 100}'
    done
done
