
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <Windows.h>
#include "complex.h"
#include "CPU_fft.h"

using namespace std;

#define PI 3.1415926535898

const int threads_per_block = 1024;
const int max_blocks_per_dimension = 65536;

complex *A_GPU, *B_GPU;

//----------------------------------- host function definitions -----------------------------------------
void copyMatricesToGPU(complex *vec, const int size);
void check_error(cudaError e);
__device__ void fft_shuffle(int thread_id, complex *vec, complex *temp, int size, int num_times);
__device__ void butterfly_computation(int thread_id, complex *vec, complex *temp, int size, int num_times);
//----------------------------------- CUDA function definitions -----------------------------------------
__device__ void fft_shuffle(int thread_id, complex *vec, complex *temp, int size, int num_times) {
	int row = thread_id / size;
	int shuffle_size, shuffle_area, location, next_location;
	int odd;
	int cur = 0, next = 1;
	// need a temp matrix to record intermediate variable
	complex *arr[2];
	arr[0] = vec;
	arr[1] = temp;
	for (int i = 0; i < num_times - 1; i++) {
		shuffle_size = size / (1 << i);
		//find the calculation area
		shuffle_area = (thread_id - row * size) / shuffle_size;
		location = thread_id - row * size - shuffle_area * shuffle_size;
		// if even, "odd" = 0, if odd, "odd" = 1;
		odd = location % 2;
		// find the new index of the vector
		next_location = odd * shuffle_size / 2 + location / 2 + shuffle_area * shuffle_size + row * size; 
		arr[next][next_location].real = arr[cur][thread_id].real;
		arr[next][next_location].imag = arr[cur][thread_id].imag;
		next = cur;
		cur = !cur;
		__syncthreads();
	}
	vec[thread_id].real = arr[cur][thread_id].real;
	vec[thread_id].imag = arr[cur][thread_id].imag;
	__syncthreads();
}

__device__ void butterfly_computation(int thread_id, complex *vec, complex *temp, int size, int num_times) {
	int row = thread_id / size;
	int col = thread_id - row * size;
	int cal_size, cal_area, flag;
	int location, another_location;
	float real, imag;
	int cur = 0, next = 1;
	// need a temp matrix to record intermediate variable
	complex *arr[2];
	arr[0] = vec;
	arr[1] = temp;
	for (int i = 0; i < num_times; i++) {
		cal_size = 1 << (i + 1);
		cal_area = (thread_id - row * size) / cal_size;
		// first input element
		location = thread_id - row * size - cal_area * cal_size;
		flag = location / (cal_size / 2) * 2 - 1;
		// second input element
		another_location = location + (-1 * flag) * (cal_size / 2) + cal_area * cal_size + row * size;

		real = cosf(1.0 * location / (1 << (i + 1)) * 2 * PI);
		imag = -sinf(1.0 * location / (1 << (i + 1)) * 2 * PI);

		// butterfly computation
		arr[next][thread_id].real = (arr[cur][thread_id].real + arr[cur][another_location].real * real - arr[cur][another_location].imag * imag)* (!((flag + 1) / 2))
			+ (arr[cur][another_location].real + arr[cur][thread_id].real * real - arr[cur][thread_id].imag * imag) * (((flag + 1) / 2));
		arr[next][thread_id].imag = (arr[cur][thread_id].imag + arr[cur][another_location].real * imag + arr[cur][another_location].imag * real)* (!((flag + 1) / 2))
			+ (arr[cur][another_location].imag + arr[cur][thread_id].real * imag + arr[cur][thread_id].imag * real) * (((flag + 1) / 2));

		next = cur;
		cur = !cur;
		__syncthreads();
	}
	vec[thread_id].real = arr[cur][thread_id].real;
	vec[thread_id].imag = arr[cur][thread_id].imag;
	__syncthreads();
	/* matrix inversion */
	temp[col * size + row].real = vec[thread_id].real;
	temp[col * size + row].imag = vec[thread_id].imag;
	__syncthreads();
}

__global__ void fft2_kernel(complex *vec, complex *temp, int size, int num_times)
{
	int thread_id = blockIdx.x * threads_per_block + threadIdx.x;
	//rearrange the matrix
	fft_shuffle(thread_id, vec, temp, size, num_times);  
	// do butterfly_computation
	butterfly_computation(thread_id, vec, temp, size, num_times);
	fft_shuffle(thread_id, temp, vec, size, num_times);
	butterfly_computation(thread_id, temp, vec, size, num_times);
}
//-------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	complex *A_CPU;
	int vec_size = (argc > 1) ? atoi(argv[1]) : 2048;
	A_CPU = (complex *)malloc(sizeof(complex) * vec_size * vec_size);

	int num_times = log_2(vec_size);
	int num_blocks, num_threads;

	LARGE_INTEGER  large_interger;
	double dff;
	__int64  c1, c2;

	QueryPerformanceFrequency(&large_interger);
	dff = large_interger.QuadPart;

	for (int i = 0; i < vec_size * vec_size; i++) {
		A_CPU[i].real = i + 1;
		A_CPU[i].imag = 1;
	}
	/* compute at CPU */
	QueryPerformanceCounter(&large_interger);
	c1 = large_interger.QuadPart;

	CPU_fft2(A_CPU, vec_size, vec_size * vec_size);

	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("%lf \n", (c2 - c1) * 1000 / dff);

	/* compute to GPU */
	copyMatricesToGPU(A_CPU, vec_size);
	cudaDeviceSynchronize();

	if (vec_size * vec_size <= threads_per_block) {
		num_blocks = 1;
		num_threads = vec_size * vec_size;
	}
	else {
		num_threads = threads_per_block;
		num_blocks = vec_size * vec_size / num_threads;
	}

	QueryPerformanceCounter(&large_interger);
	c1 = large_interger.QuadPart;

	fft2_kernel << <num_blocks, num_threads >> > (A_GPU, B_GPU, vec_size, num_times);
	cudaDeviceSynchronize();

	QueryPerformanceCounter(&large_interger);
	c2 = large_interger.QuadPart;
	printf("%lf \n", (c2 - c1) * 1000 / dff);

	size_t sizeofA = vec_size * vec_size * sizeof(complex);
	check_error(cudaMemcpy(A_CPU, A_GPU, sizeofA, cudaMemcpyDeviceToHost));
	cudaDeviceSynchronize();

	cudaFree(A_GPU);
	cudaFree(B_GPU);
    return 0;
}

void copyMatricesToGPU(complex *vec, int size) {
	size_t sizeofA = size * size * sizeof(complex);
	check_error(cudaMalloc((void **)&A_GPU, sizeofA));
	check_error(cudaMemcpy(A_GPU, vec, sizeofA, cudaMemcpyHostToDevice));

	check_error(cudaMalloc((void **)&B_GPU, sizeofA));
}

void check_error(cudaError e) {
	if (e != cudaSuccess) {
		printf("\nCUDA error: %s\n", cudaGetErrorString(e));
		//exit(1);
	}
}
