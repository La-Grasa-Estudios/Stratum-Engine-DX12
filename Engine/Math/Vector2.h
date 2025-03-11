#pragma once

#include "../Common.h"

struct Vector2i
{
	int x, y;

	Vector2i()
	{
		x = 0;
		y = 0;
	}
	Vector2i(int a, int b)
	{
		x = a;
		y = b;
	}
	Vector2i(int a)
	{
		x = a;
		y = a;
	}

	Vector2i operator +(const Vector2i& a)
	{
		return Vector2i(x + a.x, y + a.y);
	}

	Vector2i operator -(const Vector2i& a)
	{
		return Vector2i(x - a.x, y - a.y);
	}

	Vector2i operator /(const Vector2i& a)
	{
		return Vector2i(x / a.x, y / a.y);
	}

	Vector2i operator *(const Vector2i& a)
	{
		return Vector2i(x * a.x, y * a.y);
	}

};