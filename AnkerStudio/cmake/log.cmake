macro(__enable_spdlog)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/spdlog/CMakeLists.txt)
		add_subdirectory(src/spdlog)
		set(SPDLOG_FROM_SOURCE 1)
	else()
		__ak_find(Cxlog)
		set(SPDLOG_FROM_SOURCE 0)
	endif()
	
	if(TARGET cxlog)
		message(STATUS "__enable_spdlog success.")
		set(CC_GLOBAL_SPDLOG_ENABLED 1)
		if(SPDLOG_FROM_SOURCE)
			message(STATUS "from source.")
		else()
			message(STATUS "from binary.")
		endif()
	else()
		message(STATUS "__enable_spdlog failed.")
		set(CC_GLOBAL_SPDLOG_ENABLED 0)
	endif()
endmacro()