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
#else
#pragma comment(lib, "rel/SDL3.lib")
#endif

#endif