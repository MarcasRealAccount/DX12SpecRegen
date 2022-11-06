#pragma once

#include <spdlog/spdlog.h>

#include <memory>

namespace Log
{
	std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name);
	std::shared_ptr<spdlog::logger> GetMainLogger();
	std::shared_ptr<spdlog::logger> GetLogger(const std::string& name);
	std::shared_ptr<spdlog::logger> GetOrCreateLogger(const std::string& name);

	template <class... Args>
	void Trace(Args&&... args)
	{
		GetMainLogger()->trace(std::forward<Args>(args)...);
	}

	template <class... Args>
	void Info(Args&&... args)
	{
		GetMainLogger()->info(std::forward<Args>(args)...);
	}

	template <class... Args>
	void Debug(Args&&... args)
	{
		GetMainLogger()->debug(std::forward<Args>(args)...);
	}

	template <class... Args>
	void Warn(Args&&... args)
	{
		GetMainLogger()->warn(std::forward<Args>(args)...);
	}

	template <class... Args>
	void Error(Args&&... args)
	{
		GetMainLogger()->error(std::forward<Args>(args)...);
	}

	template <class... Args>
	void Critical(Args&&... args)
	{
		GetMainLogger()->critical(std::forward<Args>(args)...);
	}
} // namespace Log