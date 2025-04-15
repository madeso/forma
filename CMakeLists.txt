cmake_minimum_required(VERSION 3.14...3.22)

project(
  forma
  VERSION 1.0
  LANGUAGES CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

add_subdirectory(external)

set(em_src
    src/forma/core.cc src/forma/core.hh
    src/forma/template.cc src/forma/template.hh
    src/forma/scanner.cc src/forma/scanner.hh
    src/forma/parser.cc src/forma/parser.hh
)
set(test_src
    src/forma/template.test.cc
)
source_group("" FILES ${em_src} ${main_src} ${test_src})

add_library(forma STATIC ${em_src})
target_compile_features(forma PRIVATE cxx_std_20)
add_library(g::forma ALIAS forma)
add_executable(test ${test_src})
target_compile_features(test PUBLIC cxx_std_20)

target_include_directories(forma PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test PRIVATE
    g::forma
    external::catch
)


set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)