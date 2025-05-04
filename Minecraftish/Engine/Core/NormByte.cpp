#include "NormByte.h"

using namespace ENGINE_NAMESPACE;

normbyte::normbyte()
{
	data = 0;
}

normbyte::normbyte(const normbyte& other)
{
	data = other.data;
}

normbyte::normbyte(uint8_t& h)
{
	data = h;
}

normbyte::normbyte(float f)
{
	data = f * 255.0f;
}

normbyte::operator float()
{
	return data / 255.0f;
}

normbyte::operator float() const
{
	return data / 255.0f;
}

normbyte4::normbyte4()
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
}

normbyte4::normbyte4(const normbyte4& other)
{
	memcpy(this, &other, sizeof(normbyte4));
}

normbyte4::normbyte4(const normbyte& x, const normbyte& y, const normbyte& z, const normbyte& w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

normbyte& normbyte4::operator[](int index)
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