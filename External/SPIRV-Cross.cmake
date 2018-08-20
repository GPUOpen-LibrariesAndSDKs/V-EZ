option(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS "Instead of throwing exceptions assert" OFF)
set(spirv-compiler-options "")
set(spirv-compiler-defines "")

if(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
  set(spirv-compiler-defines ${spirv-compiler-defines} SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
endif()

if (NOT "${MSVC}")
  set(spirv-compiler-options ${spirv-compiler-options} -std=c++11 -Wall -Wextra -Werror -Wshadow)
  set(spirv-compiler-defines ${spirv-compiler-defines} __STDC_LIMIT_MACROS)

  if(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
    set(spirv-compiler-options ${spirv-compiler-options} -fno-exceptions)
  endif()
endif()

macro(extract_headers out_abs file_list)
  set(${out_abs}) # absolute paths
  foreach(_a ${file_list})
    # get_filename_component only returns the longest extension, so use a regex
    string(REGEX REPLACE ".*\\.(h|hpp)" "\\1" ext ${_a})
    if(("${ext}" STREQUAL "h") OR ("${ext}" STREQUAL "hpp"))
      list(APPEND ${out_abs} "${_a}")
    endif()
  endforeach()
endmacro()

macro(spirv_cross_add_library name config_name)
  add_library(${name} ${ARGN})
  extract_headers(hdrs "${ARGN}")
  target_include_directories(${name} PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include/spirv_cross>)
  set_target_properties(${name} PROPERTIES
      PUBLIC_HEADERS "${hdrs}")
  target_compile_options(${name} PRIVATE ${spirv-compiler-options})
  target_compile_definitions(${name} PRIVATE ${spirv-compiler-defines})
  install(TARGETS ${name}
      EXPORT ${config_name}Config
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      PUBLIC_HEADER DESTINATION include/spirv_cross)
  install(FILES ${hdrs} DESTINATION include/spirv_cross)
  install(EXPORT ${config_name}Config DESTINATION share/${config_name}/cmake)
  export(TARGETS ${targets} FILE ${config_name}Config.cmake)
endmacro()


spirv_cross_add_library(spirv-cross-core spirv_cross_core STATIC
    External/SPIRV-Cross/GLSL.std.450.h
    External/SPIRV-Cross/spirv_common.hpp
    External/SPIRV-Cross/spirv.hpp
    External/SPIRV-Cross/spirv_cross.hpp
    External/SPIRV-Cross/spirv_cross.cpp
    External/SPIRV-Cross/spirv_cfg.hpp
    External/SPIRV-Cross/spirv_cfg.cpp)

spirv_cross_add_library(spirv-cross-glsl spirv_cross_glsl STATIC
    External/SPIRV-Cross/spirv_glsl.cpp
    External/SPIRV-Cross/spirv_glsl.hpp)
