# ponyo-bootstrap

__ponyo-bootstrap__ is an interpreter for a limited subset of Scheme, written
in C. Its raison d'être is to bootstrap __ponyo__, a self-hosting Scheme-to-?
compiler.

To that end, __ponyo-bootstrap__ is a quick-and-dirty implementation, but not
_so_ quick-and-dirty such that it would make debugging Scheme programs overly
difficult.

## Use

```
$ make
$ ./ponyo
```

## Test

```
$ make test
```

## TODO

* [x] Implement garbage collector. (This was undertaken as a learning exercise.
The changes cloud the implementation a fair bit, read [this][6] to understand
why.)

## Metacircular

__ponyo-bootstrap__ can run the metacircular evaluator from [SICP][4] Chapter
4.1. This is a source of some amusement to me, as I made a start on the chapter
some several years back but never got around to finishing it.

```scheme
$ ./ponyo

(load "metac.scm")

;;; M-Eval input:
(define x (cons 3 (cons 2 (cons 1 '()))))

;;; M-Eval value:
ok

;;; M-Eval input:
x

;;; M-Eval value:
(3 2 1)

;;; M-Eval input:
(car x)

;;; M-Eval value:
3

;;; M-Eval input:
(cdr x)

;;; M-Eval value:
(2 1)

;;; M-Eval input:
(define y (lambda (x) (if x 'foo 'bar)))

;;; M-Eval value:
ok

;;; M-Eval input:
y

;;; M-Eval value:
(compound-procedure (x) ((if x (quote foo) (quote bar))) <procedure-env>)

;;; M-Eval input:
(y false)

;;; M-Eval value:
bar
```

## Inspiration / Resources

* [Crafting Interpreters][1]
* [minilisp][2]
* [Scheme from Scratch][3]
* [SICP][4]
* [The Scheme Programming Language, 4th Edition][5]
* [Baby's First Garbage Collector][7]
* [GC in 50 lines][8]

[1]: https://craftinginterpreters.com
[2]: https://github.com/rui314/minilisp
[3]: http://michaux.ca/articles/scheme-from-scratch-introduction
[4]: https://mitpress.mit.edu/sites/default/files/sicp/index.html
[5]: https://www.scheme.com/tspl4/
[6]: http://peter.michaux.ca/articles/scheme-from-scratch-bootstrap-v0_22-garbage-collection
[7]: https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/
[8]: https://github.com/jorendorff/gc-in-50-lines
