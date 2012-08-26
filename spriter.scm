(load "xml2")
(##include "common.scm")

(define (resource-id-make folder-id file-id)
  (cons folder-id file-id))

(define-structure scml resource-names)

(define (folders-markup scml)
  (filter (sml:node-named? "folder") (sml:children scml)))

(define (entities-markup scml)
  (filter (sml:node-named? "entity") (sml:children scml)))

(define (entity->animations-markup entity)
  (filter (sml:node-named? "animation") (sml:children entity)))

(define (animation->mainline-markup animation)
  (car (filter (sml:node-named? "mainline") (sml:children animation))))

(define (animation->timelines-markup animation)
  (filter (sml:node-named? "timeline") (sml:children animation)))

(define (timeline->keys-markup timeline)
  (filter (sml:node-named? "key") (sml:children timeline)))

(define mainline->keys-markup timeline->keys-markup)

(define (resources scml base)
  (mapcat (lambda (folder)
            (map (lambda (file)
                   (cons (resource-id-make
                          (sml:attr folder "id")
                          (sml:attr file "id"))
                         (path-expand (sml:attr file "name") base)))
                 (sml:children folder)))
          (folders-markup scml)))

(define-structure tkey time name x y cx cy angle spin)

(define (tkey-make time name x y cx cy angle spin)
  (make-tkey (string->number (or time "0"))
             name
             (string->number (or x "0"))
             (string->number (or y "0"))
             (string->number (or cx "0"))
             (string->number (or cy "1"))
             (string->number (or angle "0"))
             (string->number (or spin "1"))))

(define (timeline-parse timeline-markup resources)
  (map (lambda (key)
         (let* ((obj (car (filter (sml:node-named? "object")
                                  (sml:children key))))
                (name (cdr (assoc (resource-id-make
                                   (sml:attr obj "folder")
                                   (sml:attr obj "file"))
                                  resources))))
           (cons (sml:attr key "id")
                 (tkey-make
                  (sml:attr key "time")
                  name
                  (sml:attr obj "x")
                  (sml:attr obj "y")
                  (sml:attr obj "pivot_x")
                  (sml:attr obj "pivot_y")
                  (sml:attr obj "angle")
                  (sml:attr obj "spin")))))
       (timeline->keys-markup timeline-markup)))

(define (timelines-parse timelines resources)
  (map (lambda (timeline)
         (cons (sml:attr timeline "id")
               (timeline-parse timeline resources)))
       timelines))


(define (mainline-parse mainline-markup timelines)
  (map (lambda (key)
         (cons (string->number (or (sml:attr key "time") "0"))
               (map (lambda (obj)
                      (cons
                       (sml:attr obj "id")
                       (cond
                        ((equal? (sml:name obj) "object_ref")
                         (cdr (assoc (sml:attr obj "key")
                                     (cdr (assoc (sml:attr obj "timeline")
                                                 timelines)))))
                        (#t (error "unrecognized tag " (sml:name obj))))))
                    (sml:children key))))
       (mainline->keys-markup mainline-markup)))

(define-structure animation id length mainline)

(define (animation-make id length mainline)
  (make-animation id (string->number length) mainline))

(define (animations entity-markup resources)
  (map (lambda (animation)
         (let* ((timelines (timelines-parse
                            (animation->timelines-markup animation)
                            resources))
                (mainline (mainline-parse
                           (animation->mainline-markup animation)
                           timelines)))
           (cons (sml:attr animation "name")
                 (animation-make
                  (sml:attr animation "id")
                  (sml:attr animation "length")
                  mainline))))
       (entity->animations-markup entity-markup)))

(define (scml-load filename)
  (let* ((doc (xml:parse-file filename))
         (node (if doc
                   (xml:root-element doc)
                   (error "failed to load " filename)))
         (scml (xml->sml node))
         (res (resources scml (path-directory filename))))

    (map (lambda (entity)
           (cons (sml:attr entity "id")
                 (animations entity res)))
         (entities-markup scml))))

(define (entity data entity)
  (cdr (assoc entity data)))

(define (animation entity animation)
  (cdr (assoc animation entity)))

(define (find-frame animation time)
  (let ((mainline (animation-mainline animation)))
    (let loop((keys mainline)
              (nkeys (if (null? mainline) '()
                         (cdr mainline))))
      (cond
       ((null? nkeys) (error "time exceeded max " time))
       ((and (>= time (caar keys)) (<= time (caar nkeys))) keys)
       (#t (loop (cdr keys)
                 (cddr keys)))))))

(define (lerp-internal a s d)
  (+ a (* s d)))

(define (lerp a ta b tb t)
  (let ((s (/ (- t ta)
              (- tb ta)))
        (d (- b a)))
    (lerp-internal a s d)))

(define (signed-circular-dist a1 a2 d)
  (if (= d 1)
      (if (> a2 a1)
          (- a2 a1)
          (+ a2 (- 360 a1)))
      (if (< a2 a1)
          (- a2 a1)
          (- (+ a1 (- 360 a2))))))

(define (angle-restricted angle)
  (cond
   ((< angle 0)
    (+ 360 angle))
   
   ((>= angle 360)
    (- angle 360))
   
   (#t angle)))

(define (clerp a ta b tb d t)
  (let ((s (/ (- t ta)
              (- tb ta)))
        (d (signed-circular-dist a b d)))
    (angle-restricted (lerp-internal a s d))))

(define (interp-objects obj1 obj2 t)
  (let ((t1 (tkey-time obj1))
        (t2 (tkey-time obj2)))
    (make-tkey t
               (tkey-name obj1)
               (lerp (tkey-x obj1) t1 (tkey-x obj2) t2 t)
               (lerp (tkey-y obj1) t1 (tkey-y obj2) t2 t)
               (lerp (tkey-cx obj1) t1 (tkey-cx obj2) t2 t)
               (lerp (tkey-cy obj1) t1 (tkey-cy obj2) t2 t)
               (clerp (tkey-angle obj1) t1 (tkey-angle obj2) t2
                      (tkey-spin obj1) t)
               (tkey-spin obj1))))

(define (interp-anim anim t)
  (let* ((frames (find-frame anim t))
         (f1 (cdar frames))
         (f2 (cdadr frames)))
    (map (lambda (f)
           (let* ((id (car f))
                  (obj1 (cdr f))
                  (obj2 (cdr (assoc id f2))))
             (interp-objects obj1 obj2 t)))
         f1)))

;(define timeline (car (animation->timelines-markup (car (entity->animations-markup (car (entities-markup sml)))))))

 
