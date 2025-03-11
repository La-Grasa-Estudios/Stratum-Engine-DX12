#pragma once

#include "znmsp.h"
#include "Core/Ref.h"

#include <glm/glm.hpp>

BEGIN_ENGINE

namespace Render {


	class ConstantBuffer {

	public:

		DLLEXPORT ConstantBuffer(size_t size);
		DLLEXPORT ~ConstantBuffer();

		DLLEXPORT void Update(void* src);

		template<typename T>
		T* cast() {
			return (T*)NativePointer;
		}

		void* NativePointer;
		size_t Size;

	private:


	};

	typedef glm::mat4 CMatrix4;
	class CMatrix3 {
		CMatrix4 data;
	public:
		CMatrix3& operator=(const glm::mat3& other)
		{
			
			memcpy(&data, &other, sizeof(glm::mat3));

			return *this;
		}
	};

	typedef glm::vec4 CVector4;

	struct CVector3
	{
	public:

		CVector3& operator=(const glm::vec3& other)
		{
			v.x = other.x;
			v.y = other.y;
			v.z = other.z;
			v.w = 1.0f;

			return *this;
		}

	private:
		CVector4 v;
	};

	struct CVector2
	{
	public:

		CVector2& operator=(const glm::vec2& other)
		{
			v.x = other.x;
			v.y = other.y;
			v.z = 0.0f;
			v.w = 1.0f;

			return *this;
		}

	private:
		CVector4 v;
	};

}

END_ENGINE