#include "HalfFloat.h"

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>

using namespace ENGINE_NAMESPACE;

half::half()
{
	data = 0;
}

half::half(const half& other)
{
	data = other.data;
}

half::half(int16_t& h)
{
	data = h;
}

half::half(float f)
{
	int16_t   fltInt16;
	fltInt16 = glm::detail::toFloat16(f);
	data = fltInt16;
}

half::operator float()
{
	return glm::detail::toFloat32(data);
}

half::operator float() const
{
	return glm::detail::toFloat32(data);
}

half2::half2()
{
	x = 0;
	y = 0;
}

half2::half2(const half2& other)
{
	x = other.x;
	y = other.y;
}

half2::half2(const half& x, const half& y)
{
	this->x = x;
	this->y = y;
}

half2::half2(float x, float y)
{
	this->x = x;
	this->y = y;
}

half3::half3()
{
	x = 0;
	y = 0;
	z = 0;
}

half3::half3(const half3& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
}

half3::half3(const half2& xy, const half& z)
{
	x = xy.x;
	y = xy.y;
	this->z = z;
}

half3::half3(const half& x, const half& y, const half& z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

half3::half3(float x, float y, float z)
{

	this->x = x;
	this->y = y;
	this->z = z;
}

half4::half4()
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
}

half4::half4(const half4& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.z;
}

half4::half4(const half2& xy, const half& z, const half& w)
{
	x = xy.x;
	y = xy.y;
	this->z = z;
	this->w = w;
}

half4::half4(const half3& xyz, const half& w)
{
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	this->w = w;
}

half4::half4(const half& x, const half& y, const half& z, const half& w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

half4::half4(float x, float y, float z, float w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

half& half4::operator[](int index)
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
	}
}
