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
test add-1 '(+)' '0'
test add-2 '(+ 1)' '1'
test add-3 '(+ 1 2)' '3'
test add-4 '(+ 1 2 3)' '6'
test add-5 '(+ 1 2 (+ 3 4))' '10'
test add-6 '(define a 1) (define b 2) (+ a b)' '3'
test_fail add-fail-1 '(+ 1 2 #t 4)'
test_fail add-fail-2 '(define a #f) (+ 1 2 a 4)'

println
test sub-1 '(- 1)' '-1'
test sub-2 '(- 1 3)' '-2'
test sub-4 '(- 10 3 2)' '5'
test sub-5 '(- 10 3 (- 2 1))' '6'
test sub-6 '(define a 1) (define b 5) (- a b)' '-4'
test_fail sub-fail-1 '(-)'
test_fail sub-fail-2 '(- 1 2 #t 4)'
test_fail sub-fail-3 '(define a #f) (- 1 2 a 4)'

println
test mul-1 '(*)' '1'
test mul-2 '(* 10)' '10'
test mul-3 '(* 1 2)' '2'
test mul-4 '(* 1 2 4)' '8'
test mul-5 '(* 1 2 (* 3 4))' '24'
test mul-6 '(define a 2) (define b 4) (* a b)' '8'
test_fail mul-fail-1 '(* 1 2 #t 4)'
test_fail mul-fail-2 '(define a #f) (* 1 2 a 4)'

println
test div-1 '(/ 1)' '1'
test div-2 '(/ 1 2)' '0'
test div-4 '(/ 10 2)' '5'
test div-5 '(/ 20 2 (/ 2 1))' '5'
test div-6 '(define a 5) (define b 2) (/ a b)' '2'
test_fail div-fail-1 '(/)'
test_fail div-fail-2 '(/ 10 2 #t 1)'
test_fail div-fail-3 '(define a #f) (/ 10 2 a 1)'

println
test abs-1 '(abs -5)' '5'
test abs-2 '(abs 5)' '5'
test_fail abs-fail-1 '(abs)'

println
test lt-1 '(< 2)' '#t'
test lt-2 '(< 1 2)' '#t'
test lt-3 '(< 2 2)' '#f'
test lt-4 '(< 1 2 3)' '#t'
test lt-5 '(< 2 1 2)' '#f'
test_fail lt-fail-1 '(<)'

println
test gt-1 '(> 2)' '#t'
test gt-2 '(> 1 2)' '#f'
test gt-3 '(> 2 2)' '#f'
test gt-4 '(> 2 1 3)' '#f'
test gt-5 '(> 3 2 1)' '#t'
test_fail gt-fail-1 '(>)'

println
test num-eq-1 '(= 2)' '#t'
test num-eq-2 '(= 1 2)' '#f'
test num-eq-3 '(= 2 2)' '#t'
test num-eq-4 '(= 2 1 3)' '#f'
test num-eq-5 '(= 3 2 1)' '#f'
test num-eq-6 '(= 2 2 2)' '#t'
test_fail num-eq-fail-1 '(=)'

println
test eq-1 '(eq? 2 2)' '#t'
test eq-2 '(eq? "string" "string")' '#f'
test eq-3 "(eq? 'symbol 'symbol)" '#t'
test eq-4 "(eq? '() '())" '#t'
test eq-5 "(define a '(1 2)) (define b '(1 2)) (eq? a b)" '#f'
test eq-6 "(define a '(1 2)) (define b a) (eq? a b)" '#t'
test_fail eq-fail-1 '(eq?)'
test_fail eq-fail-2 '(eq? 1)'

println
if [ "$errors" -gt 0 ]; then
    println_red "test result: $errors failed"
    exit 1
else
    println_green 'test result: ok'
fi
