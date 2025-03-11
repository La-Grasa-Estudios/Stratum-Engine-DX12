#include "ShaderPreprocessor.h"

#include "Core/Logger.h"
#include "VFS/ZVFS.h"

#include <fstream>
#include <sstream>
#include <bitset>

std::vector<std::string> GetParameters(std::string params) {

    std::vector<std::string> p;

    std::string s;
    for (int i = 0; i < params.size(); i++)
    {
        char c = params[i];
        if (c == ' ')
        {
            if (!s.empty()) p.push_back(s);
            s.clear();
            continue;
        }
        s += c;
    }
    p.push_back(s);

    return p;
}

std::bitset<256> increment(std::bitset<256> in) {
    //  add 1 to each value, and if it was 1 already, carry the 1 to the next.
    for (size_t i = 0; i < 256; ++i) {
        if (in[i] == 0) {  // There will be no carry
            in[i] = 1;
            break;
        }
        in[i] = 0;  // This entry was 1; set to zero and carry the 1
    }
    return in;
}

ShaderPreprocessor::ShaderPreprocessor()
{
    AddSearchPath("Engine/Include/");
}

std::string ShaderPreprocessor::PreProcessSource(std::string& code)
{
    std::string newCode = PreProcessCode(code);
    s_Included.clear();
    return newCode;
}

void ShaderPreprocessor::AnalyzeDependencies(std::string& code, std::filesystem::file_time_type* lastTime, uint32_t depth)
{

    if (depth == 0)
    {
        m_Permutations.clear();
        std::vector<std::string> permutationBases;
        std::map<std::string, std::vector<std::string>> permutations;

        std::istringstream f(code);
        std::string line;

        std::vector<std::string> conds;

        while (std::getline(f, line)) {

            std::string s;
            for (int i = 0; i < line.size(); i++)
            {
                char c = line[i];
                if (c == '\r')
                {
                    continue;
                }
                s += c;
            }

            line = s;

            if (line.starts_with("#permutationbase"))
            {
                conds.clear();
                permutationBases.push_back(line.substr(17));
                permutations[line.substr(17)] = { line.substr(17) };
                continue;
            }

            if (line.starts_with("#permutationcond"))
            {
                conds = GetParameters(line.substr(17));
                continue;
            }

            if (line.starts_with("#permutationadd") && !conds.empty())
            {
                auto perms = GetParameters(line.substr(16));

                for (auto str : perms)
                {
                    for (auto bperm : conds)
                    {
                        if (bperm.compare("@ALL") == 0)
                        {
                            for (auto p : permutationBases)
                            {
                                permutations[p].push_back(str);
                            }
                            break;
                        }
                        if (permutations.contains(bperm))
                        {
                            permutations[bperm].push_back(str);
                        }
                    }
                }
                continue;
            }

        }

        for (auto kp : permutations)
        {

            uint32_t count = (kp.second.size() - 1);
            count *= count;
            count += 1;

            if (count == 1)
            {
                std::vector<std::string> permutationIndex;
                permutationIndex.push_back(kp.first);
                m_Permutations.push_back(permutationIndex);
                continue;
            }

            std::bitset<256> bits{};

            for (int i = 0; i < count; i++)
            {
                std::vector<std::string> permutationIndex;
                permutationIndex.push_back(kp.second[0]);

                for (int j = 1; j < kp.second.size(); j++)
                {
                    if (bits.test(j - 1))
                    {
                        permutationIndex.push_back(kp.second[j]);
                    }
                }

                if (i == 0 || permutationIndex.size() > 1) m_Permutations.push_back(permutationIndex);

                bits = increment(bits);

            }

        }

        m_Permutations.push_back({});
    }

    depth = depth + 1;

    std::filesystem::file_time_type lt = *lastTime;
    constexpr const char* includeDef = "#include";

    size_t offset = code.find(includeDef, 0);

    while (offset != std::string::npos) {

        char* str = (char*)code.c_str() + offset;

        size_t first = code.find_first_of('"', offset);
        size_t second = code.find_first_of('"', first + 1);
        size_t includeSize = second - first;

        std::string include(code.c_str() + first + 1, includeSize - 1);
        std::string includePath;

        std::filesystem::path p(include);

        std::string name = p.filename().string();
        std::string includeCode = OpenFile(include.c_str(), includePath);

        if (!includeCode.empty())
        {
            std::filesystem::path p1(includePath);
            p1.remove_filename();
            std::string p1s = p1.string();
            AddSearchPath(p1s);
        }

        std::string space;

        for (int i = 0; i < depth; i++) {
            space.append("----");
        }

        //ENGINE_NAMESPACE::Logger::LogInfo("Analyzing Dependencies for: |{}[ {}", space, include);
        AnalyzeDependencies(includeCode, &lt, depth + 1);

        offset = code.find(includeDef, offset + 1);

    }

}

