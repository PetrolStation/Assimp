#assimp
set( BUILD_SHARED_LIBS         ON )
set( ASSIMP_BUILD_ZLIB         ON  CACHE BOOL "xxx" FORCE )
set( ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "xxx" FORCE )
set( ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "xxx" FORCE )
set( ASSIMP_BUILD_TESTS        OFF CACHE BOOL "xxx" FORCE )

add_subdirectory(assimp)
