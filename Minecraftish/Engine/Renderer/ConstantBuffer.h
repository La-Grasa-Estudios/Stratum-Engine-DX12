#pragma once

#include "znmsp.h"
#include "RendererContext.h"

#include <glm/glm.hpp>

BEGIN_ENGINE

namespace Render {


	class ConstantBuffer {

	public:

		/// <summary>
		/// Creates a constant buffer
		/// </summary>
		/// <param name="size">Size in bytes</param>
		/// <param name="defaultData">Default data, used to create immutable constant buffers, default is nullptr</param>
		ConstantBuffer(size_t size, void* defaultData = NULL);
		~ConstantBuffer();

		bool IsResourceReady(); // Used when defaultData is set in the constructor, returns true when the data is ready on the gpu

		nvrhi::BufferHandle Handle;
		size_t Size; // Size of the constant buffer as specified in the constructor

	private:

		bool mIsReady = false;

	};

}

END_ENGINE