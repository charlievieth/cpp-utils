cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
cmake_policy(VERSION 3.12)

# set the project name
project(
	histdb
	VERSION 0.1
        LANGUAGES C CXX)

# Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 17)
elseif(CMAKE_CXX_STANDARD LESS 17)
	message(WARNING "CMAKE_CXX_STANDARD has been set to '${CMAKE_CXX_STANDARD}' which is lower than the minimum required standard (c++17).")
endif()

set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

set(default_install_prefix "/Users/cvieth/Projects/C++/utils/histdb/xinstall")
if (NOT CMAKE_INSTALL_PREFIX)
    message(STATUS "Setting install prefix to '${default_install_prefix}' as none was specified.")
    set(CMAKE_INSTALL_PREFIX "${default_install_prefix}" CACHE
            STRING "Choose the install prefix." FORCE)
endif ()

# Create compile commands by default
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_SYSTEM_ABSEIL "Use the system installed version of abseil." ON)
option(USE_SYSTEM_SQLITECPP "Use the system installed version of SqliteCpp." OFF)

# Superbuild variables for sub projects
include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE ${CMAKE_BINARY_DIR}/subprojects)
set(STAGED_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/stage)
set(DEPENDENCIES_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/subprojects/Install)
set(DEPENDENCIES_SOURCE_PREFIX ${CMAKE_BINARY_DIR}/subprojects/Source)
include(GNUInstallDirs)

# Verify or install dependencies
add_subdirectory(third_party)
# add_subdirectory(${DEPENDENCIES_SOURCE_PREFIX}/sqlitecpp_dependency)

# Build the project itself
ExternalProject_Add(HistdbSource
        SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
        DEPENDS
            absl_dependency
            sqlitecpp_dependency
        CMAKE_ARGS
            -DPKG_CONFIG_EXECUTABLE:PATH=${PKG_CONFIG_EXECUTABLE}
        CMAKE_CACHE_ARGS
            -DCMAKE_PROJECT_VERSION:STRING=${CMAKE_PROJECT_VERSION}
            -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
            -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_FIND_ROOT_PATH:PATH=${CMAKE_FIND_ROOT_PATH}
            -DCMAKE_IGNORE_PATH:PATH=${CMAKE_IGNORE_PATH}
            -DCMAKE_SYSROOT:PATH=${CMAKE_SYSROOT}
            -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
            -DCMAKE_C_COMPILER_TARGET:STRING=${CMAKE_C_COMPILER_TARGET}
            -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
            -DCMAKE_CXX_COMPILER_TARGET:STRING=${CMAKE_CXX_COMPILER_TARGET}
            -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
            -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
            -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
            -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
            -DCMAKE_INSTALL_PREFIX:PATH=${STAGED_INSTALL_PREFIX}
            -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
            -DROOT_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        )

# Install the project artifacts from the staged directory
include(GNUInstallDirs)
install(DIRECTORY
            ${STAGED_INSTALL_PREFIX}/
        DESTINATION
            .
)
