#pragma once

#include "znmsp.h"

BEGIN_ENGINE

template<typename T, typename size_t size>
class StaticAllocator
{

public:

	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T         value_type;

	StaticAllocator()
	{

	}

	T* allocate(size_t size)
	{
		size_t offset = m_Offset;
		m_Offset += size;
		return (T*)(s_Memory.data() + offset);
	}

	void deallocate(void* ptr, size_t size)
	{

	}

	pointer           address(reference x) const { return &x; }
	const_pointer     address(const_reference x) const { return &x; }
	StaticAllocator<T, size>& operator=(const StaticAllocator&) { return *this; }
	void              construct(pointer p, const T& val)
	{
		new ((T*)p) T(val);
	}
	void              destroy(pointer p) { p->~T(); }

	size_type         max_size() const { return size_t(-1); }

	template <class U>
	struct rebind { typedef StaticAllocator<U, size> other; };

	template <class U>
	StaticAllocator(const StaticAllocator<U, size>&) {}

	template <class U>
	StaticAllocator& operator=(const StaticAllocator<U, size>&) { return *this; }

private:

	size_t m_Offset = 0;

	inline static std::vector<T> s_Memory = std::vector<T>(size / sizeof(T));

};

END_ENGINE