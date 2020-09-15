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
test_fail empty-list-unquoted '()'
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
test lt-6 '(< 1 2 1 2)' '#f'
test_fail lt-fail-1 '(<)'

println
test lte-1 '(<= 2)' '#t'
test lte-2 '(<= 1 2)' '#t'
test lte-3 '(<= 2 2)' '#t'
test lte-4 '(<= 1 2 2)' '#t'
test lte-5 '(<= 2 2 1)' '#f'
test lte-6 '(<= 2 2 1 2)' '#f'
test_fail lte-fail-1 '(<=)'

println
test gt-1 '(> 2)' '#t'
test gt-2 '(> 1 2)' '#f'
test gt-3 '(> 2 2)' '#f'
test gt-4 '(> 2 1 3)' '#f'
test gt-5 '(> 3 2 1)' '#t'
test gt-6 '(> 2 1 2 1)' '#f'
test_fail gt-fail-1 '(>)'

println
test gte-1 '(>= 2)' '#t'
test gte-2 '(>= 1 2)' '#f'
test gte-3 '(>= 2 2)' '#t'
test gte-4 '(>= 2 1 2)' '#f'
test gte-5 '(>= 3 2 2)' '#t'
test gte-6 '(>= 2 1 2 2)' '#f'
test_fail gte-fail-1 '(>=)'

println
test num-eq-1 '(= 2)' '#t'
test num-eq-2 '(= 1 2)' '#f'
test num-eq-3 '(= 2 2)' '#t'
test num-eq-4 '(= 2 1 3)' '#f'
test num-eq-5 '(= 3 2 1)' '#f'
test num-eq-6 '(= 2 2 2)' '#t'
test num-eq-7 '(= 2 2 1 1)' '#f'
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
test_fail proc-fail-1 '(define (f x y) 555) (f)'
test_fail proc-fail-2 '(define (f x y) 555) (f 1)'
test_fail proc-fail-3 '(define (f x y) 555) (f 1 2 3)'
test_fail proc-fail-4 '(define (#t x y) 555)'
test_fail proc-fail-5 '(define (f #t y) 555)'
test_fail proc-fail-6 '(define (f x #t) 555)'

println
test proc-varargs-1 '(define (f . args) args) (f)' '()'
test proc-varargs-2 '(define (f . args) args) (f 1)' '(1)'
test proc-varargs-3 '(define (f . args) args) (f 1 2)' '(1 2)'

test proc-varargs-4 '(define (f x . args)
    (display x) (display args)) (f 1)' '1()'
test proc-varargs-5 '(define (f x . args)
    (display x) (display args)) (f 1 2)' '1(2)'
test proc-varargs-6 '(define (f x . args)
    (display x) (display args)) (f 1 2 3)' '1(2 3)'

test_fail proc-varargs-fail-1 '(define (f x . args) 1) (f)'
test_fail proc-varargs-fail-2 '(define (f x y . args) 1) (f)'
test_fail proc-varargs-fail-3 '(define (f x y . args) 1) (f 1)'

println
test listproc-1 '(list)' '()'
test listproc-2 '(list 1 2)' '(1 2)'
test listproc-3 '(list (list 1 2) (list 3))' '((1 2) (3))'

println
test let-1 '(let ((v 30)) (+ v v))' '60'
test let-2 '(let ((x 2) (y 3)) (* x y))' '6'
test let-3 '(let ((x 2) (y 3)) (let ((x 7) (z (+ x y))) (* z x)))' '35'
test_fail test-fail-let-1 '(let)'
test_fail test-fail-let-2 '(let 1)'
test_fail test-fail-let-3 '(let (x) x)'
test_fail test-fail-let-4 '(let ((1)) 1)'
test_fail test-fail-let-5 '(let ((x)) x)'

println
test cond-1 "(cond ((= 1 1) 'a) ((= 2 2) 'b))" 'a'
test cond-2 "(cond ((= 1 1) 'a 'c) ((= 2 2) 'b))" 'c'
test cond-3 "(cond ((> 1 1) 'a) ((= 2 2) 'b))" 'b'
test cond-4 "(cond ((> 1 1) 'a) ((< 2 2) 'b) (else 'c))" 'c'
test cond-5 "(cond ((> 1 1) 'a) ((< 2 2) 'b) (else 'c 'd))" 'd'
test cond-6 "(cond (1 'a) ((< 2 2) 'b))" 'a'
test cond-7 "(cond (1 'a 'c) ((< 2 2) 'b))" 'c'
test_fail cond-fail-1 '(cond)'

