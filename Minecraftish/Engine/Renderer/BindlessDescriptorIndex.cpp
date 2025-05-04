#include "BindlessDescriptorTable.h"
#include "RendererContext.h"

using namespace ENGINE_NAMESPACE;
using namespace Render;

BindlessDescriptorIndex::BindlessDescriptorIndex(BindlessDescriptorTable* pTable, int32_t index, nvrhi::IResource* pResource)
{
	mResourcePointer = pResource;
	mResourcePointer->AddRef();

	mDescriptorTable = pTable;
	mIndex = index;
}

void BindlessDescriptorIndex::Release()
{
	if (!mDescriptorTable->mIsTableValid)
	{
		return;
	}

	std::scoped_lock l(mDescriptorTable->lockMutex);

	mDescriptorTable->mAllocatedDescriptorIndexes[mIndex] = false;
	mDescriptorTable->mDescriptors[mIndex] = nvrhi::BindingSetItem::None();

	auto indexMap = mDescriptorTable->mAllocatedDescriptorsMap.find(mDescriptorTable->mDescriptors[mIndex]);
	if (indexMap != mDescriptorTable->mAllocatedDescriptorsMap.end())
		mDescriptorTable->mAllocatedDescriptorsMap.erase(indexMap);

	auto item = nvrhi::BindingSetItem::None(mIndex);

	RendererContext::GetDevice()->writeDescriptorTable(mDescriptorTable->GetDescriptorTable(), item);

	mDescriptorTable->mSearchStart = std::min(mIndex, mDescriptorTable->mSearchStart);

	mResourcePointer->Release();
}

int32_t Render::BindlessDescriptorIndex::GetIndex()
{
	if (mIndex >= 0 && mDescriptorTable->mIsTableValid)
	{
		return mIndex;
	}
	return -1;
}