#pragma once

#include "znmsp.h"

BEGIN_ENGINE

class PathUtils
{
public:
	/// <summary>
	/// Processes a path and deletes terminator symbols "\n-\r"
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	static std::string ResolvePath(const std::string& path);
};

END_ENGINE