println
test is-int-1 "(integer? '())" '#f'
test is-int-2 '(integer? 123)' '#t'
test is-int-3 "(integer? (car '(1)))" '#t'

println
test is_list-1 "(list? '(a b c))" '#t'
test is_list-2 "(list? '())" '#t'
test is_list-3 "(list? '(a . b))" '#f'

println
test is-null-1 "(null? '())" '#t'
test is-null-2 '(null? #t)' '#f'
test is-null-3 "(null? (cdr '(1)))" '#t'

println
test is-pair-1 '(pair? #t)' '#f'
test is-pair-2 "(pair? '())" '#f'
test is-pair-3 "(pair? '(1))" '#t'
test is-pair-4 "(pair? '(1 2))" '#t'
test is-pair-5 "(pair? (list 1 2))" '#t'

println
test is-proc-1 '(procedure? car)' '#t'
test is-proc-2 "(procedure? 'car)" '#f'
test is-proc-3 '(procedure? (lambda (x) (* x x)))' '#t'
test is-proc-4 "(procedure? '(lambda (x) (* x x)))" '#f'

println
test is-str-1 "(string? 'string)" '#f'
test is-str-2 '(string? "123")' '#t'
test is-str-3 '(string? "")' '#t'

println
test is-sym-1 "(symbol? 'symbol)" '#t'
test is-sym-2 '(symbol? "123")' '#f'
test is-sym-3 "(symbol? (if #f 'a 2))" '#f'

println
test set-1 '(define x #t) (set! x 123) x' '123'
test set-2 '(define x 123) (set! x (= 1 2)) x' '#f'
test set-3 '(define x 1) (define (f) (set! x (+ x 1))) (f) (f) x' '3'
test_fail set-fail-1 '(set!)'
test_fail set-fail-2 '(set! x)'
test_fail set-fail-3 '(set! 1 1)'
test_fail set-fail-4 '(set! x 1) x' '1'

println
test set-car-1 "(define x '(1 2)) (set-car! x 555) x" '(555 2)'
test set-car-2 "(define x '(1 2)) (set-car! (cdr x) 555) x" '(1 555)'
test set-car-3 "(define x '(1 . 2)) (set-car! x 555) x" '(555 . 2)'
test set-car-4 "(define x '(1 . 2)) (set-car! x '(5 6 7)) x" '((5 6 7) . 2)'
test_fail set-car-fail-1 '(set-car!)'
test_fail set-car-fail-2 '(set-car! x)'
test_fail set-car-fail-3 '(set-car! 1 1)'
test_fail set-car-fail-4 '(define x 1) (set-car! x 5)'
test_fail set-car-fail-5 "(define x '(1 . 2)) (set-car! (cdr x) 555)"
test_fail set-car-fail-6 "(define x '(1)) (set-car! (cdr x) 555)"

println
test set-cdr-1 "(define x '(1 2)) (set-cdr! x 555) x" '(1 . 555)'
test set-cdr-2 "(define x '(1 2)) (set-cdr! (cdr x) 555) x" '(1 2 . 555)'
test set-cdr-3 "(define x '(1 2)) (set-cdr! (cdr x) '(555)) x" '(1 2 555)'
test set-cdr-4 "(define x '(1 . 2)) (set-cdr! x 555) x" '(1 . 555)'
test set-cdr-5 "(define x '(1 . 2)) (set-cdr! x '(555)) x" '(1 555)'
test_fail set-cdr-fail-1 '(set-cdr!)'
test_fail set-cdr-fail-2 '(set-cdr! x)'
test_fail set-cdr-fail-3 '(set-cdr! 1 1)'
test_fail set-cdr-fail-4 '(define x 1) (set-cdr! x 5)'

