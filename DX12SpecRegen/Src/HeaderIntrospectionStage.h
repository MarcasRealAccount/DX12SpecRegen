#pragma once

#include "SDKInfo.h"

#include <filesystem>

namespace ClangIntrospection
{
	bool Introspect(const std::vector<std::string>& args, std::filesystem::path path, SDKInfo::Header* headerInfo);
}