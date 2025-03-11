#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/ext.hpp>

#include "znmsp.h"

#include "Core/Ref.h"

BEGIN_ENGINE
namespace Render {

    class ComputeShader
    {
    public:

        DLLEXPORT ComputeShader(const std::string_view path, uint32_t permIndex);
        DLLEXPORT  ~ComputeShader();
    };
}
END_ENGINE