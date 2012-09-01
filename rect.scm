(define-structure rect minx miny maxx maxy)

(define rect-make make-rect)

(define (rect-width rect)
  (- (rect-maxx rect) (rect-minx rect)))

(define (rect-height rect)
  (- (rect-maxy rect) (rect-miny rect)))

(define (rect-make-point x y)
  (let* ((minx (floor x))
         (maxx (ceiling x))
         (miny (floor y))
         (maxy (ceiling y))

         (maxx (if (= minx maxx)
                   (+ maxx 1)
                   maxx))
         (maxy (if (= miny maxy)
                   (+ maxy 1)
                   maxy)))
    (rect-make minx miny maxx maxy)))

(define (rect-make-radius cx cy radius)
  (rect-make (max 0 (- cx radius))
             (max 0 (- cy radius))
             (+ cx radius)
             (+ cy radius)))

(define (rect-intersect a b)
  (let ((aminx (rect-minx a))
        (aminy (rect-miny a))
        (amaxx (rect-maxx a))
        (amaxy (rect-maxy a))

        (bminx (rect-minx b))
        (bminy (rect-miny b))
        (bmaxx (rect-maxx b))
        (bmaxy (rect-maxy b)))

    (cond
     ((< amaxx bminx) #f)
     ((> aminx bmaxx) #f)
     ((< amaxy bminy) #f)
     ((> aminy bmaxy) #f)
     (#t #t))))
