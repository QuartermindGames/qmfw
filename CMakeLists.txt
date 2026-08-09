#[[
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]

cmake_minimum_required(VERSION 3.16)

project(PlatformLibrary)

option(PL_BUILD_PLUGINS "Build plugins" ON)
option(PL_BUILD_EXAMPLES "Build example projects" ON)

# Set all of our output directories.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/lib/>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/>)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3")
endif ()

enable_testing()

add_subdirectory(plcore)
add_subdirectory(plgraphics)
add_subdirectory(plmodel)
add_subdirectory(plwindow)

if (CMAKE_TESTING_ENABLED)
    add_subdirectory(tests)
endif ()

if (PL_BUILD_PLUGINS)
    add_subdirectory(plugins/driver_opengl/)
    if (WIN32)
        if (EXISTS "${CMAKE_SOURCE_DIR}/plugins/driver_glide/CMakeLists.txt")
            add_subdirectory(plugins/driver_glide/)
        endif ()
    endif ()
endif (PL_BUILD_PLUGINS)

if (PL_BUILD_EXAMPLES)
    add_subdirectory(examples/cpj_dumper)
endif ()
