;;;-----------------------------------------------------------------------------
;;; PONYO STANDARD LIBRARY
;;;-----------------------------------------------------------------------------

;;;-----------------------------------------------------------------------------
;;; NUMBERS
;;;-----------------------------------------------------------------------------

(define number? integer?)

(define (abs n)
  (if (< n 0) (- n) n))

;;;-----------------------------------------------------------------------------
;;; BOOLEANS
;;;-----------------------------------------------------------------------------

(define (not x)
  (if x #f #t))

;;;-----------------------------------------------------------------------------
;;; PAIRS AND LISTS
;;;-----------------------------------------------------------------------------

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))
(define (caaar x) (car (car (car x))))
(define (caadr x) (car (car (cdr x))))
(define (cadar x) (car (cdr (car x))))
(define (caddr x) (car (cdr (cdr x))))
(define (cdaar x) (cdr (car (car x))))
(define (cdadr x) (cdr (car (cdr x))))
(define (cddar x) (cdr (cdr (car x))))
(define (cdddr x) (cdr (cdr (cdr x))))
(define (caaaar x) (car (car (car (car x)))))
(define (caaadr x) (car (car (car (cdr x)))))
(define (caadar x) (car (car (cdr (car x)))))
(define (caaddr x) (car (car (cdr (cdr x)))))
(define (cadaar x) (car (cdr (car (car x)))))
(define (cadadr x) (car (cdr (car (cdr x)))))
(define (caddar x) (car (cdr (cdr (car x)))))
(define (cadddr x) (car (cdr (cdr (cdr x)))))
(define (cdaaar x) (cdr (car (car (car x)))))
(define (cdaadr x) (cdr (car (car (cdr x)))))
(define (cdadar x) (cdr (car (cdr (car x)))))
(define (cdaddr x) (cdr (car (cdr (cdr x)))))
(define (cddaar x) (cdr (cdr (car (car x)))))
(define (cddadr x) (cdr (cdr (car (cdr x)))))
(define (cdddar x) (cdr (cdr (cdr (car x)))))
(define (cddddr x) (cdr (cdr (cdr (cdr x)))))

(define (length list)
  (define (iter list count)
    (if (null? list)
        count
        (iter (cdr list) (+ count 1))))
  (iter list 0))

(define (append . args)
  (define (f ls args)
    (define (g ls)
      (if (null? ls)
          (f (car args) (cdr args))
          (cons (car ls) (g (cdr ls)))))
    (if (null? args)
        ls
        (g ls)))
  (f '() args))

(define (reverse list)
  (define (iter list rev)
    (if (null? list)
        rev
        (iter (cdr list) (cons (car list) rev))))
  (iter list '()))

(define (memq obj list)
  (cond ((null? list) #f)
        ((eq? obj (car list)) list)
        (else (memq obj (cdr list)))))

;;;-----------------------------------------------------------------------------
;;; CONTROL FEATURES
;;;-----------------------------------------------------------------------------

(define (map proc ls . more)
  (define (map1 ls)
    (if (null? ls)
        '()
        (cons (proc (car ls))
              (map1 (cdr ls)))))
  (define (map-more ls more)
    (if (null? ls)
        '()
        (cons (apply proc (car ls) (map car more))
              (map-more (cdr ls) (map cdr more)))))
  (if (null? more)
      (map1 ls)
      (map-more ls more)))

;;;-----------------------------------------------------------------------------
;;; INPUT AND OUTPUT
;;;-----------------------------------------------------------------------------

(define (newline) (display "\n"))
