#pragma once

#include "znmsp.h"

#include "RendererContext.h"
#include "RenderCommands.h"
#include "TextureSampler.h"
#include "Texture3D.h"
#include "ComputePipeline.h"
#include "Buffer.h"

BEGIN_ENGINE

namespace Render {

	class GraphicsCommandBuffer;

	/// <summary>
	/// Is used to record and send compute related commands to the gpu
	/// </summary>
	class ComputeCommandBuffer
	{
	public:

		ComputeCommandBuffer(bool async = false); // Default constructor, async is used to set which commandQueue this buffer is send to when using async you should use wait to synchronize between queues
		ComputeCommandBuffer(GraphicsCommandBuffer* pCmdBuffer); // Creates this buffer with a GraphicsCommandBuffer as the parent, only need to call begin and no end or submit, it appends its command to the parent command buffer

		void Begin(); // Clears the command buffer state and opens the underlying command list, not needed when used with a GraphicsCommandBuffer
		void End(); // Serializes the command buffer
		void Submit(); // Submits the command buffer to the respective graphics queue
		void Wait(); // If using async compute it syncs beetween the graphics and compute queue
		void ClearState(); // Clears the whole compute state

		void SetComputePipeline(ComputePipeline* pipeline); // Sets the compute pipeline

		/// <summary>
		/// Sets a constant buffer to the specified slot
		/// </summary>
		/// <param name="buffer">The buffer to be set</param>
		/// <param name="slot">The buffer register</param>
		void SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets a texture resource to the specified slot
		/// </summary>
		/// <param name="texture">The texture to be set</param>
		/// <param name="slot">The resource register</param>
		void SetTextureResource(ImageResource* texture, uint32_t slot);

		/// <summary>
		/// Sets a texture resource to the specified slot
		/// </summary>
		/// <param name="texture">The texture to be set</param>
		/// <param name="slot">The resource register</param>
		void SetTextureResource(Texture3D* texture, uint32_t slot);

		/// <summary>
		/// Sets a texture sampler to the specified slot
		/// </summary>
		/// <param name="sampler">The sampler to be set</param>
		/// <param name="slot">The sampler register</param>
		void SetTextureSampler(TextureSampler* sampler, uint32_t slot);

		/// <summary>
		/// Sets a buffer resource to the specified slot
		/// </summary>
		/// <param name="buffer">The buffer to be set</param>
		/// <param name="slot">The resource register</param>
		void SetBufferResource(Buffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets a buffer as a uav to the specified slot
		/// </summary>
		/// <param name="buffer">The buffer to be set</param>
		/// <param name="slot">The uav register</param>
		void SetBufferCompute(Buffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets a texture as a uav to the specified slot
		/// </summary>
		/// <param name="texture">The texture to be set</param>
		/// <param name="slot">The uav register</param>
		void SetTextureCompute(ImageResource* texture, uint32_t slot);

		/// <summary>
		/// Sets a texture as a uav to the specified slot
		/// </summary>
		/// <param name="texture">The texture to be set</param>
		/// <param name="slot">The uav register</param>
		void SetTextureCompute(Texture3D* texture, uint32_t slot);

		/// <summary>
		/// Sets the root/push constants on the pipeline
		/// </summary>
		/// <param name="ptr">Pointer to memory</param>
		/// <param name="size">The size in bytes (max 128 bytes on root/push constants)</param>
		void PushConstants(void* ptr, size_t size);

		/// <summary>
		/// Dispatches a compute shader instance
		/// </summary>
		/// <param name="x">Number of workgroups X</param>
		/// <param name="y">Number of workgroups Y</param>
		/// <param name="z">Number of workgroups Z</param>
		void Dispatch(uint32_t x, uint32_t y, uint32_t z);


		/// <summary>
		/// Updates a constant buffer with the specified memory
		/// </summary>
		/// <param name="pBuffer">The constant buffer to update</param>
		/// <param name="data">Pointer to memory must be the same or greater size as the constant buffer</param>
		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		void Barrier(ImageResource* resource);
		void Barrier(Texture3D* resource);
		void Barrier(Buffer* resource);

		void CommitBarriers();

		void ToggleAutomaticBarrierPlacement();

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		void UpdateComputeState();

		nvrhi::CommandListHandle mCommandList;
		nvrhi::IComputePipeline* mSetComputePipeline;

		nvrhi::BindingSetHandle mBindingSet;

		nvrhi::EventQueryHandle mEventQuery;

		std::array<nvrhi::IBuffer*, 32> mBufferUnits;
		std::array<nvrhi::ITexture*, 32> mTextureUnits;
		std::array<nvrhi::ISampler*, 32> mSamplerUnits;
		std::array<nvrhi::IBuffer*, 32> mConstantBuffers;

		std::array<nvrhi::ITexture*, 32> mTextureCompute;
		std::array<nvrhi::IBuffer*, 32> mBufferCompute;

		bool mComputeStateDirty = true;
		bool mBindingStateDirty = true;
		bool mCommitedAnyConstantBuffer = false;
		bool mAutomaticBarriers = true;
		bool mIsOwner = true;

		ComputePipeline* mCurrentPipeline;
	};
}

END_ENGINE