set(Catch2_VERSION "v3.5.3") # git tag


Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        "${Catch2_VERSION}"
)

FetchContent_MakeAvailable(Catch2)

add_library(cmake_dep INTERFACE)
add_library(external::catch ALIAS cmake_dep)

target_link_libraries(cmake_dep INTERFACE Catch2::Catch2WithMain)

