#!/bin/bash

prog=ponyo

dir=tests
ext=scm
exp=exp

green='\033[0;32m'
greenbold='\033[1;32m'
red='\033[0;31m'
redbold='\033[1;31m'
reset='\033[0m'

println() { printf '\n'; }
println_green() { printf '%b%s%b\n' "$green" "$1" "$reset"; }
println_greenbold() { printf '%b%s%b\n' "$greenbold" "$1" "$reset"; }
println_red() { printf '%b%s%b\n' "$red" "$1" "$reset"; }
println_redbold() { printf '%b%s%b\n' "$redbold" "$1" "$reset"; }

errors=0
for testfile in "$dir"/*."$ext"; do
    basename=$(basename "$testfile")
    filename=${basename%.$ext}

    padding=$(printf '.%.0s' {1..25})
    printf 'testing %s %s ' "$filename" "${padding:${#filename}}"

    if [ ! -f "$testfile" ]; then
        println_redbold "ERROR [missing $testfile]"

        errors=$((errors+1))
        continue
    fi

    expfile="$dir/$filename.$exp"
    if [ ! -f "$expfile" ]; then
        println_redbold "ERROR [missing $expfile]"

        errors=$((errors+1))
        continue
    fi

    expected=$(cat "$expfile")
    actual=$(./"$prog" "$testfile" 2>&1)

    if [ "$expected" = "$actual" ]; then
        println_greenbold 'ok'
    else
        println_redbold 'failed'

        println
        println_greenbold 'expected:'
        println_green "$expected"

        println
        println_redbold 'actual:'
        println_red "$actual"

        println

        errors=$((errors+1))
    fi
done

println
if [ "$errors" -gt 0 ]; then
    println_redbold "test result: $errors failed"
    exit 1
else
    println_greenbold "test result: ok"
fi
