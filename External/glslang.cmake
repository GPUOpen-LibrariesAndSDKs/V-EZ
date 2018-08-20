option(BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
set(LIB_TYPE STATIC)
set(ENABLE_GLSLANG_INSTALL OFF)
option(ENABLE_SPVREMAPPER "Enables building of SPVRemapper" ON)
option(ENABLE_AMD_EXTENSIONS "Enables support of AMD-specific extensions" ON)
option(ENABLE_GLSLANG_BINARIES "Builds glslangValidator and spirv-remap" ON)
option(ENABLE_NV_EXTENSIONS "Enables support of Nvidia-specific extensions" ON)
option(ENABLE_OPT "Enables spirv-opt capability if present" OFF)
set(CMAKE_INSTALL_PREFIX "install" CACHE STRING "..." FORCE)

add_definitions(-DAMD_EXTENSIONS)
add_definitions(-DNV_EXTENSIONS)

if(WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
    if(MSVC)
        include(External/glslang/ChooseMSVCCRT.cmake)
    endif(MSVC)
    add_definitions(-DGLSLANG_OSINCLUDE_WIN32)
elseif(UNIX)
    add_definitions(-DGLSLANG_OSINCLUDE_UNIX)
else(WIN32)
    message("unknown platform")
endif(WIN32)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(glslang_set_link_args TARGET)
    if(WIN32 AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
        set_target_properties(${TARGET} PROPERTIES
                              LINK_FLAGS "-static -static-libgcc -static-libstdc++")
    endif()
endfunction(glslang_set_link_args)

add_subdirectory(External/glslang/External)

add_definitions(-DENABLE_OPT=0)

add_subdirectory(External/glslang/glslang)
add_subdirectory(External/glslang/OGLCompilersDLL)
add_subdirectory(External/glslang/SPIRV)
