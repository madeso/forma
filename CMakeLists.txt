cmake_minimum_required(VERSION 3.14...3.22)

project(forma VERSION 1.0 LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(forma_project_options INTERFACE)
target_compile_features(forma_project_options INTERFACE cxx_std_20)

if(GENERATE_COVERAGE)
	# inspired by https://github.com/JoakimSoderberg/coveralls-cmake
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
		# supported
	else()
		message(FATAL_ERROR "Compiler ${CMAKE_C_COMPILER_ID} is not supported!")
	endif()

	if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		message(FATAL_ERROR "Code coverage results with an optimised (non-Debug) build may be misleading!")
	endif()

	message(STATUS "Coverage enabled")

	# based on
	# https://github.com/cpp-best-practices/cpp_starter_project/blob/main/cmake/Sanitizers.cmake#L6

	# --coverage is a synonym for -fprofile-arcs -ftest-coverage (when compiling) and -lgcov (when linking).
	# https://gcc.gnu.org/onlinedocs/gcc-6.2.0/gcc/Instrumentation-Options.html
	target_compile_options(forma_project_options INTERFACE -fprofile-arcs -ftest-coverage -O0 -g)
	target_link_libraries(forma_project_options INTERFACE -lgcov)
endif()

add_subdirectory(external)


# sources
set(forma_src
	src/forma/core.cc src/forma/core.hh
	src/forma/template.cc src/forma/template.hh
	src/forma/scanner.cc src/forma/scanner.hh
	src/forma/parser.cc src/forma/parser.hh
)
set(test_src
	src/forma/template.test.cc
)
source_group("" FILES ${forma_src} ${test_src})


# forma library
add_library(forma STATIC ${forma_src})
add_library(g::forma ALIAS forma)
target_include_directories(forma PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(forma PRIVATE forma_project_options)


# unit test
add_executable(test ${test_src})
target_link_libraries(test PRIVATE
	g::forma
	external::catch
	forma_project_options
)
