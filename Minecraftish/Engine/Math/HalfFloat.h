#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class half {
public:

	DLLEXPORT half();
	DLLEXPORT half(const half& other);
	DLLEXPORT half(int16_t& h);
	DLLEXPORT half(float f);

	DLLEXPORT operator float();
	DLLEXPORT operator float() const;

	int16_t data;
	
};

class half2 {

public:

	DLLEXPORT half2();
	DLLEXPORT half2(const half2& other);
	DLLEXPORT half2(const half& x, const half& y);
	DLLEXPORT half2(float x, float y);

	half x, y;

};

class half3 {

public:

	DLLEXPORT half3();
	DLLEXPORT half3(const half3& other);
	DLLEXPORT half3(const half2& xy, const half& z);
	DLLEXPORT half3(const half& x, const half& y, const half& z);
	DLLEXPORT half3(float x, float y, float z);

	half x, y, z;

};

class half4 {

public:

	DLLEXPORT half4();
	DLLEXPORT half4(const half4& other);
	DLLEXPORT half4(const half2& xy, const half& z, const half& w);
	DLLEXPORT half4(const half3& xyz, const half& w);
	DLLEXPORT half4(const half& x, const half& y, const half& z, const half& w);
	DLLEXPORT half4(float x, float y, float z, float w);

	DLLEXPORT half& operator[](int index);

	half x, y, z, w;

};

END_ENGINE