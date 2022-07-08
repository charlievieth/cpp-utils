message(STATUS "Looking for CLI11 dependency")

find_package(PkgConfig REQUIRED)
if (USE_SYSTEM_CLI11)
    find_package(CLI11 2.2.0 REQUIRED)
else ()
    include(ExternalProject)
    ExternalProject_Add(cli11_dependency
            URL
                https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.2.0.tar.gz
            URL_HASH
                SHA256=d60440dc4d43255f872d174e416705f56ba40589f6eb07727f76376fb8378fd6
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
                -DCLI11_TESTING:BOOL=OFF
                -DCLI11_SINGLE_FILE:BOOL=OFF
                -DCLI11_EXAMPLES:BOOL=OFF
                -DCLI11_BUILD_EXAMPLES_JSON:BOOL=OFF
                -DCLI11_INCLUDE_DIR:PATH=${CLI11_INCLUDE_DIR}
                -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
                -DCMAKE_INSTALL_PREFIX:PATH=${DEPENDENCIES_INSTALL_PREFIX}/cli11_dependency
            )

    include(GNUInstallDirs)
    list(APPEND PREFIX_PATH "${DEPENDENCIES_INSTALL_PREFIX}/cli11_dependency")
    set(CMAKE_PREFIX_PATH ${PREFIX_PATH};${CMAKE_PREFIX_PATH}
        CACHE PATH "append CLI11 library into the search path"
            FORCE)

    list(APPEND INCLUDE_PATH "${DEPENDENCIES_INSTALL_PREFIX}/cli11_dependency")
    set(CMAKE_INCLUDE_PATH ${INCLUDE_PATH};${CMAKE_INCLUDE_PATH}
        CACHE PATH "append CLI11 library into the include search path"
            FORCE)

    # TODO: use this pattern for other deps?
    ExternalProject_Get_Property(cli11_dependency INSTALL_DIR)
    # set(CLI11_INCLUDE_DIR "${INSTALL_DIR}/include")
    set(CLI11_INCLUDE_DIR ${INSTALL_DIR} CACHE INTERNAL "")
    message(WARNING "Setting CLI11 include directory to: ${CLI11_INCLUDE_DIR}")
    include_directories(${INSTALL_DIR})

    find_package(CLI11 2.2.0 QUIET CONFIG)
endif ()

if (CLI11_FOUND)
    message(STATUS "Looking for CLI11 dependency -- found")
    # add_custom_target(cli11_dependency)
else ()
    message(STATUS "Looking for CLI11 dependency -- not found")
endif ()

# find_package(PkgConfig REQUIRED)
# if (USE_SYSTEM_CLI11)
#     find_package(CLI11 2.2.0 REQUIRED)
# else ()
#     find_package(CLI11 2.2.0 QUIET CONFIG)
# endif ()
# if (CLI11_FOUND)
#     message(STATUS "Looking for CLI11 dependency -- found")
#     add_custom_target(cli11_dependency)
# else ()
#     message(STATUS "Looking for CLI11 dependency -- not found")

#     include(ExternalProject)
#     ExternalProject_Add(cli11_dependency
#             URL
#                 https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.2.0.tar.gz
#             URL_HASH
#                 SHA256=d60440dc4d43255f872d174e416705f56ba40589f6eb07727f76376fb8378fd6
#             DOWNLOAD_NO_PROGRESS
#                 1
#             UPDATE_COMMAND
#                 ""
#             LOG_CONFIGURE
#                 1
#             LOG_BUILD
#                 1
#             LOG_INSTALL
#                 1
#             CMAKE_CACHE_ARGS
#                 -DCLI11_TESTING:BOOL=OFF
#                 -DCLI11_SINGLE_FILE:BOOL=OFF
#                 -DCLI11_EXAMPLES:BOOL=OFF
#                 -DCLI11_BUILD_EXAMPLES_JSON:BOOL=OFF
#                 -DCLI11_INCLUDE_DIR:PATH=${CLI11_INCLUDE_DIR}
#                 -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
#                 -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
#                 -DCMAKE_INSTALL_PREFIX:PATH=${DEPENDENCIES_INSTALL_PREFIX}/cli11_dependency
#             )

#     include(GNUInstallDirs)
#     list(APPEND PREFIX_PATH "${DEPENDENCIES_INSTALL_PREFIX}/cli11_dependency")
#     set(CMAKE_PREFIX_PATH ${PREFIX_PATH};${CMAKE_PREFIX_PATH}
#         CACHE PATH "append CLI11 library into the search path"
#             FORCE)

#     list(APPEND INCLUDE_PATH "${DEPENDENCIES_INSTALL_PREFIX}/cli11_dependency")
#     set(CMAKE_INCLUDE_PATH ${INCLUDE_PATH};${CMAKE_INCLUDE_PATH}
#         CACHE PATH "append CLI11 library into the include search path"
#             FORCE)

#     # TODO: use this pattern for other deps?
#     ExternalProject_Get_Property(cli11_dependency INSTALL_DIR)
#     # set(CLI11_INCLUDE_DIR "${INSTALL_DIR}/include")
#     set(CLI11_INCLUDE_DIR ${INSTALL_DIR} CACHE INTERNAL "")
#     message(WARNING "Setting CLI11 include directory to: ${CLI11_INCLUDE_DIR}")
#     include_directories(${INSTALL_DIR})
# endif ()
