#include "FileReader.h"

#include <fstream>
#include <sstream>

using namespace ENGINE_NAMESPACE;

std::string FileUtils::ReadAllText(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);

    if (!in.is_open() || !in.good())
    {
        return "";
    }

    std::stringstream str;
    str << in.rdbuf();

    return str.str();
}
