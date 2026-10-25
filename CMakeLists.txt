cmake_minimum_required(VERSION 3.20)

# Standalone configuration when built directly
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(EterLib LANGUAGES C CXX VERSION 1.0.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    if(WIN32)
        add_compile_definitions(WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS)
    else()
        add_compile_definitions(__LINUX__ _GNU_SOURCE)
        add_compile_options(
            -Wall
            -Wextra
            -Wno-unused-parameter
            -Wno-invalid-offsetof
            -Wno-multichar
            -Wno-sign-compare
            -fPIC
        )
    endif()

    option(BUILD_TESTING "Build EterLib unit tests" ON)
    if(BUILD_TESTING)
        enable_testing()
    endif()

    # Resolve sibling dependencies when built standalone inside the workspace or with deps/
    set(_ETERLIB_DEPS EterBase EterLocale EterPack EterImageLib SphereLib)
    foreach(_dep IN LISTS _ETERLIB_DEPS)
        if(NOT TARGET ${_dep})
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../${_dep}/CMakeLists.txt")
                add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../${_dep}" "${CMAKE_BINARY_DIR}/deps/${_dep}" EXCLUDE_FROM_ALL)
            elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/${_dep}/CMakeLists.txt")
                add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/${_dep}" "${CMAKE_BINARY_DIR}/deps/${_dep}" EXCLUDE_FROM_ALL)
            endif()
        endif()
    endforeach()
endif()

# Find dependencies: Threads, libjpeg, SDL2, and OpenGL
find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)
if(UNIX)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
endif()

file(GLOB ETERLIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB ETERLIB_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/EterLib/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Platform/*.h"
)

add_library(EterLib STATIC
    ${ETERLIB_SOURCES}
    ${ETERLIB_HEADERS}
)
add_library(EterLib::EterLib ALIAS EterLib)

target_include_directories(EterLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/EterLib>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(EterLib
    PUBLIC
        EterBase
        EterLocale
        EterPack
        EterImageLib
        SphereLib
        JPEG::JPEG
        Threads::Threads
        $<$<PLATFORM_ID:Linux>:SDL2::SDL2>
        $<$<PLATFORM_ID:Linux>:OpenGL::GL>
)

# Unit Tests
if((CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND BUILD_TESTING) OR BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()
