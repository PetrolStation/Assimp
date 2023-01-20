cmake_minimum_required(VERSION 3.16)

PA_NEW(Assimp)

PA_LINK_LIBRARIES( PRIVATE assimp )

PA_INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/assimp/include
    ${CMAKE_CURRENT_BINARY_DIR}/deps/assimp/include
)

File(GLOB_RECURSE SOURCE_FILES src/*.cpp )
File(GLOB_RECURSE HEADER_FILES src/*.h   )

pa_add_source_files(
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

add_subdirectory(deps)