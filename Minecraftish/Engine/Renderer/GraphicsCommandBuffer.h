#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "RendererContext.h"
#include "RenderCommands.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "TextureSampler.h"
#include "Texture3D.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include <unordered_set>

BEGIN_ENGINE

namespace Render {

	class BindlessDescriptorTable;

	/// <summary>
	/// Is used to record and send graphics related commands to the gpu
	/// </summary>
	class GraphicsCommandBuffer {

	public:

		GraphicsCommandBuffer();
		~GraphicsCommandBuffer();

		/// Clears the command buffer state and opens the underlying command list
		void Begin(); 
		/// Serializes the command buffer
		void End(); 
		/// Submits the command buffer to the respective graphics queue
		void Submit(); 

		/// <summary>
		/// Sets the graphics pipeline to be used on rendering
		/// </summary>
		/// <param name="pipeline">The pipeline object</param>
		void SetPipeline(GraphicsPipeline* pipeline);

		/// <summary>
		/// Sets a constant buffer to the specified slot
		/// </summary>
		/// <param name="buffer">The buffer to be set</param>
		/// <param name="slot">The buffer register</param>
		void SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets a vertex buffer to the specified slot, it does not accept null buffers, use ClearVertexBuffers for the same behaviour
		/// </summary>
		/// <param name="buffer">The vertex buffer view</param>
		/// <param name="slot">The slot to be set</param>
		void SetVertexBuffer(VertexBuffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets an index buffer to be used with DrawIndexedXXXX calls
		/// </summary>
		/// <param name="buffer">The index buffer view</param>
		void SetIndexBuffer(IndexBuffer* buffer);

		/// <summary>
		/// Sets the framebuffer to be used with rendering
		/// </summary>
		/// <param name="framebuffer">The frambuffer object</param>
		void SetFramebuffer(Framebuffer* framebuffer);

		/// <summary>
		/// Sets the root/push constants on the pipeline
		/// </summary>
		/// <param name="ptr">Pointer to memory</param>
		/// <param name="size">The size in bytes (max 128 bytes on root/push constants)</param>
		void PushConstants(void* ptr, size_t size);

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
		/// Sets a buffer resource to the specified slot
		/// </summary>
		/// <param name="buffer">The buffer to be set</param>
		/// <param name="slot">The resource register</param>
		void SetBufferResource(Buffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets a texture sampler to the specified slot
		/// </summary>
		/// <param name="sampler">The sampler to be set</param>
		/// <param name="slot">The sampler register</param>
		void SetTextureSampler(TextureSampler* sampler, uint32_t slot);

		/// <summary>
		/// Sets a buffer as a uav
		/// </summary>
		/// <param name="buffer">The buffer to be bound, needs to be created with AllowComputeResourceUsage set to true</param>
		/// <param name="slot"></param>
		void SetBufferUav(Buffer* buffer, uint32_t slot);

		/// <summary>
		/// Sets and owns a BindlessDescriptor table until command list as finished execution
		/// </summary>
		/// <param name="table">The table to be set</param>
		void SetBindlessDescriptorTable(Ref<BindlessDescriptorTable> table);

		/// <summary>
		/// Sets the indirect buffer to be used with DrawIndirect and DrawIndexedIndirect
		/// </summary>
		/// <param name="buffer">The buffer to be used, needs to be created with type INDIRECT_BUFFER</param>
		void SetIndirectBuffer(Buffer* buffer);

		/// <summary>
		/// Sets the viewport to be used in rendering
		/// </summary>
		/// <param name="vp">The viewport</param>
		void SetViewport(Viewport* vp);

		void Draw(uint32_t count);
		void DrawInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance);
		void DrawIndirect(uint32_t count, uint64_t offsetBytes);

		void DrawIndexed(uint32_t count);
		void DrawIndexedInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance);
		void DrawIndexedIndirect(uint32_t count);

		/// <summary>
		/// Clears a previously bound framebuffer
		/// </summary>
		/// <param name="index">The index of the attachment (needs to be 0 if it is the window surface)</param>
		/// <param name="color"></param>
		void ClearBuffer(uint32_t index, const glm::vec4 color);

		/// <summary>
		/// Clears a previously bound framebuffer depth attachment
		/// </summary>
		/// <param name="depth">The depth value to be cleared to</param>
		/// <param name="stencil">Optional: the stencil value to be set to</param>
		void ClearDepth(float depth, uint32_t stencil = 0xFF);

		/// <summary>
		/// Updates a constant buffer with the specified memory
		/// </summary>
		/// <param name="pBuffer">The constant buffer to update</param>
		/// <param name="data">Pointer to memory must be the same or greater size as the constant buffer</param>
		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);

		/// <summary>
		/// Clears the underlying vertex buffers bindings
		/// </summary>
		void ClearVertexBuffers();

		nvrhi::ICommandList* GetNativeCommandList();

	private:

		void UpdateGraphicsState();

		struct BoundBindlessTable
		{
			uint64_t FrameIndex;
			Ref<BindlessDescriptorTable> Table;
		};

		struct BindlessTableHasher
		{
			std::size_t operator()(const BoundBindlessTable& item) const;
		};

		struct BindlessTableEqual
		{
			bool operator()(const BoundBindlessTable& a, const BoundBindlessTable& b) const;
		};

		nvrhi::CommandListHandle mCommandList;
		nvrhi::IGraphicsPipeline* mSetGraphicsPipeline;
		nvrhi::IFramebuffer* mSetFramebuffer;
		nvrhi::ViewportState mSetViewport;
		nvrhi::IndexBufferBinding mSetIndexBuffer;
		nvrhi::IDescriptorTable* mSetDescriptorTable;

		nvrhi::IBuffer* mIndirectBufferParams;

		nvrhi::BindingSetHandle mBindingSet;

		std::array<nvrhi::ITexture*, 32> mTextureUnits;
		std::array<nvrhi::ISampler*, 32> mSamplerUnits;
		std::array<nvrhi::IBuffer*, 32> mConstantBuffers;
		std::array<nvrhi::IBuffer*, 16> mBufferResources;
		std::array<nvrhi::IBuffer*, 8> mBufferUavs;
		std::array<nvrhi::IBuffer*, nvrhi::c_MaxVertexAttributes> mVertexBuffers;

		bool mGraphicsStateDirty = true;
		bool mBindingStateDirty = true;
		bool mCommitedAnyConstantBuffer = false;

		std::unordered_set<BoundBindlessTable, BindlessTableHasher, BindlessTableEqual> mBoundTables;

		GraphicsPipeline* mCurrentPipeline;
	};

}

END_ENGINE