#pragma once

#include "znmsp.h"

#include <string>
#include <format>
#include <utility>
#include <mutex>

#ifndef _DEBUG

#define Z_INFO(...) ENGINE_NAMESPACE::Logger::LogInfo(__VA_ARGS__);
#define Z_WARN(...) ENGINE_NAMESPACE::Logger::LogWarn(__VA_ARGS__);
#define Z_ERROR(...) ENGINE_NAMESPACE::Logger::LogError(__VA_ARGS__);

#else

template <typename T, size_t S>
inline constexpr size_t get_file_name_offset(const T(&str)[S], size_t i = S - 1)
{
	return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? get_file_name_offset(str, i - 1) : 0);
}

template <typename T>
inline constexpr size_t get_file_name_offset(T(&str)[1])
{
	return 0;
}

#ifndef _MSC_VER
#define __FUNCTION__ ""
#endif

#define __FILENAME__ &__FILE__[get_file_name_offset(__FILE__)]

#define Z_INFO(...) ENGINE_NAMESPACE::Logger::LogInfo(__FILENAME__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define Z_WARN(...) ENGINE_NAMESPACE::Logger::LogWarn(__FILENAME__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define Z_ERROR(...) ENGINE_NAMESPACE::Logger::LogError(__FILENAME__, __LINE__, __FUNCTION__, __VA_ARGS__);

#endif

BEGIN_ENGINE

	class Logger;

	enum class LogLevel {
		INFO,
		WARNING,
		LERROR
	};

	class LogReceiver {

		friend Logger;

	protected:

		DLLEXPORT virtual void Log(std::string_view msg, LogLevel level);
		DLLEXPORT const char* Format(LogLevel level);

		DLLEXPORT void Lock();
		DLLEXPORT void Release();

	private:

		std::mutex g_SyncLock;

	};

	class Logger {

		inline static LogReceiver* s_LogReceiver = new LogReceiver();

#ifndef _DEBUG
		template<typename ... Args>
		static void Log(std::string_view fmt, LogLevel level, Args&&... args) {
			s_LogReceiver->Log(std::vformat(fmt, std::make_format_args(args...)), level);
		}
#else
		template<typename ... Args>
		static void Log(const char* filename, int line, const char* fnname, std::string_view fmt, LogLevel level, Args&&... args) {
			s_LogReceiver->Log(std::format("<{}:{}> ({}) {}", filename, line, fnname, std::vformat(fmt, std::make_format_args(args...))), level);
		}
#endif
	public:

#ifndef _DEBUG
		template<typename ... Args>
		static void LogInfo(std::string_view fmt, Args&&... args) {
			Log(fmt, LogLevel::INFO, std::forward<Args>(args)...);
		}

		template<typename ... Args>
		static void LogWarn(std::string_view fmt, Args&&... args) {
			Log(fmt, LogLevel::WARNING, std::forward<Args>(args)...);
		}

		template<typename ... Args>
		static void LogError(std::string_view fmt, Args&&... args) {
			Log(fmt, LogLevel::LERROR, std::forward<Args>(args)...);
		}
#else
		template<typename ... Args>
		static void LogInfo(const char* filename, int line, const char* fnname, std::string_view fmt, Args&&... args) {
			Log(filename, line, fnname, fmt, LogLevel::INFO, std::forward<Args>(args)...);
		}

		template<typename ... Args>
		static void LogWarn(const char* filename, int line, const char* fnname, std::string_view fmt, Args&&... args) {
			Log(filename, line, fnname, fmt, LogLevel::WARNING, std::forward<Args>(args)...);
		}

		template<typename ... Args>
		static void LogError(const char* filename, int line, const char* fnname, std::string_view fmt, Args&&... args) {
			Log(filename, line, fnname, fmt, LogLevel::LERROR, std::forward<Args>(args)...);
		}
#endif

		__declspec(dllexport) static void SetLogger(LogReceiver* pReceiver);

	private:

};

END_ENGINE