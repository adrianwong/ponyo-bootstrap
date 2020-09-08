#!/bin/bash

prog=ponyo

green='\033[0;32m'
red='\033[0;31m'
reset='\033[0m'

println() { printf '\n'; }
println_green() { printf '%b%s%b\n' "$green" "$1" "$reset"; }
println_red() { printf '%b%s%b\n' "$red" "$1" "$reset"; }

errors=0
padding=$(printf '.%.0s' {1..50})

function run_test() {
    printf 'testing %s %s ' "$1" "${padding:${#1}}"

    exp=$(printf '%b' "$3")
    act=$(printf '%b' "$2" | ./"$prog" 2>&1)
}

function test() {
    run_test "$1" "$2" "$3"

    if [ "$exp" = "$act" ]; then
        println_green 'ok'
    else
        println_red 'failed'

        println
        println_green 'expected:'
        println_green "$exp"

        println
        println_red 'actual:'
        println_red "$act"

        println

        errors=$((errors+1))
    fi
}

function test_fail() {
    run_test "$1" "$2"

    if [ "$?" -eq 0 ]; then
        println_red 'failed'

        println
        println_red 'expected test to fail'

        println

        errors=$((errors+1))
    else
        println_green 'ok'
    fi
}

println
test whitespace-1 '; this is a comment' ''
test whitespace-2 '  #t  \r\n\t#f  ' '#t\n#f'
test whitespace-3 '#t;this is a comment\r\n#f' '#t\n#f'

println
test bool-t '#t' '#t'
test bool-f '#f' '#f'
test bool-nospace '#t#f' '#t\n#f'
test_fail bool-incomplete '#'

println
test int-pos '12345' '12345'
test int-neg '-12345' '-12345'
test int-pos-neg '12345 -12345' '12345\n-12345'
# Existing implementations treat '12345-12345' as an identifier.
test int-nospace '12345-12345' '12345\n-12345'

println
test string-single '"string"' '"string"'
test string-multi-1 '"multiline\nstring"' '"multiline\\nstring"'
test string-multi-2 '"multiline\\nstring"' '"multiline\\nstring"'
test string-escaped-quote '"str\\"ing"' '"str\\"ing"'
test string-escaped-backslash '"str\\\\ing"' '"str\\\\ing"'
test string-nospace '"string1""string2"' '"string1"\n"string2"'
test string-empty '""' '""'
test_fail string-unterminated '"unterminated'

println
test empty-list-1 "'()" '()'
test empty-list-2 "'(     )" '()'
test list-1 "'(1 \"string\" #t)" '(1 "string" #t)'
test list-2 "'(  1    \"string\"    #t  )" '(1 "string" #t)'
test list-nested "'((123 \"string\") #f #t ())" '((123 "string") #f #t ())'
test_fail empty_list_unquoted '()'
test_fail list-unclosed-list '('
test_fail list-unexpected-terminator ')'

println
test improper-list-1 "'(1 . 2)" '(1 . 2)'
test improper-list-2 "'(    1  .  2    )" '(1 . 2)'
test improper-list-nested "'(1 . (2 . 3))" '(1 2 . 3)'
test improper-proper-list "'(1 . (2 . (3 . ())))" '(1 2 3)'
test_fail improper-list-rparen '(1 . 2 . 3)'

println
test quote-bool "'#t" '#t'
test quote-int "'12345" '12345'
test quote-string "'\"string\"" '"string"'
test quote-list "'(#t 12345 \"string\")" '(#t 12345 "string")'
test quote-list-quote "'(#t 555 '(#f ()))" '(#t 555 (quote (#f ())))'
test_fail quote-quote "''"
test_fail quote-no-args '(quote)'

println
test define '(define a 1) a' '1'
test define-change '(define a 1) (define a 2) a' '2'
test define-define '(define a 1) (define b a) b' '1'
test_fail define-no-args '(define)'
test_fail define-not-symbol '(define 1 1)'

println
if [ "$errors" -gt 0 ]; then
    println_red "test result: $errors failed"
    exit 1
else
    println_green 'test result: ok'
fi
