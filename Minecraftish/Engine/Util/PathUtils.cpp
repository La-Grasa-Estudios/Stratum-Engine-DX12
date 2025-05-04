#include "PathUtils.h"

using namespace ENGINE_NAMESPACE;

std::string PathUtils::ResolvePath(const std::string& path)
{
	std::string r;
	for (int i = 0; i < path.length(); i++)
	{
		char c = path[i];

		if (c == '\r' || c == '\n')
		{
			continue;
		}

		r += c;
	}
	return r;
}
