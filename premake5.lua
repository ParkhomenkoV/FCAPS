#!lua

-- A solution contains projects, and defines the available configurations
solution "Sofia-PS"
	location "build"
	language "C++"
	startproject "Sofia-PS"

	configurations { "release", "debug" }

	function DefaultConfig(complimentName)
		configuration "Debug"
			defines { "DEBUG", "_DEBUG" }
			symbols "On"
			characterset "MBCS"
			targetdir ("build/debug/" .. complimentName)

		configuration "Release"
			defines { "NDEBUG", "BOOST_DISABLE_ASSERTS" }
			optimize "On"
			characterset "MBCS"
			targetdir ("build/release/" .. complimentName)


		configuration "*"
	end

	project "SharedTools"
		DefaultConfig("lib")
		kind "StaticLib"
		pic "On"
		includedirs { 
			"Tools/inc/",
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
		}
		files{ "Tools/inc/**.h", "Tools/src/**.cpp" }

		configuration "Debug"
			targetname( "SharedTools" )

		configuration "Release"
			targetname( "SharedTools" )

	project "Storages"
		DefaultConfig("lib")
		kind "StaticLib"
		pic "On"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
		}
		files{ "FCAPS/src/fcaps/storages/**.h", "FCAPS/src/fcaps/storages/**.cpp" }

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools"
			}

		configuration "Release"
			links{ 
				"SharedTools"
			}

	project "SharedModulesLib"
		DefaultConfig("lib")
		kind "StaticLib"
		pic "On"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
		}
		files{ "FCAPS/src/fcaps/SharedModulesLib/**.h", "FCAPS/src/fcaps/SharedModulesLib/**.cpp" }

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools"
			}

		configuration "Release"
			links{ 
				"SharedTools"
			}

	project "StdFCAModule"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/src/", 
			"FCAPS/include/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/StdFCA/**.h", "FCAPS/src/fcaps/StdFCA/**.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/StdFCA/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools",
				"Storages",
				"SharedModulesLib"
			}

		configuration "Release"
			links{ 
				"SharedTools",
				"Storages",
				"SharedModulesLib"
			}

	project "PS-Modules"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/PS-Modules/**.h", "FCAPS/src/fcaps/PS-Modules/**.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/PS-Modules/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools",
				"SharedModulesLib"
			}

		configuration "Release"
			links{ 
				"SharedTools",
				"SharedModulesLib"
			}

	project "SofiaModules"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/SofiaModules/**.h", "FCAPS/src/fcaps/SofiaModules/**.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/SofiaModules/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools",
				"SharedModulesLib"
			}

		configuration "Release"
			links{ 
				"SharedTools",
				"SharedModulesLib"
			}

	project "ParallelPatternEnumeratorModules"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
			"Sofia-PS/inc/",
			"../LibgSpanForSofia/inc",
			"../LibGastonForSofia/inc"
		}
		files{ "FCAPS/src/fcaps/ParallelPatternEnumeratorModules/*.h", "FCAPS/src/fcaps/ParallelPatternEnumeratorModules/*.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/ParallelPatternEnumeratorModules/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/"
		}

		links{ 
			"SharedTools",
			"SharedModulesLib"
		}
		filter{ "system:not windows" }
			links{
				"boost_thread",
				"pthread",
			}
		filter{}

	project "ClassifierModules"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/ClassifierModules/**.h", "FCAPS/src/fcaps/ClassifierModules/**.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/ClassifierModules/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools",
				"SharedModulesLib",
				"Storages"
			}

		configuration "Release"
			links{ 
				"SharedTools",
				"SharedModulesLib",
				"Storages"
			}

	project "StabilityEstimatorContextProcessorModules"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/StabilityEstimatorContextProcessor/**.h", "FCAPS/src/fcaps/StabilityEstimatorContextProcessor/**.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/StabilityEstimatorContextProcessor/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools",
				"SharedModulesLib",
				"Storages"
			}

		configuration "Release"
			links{ 
				"SharedTools",
				"SharedModulesLib",
				"Storages"
			}

	project "FilterModules"
		DefaultConfig("modules")
		kind "SharedLib"
		language "C++"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"FCAPS/src/", 
			"Tools/inc/", 
			"Sofia-PS/inc/"
		}
		files{ "FCAPS/src/fcaps/Filters/**.h", "FCAPS/src/fcaps/Filters/**.cpp" }
		filter{ "system:windows" }
			files{ "FCAPS/src/fcaps/Filters/**.def" }
		filter{}

		libdirs {
			"boost/stage/lib/",
		}

		configuration "Debug"
			links{ 
				"SharedTools",
				"SharedModulesLib"
			}

		configuration "Release"
			links{ 
				"SharedTools",
				"SharedModulesLib"
			}

	project "Sofia-PS"
		DefaultConfig("")
		kind "ConsoleApp"
		includedirs { 
			"boost/", -- There is no search for the include dirs (in particular on windows it is prety difficult
			"rapidjson/include",
			"FCAPS/include/", 
			"Tools/inc/", 
			"Sofia-PS/inc/",
		}
		files { "Sofia-PS/**.cpp" } 

		libdirs {
			"boost/stage/lib/",
		}

		rapidjsonDir="rapidjson/"
		if ( not os.isdir(rapidjsonDir)  ) then
			os.execute( "git clone https://github.com/miloyip/rapidjson.git " .. rapidjsonDir )
			os.execute( "git --git-dir " .. rapidjsonDir .. ".git --work-tree " .. rapidjsonDir .. " reset --hard c745c953adf68" )
			os.execute( "git --git-dir " .. rapidjsonDir .. ".git --work-tree " .. rapidjsonDir .. " apply --whitespace=fix rapidjson.patch" )
		end

		links{ 
			"SharedTools"
		}
		filter{ "system:not windows" }
			links{ 
				"dl",
				"boost_filesystem",
				"boost_system"
			}
		filter{}
		configuration "Debug"
			targetname( "Sofia-PS" )

		configuration "Release"
			targetname( "Sofia-PS" )

