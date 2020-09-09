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
test car-1 "(car '(1))" '1'
test car-2 "(car '(1 2))" '1'
test car-3 "(car '((1 2 4) 8))" '(1 2 4)'
test car-4 "(define a '(8 4 2)) (car a)" '8'
test_fail car-fail-1 "(car '())"
test_fail car-fail-2 '(car)'
test_fail car-fail-3 '(car a)'

println
test cdr-1 "(cdr '(1))" '()'
test cdr-2 "(cdr '(1 2))" '(2)'
test cdr-3 "(cdr '((1 2) (4 8)))" '((4 8))'
test cdr-4 "(define a '(8 4 2)) (cdr a)" '(4 2)'
test_fail cdr-fail-1 "(cdr '())"
test_fail cdr-fail-2 '(cdr)'
test_fail cdr-fail-3 '(cdr a)'

println
test cons-1 '(cons 1 2)' '(1 . 2)'
test cons-2 "(cons 1 '(2))" '(1 2)'
test cons-3 "(cons '(1 2) '(3 4))" '((1 2) 3 4)'
test cons-4 "(cons '(1 2) '(3 . 4))" '((1 2) 3 . 4)'
test cons-5 "(cons 4 (cons 8 '()))" '(4 8)'
test_fail cons-fail-1 '(cons)'
test_fail cons-fail-2 '(cons 1)'

println
test if-1 '(if #t 1 2)' '1'
test if-2 '(if 10 1 2)' '1'
test if-3 "(if '() 1 2)" '1'
test if-4 '(if #f 1 2)' '2'
test if-5 '(if #t 1)' '1'
test if-6 '(if #f 1)' ''
test_fail if-fail-1 '(if)'
test_fail if-fail-2 '(if 1)'

println
test not-1 '(not #t)' '#f'
test not-2 '(not 3)' '#f'
test not-3 "(not '(3))" '#f'
test not-4 '(not #f)' '#t'
test not-5 "(not '())" '#f'
test not-6 '(not "string")' '#f'
test_fail not-fail-1 '(not)'

println
test and-1 '(and (= 2 2) (> 2 1))' '#t'
test and-2 '(and (= 2 2) (< 2 1) #t)' '#f'
test and-3 "(and 1 2 'c '(f g))" '(f g)'
test and-4 '(and)' '#t'

println
test or-1 '(or (= 2 2) (> 2 1))' '#t'
test or-2 '(or (< 2 1) (= 2 1) #f)' '#f'
test or-3 "(or #f 2 'c '(f g))" '2'
test or-4 '(or)' '#f'

println
test lambda-1 '((lambda () 555))' '555'
test lambda-2 '((lambda (n) (* n n)) 10)' '100'
test lambda-3 '(lambda () 555)' '#<compound-procedure>'
test lambda-4 '((lambda (n) (* ((lambda (n) (+ n n)) 5) n)) 10)' '100'
test lambda-5 '((lambda (x y) (+ ((lambda (x) (+ x y)) 5) x)) 8 10)' '23'

println
test proc-1 '(define (square n) (* n n)) (square 5)' '25'
test proc-2 '(define (square n) (* n n))' ''
test proc-3 '(define (square n) (* n n)) (square (square 5))' '625'
test proc-4 '(define (f n) (if (= n 0) 1 (* n (f (- n 1))))) (f 6)' '720'
test proc-5 '(define (g n)
    (define (f n a) (if (= n 0) a (f (- n 1) (* n a)))) (f n 1))
    (g 6)' '720'
test proc-6 '(define (square) (define (f n) (* n n)) f) ((square) 15)' '225'

println
if [ "$errors" -gt 0 ]; then
    println_red "test result: $errors failed"
    exit 1
else
    println_green 'test result: ok'
fi
