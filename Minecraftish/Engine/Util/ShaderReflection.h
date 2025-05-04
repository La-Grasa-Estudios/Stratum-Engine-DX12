#pragma once

#include "znmsp.h"

BEGIN_ENGINE

enum class ShaderReflectionResourceType : uint8_t
{
	CBUFFER,
	TEXTURE,
	SAMPLER,
	STRUCTURED_BUFFER,
	BYTEADDRESS,
	UAV_RW_TYPED,
	UAV_RW_STRUCTURED,
	UAV_RW_BYTEADDRESS,
};

enum class ShaderReflectionResourceDimension : uint8_t
{
	UNKNOWN,
	BUFFER,
	TEXTURE1D,
	TEXTURE1DARRAY,
	TEXTURE2D,
	TEXTURE2DARRAY,
	TEXTURE3D,
	TEXTURECUBE,
	TEXTURECUBEARRAY,
};

struct shaderbinding_t
{
	ShaderReflectionResourceType Type;
	ShaderReflectionResourceDimension Dimension;
	uint8_t BindingPoint;
	uint8_t BindingSpace;
};

struct shaderreflection_t
{
	uint32_t NumShaderBidings;
};

END_ENGINE