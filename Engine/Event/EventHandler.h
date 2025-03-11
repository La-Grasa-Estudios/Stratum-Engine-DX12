#pragma once

#include "znmsp.h"

#include <functional>
#include <map>
#include <unordered_map>
#include <tuple>
#include <mutex>
#include <atomic>

BEGIN_ENGINE

using EventFunction = std::function<void(void*, void**, uint32_t)>;
using Event = std::tuple<EventFunction, void*, std::vector<void*>, uint32_t>;

class EventHandler {

	struct EventListener
	{
		EventFunction func;
		bool threadSafe;
	};

public:

	DLLEXPORT static void Process();
	DLLEXPORT static void InvokeEvent(uint64_t eventId, void* sender, std::vector<void*> args = {}, uint32_t argc = 0);

	DLLEXPORT static void RegisterListener(EventFunction func, uint64_t eventId, bool threadSafe = false);
	DLLEXPORT static uint64_t GetEventID(const std::string& name);

private:

	inline static std::mutex s_EventQueueLock;
	inline static std::vector<Event> s_EventQueue;
	inline static std::map<uint64_t, std::vector<EventListener>> s_EventHandlers;
	inline static std::unordered_map<std::string, uint64_t> s_EventIDs;
	inline static std::unordered_map<uint64_t, std::string> s_EventNames;
	inline static std::atomic<uint64_t> s_FreeEventIDs;

};

END_ENGINE