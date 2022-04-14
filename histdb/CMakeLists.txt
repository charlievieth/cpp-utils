# 3.12 is required for file globbing
cmake_minimum_required(VERSION 3.12)

# set the project name
project(
	histdb
	VERSION 0.1
	DESCRIPTION "histdb: shell history tool"
	LANGUAGES CXX)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	# Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	if(NOT CMAKE_CXX_STANDARD)
		set(CMAKE_CXX_STANDARD 20)
	elseif(CMAKE_CXX_STANDARD LESS 20)
		message(WARNING "CMAKE_CXX_STANDARD has been set to '${CMAKE_CXX_STANDARD}' which is lower than the minimum required standard (c++17).")
	endif()

	# Let's nicely support folders in IDEs
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -pedantic-errors -Wshadow")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-enum -Wcast-qual -Wpointer-arith -Wunused")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-overflow=5 -Wcast-align")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstring-compare")

	# WARM: replace this with cmakes builtin
	# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
endif()

# The compiled library code is here
# add_subdirectory(src)

# main
# add_executable(main src/main.cc)

# SQLiteCpp
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)
# target_link_libraries(main SQLiteCpp)

# abseil-cpp
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/abseil-cpp)
# target_link_libraries(main absl::strings)

add_subdirectory(src)
# target_link_libraries(main PRIVATE histdb_library)


# target_link_libraries(main
# 	sqlite3
# 	SQLiteCpp
# 	pthread
# 	dl)


# include(ExternalProject)
# ExternalProject_Add(SQLiteCpp
# 	PREFIX SQLiteCpp
# 	SOURCE_DIR "${third_party_dir}"
# 	GIT_REPOSITORY "https://github.com/SRombauts/SQLiteCpp.git"
# 	GIT_TAG "3.1.1"
#     CMAKE_CACHE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
# 					 ${ARGN})
# add_dependencies(histdb, SQLiteCpp)

# set(ABSL_PROPAGATE_CXX_STD ON)
# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/abseil-cpp)

# # Let's ensure -std=c++xx instead of -std=g++xx
# # set(CMAKE_CXX_EXTENSIONS OFF)

# # Let's nicely support folders in IDEs
# # set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# # Testing only available if this is the main app
# # Note this needs to be done in the main CMakeLists
# # since it calls enable_testing, which must be in the
# # main CMakeLists.
# # include(CTest)

# find_package(SQLite3 REQUIRED)

# # TODO: use pkg-config
# # if(EXISTS "/usr/local/opt/sqlite/lib/pkgconfig")
# # 	message("ok")
# # 	set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/sqlite/lib/pkgconfig")
# # 	find_package(PkgConfig REQUIRED)
# # 	pkg_check_modules(SQLite3 REQUIRED)
# # else()
# # endif()

# # target_link_libraries(SQLiteCpp PUBLIC SQLite::SQLite3)

# # This is a "default" library, and will match the *** variable setting.
# # Other common choices are STATIC, SHARED, and MODULE
# # Including header files here helps IDEs but is not required.
# # Output libname matches target name, with the usual extensions on your system
# add_library(SQLiteCpp sqlite3_db.cc sqlite3_db.hh)

# # Link each target with other targets or add options, etc.

# # Adding something we can run - Output name matches target name
# add_executable(histdb main.cc)

# # Make sure you link your targets with this command. It can also link libraries and
# # even flags, so linking a target that does not exist will not give a configure-time error.
# target_link_libraries(histdb PRIVATE SQLiteCpp)

# # # The compiled library code is here
# # add_subdirectory(src)

# # # The executable code is here
# # add_subdirectory(apps)

# Experimental
#
# GCC only
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-attribute=pure")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-attribute=const")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-attribute=noreturn")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-attribute=format")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-attribute=cold")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-attribute=malloc")

# Address Sanitizer
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
