#pragma once

#include <cstdint>

#include <filesystem>
#include <vector>

struct DXGIVersion
{
public:
	std::uint32_t major, minor;

	std::filesystem::path path;
};

struct SDKVersion
{
public:
	std::uint32_t major, variant, minor, patch;

	std::filesystem::path    path;
	std::vector<DXGIVersion> dxgis;

	std::vector<std::filesystem::path> headers;
};

std::vector<SDKVersion>  LocateAvailableSDKVersions(const std::filesystem::path& path);
std::vector<DXGIVersion> LocateAvailableDXGIVersions(const std::filesystem::path& path);