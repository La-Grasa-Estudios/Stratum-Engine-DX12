#pragma once

#include "znmsp.h"

#include <vector>
#include <functional>
#include <mutex>

BEGIN_ENGINE

struct JobDispatchArgs
{
    uint32_t jobIndex;
    uint32_t groupIndex;
};

class JobManager {

public:

    DLLEXPORT static void Init();

    DLLEXPORT static void EnqueueMainThread(const std::function<void()>& func);

    DLLEXPORT static void Execute(const std::function<void()>& job);
    DLLEXPORT static void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job);

    DLLEXPORT static void ExecuteMainJobs();

    DLLEXPORT static bool IsBusy();
    DLLEXPORT static void Wait();

private:

    inline static JobManager* s_Instance = NULL;

    std::vector<std::function<void()>> m_MainQueue;

    std::mutex m_MainMutex;

    uint32_t m_NbThreads = 0;

};

END_ENGINE
