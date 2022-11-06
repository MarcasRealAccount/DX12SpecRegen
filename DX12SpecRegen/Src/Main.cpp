#include "HeaderIntrospectionStage.h"
#include "JobSystem/JobSystem.h"
#include "SDKInfo.h"
#include "SDKVersion.h"
#include "Serialization/XMLSerialization.h"
#include "Utils/Exception.h"
#include "Utils/Log.h"

#include <fstream>
#include <thread>

// TODO(MarcasRealAccount): Handle anonymous unions in structs

struct CompileHeaderJob
{
public:
	CompileHeaderJob(const std::vector<std::string>& args, std::filesystem::path path, SDKInfo::Header* headerInfo)
	    : m_Args(args), m_Path(path), m_HeaderInfo(headerInfo) {}

	void operator()(JobSystem::JobRef currentJob)
	{
		Log::Info("Regenerating header '{}'", m_Path.string());
		ClangIntrospection::Introspect(m_Args, m_Path, m_HeaderInfo);
		Log::Info("Regenerated header '{}'", m_Path.string());
	}

private:
	std::vector<std::string> m_Args;
	std::filesystem::path    m_Path;
	SDKInfo::Header*         m_HeaderInfo;
};

struct RemoverJob
{
public:
	RemoverJob(SDKInfo::SDK* newer, SDKInfo::SDK* older)
	    : m_Newer(newer), m_Older(older) {}

	void operator()(JobSystem::JobRef currentJob)
	{
		Log::Info("Removing old stuff from '{}'", m_Newer->version);
		SDKInfo::RemovePreviousSDK(*m_Newer, *m_Older);
		Log::Info("Removed old stuff from '{}'", m_Newer->version);
	}

private:
	SDKInfo::SDK* m_Newer;
	SDKInfo::SDK* m_Older;
};

struct PostProcessJob
{
public:
	PostProcessJob(std::vector<SDKInfo::SDK>* sdk)
	    : m_SDK(sdk) {}

	void operator()(JobSystem::JobRef currentJob)
	{
		Log::Info("Preprocessing");

		Log::Info("Preprocessed");
	}

private:
	std::vector<SDKInfo::SDK>* m_SDK;
};

int safeMain([[maybe_unused]] int argc, [[maybe_unused]] const char** argv)
{
	std::size_t threadCount = 16;

	std::vector<SDKVersion>   sdkVersions;
	std::vector<SDKInfo::SDK> sdkInfos;
	JobSystem::JobSystem      jobSystem { threadCount };
	JobSystem::JobRef         preProcessJob;
	{
		auto firstJob = jobSystem.createJob(
		    [&](JobSystem::JobRef currentJob)
		    {
			    std::vector<JobSystem::JobRef> allJobs;
			    sdkVersions = LocateAvailableSDKVersions("./");
			    Log::Info("Found {} sdks", sdkVersions.size());
			    for (auto& sdk : sdkVersions)
				    Log::Info("{}.{}.{}.{} at '{}' with {} dxgis", sdk.major, sdk.variant, sdk.minor, sdk.patch, sdk.path.string(), sdk.dxgis.size());

			    sdkInfos.resize(sdkVersions.size(), {});
			    std::vector<JobSystem::JobRef> refs;
			    for (std::size_t i = sdkVersions.size(); i > 0; --i)
			    {
				    auto& sdkVersion = sdkVersions[i - 1];
				    auto& sdkInfo    = sdkInfos[i - 1];
				    sdkInfo.version  = fmt::format("{}.{}.{}.{}", sdkVersion.major, sdkVersion.variant, sdkVersion.minor, sdkVersion.patch);

				    std::vector<std::string> args;
				    args.emplace_back("-isystem");
				    args.emplace_back(sdkVersion.path.string());
				    args.emplace_back("-isystem");
				    args.emplace_back((sdkVersion.path / "shared").string());
				    args.emplace_back("-isystem");
				    args.emplace_back((sdkVersion.path / "ucrt").string());
				    args.emplace_back("-isystem");
				    args.emplace_back((sdkVersion.path / "um").string());
				    args.emplace_back("-I");
				    args.emplace_back(sdkVersion.path.string());
				    args.emplace_back("-I");
				    args.emplace_back((sdkVersion.path / "shared").string());
				    args.emplace_back("-I");
				    args.emplace_back((sdkVersion.path / "ucrt").string());
				    args.emplace_back("-I");
				    args.emplace_back((sdkVersion.path / "um").string());
				    args.emplace_back("-x");
				    args.emplace_back("c++");
				    args.emplace_back("-std=c++20");

				    sdkInfo.headers.reserve(sdkVersion.headers.size());
				    for (auto& header : sdkVersion.headers)
				    {
					    auto& sdkHeader = sdkInfo.headers.emplace_back();
					    sdkHeader.name  = std::filesystem::relative(header, sdkVersion.path).string();
					    allJobs.emplace_back(refs.emplace_back(currentJob.m_JobSystem->createJob(CompileHeaderJob { args, header, &sdkHeader }, { currentJob })));
				    }

				    if (i < sdkVersions.size())
				    {
					    auto removerJob = currentJob.m_JobSystem->createJob(RemoverJob { &sdkInfos[i], &sdkInfo }, refs);
					    refs.erase(refs.begin(), refs.begin() + sdkInfo.headers.size());
					    allJobs.emplace_back(removerJob);
				    }
			    }

			    preProcessJob = currentJob.m_JobSystem->createJob(PostProcessJob { &sdkInfos }, allJobs);

			    Log::Info("Readied all compile jobs");
		    },
		    {});

		jobSystem.waitForJob(firstJob);
	}

	{
		auto lastJob = jobSystem.createJob(
		    [&sdkInfos](JobSystem::JobRef currentJob)
		    {
			    std::ofstream output { "spec.xml" };
			    if (!output)
			    {
				    Log::Critical("Failed to open 'spec.xml'");
				    return;
			    }

			    Serialization::XMLDocument document;
			    document.root.tag = "root";
			    Serialization::XMLSerialiaze(sdkInfos, document.root);
			    std::string str = Serialization::XMLToString(document);
			    output.write(str.c_str(), str.size());
			    output.close();
			    Log::Info("Serialized output!");
		    },
		    { preProcessJob });

		jobSystem.waitForJob(lastJob);
	}

	return 0;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv)
{
	try
	{
		Utils::HookThrow();
		return safeMain(argc, argv);
	}
	catch (const Utils::Exception& exception)
	{
		Log::GetOrCreateLogger(exception.title())->critical("{}", exception);
		return 0x7FFF'FFFF;
	}
	catch (const std::exception& exception)
	{
		auto& backtrace = Utils::LastBackTrace();
		if (backtrace.frames().empty())
			Log::Critical("{}", exception.what());
		else
			Log::Critical("{}\n{}", exception.what(), backtrace);
		return 0x0000'7FFF;
	}
	catch (...)
	{
		auto& backtrace = Utils::LastBackTrace();
		if (backtrace.frames().empty())
			Log::Critical("Uncaught exception occurred");
		else
			Log::Critical("Uncaught exception occurred\n{}", backtrace);
		return 0x0000'7FFF;
	}
}