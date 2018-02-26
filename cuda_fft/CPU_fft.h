#pragma once
#include <iostream>
#include <stdlib.h>
#include "math.h"
#include "complex.h"

using namespace std;

#define PI 3.1415926535898
//const float PI = 3.1415926535898;

void CPU_fft2(complex *&vec, int size, int n);
void CPU_fft(complex *&vec, int n);
void shuffle(complex *vec_in, complex *vec_out, int start, int end, int &count);
void fft_kernel(complex *vec_in, complex *vec_out, int start, int end, int N, int &count, float cos_unit, float sin_unit);
void matrix_t(complex *vec, int size);
int log_2(int x);
void die(const char *error);