println
test display-01 '(display #t)' '#t'
test display-02 '(display #f)' '#f'
test display-03 "(display '())" '()'
test display-04 "(display '(1 2 3))" '(1 2 3)'
test display-05 '(display 55)' '55'
test display-06 '(display 55)(display 88)' '5588'
test display-07 '(display "str!")' 'str!'
test display-08 '(display "THIS.\nIS.\n\\"PONYO\\"!\n")' 'THIS.\nIS.\n"PONYO"!'
test display-09 "(display 'sym)" 'sym'
test display-10 '(display #t)' '#t'
test display-11 '(display (if #t 12345))' '12345'
test display-12 '(display (define x 1))' '#<void>'
test_fail display-fail-1 '(display)'
test_fail display-fail-2 '(display 1 2)'

println
test apply-01 "(apply + '())" '0'
test apply-02 "(apply + 1 2 '(3 4 5))" '15'
test apply-03 "(define (test . ns)
                (define (iter ns res)
                  (if (null? ns)
                      res
                      (iter (cdr ns) (+ res (* (car ns) (car ns))))))
              (iter ns 0))
              (apply test 1 2 3 4 '(5))" '55'
test apply-04 "(define first
                (lambda (ls)
                  (apply (lambda (x . y) x) ls)))
              (first '(a b c d))" 'a'
test apply-05 "(define rest
                (lambda (ls)
                  (apply (lambda (x . y) y) ls)))
              (rest '(a b c d))" '(b c d)'
test apply-06 "(apply append
                '(1 2 3)
                '((a b) (c d e) (f)))" '(1 2 3 a b c d e f)'
test apply-07 "(apply cons 1 '(()))" '(1)'
test apply-08 "(apply cons '(1 ()))" '(1)'
test apply-09 "(define a 3) (apply list 1 2 '(a))" '(1 2 a)'
test apply-10 "(define a 2) (apply list 1 a '(3))" '(1 2 3)'
test_fail apply-fail-1 "(apply)"
test_fail apply-fail-2 "(apply 1)"
test_fail apply-fail-3 "(apply 1 2)"
test_fail apply-fail-4 "(apply + 1)"
test_fail apply-fail-5 "(apply + 1 2)"

println
test length-1 "(length '(a b c))" '3'
test length-2 "(length '(a (b) (c d e)))" '3'
test length-3 "(length '())" '0'
test_fail length-fail-1 "(length '(a . b))"
test_fail length-fail-2 "(length 'a)"

println
test append-1 "(append '() '())" '()'
test append-2 "(append '(x) '(y))" '(x y)'
test append-3 "(append '(a) '(b c d))" '(a b c d)'
test append-4 "(append '(a (b)) '((c)))" '(a (b) (c))'
test append-5 "(append '(x) '(y) '(z))" '(x y z)'
test append-6 '(append)' '()'
test append-7 '(append 1)' '1'
test append-8 "(append '(1))" '(1)'

println
test reverse-1 "(reverse '(a b c))" '(c b a)'
test reverse-2 "(reverse '(a (b c) d (e (f))))" '((e (f)) d (b c) a)'
test reverse-3 "(reverse '())" '()'
test_fail reverse-fail-1 "(reverse '(a . b))"
test_fail reverse-fail-2 "(reverse 'a)"

println
test memq-1 "(memq 'a '(a b c))" '(a b c)'
test memq-2 "(memq 'b '(a b c))" '(b c)'
test memq-3 "(memq 'a '(b c d))" '#f'
test memq-4 "(memq (list 'a) '(b (a) c))" '#f'
test memq-5 "(memq 101 '(100 101 102))" '(101 102)'

println
test map-1 "(map cadr '((a b) (d e) (g h)))" '(b e h)'
test map-2 "(map (lambda (n) (* n n)) '(1 2 3 4 5))" '(1 4 9 16 25)'
test map-3 "(map + '(1 2 3) '(4 5 6))" '(5 7 9)'
test map-4 "(let ((count 0))
              (map (lambda (ignored)
                     (set! count (+ count 1))
                     count)
                   '(a b)))" '(1 2)'

println
if [ "$errors" -gt 0 ]; then
    println_red "test result: $errors failed"
    exit 1
else
    println_green 'test result: ok'
fi
