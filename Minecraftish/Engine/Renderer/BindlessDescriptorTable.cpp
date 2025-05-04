#include "BindlessDescriptorTable.h"
#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;
using namespace Render;

BindlessDescriptorTable::BindlessDescriptorTable(nvrhi::IBindingLayout* layout)
{

	mDescriptorTable = RendererContext::GetDevice()->createDescriptorTable(layout);

	size_t capacity = mDescriptorTable->getCapacity();

	mAllocatedDescriptorIndexes.resize(capacity);
	mDescriptors.resize(capacity);

	memset(mDescriptors.data(), 0, sizeof(nvrhi::BindingSetItem) * mDescriptors.size());
}

Render::BindlessDescriptorTable::~BindlessDescriptorTable()
{
	mIsTableValid = false;
	for (auto& descriptor : mDescriptors)
	{
		if (descriptor.resourceHandle)
		{
			descriptor.resourceHandle->Release();
		}
	}
}

BindlessDescriptorIndex Render::BindlessDescriptorTable::AllocateDescriptor(nvrhi::BindingSetItem item)
{
	std::scoped_lock l(lockMutex);

	auto wasFound = mAllocatedDescriptorsMap.find(item);
	if (wasFound != mAllocatedDescriptorsMap.end())
	{
		return wasFound->second;
	}

	uint32_t capacity = mDescriptorTable->getCapacity();
	int32_t slot = -1;

	for (size_t i = mSearchStart; i < capacity; i++)
	{
		if (!mAllocatedDescriptorIndexes[i])
		{
			slot = i;
			break;
		}
	}

	if (slot == -1)
	{
		
		Grow(capacity);
		slot = capacity;

	}

	mSearchStart = slot + 1;
	mAllocatedDescriptorIndexes[slot] = true;
	mDescriptors[slot] = item;

	BindlessDescriptorIndex index(this, slot, item.resourceHandle);

	mAllocatedDescriptorsMap[item] = index;

	item.slot = slot;
	RendererContext::GetDevice()->writeDescriptorTable(mDescriptorTable, item);

	return index;
}

void Render::BindlessDescriptorTable::ReplaceDescriptor(nvrhi::BindingSetItem item, BindlessDescriptorIndex& descriptor)
{
	std::scoped_lock l(lockMutex);
	assert(descriptor.mIndex >= 0);

	item.slot = descriptor.mIndex;

	mDescriptors[descriptor.mIndex] = item;
	RendererContext::GetDevice()->writeDescriptorTable(mDescriptorTable, item);

	descriptor.mResourcePointer->Release();
	descriptor.mResourcePointer = item.resourceHandle;
	descriptor.mResourcePointer->AddRef();
}

nvrhi::IDescriptorTable* Render::BindlessDescriptorTable::GetDescriptorTable() const
{
	return mDescriptorTable;
}

void Render::BindlessDescriptorTable::Grow(uint32_t size)
{
	uint32_t newCapacity = std::max(64u, size * 2);
	RendererContext::GetDevice()->resizeDescriptorTable(mDescriptorTable, newCapacity);

	mAllocatedDescriptorIndexes.resize(newCapacity);
	mDescriptors.resize(newCapacity);

	memset(&mDescriptors[size], 0, sizeof(nvrhi::BindingSetItem) * (newCapacity - size));
}
