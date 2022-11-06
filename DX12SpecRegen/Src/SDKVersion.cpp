#include "SDKVersion.h"

#include "Utils/Log.h"

std::vector<SDKVersion> LocateAvailableSDKVersions(const std::filesystem::path& path)
{
	std::vector<SDKVersion> versions;
	for (auto& version : std::filesystem::directory_iterator(path / "versions", std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied))
	{
		if (!version.is_directory())
			continue;

		auto& sdk = versions.emplace_back();
		sdk.name  = version.path().filename().string();

		sdk.path  = version.path().lexically_normal();
		sdk.dxgis = LocateAvailableDXGIVersions(sdk.path);

		sdk.headers.emplace_back(sdk.path / "shared/dxgitype.h");
		sdk.headers.emplace_back(sdk.path / "shared/dxgiformat.h");
		for (auto& dxgi : sdk.dxgis)
			sdk.headers.emplace_back(dxgi.path);
		sdk.headers.emplace_back(sdk.path / "um/d3dcommon.h");
		sdk.headers.emplace_back(sdk.path / "um/d3d12shader.h");
		sdk.headers.emplace_back(sdk.path / "um/d3dcompiler.h");
		sdk.headers.emplace_back(sdk.path / "um/d3d12sdklayers.h");
		sdk.headers.emplace_back(sdk.path / "um/d3d12.h");
	}

	std::ifstream versionOrderFile { "versions/sdk.versions", std::ios::ate };
	if (!versionOrderFile)
		return versions;

	std::vector<std::string> versionOrder;
	{
		std::string str(versionOrderFile.tellg(), '\0');
		versionOrderFile.seekg(0);
		versionOrderFile.read(str.data(), str.size());
		str.resize(str.find_last_not_of('\0') + 1);
		std::size_t offset = 0;
		while (offset < str.size())
		{
			std::size_t end = str.find_first_of('\n', offset);
			if (end == std::string::npos)
			{
				versionOrder.emplace_back(str.substr(offset));
				break;
			}

			versionOrder.emplace_back(str.substr(offset, end - offset));
			offset = end + 1;
		}
		versionOrderFile.close();
	}

	std::vector<SDKVersion> orderedVersions;
	orderedVersions.reserve(versions.size());
	for (auto& version : versionOrder)
	{
		bool found = false;
		for (auto itr = versions.begin(); itr != versions.end();)
		{
			if (itr->name == version)
			{
				orderedVersions.emplace_back(std::move(*itr));
				itr   = versions.erase(itr);
				found = true;
				break;
			}

			++itr;
		}
		if (!found)
			Log::Critical("BRUH WHY YOU NO HAVE SDK '{}'", version);
	}

	return orderedVersions;
}

std::vector<DXGIVersion> LocateAvailableDXGIVersions(const std::filesystem::path& path)
{
	std::vector<DXGIVersion> versions;
	for (auto& version : std::filesystem::directory_iterator(path / "shared", std::filesystem::directory_options::skip_permission_denied))
	{
		if (!version.is_regular_file())
			continue;

		auto filename = version.path().filename().string();
		if (!filename.starts_with("dxgi") || !filename.ends_with(".h"))
			continue;

		if (filename.size() < 5 || filename[4] == '.')
		{
			auto& dxgi = versions.emplace_back();
			dxgi.major = 1;
			dxgi.minor = 1;
			dxgi.path  = version.path().lexically_normal();
			continue;
		}

		std::int32_t major, minor;
		if (std::sscanf(filename.c_str(), "dxgi%d_%d.h", &major, &minor) < 2)
			continue;

		auto& dxgi = versions.emplace_back();
		dxgi.major = major;
		dxgi.minor = minor;
		dxgi.path  = version.path().lexically_normal();
	}
	std::sort(versions.begin(), versions.end(),
	          [](const DXGIVersion& lhs, const DXGIVersion& rhs) -> bool
	          {
		          if (lhs.major == rhs.major)
			          return lhs.minor < rhs.minor;
		          else
			          return lhs.major < rhs.major;
	          });
	return versions;
}