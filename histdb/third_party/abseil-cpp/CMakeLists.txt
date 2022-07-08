message(STATUS "Looking for abseil-cpp dependency")

find_package(PkgConfig REQUIRED)

if (USE_SYSTEM_ABSEIL)
    find_package(absl 20220623 REQUIRED)
    if (absl_FOUND)
        message(STATUS "Looking for abseil-cpp dependency -- found")
        add_custom_target(absl_dependency)
    endif ()
    # TODO: fallback to using ExternalProject_Add
else ()
    message(STATUS "Adding external project: abseil-cpp")
    include(ExternalProject)
    ExternalProject_Add(absl_dependency
            URL
                https://github.com/abseil/abseil-cpp/archive/refs/tags/20220623.0.tar.gz
            URL_HASH
                SHA256=4208129b49006089ba1d6710845a45e31c59b0ab6bff9e5788a87f55c5abd602
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

    find_package(absl 20220623 QUIET CONFIG)
endif ()

# # WARN WARN WARN
# include(ExternalProject)
# ExternalProject_Add(absl_dependency
#         URL
#             https://github.com/abseil/abseil-cpp/archive/refs/tags/20220623.0.tar.gz
#         URL_HASH
#             SHA256=4208129b49006089ba1d6710845a45e31c59b0ab6bff9e5788a87f55c5abd602
#         DOWNLOAD_NO_PROGRESS
#             1
#         UPDATE_COMMAND
#             ""
#         LOG_CONFIGURE
#             1
#         LOG_BUILD
#             1
#         LOG_INSTALL
#             1
#         CMAKE_CACHE_ARGS
#             # TODO: link to the installed version of sqlite
#             -DABSL_BUILD_TESTING:BOOL=OFF
#             -DBUILD_TESTING:BOOL=OFF
#             -DABSL_PROPAGATE_CXX_STD:BOOL=ON
#             -DBUILD_SHARED_LIBS:BOOL=ON # WARN
#             -DCMAKE_CXX_STANDARD:STRING=17 # WARN
#             -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
#             -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
#             -DCMAKE_INSTALL_PREFIX:PATH=${DEPENDENCIES_INSTALL_PREFIX}/absl_dependency
#         )

#   # "-DCMAKE_INSTALL_RPATH=#{rpath}",
#   # "-DCMAKE_CXX_STANDARD=17",
#   # "-DBUILD_SHARED_LIBS=ON"


# include(GNUInstallDirs)
# list(APPEND PREFIX_PATH "${DEPENDENCIES_INSTALL_PREFIX}/absl_dependency")
# set(CMAKE_PREFIX_PATH ${PREFIX_PATH};${CMAKE_PREFIX_PATH}
#     CACHE PATH "append absl library into the search path"
#         FORCE)

# # ExternalProject_Get_Property(project_luajit install_dir)
# # add_library(luajit STATIC IMPORTED)

# # WARN WARN WARN
