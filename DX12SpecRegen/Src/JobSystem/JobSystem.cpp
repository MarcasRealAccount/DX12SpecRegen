#include "JobSystem.h"
#include "Utils/Exception.h"
#include "Utils/Log.h"

namespace JobSystem
{
	JobRef::JobRef(JobSystem* jobSystem, Job* job)
	    : m_JobSystem(jobSystem), m_Job(job)
	{
		++m_Job->m_Refs;
	}

	JobRef::~JobRef()
	{
		if (m_JobSystem && m_Job && --m_Job->m_Refs == 0 && m_Job->m_Done)
			m_JobSystem->killJob(*this);
	}

	void JobRef::reset()
	{
		m_Job       = nullptr;
		m_JobSystem = nullptr;
	}

	JobSystem::JobSystem(std::size_t numThreads)
	    : m_Alive(true)
	{
		m_Threads.reserve(numThreads);
		for (std::size_t i = 0; i < numThreads; ++i)
			m_Threads.emplace_back(&JobSystem::ThreadFunc, this);
	}

	JobSystem::~JobSystem()
	{
		kill();

		for (auto& job : m_Jobs)
			for (auto& signal : job->m_Signals)
				signal.reset();
	}

	void JobSystem::waitForJob(JobRef job)
	{
		job.m_Job->m_Done.wait(false);
	}

	void JobSystem::kill()
	{
		m_Alive = false;
		m_JobAtomic.fetch_add(1);
		m_JobAtomic.notify_all();
		for (auto& thread : m_Threads)
			thread.join();
	}

	void JobSystem::killJob(JobRef job)
	{
		m_JobMutex.lock();
		auto itr = std::find_if(m_Jobs.begin(), m_Jobs.end(),
		                        [job](const std::unique_ptr<Job>& j) -> bool
		                        {
			                        return j.get() == job.m_Job;
		                        });
		m_Jobs.erase(itr);
		m_JobMutex.unlock();
	}

	JobRef JobSystem::createJob(Job::Func&& func, const std::vector<JobRef>& dependencies)
	{
		m_JobMutex.lock();
		auto job = m_Jobs.emplace_back(std::make_unique<Job>(std::move(func), dependencies.size())).get();
		m_JobMutex.unlock();
		++m_JobsLeft;
		for (auto dependency : dependencies)
		{
			if (dependency.m_Job->m_Done)
				--job->m_ReadyCounter;
			else
				dependency.m_Job->m_Signals.emplace_back(this, job);
		}
		m_JobAtomic.fetch_add(1);
		m_JobAtomic.notify_one();
		return { this, job };
	}

	void JobSystem::SafeThreadFunc()
	{
		while (m_Alive)
		{
			m_JobMutex.lock();
			if (m_Jobs.empty())
			{
				m_JobMutex.unlock();
				std::uint64_t expected = m_JobAtomic.load();
				m_JobAtomic.wait(expected);
				continue;
			}
			auto job = getNextJob();
			if (job == nullptr)
			{
				m_JobMutex.unlock();
				std::uint64_t expected = m_JobAtomic.load();
				m_JobAtomic.wait(expected);
				continue;
			}
			job->m_Taken = true;
			m_JobMutex.unlock();
			--m_JobsLeft;

			try
			{
				job->m_Func({ this, job });
			}
			catch (const Utils::Exception& exception)
			{
				Log::GetOrCreateLogger(exception.title())->critical("{}", exception);
			}
			catch (const std::exception& exception)
			{
				auto& backtrace = Utils::LastBackTrace();
				if (backtrace.frames().empty())
					Log::Critical("{}", exception.what());
				else
					Log::Critical("{}\n{}", exception.what(), backtrace);
			}
			catch (...)
			{
				auto& backtrace = Utils::LastBackTrace();
				if (backtrace.frames().empty())
					Log::Critical("Uncaught exception occurred");
				else
					Log::Critical("Uncaught exception occurred\n{}", backtrace);
			}
			job->m_Done = true;
			job->m_Done.notify_all();
			for (auto& signal : job->m_Signals)
			{
				if (--signal.m_Job->m_ReadyCounter == 0)
				{
					m_JobAtomic.fetch_add(1);
					m_JobAtomic.notify_one();
				}
			}
			if (job->m_Refs == 0)
				killJob({ this, job });
		}
	}

	void JobSystem::ThreadFunc()
	{
		//Log::Trace("Job System worker was ressurected from the shadow realm");
		try
		{
			Utils::HookThrow();
			SafeThreadFunc();
		}
		catch (const Utils::Exception& exception)
		{
			Log::GetOrCreateLogger(exception.title())->critical("{}", exception);
		}
		catch (const std::exception& exception)
		{
			auto& backtrace = Utils::LastBackTrace();
			if (backtrace.frames().empty())
				Log::Critical("{}", exception.what());
			else
				Log::Critical("{}\n{}", exception.what(), backtrace);
		}
		catch (...)
		{
			auto& backtrace = Utils::LastBackTrace();
			if (backtrace.frames().empty())
				Log::Critical("Uncaught exception occurred");
			else
				Log::Critical("Uncaught exception occurred\n{}", backtrace);
		}
		//Log::Trace("Job System worker died of unnatural causes");
	}

	Job* JobSystem::getNextJob()
	{
		for (auto& job : m_Jobs)
			if (!job->m_Done && !job->m_Taken && job->m_ReadyCounter == 0)
				return job.get();
		return nullptr;
	}
} // namespace JobSystem