#pragma once

#include "znmsp.h"

#include <nvrhi/nvrhi.h>

BEGIN_ENGINE

namespace Render
{
    class BindlessDescriptorTable;

    class BindlessDescriptorIndex
    {
    public:

        void Release();
        int32_t GetIndex();

        BindlessDescriptorIndex() = default;

        operator int32_t()
		{
			return mIndex;
		}

    private:

        friend BindlessDescriptorTable;

        BindlessDescriptorIndex(BindlessDescriptorTable* pTable, int32_t index, nvrhi::IResource* pResource);

        int32_t mIndex;
        nvrhi::IResource* mResourcePointer;
        BindlessDescriptorTable* mDescriptorTable;
    };
}

END_ENGINE