cmake_minimum_required(VERSION 3.11)

# set the project name
project(
	histdb
	VERSION 0.1
	LANGUAGES CXX)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

	# Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here
	if(NOT CMAKE_CXX_STANDARD)
		set(CMAKE_CXX_STANDARD 17)
	elseif(CMAKE_CXX_STANDARD LESS 17)
		message(WARNING "CMAKE_CXX_STANDARD has been set to '${CMAKE_CXX_STANDARD}' which is lower than the minimum required standard (c++17).")
	endif()

	# Let's ensure -std=c++xx instead of -std=g++xx
	set(CMAKE_CXX_EXTENSIONS OFF)

	# Let's nicely support folders in IDEs
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Testing only available if this is the main app
	# Note this needs to be done in the main CMakeLists
	# since it calls enable_testing, which must be in the
	# main CMakeLists.
	# include(CTest)

	# Docs only available if this is the main app
	find_package(SQLite3 REQUIRED)
    # target_link_libraries(SQLiteCpp PUBLIC SQLite::SQLite3)
endif()



# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)
