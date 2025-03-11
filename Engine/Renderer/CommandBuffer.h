#pragma once

#include "znmsp.h"
#include "Core/Ref.h"
#include "RendererContext.h"
#include "RenderCommands.h"
#include "GraphicsPipeline.h"
#include "ComputeShader.h"
#include "ComputeResource.h"
#include "TextureSampler.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

BEGIN_ENGINE

namespace Render {

	constexpr uint32_t COMMAND_BUFFER_COMPUTE_FLAG = 0x1;

	class CommandBuffer {

	public:

		CommandBuffer(uint32_t flags);
		~CommandBuffer();

		void Begin();
		void End();
		void Submit();

		void SetPipeline(GraphicsPipeline* pipeline);

		void SetConstantBuffer(ConstantBuffer* buffer, uint32_t slot);
		void SetVertexBuffer(VertexBuffer* buffer, uint32_t slot);
		void SetIndexBuffer(IndexBuffer* buffer);
		void SetFramebuffer(Framebuffer* framebuffer, ComputeResource** pUavs = NULL, uint32_t nbUavs = 0);
		void PushConstants(void* ptr, size_t size, uint32_t index);

		void SetTextureResource(TextureSampler* texture, uint32_t slot);

		void SetViewport(Viewport* vp);

		void Draw(uint32_t count);

		void DrawIndexed(uint32_t count);
		void DrawIndexedInstanced(uint32_t count, uint32_t instanceCount, uint32_t baseInstance);
		void DrawIndexedIndirect(uint32_t count);

		void SetComputeShader(ComputeShader* shader);
		void SetComputeResource(ComputeResource* resource, uint32_t slot);

		void Dispatch(uint32_t x, uint32_t y, uint32_t z);

		void ClearBuffer(Framebuffer* framebuffer, const glm::vec4 color, float depth = -1.0f);

		static Ref<CommandBuffer> Create(uint32_t flags = 0);

		void UpdateConstantBuffer(ConstantBuffer* pBuffer, void* data);
		void BlitImage(ImageResource* pDst, ImageResource* pSrc);

		void Barrier(ComputeResource* resource);

		bool CreatedWithCompute;
		void* NativeData;

		void* TextureSamplers[32];

		inline static std::binary_semaphore SyncPrimitive = std::binary_semaphore(1);
		inline static std::vector<void*> GlobalPointers;

	private:

		GraphicsPipeline* pCurrentPipeline;
	};

}

END_ENGINE