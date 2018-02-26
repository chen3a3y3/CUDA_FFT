#include "complex.h"
#include <iostream>
#include "math.h"


complex::complex()
{
}

complex::complex(float real_1, float image_1) {
	real = real_1;
	imag = image_1;
}


complex::~complex() {}

complex::complex(const complex& c) {
	real = c.real;
	imag = c.imag;
}


complex complex::operator+(const complex &d) {
	complex temp;
	temp.real = real + d.real;
	temp.imag = imag + d.imag;
	return temp;
}

complex complex::operator-(const complex &d) {
	complex temp;
	temp.real = real - d.real;
	temp.imag = imag - d.imag;
	return temp;
}

complex complex::operator*(const complex &d) {
	complex temp;
	temp.real = real * d.real - imag * d.imag;
	temp.imag = real * d.imag + imag * d.real;
	return temp;
}

float complex::absolute() {
	float temp;
	temp = sqrt(real * real + imag * imag);
	return temp;
}

void complex::show() {
	std::cout << real << "+" << imag << "i" << std::endl;
}

void complex::assign(float real_a, float imag_a) {
	real = real_a;
	imag = imag_a;
}