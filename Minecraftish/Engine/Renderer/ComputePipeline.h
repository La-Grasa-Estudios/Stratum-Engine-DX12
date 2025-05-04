#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <nvrhi/nvrhi.h>
#include <glm/ext.hpp>

#include "znmsp.h"

#include "Core/Ref.h"

BEGIN_ENGINE
namespace Render {

    /// <summary>
    /// Compute Pipeline description
    /// </summary>
    struct ComputePipelineDesc
    {
        std::string path; // path to the cso file
        uint32_t permIndex; // index of the permutation using defines in the shader as PERM_X
        std::vector<nvrhi::BindingLayoutItem> BindingItems; // Binding items, mainly used to specify push constants

        // Short hand for adding binding items
        ComputePipelineDesc& addBindingItem(const nvrhi::BindingLayoutItem& item) { BindingItems.push_back(item); return *this; }
    };

    class ComputePipeline
    {
    public:

        ComputePipeline(const ComputePipelineDesc& desc);
         ~ComputePipeline();

        nvrhi::ComputePipelineHandle Handle;
        nvrhi::BindingLayoutHandle BindingLayout;

        ComputePipelineDesc ShaderDesc; // Description used to create the compute pipeline
    };
}
END_ENGINE