#pragma once

#include <iostream>

#define ZASSERT(reason, condition) if (!(condition)) { std::cout << reason; assert(false); }

#define DLLEXPORT

#define ENGINE_NAMESPACE Stratum
#define BEGIN_ENGINE namespace ENGINE_NAMESPACE {
#define END_ENGINE }

#ifndef InfoLog
#define InfoLog(msg) std::cout << "[INFO] " << msg << std::endl
#endif

#ifndef InfoLogCs
#define InfoLogCs(origin, msg) std::cout << "[INFO::" << origin << "]" << msg << std::endl
#endif

#ifndef ErrorLogCs
#define ErrorLogCs(origin, msg) std::cout << "[ERROR::" << origin << "]" << msg << std::endl
#endif

#ifndef WarnLog
#define WarnLog(msg) std::cout << "[WARNING] " << msg << std::endl
#endif

#ifndef ErrorLog
#define ErrorLog(msg) std::cout << "[ERROR] " << msg << std::endl
#endif

#ifndef ZGUILog
#define ZGUILog(msg) std::cout << "[GUI] " << msg << std::endl
#endif