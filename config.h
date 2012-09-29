#ifndef CONFIG_H
#define CONFIG_H

#define SAFETY(x) x
#define DEBUG_MEMORY

#define MAX_NUM_CLOCKS 20
#define MAX_NUM_IMAGES 40
#define MAX_NUM_COMMANDS 60

#define MAX_NUM_PARTICLES 60
#define MAX_NUM_PRETTYPARTICLES 30

#define MAX_NUM_AGENTS MAX_NUM_PARTICLES
#define MAX_NUM_MESSAGES (MAX_NUM_AGENTS * 2)
#define MAX_NUM_DISPATCHEES (MAX_NUM_AGENTS * 4)

#define SAMPLE_FREQ 22050
#define MAX_NUM_SAMPLERS 128


#define MAX(x,y) ((x)>(y) ? (x) : (y))

#include <stddef.h>
#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})

#endif
