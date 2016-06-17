cmake_minimum_required(VERSION 3.0.0)

cmake_policy(SET CMP0042 OLD)	# Fix MACOSX_RPATH.
cmake_policy(SET CMP0048 NEW)	# Allow VERSION argument in project().
if (POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)	# No longer implicitly dereference variables.
endif()

project(EVMJIT VERSION 0.9.0.2 LANGUAGES CXX)

message(STATUS "EVM JIT ${EVMJIT_VERSION_MAJOR}.${EVMJIT_VERSION_MINOR}.${EVMJIT_VERSION_PATCH}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_AUTOMOC OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
else()
	set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wconversion -Wno-sign-conversion -Wno-unknown-pragmas ${CMAKE_CXX_FLAGS}")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT SANITIZE)
	# Do not allow unresolved symbols in shared library (default on linux)
	# unless sanitizer is used (sanity checks produce unresolved function calls)
	set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
endif()

include(cmake/llvm.cmake)
configure_llvm_project(llvm)

add_subdirectory(libevmjit)
