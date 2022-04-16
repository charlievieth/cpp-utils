message(STATUS "Looking for abseil-cpp dependency")

find_package(PkgConfig REQUIRED)
if (USE_SYSTEM_ABSEIL)
    find_package(absl 20211102 REQUIRED)
else ()
    find_package(absl 20211102 QUIET CONFIG)
endif ()
if (absl_FOUND)
    message(STATUS "Looking for abseil-cpp dependency -- found")
    add_custom_target(absl_dependency)
else ()
    message(STATUS "Looking for abseil-cpp dependency -- not found")

    include(ExternalProject)
    ExternalProject_Add(absl_dependency
            URL
                https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.tar.gz
            URL_HASH
                SHA256=dcf71b9cba8dc0ca9940c4b316a0c796be8fab42b070bb6b7cab62b48f0e66c4
            DOWNLOAD_NO_PROGRESS
                1
            UPDATE_COMMAND
                ""
            LOG_CONFIGURE
                1
            LOG_BUILD
                1
            LOG_INSTALL
                1
            CMAKE_CACHE_ARGS
                # TODO: link to the installed version of sqlite
                -DBUILD_TESTING:BOOL=OFF
                -DABSL_PROPAGATE_CXX_STD:BOOL=ON
                -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
                -DCMAKE_INSTALL_PREFIX:PATH=${DEPENDENCIES_INSTALL_PREFIX}/absl_dependency
            )

    include(GNUInstallDirs)
    list(APPEND PREFIX_PATH "${DEPENDENCIES_INSTALL_PREFIX}/absl_dependency")
    set(CMAKE_PREFIX_PATH ${PREFIX_PATH};${CMAKE_PREFIX_PATH}
        CACHE PATH "append absl library into the search path"
            FORCE)
endif ()
