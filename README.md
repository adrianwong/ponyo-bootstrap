# ponyo-bootstrap

__ponyo-bootstrap__ is an interpreter for a limited subset of Scheme, written
in C. Its raison d'être is to bootstrap __ponyo__, a self-hosting Scheme-to-?
compiler.

To that end, __ponyo-bootstrap__ is a quick-and-dirty implementation, but not
_so_ quick-and-dirty such that it makes debugging Scheme programs overly
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

* [ ] Garbage collector.

## Metacircular

Several years ago, late to the programming party but filled with the enthusiasm
of having embraced a new career, I decided that working my way through [SICP][4]
was how I would "get good". This is how it went:

> **Wise old man of Chapter 4:** Child, you have braved the elements, slashed
your way through four hundred pages of dense text, and battled courageously
against two hundred exercises in order to reach my doorstep. You have earned the
right: ask your question, and I shall answer.
>
> **Me**: I'm tired. Do you have any pie?

Several years later, I still have no intention of completing the journey I
started. However! __ponyo-bootstrap__ _can_ run the metacircular evaluator from
Chapter 4! Surely that counts for something?

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

[1]: https://craftinginterpreters.com
[2]: https://github.com/rui314/minilisp
[3]: http://michaux.ca/articles/scheme-from-scratch-introduction
[4]: https://mitpress.mit.edu/sites/default/files/sicp/index.html
[5]: https://www.scheme.com/tspl4/
