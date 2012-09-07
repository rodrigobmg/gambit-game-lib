#ifndef SAMPLER_H
#define SAMPLER

#include <stdint.h>

#define N_(n,b) (n*b)

#define C_(n) N_(n, 261.6)
#define D_(n) N_(n, 293.7)
#define E_(n) N_(n, 329.6)
#define F_(n) N_(n, 349.2)
#define G_(n) N_(n, 392.0)
#define A_(n) N_(n, 440.0)
#define B_(n) N_(n, 493.9)

#define SAMPLE_FREQ 44100
//22050
#define NUM_SAMPLERS 128
#define SAMPLE(f, x) (((Sampler)(f))->function(f, x))
#define RELEASE_SAMPLER(f) (((Sampler)(f))->release(f))

#define array_size(a) (sizeof(a)/sizeof(a[0]))

typedef int16_t (*SamplerFunction)(void*, long);
typedef void (*ReleaseSampler)(void*);

void sampler_init();

typedef struct Sampler_ {
  SamplerFunction function;
  ReleaseSampler release;
} *Sampler;

typedef struct SinSampler_ {
  struct Sampler_ sampler;
  float phase; /* radians */
  float radians_per_sample;
  float amp;
} *SinSampler;

SinSampler sinsampler_make(float freq, float amp, float phase);

typedef struct SawSampler_ {
  struct Sampler_ sampler;
  long samples_per_period;
  long phase_samples;
  float slope;
  float amp;
} *SawSampler;

SawSampler sawsampler_make(float freq, float amp, float phase);

#define DURATION(f) (((FiniteSampler)f)->duration(f))
#define START(f) (((FiniteSampler)f)->start(f))
#define END(f) (START(f) + DURATION(f))

typedef long(*SamplerDuration)(void*);
typedef long(*SamplerStart)(void*);

typedef struct FiniteSampler_ {
  struct Sampler_ sampler;
  SamplerDuration duration;
  SamplerStart start;
} *FiniteSampler;

typedef struct StepSampler_ {
  struct FiniteSampler_ sampler;
  Sampler nested_sampler;
  long start_sample;
  long duration_samples;
} *StepSampler;

StepSampler stepsampler_make(Sampler nested_sampler,
                             long start_sample,
                             long duration_samples);

typedef struct FiniteSequence_ {
  struct FiniteSampler_ sampler;
  int nsamplers;
  FiniteSampler* samplers;
} *FiniteSequence;

FiniteSequence finitesequence_make(FiniteSampler* samplers, int nsamplers);

typedef struct Filter_ {
  struct Sampler_ sampler;
  Sampler nested_sampler;
  int na;
  int nb;
  int xi;
  float yi;

  float *as;
  float *bs;
  int16_t *xs;
  int16_t *ys;
} *Filter;

Filter filter_make(Sampler nested_sampler,
                   float* as, int na, float* bs, int nb);

int16_t filter_value(Filter filter, int16_t value);

Filter lowpass_make(Sampler nested_sampler, float cutoff, float sample_freq);

FiniteSequence make_sequence(float* freqs, int nfreqs, float amp,
                             float duration);

#endif