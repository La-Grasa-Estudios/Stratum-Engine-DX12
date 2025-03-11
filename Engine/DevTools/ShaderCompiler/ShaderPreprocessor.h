#pragma once

#include "znmsp.h"

#include <string>
#include <map>
#include <filesystem>

class ShaderPreprocessor
{

public:

	DLLEXPORT ShaderPreprocessor();

	DLLEXPORT std::string PreProcessSource(std::string& code);
	DLLEXPORT void AnalyzeDependencies(std::string& code, std::filesystem::file_time_type* lastTime, uint32_t depth = 0);

	DLLEXPORT void AddSearchPath(const std::string& path);
	DLLEXPORT uint32_t GetPermutationCount();
	DLLEXPORT std::vector<std::string> GetPermutation(uint32_t index);

private:
	std::string PreProcessCode(std::string& code);
	std::string OpenFile(const char* file, std::string& fullpath);

	std::map<std::string, std::string> s_Included;

	std::vector<std::string> m_SearchPaths;
	std::vector<std::vector<std::string>> m_Permutations;



};