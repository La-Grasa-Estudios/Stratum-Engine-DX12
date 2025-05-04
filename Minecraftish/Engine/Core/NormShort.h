#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class normshort {
public:

	DLLEXPORT normshort();
	DLLEXPORT normshort(const normshort& other);
	DLLEXPORT normshort(uint16_t& h);
	DLLEXPORT normshort(float f);

	DLLEXPORT operator float();
	DLLEXPORT operator float() const;

private:
	uint16_t data;
};

class normshort4 {

public:

	DLLEXPORT normshort4();
	DLLEXPORT normshort4(const normshort4& other);
	//normshort4(const normshort2& xy, const normshort& z, const normshort& w);
	//normshort4(const normshort3& xyz, const normshort& w);
	DLLEXPORT normshort4(const normshort& x, const normshort& y, const normshort& z, const normshort& w);

	DLLEXPORT normshort& operator[](int index);

	normshort x, y, z, w;

};

END_ENGINE