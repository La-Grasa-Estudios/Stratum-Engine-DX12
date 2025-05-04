#pragma once

#include "znmsp.h"

#include <format>

BEGIN_ENGINE

template<typename type>
class HeapArray
{
public:

	HeapArray()
	{
		m_Ptr = NULL;
		m_Size = 0;
	}

	HeapArray(size_t size)
	{
		m_Ptr = new type[size];
		memset(m_Ptr, 0, sizeof(type) * size);
		m_Size = size;
	}
	~HeapArray()
	{
		delete[] m_Ptr;
	}

	type& operator [](int index)
	{
		if (index < 0 || index > m_Size - 1) throw new std::out_of_range(std::format("Tried to access element {} when array range is {}", index, m_Size));
		return m_Ptr[index];
	}

	type* data()
	{
		return m_Ptr;
	}

	size_t size()
	{
		return m_Size;
	}

private:
	type* m_Ptr;
	size_t m_Size;
};

END_ENGINE