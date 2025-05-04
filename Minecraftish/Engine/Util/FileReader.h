#pragma once

#include "Common.h"

namespace ENGINE_NAMESPACE
{

	class FileUtils
	{

	public:

		static std::string ReadAllText(const std::string& path);

	private:
		FileUtils();

	};

}