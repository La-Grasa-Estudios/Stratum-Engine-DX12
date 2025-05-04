#pragma once

#include "znmsp.h"

#include <format>

BEGIN_ENGINE

template<typename type>
class DoubleBufferArray
{
public:

	DoubleBufferArray()
	{
		m_FrontPtr = NULL;
		m_BackPtr = NULL;
		m_Size = 0;
	}

	DoubleBufferArray(size_t size)
	{
		m_FrontPtr = new type[size];
		m_BackPtr = new type[size];
		memset(m_FrontPtr, 0, sizeof(type) * size);
		memset(m_BackPtr, 0, sizeof(type) * size);
		m_Size = size;
	}
	~DoubleBufferArray()
	{
		delete[] m_FrontPtr;
		delete[] m_BackPtr;
	}

	void Swap()
	{
		auto back = m_BackPtr;
		m_BackPtr = m_FrontPtr;
		m_FrontPtr = back;
	}

	type& operator [](int index)
	{
		if (index < 0 || index > m_Size - 1) throw new std::out_of_range(std::format("Tried to access element {} when array range is {}", index, m_Size));
		return m_FrontPtr[index];
	}

	type* GetBuffer()
	{
		return m_FrontPtr;
	}

private:
	type* m_FrontPtr;
	type* m_BackPtr;
	size_t m_Size;
};

END_ENGINE