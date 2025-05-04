#pragma once

#include "znmsp.h"

#include <string>
#include <map>
#include <filesystem>

class ShaderPreprocessor
{

public:

	ShaderPreprocessor();

	std::string PreProcessSource(std::string& code);
	void AnalyzeDependencies(std::string& code, std::filesystem::file_time_type* lastTime, uint32_t depth = 0);

	void AddSearchPath(const std::string& path);
	uint32_t GetPermutationCount();
	std::vector<std::string> GetPermutation(uint32_t index);

private:
	std::string PreProcessCode(std::string& code);
	std::string OpenFile(const char* file, std::string& fullpath);

	std::map<std::string, std::string> s_Included;

	std::vector<std::string> m_SearchPaths;
	std::vector<std::vector<std::string>> m_Permutations;



};