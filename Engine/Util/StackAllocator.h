#pragma once

#include "znmsp.h"

BEGIN_ENGINE

template<typename T, typename size_t size>
class StackAllocator
{

public:

	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T         value_type;

	StackAllocator()
	{

	}

	T* allocate(size_t size)
	{
		size_t offset = m_Offset;
		m_Offset += size * sizeof(T);
		return (T*)(m_MemBlock + offset);
	}

	void deallocate(void* ptr, size_t size)
	{

	}

	pointer           address(reference x) const { return &x; }
	const_pointer     address(const_reference x) const { return &x; }
	StackAllocator<T, size>& operator=(const StackAllocator&) { return *this; }
	void              construct(pointer p, const T& val)
	{
		new ((T*)p) T(val);
	}
	void              destroy(pointer p) { p->~T(); }

	size_type         max_size() const { return size_t(-1); }

	template <class U>
	struct rebind { typedef StackAllocator<U, size> other; };

	template <class U>
	StackAllocator(const StackAllocator<U, size>&) {}

	template <class U>
	StackAllocator& operator=(const StackAllocator<U, size>&) { return *this; }

private:

	size_t m_Offset = 0;
	uint8_t m_MemBlock[size];

};

template<typename T, typename size_t size>
using StackVector = std::vector<T, StackAllocator<T, size * sizeof(T)>>;

END_ENGINE