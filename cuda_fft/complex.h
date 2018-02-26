#pragma once

class complex
{
private:


public:
	float real = 0.0;
	float imag = 0.0;
	complex();
	complex(float real_1, float image_1);
	~complex();
	complex(const complex& c);
	complex operator+(const complex &d);
	complex operator-(const complex &d);
	complex operator*(const complex &d);
	void assign(float real_a, float imag_a);
	float absolute();
	void show();
};


