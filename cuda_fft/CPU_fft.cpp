#include "CPU_fft.h"

void CPU_fft2(complex *&vec, int size, int n) {
	if (n != size * size) {
		die("CPU_fft2() error: The size is incorrect.");
	}
	complex *input;
	/* row fft */
	for (int i = 0; i < size; i++) {
		input = &vec[0] + size * i;
		CPU_fft(input, size);
	}
	/* matrix transposition */
	matrix_t(vec, size);
	/* col fft */
	for (int i = 0; i < size; i++) {
		input = &vec[0] + size * i;
		CPU_fft(input, size);
	}
	/* matrix transposition */
	matrix_t(vec, size);
}

void CPU_fft(complex *&vec, int n) {
	complex **arr = (complex **)malloc(2 * sizeof(complex *));
	arr[0] = vec;
	arr[1] = (complex *)malloc(n * sizeof(complex));
	int cur = 0, next = 1;
	int count = 0;
	int cal_times = log_2(n);
	/* shuffle */
	for (int i = 0; i < cal_times - 1; i++) {
		for (int j = 0; j < (1 << i); j++) {
			shuffle(arr[cur], arr[next], n / (1 << i) * j, n / (1 << i) * (j + 1) - 1, count);
		}
		count = 0;
		next = cur;
		cur = !cur;
	}
	/*for (int i = 0; i < n; i++) {
		cout << arr[cur][i].real << " " << arr[cur][i].imag << endl;
	}*/
	/* butterfly computation */
	float cos_unit, sin_unit;
	for (int i = 0; i < cal_times; i++) {
		cos_unit = cos(1.0 * 1 / (1 << (i + 1)) * 2 * PI);
		sin_unit = sin(1.0 * 1 / (1 << (i + 1)) * 2 * PI);
		for (int j = 0; j < (n >> (i + 1)); j++) {
			fft_kernel(arr[cur], arr[next], j * (1 << (i + 1)), (j + 1) * (1 << (i + 1)) - 1, 1 << (i + 1), count, cos_unit, sin_unit);
		}
		count = 0;
		next = cur;
		cur = !cur;
		/*cout << "Times: " << i << endl;
		for (int i = 0; i < n; i++) {
			cout << arr[cur][i].real << " " << arr[cur][i].imag << endl;
		}
		cout << endl;*/
	}
	std::move(&arr[cur][0], &arr[cur][n-1] + 1, vec);
	free(arr[1]);
	arr[1] = NULL;
	free(arr);
	arr = NULL;
}


void shuffle(complex *vec_in, complex *vec_out, int start, int end, int &count) {
	for (int i = start; i < end; i += 2, count++) {
		vec_out[count].real = vec_in[i].real;
		vec_out[count].imag = vec_in[i].imag;
	}
	for (int i = start + 1; i <= end; i += 2, count++) {
		vec_out[count].real = vec_in[i].real;
		vec_out[count].imag = vec_in[i].imag;
	}
}

void fft_kernel(complex *vec_in, complex *vec_out, int start, int end, int N, int &count, float cos_unit, float sin_unit) {
	float real, imag;
	float real_test, imag_test;
	float cos_cur = 1, sin_cur = 0;
	float cos_next, sin_next;
	for (int i = start; i < start + N / 2; i++, count++) {
		real = cos_cur;
		imag = -sin_cur;
		//real_test = cos(1.0 * (i - start) / N * 2 * PI);
		//imag_test = -sin(1.0 * (i - start) / N * 2 * PI);
		/* (a + bj)(c + dj) = (ac - bd) + (ad + bc)j */
		vec_out[count].real = vec_in[i].real + vec_in[i + N / 2].real * real - vec_in[i + N / 2].imag * imag;
		vec_out[count].imag = vec_in[i].imag + vec_in[i + N / 2].real * imag + vec_in[i + N / 2].imag * real;
		//temp[count] = vec[i] + vec[i + N / 2] * tw;
		cos_next = cos_cur * cos_unit - sin_cur * sin_unit;
		sin_next = sin_cur * cos_unit + cos_cur * sin_unit;
		cos_cur = cos_next;
		sin_cur = sin_next;
	}
	//cos_cur = cos(1.0 * (start + N / 2) / N * 2 * PI);
	//sin_cur = sin(1.0 * (start + N / 2) / N * 2 * PI);
	for (int i = start + N / 2; i <= end; i++, count++) {
		real = cos_cur;
		imag = -sin_cur;
		//real = cos(1.0 * i / N * 2 * PI);
		//imag = -sin(1.0 * i / N * 2 * PI);
		vec_out[count].real = vec_in[i - N / 2].real + vec_in[i].real * real - vec_in[i].imag * imag;
		vec_out[count].imag = vec_in[i - N / 2].imag + vec_in[i].real * imag + vec_in[i].imag * real;
		//temp[count] = vec[i - N / 2] + (vec[i] * tw);
		cos_next = cos_cur * cos_unit - sin_cur * sin_unit;
		sin_next = sin_cur * cos_unit + cos_cur * sin_unit;
		cos_cur = cos_next;
		sin_cur = sin_next;
	}
}

void matrix_t(complex *vec, int size) {
	complex temp;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < i; j++) {
			temp = vec[i * size + j];
			vec[i*size + j] = vec[j * size + i];
			vec[j * size + i] = temp;
		}
	}
}

int log_2(int x) {
	int count = 0;
	while (x != 1) {
		x = x >> 1;
		count += 1;
	}
	return count;
}


void die(const char *error) {
	std::cout << error << std::endl;
	exit(-1);
}