require("Premake/Common")

require("Premake/ThirdParty/spdlog")

workspace("DX12SpecRegen")
	common:setConfigsAndPlatforms()
	common:addCoreDefines()

	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")

	startproject("DX12SpecRegen")

	group("Dependencies")
	project("SpdLog")
		location("ThirdParty/SpdLog/")
		warnings("Off")
		libs.spdlog:setup()
		location("ThirdParty/")

	group("Apps")
	project("DX12SpecRegen")
		location("DX12SpecRegen/")
		warnings("Extra")

		common:outDirs()
		common:debugDir()

		kind("ConsoleApp")

		common:addPCH("%{prj.location}/Src/PCH.cpp", "%{prj.location}/Src/PCH.h")

		includedirs({ "%{prj.location}/Src/" })
		files({ "%{prj.location}/Src/**" })
		removefiles({ "*.DS_Store" })

		libs.spdlog:setupDep()
		
		externalincludedirs({ "ThirdParty/LibTooling/include/" })
		libdirs({ "ThirdParty/LibTooling/libs/%{cfg.buildcfg}" })
		links({
			"Version.lib",
			"LibTooling.lib"
		})
		filter("system:windows")
			links({ "d3d12.lib", "dxgi.lib", "Dbghelp.lib" })
		filter({})
		
		-- postbuildcommands({
			-- "{COPYFILE} \"..\\ThirdParty\\LibTooling\\dll\\libclang.dll\" \"%{cfg.linktarget.directory}\""
		-- })

		common:addActions()