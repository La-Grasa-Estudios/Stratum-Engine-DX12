#pragma once

#include "Common.h"

namespace ENGINE_NAMESPACE
{
	template<typename T>
	class Handle
	{

		std::shared_ptr<T> m_Val;

	public:

		Handle()
		{
			m_Val = NULL;
		}

		Handle(T* ptr)
		{
			m_Val.reset(ptr);
		}

		operator T* ()
		{
			return m_Val.get();
		}

		T* operator ->()
		{
			return m_Val.get();
		}

		template<typename K>
		K* As() const
		{
			return (K*)m_Val.get();
		}

	};

}