#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace JobSystem
{
	class JobSystem;
	struct Job;

	struct JobRef
	{
	public:
		JobRef() : m_JobSystem(nullptr), m_Job(nullptr) {}
		JobRef(JobSystem* jobSystem, Job* job);
		~JobRef();

		void reset();

		JobSystem* m_JobSystem;
		Job*       m_Job;
	};

	struct Job
	{
	public:
		using Func = std::function<void(JobRef currentJob)>;

	public:
		Job(Func&& func, std::size_t readyCounter) : m_Func(std::move(func)), m_ReadyCounter(readyCounter) {}

		Func                m_Func;
		std::size_t         m_ReadyCounter;
		std::vector<JobRef> m_Signals;

		bool             m_Taken = false;
		std::atomic_bool m_Done  = false;
		std::size_t      m_Refs  = 0;
	};

	class JobSystem
	{
	public:
		JobSystem(std::size_t numThreads);
		~JobSystem();

		void waitForJob(JobRef job);

		void kill();
		void killJob(JobRef job);

		JobRef createJob(Job::Func&& func, const std::vector<JobRef>& dependencies);

	private:
		void SafeThreadFunc();
		void ThreadFunc();

		Job* getNextJob();

	private:
		std::atomic_uint64_t              m_JobsLeft = 0;
		std::vector<std::unique_ptr<Job>> m_Jobs;
		std::atomic_uint64_t              m_JobAtomic;
		std::mutex                        m_JobMutex;

		std::vector<std::thread> m_Threads;
		std::atomic_bool         m_Alive;
	};
} // namespace JobSystem