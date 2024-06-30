#ifndef BANDPASS2_H
#define BANDPASS2_H

#include <complex.h>

typedef short sample_t;

void die(char * s);
void sample_to_complex(sample_t * s, complex double * X, long n);
void complex_to_sample(complex double * X, sample_t * s, long n);
void fft_r(complex double * x, complex double * y, long n, complex double w);
void fft(complex double * x, complex double * y, long n);
void ifft(complex double * y, complex double * x, long n);
int pow2check(long N);
void bandpass(sample_t *input, sample_t *output, long n);

#endif