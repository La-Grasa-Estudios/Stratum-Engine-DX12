#include "EventHandler.h"
#include <Core/Logger.h>

using namespace ENGINE_NAMESPACE;

void EventHandler::Process()
{
	s_EventQueueLock.lock();

	for (int i = 0; i < s_EventQueue.size(); i++)
	{
		Event e = s_EventQueue[i];
		std::get<0>(e)(std::get<1>(e), std::get<2>(e).data(), std::get<3>(e));
	}

	s_EventQueue.clear();

	s_EventQueueLock.unlock();
}

void EventHandler::InvokeEvent(uint64_t eventId, void* sender, std::vector<void*> args, uint32_t argc)
{
	if (s_EventHandlers.contains(eventId))
	{
		auto& listeners = s_EventHandlers[eventId];

#ifdef _DEBUG
		Z_INFO("Event! ({}) listeners count: {}", s_EventNames[eventId], listeners.size());
#endif

		for (int i = 0; i < listeners.size(); i++)
		{
			auto& listener = listeners[i];
			if (listener.threadSafe)
			{
				listener.func(sender, args.data(), argc);
			}
			else
			{
				s_EventQueueLock.lock();
				s_EventQueue.push_back(Event(listener.func, sender, args, argc));
				s_EventQueueLock.unlock();
			}
		}
	}
	else
	{
		if (s_EventNames.contains(eventId))
		{
			Z_WARN("No listeners found for event: {}", s_EventNames[eventId]);
		}
		else
		{
			Z_ERROR("Invalid event id: {}", eventId);
		}
	}
}

void EventHandler::RegisterListener(EventFunction func, uint64_t eventId, bool threadSafe)
{
	if (!s_EventHandlers.contains(eventId)) s_EventHandlers[eventId] = {};

	s_EventHandlers[eventId].push_back({ func, threadSafe });
}

uint64_t EventHandler::GetEventID(const std::string& name)
{
#ifdef _DEBUG
	//Z_INFO("Get Event: {}", name);
#endif
	if (s_EventIDs.contains(name))
	{
		return s_EventIDs[name];
	}
	uint64_t c = s_FreeEventIDs.fetch_add(1);
	s_EventIDs[name] = c;
	s_EventNames[c] = name;
	return c;
}
