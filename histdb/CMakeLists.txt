cmake_minimum_required(VERSION 3.11)

# set the project name
project(
	histdb
	VERSION 0.1
	LANGUAGES CXX)

# Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here
if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
elseif(CMAKE_CXX_STANDARD LESS 17)
	message(WARNING "CMAKE_CXX_STANDARD has been set to '${CMAKE_CXX_STANDARD}' which is lower than the minimum required standard (c++17).")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -pedantic-errors -Wshadow")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch-enum -Wcast-qual -Wpointer-arith -Wunused")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-overflow=5 -Wcast-align")

# Experimental
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstring-compare")
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

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)

# set(ABSL_PROPAGATE_CXX_STD ON)
# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/abseil-cpp)

add_executable(main src/main.cc)
target_link_libraries(main
	SQLiteCpp
	sqlite3
	pthread
	dl)

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
