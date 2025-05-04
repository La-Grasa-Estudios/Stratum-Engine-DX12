#pragma once

#include "znmsp.h"

#include "Core/Ref.h"

#include <vector>

BEGIN_ENGINE

template<typename type>
class ThreadSafeResource
{
public:

	ThreadSafeResource()
	{
		
	}

	void Set(Ref<type> ref)
	{
		if (m_FrontPtr)
		{
			m_ReleaseList.push_back(m_FrontPtr);
		}
		m_FrontPtr = ref;
	}

	void Update()
	{
		if (m_ReleaseList.empty()) return;
		if (m_ReleaseFrames++ > 3)
		{
			m_ReleaseList.pop_back();
			m_ReleaseFrames = 0;
		}
	}

	type* operator ->()
	{
		return m_FrontPtr.get();
	}

	explicit operator bool() const
	{
		return m_FrontPtr.get() != NULL;
	}

	operator Ref<type>()
	{
		return m_FrontPtr;
	}

	type* GetPtr()
	{
		return m_FrontPtr.get();
	}

	Ref<type> GetRef()
	{
		return m_FrontPtr;
	}

private:
	Ref<type> m_FrontPtr;
	uint32_t m_ReleaseFrames = 0;
	std::vector<Ref<type>> m_ReleaseList;
};

END_ENGINE