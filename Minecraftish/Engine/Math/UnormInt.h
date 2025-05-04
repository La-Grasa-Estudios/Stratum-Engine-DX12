#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class unormbyte {
public:

	DLLEXPORT unormbyte();
	DLLEXPORT unormbyte(const unormbyte& other);
	DLLEXPORT unormbyte(uint8_t& h);
	DLLEXPORT unormbyte(float f);

	DLLEXPORT operator float();
	DLLEXPORT operator float() const;

private:
	int8_t data;
};

class unormbyte4 {

public:

	DLLEXPORT unormbyte4();
	DLLEXPORT unormbyte4(const unormbyte4& other);
	//normbyte4(const normbyte2& xy, const normbyte& z, const normbyte& w);
	//normbyte4(const normbyte3& xyz, const normbyte& w);
	DLLEXPORT unormbyte4(const unormbyte& x, const unormbyte& y, const unormbyte& z, const unormbyte& w);

	DLLEXPORT unormbyte& operator[](int index);

	unormbyte x, y, z, w;

};

END_ENGINE