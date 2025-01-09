include(ExternalProject)

set(Catch2_VERSION "v3.5.3") # git tag

###################################################################################################
## catch2
set(catch_install_dir ${CMAKE_CURRENT_BINARY_DIR}/catch2/install)
ExternalProject_Add(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG "${Catch2_VERSION}"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    UPDATE_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_INSTALL_PREFIX=${catch_install_dir}
        -DBUILD_TESTING=OFF
        -DCATCH_INSTALL_DOCS=OFF
        -DCATCH_INSTALL_EXTRAS=OFF
)

ExternalProject_Get_Property(Catch2 INSTALL_DIR)

add_library(cmake_dep INTERFACE)
add_library(external::catch ALIAS cmake_dep)

target_include_directories(cmake_dep INTERFACE
    ${catch_install_dir}/include
)
target_link_libraries(cmake_dep INTERFACE
    ${catch_install_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}Catch2d${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${catch_install_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}Catch2Maind${CMAKE_STATIC_LIBRARY_SUFFIX}
)

