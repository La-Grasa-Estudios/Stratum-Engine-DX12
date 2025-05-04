#include "UnormInt.h"

using namespace ENGINE_NAMESPACE;

unormbyte::unormbyte()
{
	data = 0;
}

unormbyte::unormbyte(const unormbyte& other)
{
	data = other.data;
}

unormbyte::unormbyte(uint8_t& h)
{
	data = h;
}

unormbyte::unormbyte(float f)
{
	data = f * 127.0f;
}

unormbyte::operator float()
{
	return data / 127.0f;
}

unormbyte::operator float() const
{
	return data / 127.0f;
}

unormbyte4::unormbyte4()
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
}

unormbyte4::unormbyte4(const unormbyte4& other)
{
	memcpy(this, &other, sizeof(unormbyte4));
}

unormbyte4::unormbyte4(const unormbyte& x, const unormbyte& y, const unormbyte& z, const unormbyte& w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

unormbyte& unormbyte4::operator[](int index)
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