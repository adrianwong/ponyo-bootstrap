#!/bin/bash

prog=ponyo

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
padding=$(printf '.%.0s' {1..50})
pattern='s/#\\/#\\\\/g'

# test 'name' 'input' 'expected'
function test() {
    # Escape the '\' in '#\'.
    escaped_2=$(echo "$2" | sed -E "$pattern")
    escaped_3=$(echo "$3" | sed -E "$pattern")

    printf 'testing %s %s ' "$1" "${padding:${#1}}"

    expected=$(printf '%b' "$escaped_3")
    actual=$(printf '%b' "$escaped_2" | ./"$prog" 2>&1)

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
}

println
test bool-t '#t' '#t'
test bool-f '#f' '#f'
test bool-t-f-nospace '#t#f' '#t\n#f'
test bool-dangling '#' "error: [line 1] dangling '#'"

println
test char-a '#\a' '#\a'
test char-semicolon '#\;' '#\;'
test char-space '#\ ' '#\space'
test char-tab '#\\t' '#\tab'
test char-return '#\\r' '#\return'
test char-newline '#\\n' '#\newline'
test char-space-name '#\space' '#\space'
test char-tab-name '#\tab' '#\tab'
test char-return-name '#\return' '#\return'
test char-newline-name '#\newline' '#\newline'
test char-n-newline '#\n#\newline' '#\n\n#\newline'
test char-no-backslash '#a' "error: [line 1] invalid sequence '#a'"
test char-dangling '#\' "error: [line 1] dangling '#\'"
test char-case '#\Space' "error: [line 1] invalid character name '#\Space'"
test char-spa '#\spac' "error: [line 1] invalid character name '#\spac'"

println
test int-pos '12345' '12345'
test int-neg '-12345' '-12345'
test int-pos-neg '12345 -12345' '12345\n-12345'
# FIXME existing implementations treat '12345-12345' as an identifier.
test int-pos-neg-nospace '12345-12345' '12345\n-12345'

println
test string-single '"string"' '"string"'
test string-multi '"multiline\nstring"' '"multiline\\nstring"'
test string-escape-single '"\"string"' '"\"string"'
test string-escape-multi '"multiline\n\"string"' '"multiline\\n\"string"'
test string-nospace '"string1""string2"' '"string1"\n"string2"'
test string-empty '""' '""'
test string-unterminated '"unterminated' 'error: [line 1] unterminated string'

println
test quote-bool "'#t" '#t'
test quote-char "'#\a" '#\a'
test quote-int "'12345" '12345'
test quote-string "'\"string\"" '"string"'
test quote-quote-bool "''#t" "'#t"
test quote-quote-char "''#\a" "'#\a"
test quote-quote "''" 'error: [line 1] dangling quote'
test quote-dangling "'" 'error: [line 1] dangling quote'
test quote-dangling-whitespace "'    " 'error: [line 1] dangling quote'

println
test empty-list-literal "'()" '()'
test empty-list '()' '()' # This should error once we implement 'apply'.
test list '(1 #\a #t)' '(1 #\a #t)'
test list-nested '((123 "string") #\a #t ())' '((123 "string") #\a #t ())'
test list-dangling-left '(' "error: [line 1] dangling '('"
test list-dangling-right ')' "error: [line 1] dangling ')'"

println
if [ "$errors" -gt 0 ]; then
    println_redbold "test result: $errors failed"
    exit 1
else
    println_greenbold 'test result: ok'
fi
