cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
cmake_policy(VERSION 3.12)

# set the project name
project(
	HistdbSource
	VERSION ${CMAKE_PROJECT_VERSION}
	LANGUAGES CXX)

# Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
elseif(CMAKE_CXX_STANDARD LESS 17)
	message(WARNING "CMAKE_CXX_STANDARD has been set to '${CMAKE_CXX_STANDARD}' which is lower than the minimum required standard (c++17).")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(SQLITECPP_ENABLE_ASSERT_HANDLER)

# Warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -pedantic-errors -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-enum -Wcast-qual -Wpointer-arith -Wunused")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-overflow=5 -Wcast-align")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstring-compare")
# TODO: is the correct place for this??
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSQLITECPP_ENABLE_ASSERT_HANDLER")

# Compile options
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

find_package(absl REQUIRED)
find_package(CLI11 REQUIRED)
find_package(SQLiteCpp REQUIRED)

# WARN: this does not seem to work
# include(CheckIncludeFile)
# check_include_file(getopt.h HAVE_GETOPT_H)
# check_include_file(sqlite3.h HAVE_SQLITE3_H)
# check_include_file(sys/sysctl.h HAVE_SYS_CTL_H)
# check_include_file(sys/time.h HAVE_SYS_TIME_H)
# include(CheckSymbolExists)
# set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
# set(CMAKE_REQUIRED_FLAGS -D_GNU_SOURCE)
# check_symbol_exists(sqlite3_extended_result_codes "sqlite3.h" HAVE_SQLITE3_EXTENDED_RESULT_CODES)

include(GNUInstallDirs) # The directory names are used in the config file
# configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(histdb)
