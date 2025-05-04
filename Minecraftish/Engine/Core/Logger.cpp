#include "Logger.h"

#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include <WinBase.h>
#include <consoleapi2.h>
#endif

void ENGINE_NAMESPACE::LogReceiver::Log(std::string_view msg, LogLevel level)
{
	Lock();

#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	
	switch (level)
	{
	case Stratum::LogLevel::INFO:
		SetConsoleTextAttribute(hConsole, 15);
		break;
	case Stratum::LogLevel::WARNING:
		SetConsoleTextAttribute(hConsole, 14);
		break;
	case Stratum::LogLevel::LERROR:
		SetConsoleTextAttribute(hConsole, 12);
		break;
	default:
		break;
	}

#endif

	std::cout << Format(level) << msg << std::endl;

#ifdef _WIN32
	SetConsoleTextAttribute(hConsole, 15);
#endif

	Release();
}

const char* ENGINE_NAMESPACE::LogReceiver::Format(LogLevel level)
{
	return (level == LogLevel::INFO ? "[INFO] " : level == LogLevel::WARNING ? "[WARNING] " : "[ERROR] ");
}

void ENGINE_NAMESPACE::LogReceiver::Lock()
{
	g_SyncLock.lock();
}

void ENGINE_NAMESPACE::LogReceiver::Release()
{
	g_SyncLock.unlock();
}

void ENGINE_NAMESPACE::Logger::SetLogger(LogReceiver* pReceiver)
{
	s_LogReceiver = pReceiver; // This leaks about 40 bytes of memory when first used
}
