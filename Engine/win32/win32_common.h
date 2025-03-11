#pragma once

#ifdef WIN32
#define WIN32_PLAT_GL
#endif

#ifdef WIN32_PLAT_GL

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glfw3.lib")

#pragma comment(lib, "av/avcodec.lib")
#pragma comment(lib, "av/avdevice.lib")
#pragma comment(lib, "av/avfilter.lib")
#pragma comment(lib, "av/avformat.lib")
#pragma comment(lib, "av/avutil.lib")
#pragma comment(lib, "av/swscale.lib")
#pragma comment(lib, "av/swresample.lib")

#pragma comment(lib, "zlibstatic.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "db/SDL3.lib")
#pragma comment(lib, "spirv-d/spirv-cross-cd.lib")
#pragma comment(lib, "spirv-d/spirv-cross-cored.lib")
#pragma comment(lib, "spirv-d/spirv-cross-cppd.lib")
#pragma comment(lib, "spirv-d/spirv-cross-glsld.lib")
#pragma comment(lib, "spirv-d/spirv-cross-hlsld.lib")
#pragma comment(lib, "spirv-d/spirv-cross-msld.lib")
#pragma comment(lib, "spirv-d/spirv-cross-reflectd.lib")
#pragma comment(lib, "spirv-d/spirv-cross-utild.lib")
#pragma comment(lib, "shaderc_combinedd.lib")
#else
#pragma comment(lib, "rel/SDL3.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-c.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-core.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-cpp.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-glsl.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-hlsl.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-msl.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-reflect.lib")
#pragma comment(lib, "spirv-rel/spirv-cross-util.lib")
#pragma comment(lib, "shaderc_combined.lib")
#endif

#endif