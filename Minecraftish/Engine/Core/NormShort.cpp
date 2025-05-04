#include "NormShort.h"

using namespace ENGINE_NAMESPACE;

normshort::normshort()
{
	data = 0;
}

normshort::normshort(const normshort& other)
{
	data = other.data;
}

normshort::normshort(uint16_t& h)
{
	data = h;
}

normshort::normshort(float f)
{
	data = f * 65535.0f;
}

normshort::operator float()
{
	return data / 65535.0f;
}

normshort::operator float() const
{
	return data / 65535.0f;
}

normshort4::normshort4()
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
}

normshort4::normshort4(const normshort4& other)
{
	memcpy(this, &other, sizeof(normshort4));
}

normshort4::normshort4(const normshort& x, const normshort& y, const normshort& z, const normshort& w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

normshort& normshort4::operator[](int index)
{
	switch (index)
	{
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	case 3:
		return w;
	default:
		return x;
		break;
	}
}