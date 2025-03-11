#include "JobManager.h"

#include "Timer.h"
#include "ThreadSafeRingBuffer.h"

#include <future>
#include <algorithm>    // std::max
#include <atomic>    // to use std::atomic<uint64_t>
#include <thread>    // to use std::thread
#include <condition_variable>    // to use std::condition_variable


namespace JobSystem
{
	uint32_t numThreads = 0;    // number of worker threads, it will be initialized in the Initialize() function
	ThreadSafeRingBuffer<std::function<void()>, 256> jobPool;    // a thread safe queue to put pending jobs onto the end (with a capacity of 256 jobs). A worker thread can grab a job from the beginning
	std::condition_variable wakeCondition;    // used in conjunction with the wakeMutex below. Worker threads just sleep when there is no job, and the main thread can wake them up
	std::mutex wakeMutex;    // used in conjunction with the wakeCondition above
	uint64_t currentLabel = 0;    // tracks the state of execution of the main thread
	std::atomic<uint64_t> finishedLabel;    // track the state of execution across background worker threads

	// ...And here will go all of the functions that we will implement
}

using namespace ENGINE_NAMESPACE;
using namespace JobSystem;

inline void poll()
{
    wakeCondition.notify_one(); // wake one worker thread
    std::this_thread::yield(); // allow this thread to be rescheduled
}

void JobManager::Init()
{
    if (s_Instance) return;
	s_Instance = new JobManager();
    finishedLabel.store(0);

    // Retrieve the number of hardware threads in this system:
    auto numCores = std::thread::hardware_concurrency();

    // Calculate the actual number of worker threads we want:
    numThreads = std::max(1u, numCores);

    // Create all our worker threads while immediately starting them:
    for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
    {
        std::thread worker([] {

            std::function<void()> job; // the current job for the thread, it's empty at start.

            // This is the infinite loop that a worker thread will do 
            while (true)
            {
                if (jobPool.pop_front(job)) // try to grab a job from the jobPool queue
                {
                    // It found a job, execute it:
                    job(); // execute job
                    finishedLabel.fetch_add(1); // update worker label state
                }
                else
                {
                    // no job, put thread to sleep
                    std::unique_lock<std::mutex> lock(wakeMutex);
                    wakeCondition.wait(lock);
                }
            }

            });

        // *****Here we could do platform specific thread setup...

        worker.detach(); // forget about this thread, let it do it's job in the infinite loop that we created above
    }
}

void JobManager::Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
{
    if (jobCount == 0 || groupSize == 0)
    {
        return;
    }

    // Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
    const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

    // The main thread label state is updated:
    currentLabel += groupCount;

    for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
    {
        // For each group, generate one real job:
        auto jobGroup = [jobCount, groupSize, job, groupIndex]() {

            // Calculate the current group's offset into the jobs:
            const uint32_t groupJobOffset = groupIndex * groupSize;
            const uint32_t groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);

            JobDispatchArgs args;
            args.groupIndex = groupIndex;

            // Inside the group, loop through all job indices and execute job for each index:
            for (uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
            {
                args.jobIndex = i;
                job(args);
            }
        };

        // Try to push a new job until it is pushed successfully:
        while (!jobPool.push_back(jobGroup)) { poll(); }

        wakeCondition.notify_one(); // wake one thread
    }
}

void JobManager::Execute(const std::function<void()>& job)
{
    currentLabel += 1;

    // Try to push a new job until it is pushed successfully:
    while (!jobPool.push_back(job)) { poll(); }

    wakeCondition.notify_one(); // wake one thread
}

void JobManager::EnqueueMainThread(const std::function<void()>& func)
{
	std::scoped_lock<std::mutex> lock(s_Instance->m_MainMutex);
	s_Instance->m_MainQueue.push_back(func);
}

bool JobManager::IsBusy()
{
    // Whenever the main thread label is not reached by the workers, it indicates that some worker is still alive
    return finishedLabel.load() < currentLabel;
}

void JobManager::Wait()
{
    while (IsBusy()) { poll(); }
}

void JobManager::ExecuteMainJobs()
{
	std::scoped_lock lock(s_Instance->m_MainMutex);
    while (!s_Instance->m_MainQueue.empty()) {
        s_Instance->m_MainQueue.back()();
        s_Instance->m_MainQueue.pop_back();
    }
}
