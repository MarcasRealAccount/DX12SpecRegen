#include "SDKVersion.h"

std::vector<SDKVersion> LocateAvailableSDKVersions(const std::filesystem::path& path)
{
	std::vector<SDKVersion> versions;
	for (auto& version : std::filesystem::directory_iterator(path / "versions", std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied))
	{
		if (!version.is_directory())
			continue;

		auto         dirname = version.path().filename().string();
		std::int32_t major, variant, minor, patch;
		if (std::sscanf(dirname.c_str(), "%d.%d.%d.%d", &major, &variant, &minor, &patch) < 4)
			continue;

		auto& sdk   = versions.emplace_back();
		sdk.major   = major;
		sdk.variant = variant;
		sdk.minor   = minor;
		sdk.patch   = patch;

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
	return versions;
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