void ShaderPreprocessor::AddSearchPath(const std::string& path)
{
    m_SearchPaths.push_back(path);
}

uint32_t ShaderPreprocessor::GetPermutationCount()
{
    return m_Permutations.size();
}

std::vector<std::string> ShaderPreprocessor::GetPermutation(uint32_t index)
{
    return m_Permutations[index];
}

std::string ShaderPreprocessor::PreProcessCode(std::string& code)
{
    std::vector<std::string> permutationBases;
    std::map<std::string, std::vector<std::string>> permutations;

    std::istringstream f(code);
    std::string line;

    std::string ncode;
    std::vector<std::string> conds;

    while (std::getline(f, line)) {

        std::string s;
        for (int i = 0; i < line.size(); i++)
        {
            char c = line[i];
            if (c == '\r')
            {
                continue;
            }
            s += c;
        }

        line = s;

        if (line.starts_with("#permutationbase"))
        {
            conds.clear();
            permutationBases.push_back(line.substr(17));
            permutations[line.substr(17)] = { line.substr(17) };
            continue;
        }

        if (line.starts_with("#permutationcond"))
        {
            conds = GetParameters(line.substr(17));
            continue;
        }

        if (line.starts_with("#permutationadd") && !conds.empty())
        {
            auto perms = GetParameters(line.substr(16));

            for (auto str : perms)
            {
                for (auto bperm : conds)
                {
                    if (bperm.compare("@ALL") == 0)
                    {
                        for (auto p : permutationBases)
                        {
                            permutations[p].push_back(str);
                        }
                        break;
                    }
                    if (permutations.contains(bperm))
                    {
                        permutations[bperm].push_back(str);
                    }
                }
            }
            continue;
        }

        ncode.append(line);
        ncode.append("\n");

    }

    code = ncode;

    constexpr const char* includeDef = "#include";

    size_t offset = code.find(includeDef, 0);
    size_t lastValidOffset = 0;
    size_t nbIncludes = 0;
    size_t prev = offset != std::string::npos ? offset : 0;

    std::string newCode(code.c_str(), prev);

    while (offset != std::string::npos) {

        char* str = (char*)code.c_str() + offset;

        size_t first = code.find_first_of('"', offset);
        size_t second = code.find_first_of('"', first + 1);
        size_t includeSize = second - first;
        size_t nextInclude = 0;

        std::string include(code.c_str() + first + 1, includeSize - 1);

        std::filesystem::path p(include);

        std::string name = p.filename().string();

        if (!s_Included.contains(name)) {
            s_Included[name] = name;


            std::string includePath;

            std::string includeCode = OpenFile(include.c_str(), includePath);

            Z_INFO("Including: {}", includePath);

            std::filesystem::path p1(includePath);
            p1.remove_filename();
            std::string p1s = p1.string();
            AddSearchPath(p1s);

            std::string appendableCode = PreProcessCode(includeCode);

            newCode.append(appendableCode);
            newCode.append("\n");

        }

        if ((nextInclude = code.find(includeDef, offset + 1)) != std::string::npos && (nextInclude - second) > 3) {
            std::string codeBetween(code.c_str() + second + 1, nextInclude - second - 1);
            newCode.append(codeBetween);
        }

        lastValidOffset = offset;
        offset = code.find(includeDef, offset + 1);

        nbIncludes++;

    }

    if (nbIncludes > 0) {

        size_t eol = code.find_first_of('\n', lastValidOffset);

        if (eol == std::string::npos) {
            code = "";
        }
        else {
            code = code.substr(eol + 1);
        }


    }

    newCode.append(code);

    return newCode;
}

std::string ShaderPreprocessor::OpenFile(const char* file, std::string& fullpath)
{

    if (ENGINE_NAMESPACE::ZVFS::Exists(file))
    {
        ENGINE_NAMESPACE::RefBinaryStream stream = ENGINE_NAMESPACE::ZVFS::GetFile(file);
        return stream->Str();
    }

    for (int i = 0; i < m_SearchPaths.size(); i++)
    {
        std::string fullPath = m_SearchPaths[i];
        fullPath.append(file);
        if (ENGINE_NAMESPACE::ZVFS::Exists(fullPath.c_str()))
        {
            fullpath.clear();
            fullpath.append(fullPath);
            ENGINE_NAMESPACE::RefBinaryStream stream = ENGINE_NAMESPACE::ZVFS::GetFile(fullPath.c_str());
            return stream->Str();
        }
    }

    Z_ERROR("Failed to open include file: {}", file);

    return "";
}
