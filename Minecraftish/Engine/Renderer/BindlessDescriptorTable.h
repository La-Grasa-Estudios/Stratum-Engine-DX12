#pragma once

#include "znmsp.h"
#include "BindlessDescriptorIndex.h"

#include <unordered_map>
#include <mutex>

BEGIN_ENGINE

namespace Render
{

	class BindlessDescriptorTable
	{
        friend BindlessDescriptorIndex;

	public:

        BindlessDescriptorTable(nvrhi::IBindingLayout* layout);
        ~BindlessDescriptorTable();

        BindlessDescriptorIndex AllocateDescriptor(nvrhi::BindingSetItem item);
        void ReplaceDescriptor(nvrhi::BindingSetItem item, BindlessDescriptorIndex& descriptor);

        nvrhi::IDescriptorTable* GetDescriptorTable() const;

        std::mutex lockMutex;

    private:

        void Grow(uint32_t size);

        struct BindingSetItemHasher
        {
            std::size_t operator()(const nvrhi::BindingSetItem& item) const
            {
                size_t hash = 0;
                nvrhi::hash_combine(hash, item.resourceHandle);
                nvrhi::hash_combine(hash, item.type);
                nvrhi::hash_combine(hash, item.format);
                nvrhi::hash_combine(hash, item.dimension);
                nvrhi::hash_combine(hash, item.rawData[0]);
                nvrhi::hash_combine(hash, item.rawData[1]);
                return hash;
            }
        };

        struct BindingSetItemsEqual
        {
            bool operator()(const nvrhi::BindingSetItem& a, const nvrhi::BindingSetItem& b) const
            {
                return a.resourceHandle == b.resourceHandle
                    && a.type == b.type
                    && a.format == b.format
                    && a.dimension == b.dimension
                    && a.subresources == b.subresources;
            }
        };

		nvrhi::DescriptorTableHandle mDescriptorTable;
        std::unordered_map<nvrhi::BindingSetItem, BindlessDescriptorIndex, BindingSetItemHasher, BindingSetItemsEqual> mAllocatedDescriptorsMap;
        std::vector<bool> mAllocatedDescriptorIndexes;
        std::vector<nvrhi::BindingSetItem> mDescriptors;
        bool mIsTableValid = true;

        int32_t mSearchStart = 0;

	};
}

END_ENGINE