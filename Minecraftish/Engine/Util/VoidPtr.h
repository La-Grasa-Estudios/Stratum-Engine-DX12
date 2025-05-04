#pragma once

#include "znmsp.h"

BEGIN_ENGINE


/// <summary>
/// Void pointer with cast helper, used to avoid includes in some header files
/// </summary>
class VoidPtr
{
public:

	VoidPtr()
	{
		m_Ptr = NULL;
	}

	VoidPtr(void* ptr)
	{
		m_Ptr = ptr;
	}

	template<typename T>
	T* As()
	{
		return (T*)m_Ptr;
	}

	operator void* ()
	{
		return m_Ptr;
	}

private:

	void* m_Ptr;

};

END_ENGINE