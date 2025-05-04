#pragma once

#include "znmsp.h"
#include "Core/Ref.h"

BEGIN_ENGINE

template<typename T>
class DoubleBufferResource
{
public:

	DoubleBufferResource()
	{
		m_FrontPtr = NULL;
		m_BackPtr = NULL;
	}
	template<typename ... Args>
	DoubleBufferResource(Args&& ... args)
	{
		m_FrontPtr = CreateRef<T>(std::forward<Args>(args)...);
		m_BackPtr = CreateRef<T>(std::forward<Args>(args)...);
	}

	void Swap()
	{
		auto back = m_BackPtr;
		m_BackPtr = m_FrontPtr;
		m_FrontPtr = back;
	}

	T* GetBuffer()
	{
		return m_FrontPtr.get();
	}
	Ref<T> GetRef()
	{
		return m_FrontPtr;
	}

	T* operator ->()
	{
		return m_FrontPtr.get();
	}

	explicit operator bool() const
	{
		return m_FrontPtr.get() != NULL;
	}

private:
	Ref<T> m_FrontPtr;
	Ref<T> m_BackPtr;
};

END_ENGINE