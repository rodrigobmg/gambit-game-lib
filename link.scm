(c-declare #<<c-declare-end

#include "testlib.h"

c-declare-end
)

(load "scmlib")

;(##include "scmlib.scm")

(c-define-type ImageResource (pointer (struct "ImageResource_")))
(c-define-type Clock (pointer (struct "Clock_")))
(c-define-type Sprite (pointer (struct "Sprite_")))
(c-define-type SpriteList (pointer (struct "SpriteList_")))
(c-define-type InputState (pointer (struct "InputState_")))

(define image-load-internal
  (c-lambda (nonnull-char-string)
            ImageResource
            "image_load"))

(define image-width
  (c-lambda (ImageResource)
            int
            "image_width"))

(define image-height
  (c-lambda (ImageResource)
            int
            "image_height"))

(define images-free
  (c-lambda ()
            void
            "images_free"))

(define clock-free
  (c-lambda (Clock)
            void
            "clock_free"))

(define clock-make
  (c-lambda ()
            Clock
            "clock_make"))

(define clock-time-scale
  (c-lambda (Clock)
            float
            "___result = ___arg1->time_scale;"))

(define clock-time-scale-set!
  (c-lambda (Clock float)
            void
            "___arg1->time_scale = ___arg2;"))

(define (comp2 f g)
  (lambda (item x) (f item (g x))))

(define (compose f g)
  (lambda (x) (f (g x))))

(define %clock-update
  (c-lambda (Clock float)
            float
            "clock_update"))

(define clock-update (comp2 %clock-update exact->inexact))

(define clock-time
  (c-lambda (Clock)
            long
            "clock_time"))

(define %cycles->seconds
  (c-lambda (long)
            float
            "clock_cycles_to_seconds"))

(define %seconds->cycles
  (c-lambda (float)
            long
            "clock_seconds_to_cycles"))

(define input-leftright
  (c-lambda (InputState)
	    int
	    "___result = ___arg1->leftright;"))

(define input-updown
  (c-lambda (InputState)
	    int
	    "___result = ___arg1->updown;"))

(define cycles->seconds (compose %cycles->seconds inexact->exact))
(define seconds->cycles (compose %seconds->cycles exact->inexact))

(define *screen-width* #f)

(define *screen-height* #f)

(define frame/make-sprite
  (c-lambda ()
            Sprite
            "frame_make_sprite"))

(define sprite-resource-set!
  (c-lambda (Sprite ImageResource)
            void
            "___arg1->resource = ___arg2;"))

(define %sprite-x-set!
  (c-lambda (Sprite float)
            void
            "___arg1->displayX = ___arg2;"))

(define %sprite-y-set!
  (c-lambda (Sprite float)
            void
            "___arg1->displayY = ___arg2;"))

(define %sprite-origin-x-set!
  (c-lambda (Sprite float)
            void
            "___arg1->originX = ___arg2;"))

(define %sprite-origin-y-set!
  (c-lambda (Sprite float)
            void
            "___arg1->originY = ___arg2;"))

(define %sprite-angle-set!
  (c-lambda (Sprite float)
            void
            "___arg1->angle = ___arg2;"))

(define sprite-x-set! (comp2 %sprite-x-set! exact->inexact))
(define sprite-y-set! (comp2 %sprite-y-set! exact->inexact))
(define sprite-origin-x-set! (comp2 %sprite-origin-x-set! exact->inexact))
(define sprite-origin-y-set! (comp2 %sprite-origin-y-set! exact->inexact))
(define sprite-angle-set! (comp2 %sprite-angle-set! exact->inexact))

(define frame/spritelist-append
  (c-lambda (SpriteList Sprite)
            SpriteList
            "frame_spritelist_append"))

(define spritelist-enqueue-for-screen!
  (c-lambda (SpriteList)
            void
            "spritelist_enqueue_for_screen"))

;;; game lifecycle
(define *game-clock* #f)

;; wild hack to keep gambit from trying to kill our process before
;; we're done. We use a continuation to send the gambit exit system
;; off into space after we've told the C side that we don't need to
;; tear us down when it's ready. Seems to only work on non-arm. Call
;; exit instead of giving the repl a ,q

(define ##exit-cc-hack #f)
(call/cc (lambda (cc) (set! ##exit-cc-hack cc)))

(define (exit)
  (terminate)
  (%notify-terminate))

(define quit exit)

(c-define (scm-init) () void "scm_init" ""
          (set! *game-clock* (clock-make))
          (##add-exit-job!
           (lambda ()
	     (exit)
	     (##exit-cc-hack)))

          (set! *screen-width*
                ((c-lambda () int "___result = screen_width;")))
          (set! *screen-height*
                ((c-lambda () int "___result = screen_height;")))

          ;(clock-time-scale-set! *game-clock* 0.2)
          (display "initializing") (newline)
          (ensure-resources)
          (thread-start!
           (make-thread
            (lambda ()
              (thread-sleep 1)
              (##repl-debug-,bmain)))))

;;; resource lifecycle
(define *resources* (make-table))

(define (image-load path)
  (let ((resource (table-ref *resources* path #f)))
    (if resource resource
        (begin
          (let ((new-resource (image-load-internal path)))
            (table-set! *resources* path new-resource)
            new-resource)))))

(c-define (resources-released) () void "resources_released" ""
          (set! *resources* (make-table)))

;;; gameloop
(c-define (step msecs input) (int InputState) void "step" ""
          (update-view (clock-update *game-clock* (/ msecs 1000.0)) input))


;;; termination

;; c tells us to terminate
(c-define (terminate) () void "terminate" ""
          (display "terminating") (newline))

;; we tell c to terminate
(define %notify-terminate
  (c-lambda ()
            void
            "notify_gambit_terminated"))
