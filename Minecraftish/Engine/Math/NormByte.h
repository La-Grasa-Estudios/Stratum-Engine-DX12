#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class normbyte {
public:

	DLLEXPORT normbyte();
	DLLEXPORT normbyte(const normbyte& other);
	DLLEXPORT normbyte(uint8_t& h);
	DLLEXPORT normbyte(float f);

	DLLEXPORT operator float();
	DLLEXPORT operator float() const;

private:
	uint8_t data;
};

class normbyte4 {

public:

	DLLEXPORT normbyte4();
	DLLEXPORT normbyte4(const normbyte4& other);
	//normbyte4(const normbyte2& xy, const normbyte& z, const normbyte& w);
	//normbyte4(const normbyte3& xyz, const normbyte& w);
	DLLEXPORT normbyte4(const normbyte& x, const normbyte& y, const normbyte& z, const normbyte& w);

	DLLEXPORT normbyte& operator[](int index);

	normbyte x, y, z, w;

};

END_ENGINE