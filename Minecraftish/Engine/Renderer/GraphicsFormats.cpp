#include "GraphicsFormats.h"

using namespace ENGINE_NAMESPACE;

nvrhi::Format Render::FormatUtil::ConvertEngineFormatToNV(const ImageFormat format)
{
    return EngineToNVFormats[(int)format];
}
