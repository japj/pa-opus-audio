cmake_minimum_required(VERSION 3.17)

# Integrate with vcpkg installed ports
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_CXX_STANDARD 17)

# on windows we need to use static (crt) linking
if (WIN32)
  set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
endif()

# workaround for cmake-js not setting MSVC multi-threaded statically-linked runtime library.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (pa-opus-audio)

option(USE_ASAN "Use address sanitizer by adding -fsanitize=address -fno-omit-frame-pointer flags" OFF)
option(USE_UBSAN "Use address sanitizer by adding -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer flags" OFF)

find_package(portaudio CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
include_directories(${NODE_ADDON_API_DIR})

# Essential include files to build a node addon,
# you should add this line in every CMake.js based project.
include_directories(${CMAKE_JS_INC})

# Declare the location of the source files
file(GLOB SOURCE_FILES "src/*.c*")

# This line will tell CMake that we're building a shared library
# from the above source files
# named after the project's name
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})


if(USE_ASAN)
  target_compile_options(${PROJECT_NAME}  INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer>)
#  target_link_libraries(${PROJECT_NAME}  INTERFACE $<$<CONFIG:Debug>:-fsanitize=address>)
  list(APPEND -fsanitize=address)
endif()

if(USE_UBSAN)
  target_compile_options(${PROJECT_NAME}  INTERFACE $<$<CONFIG:Debug>:-fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer>)
#  target_link_libraries(${PROJECT_NAME}  INTERFACE $<$<CONFIG:Debug>:-fsanitize=undefined>)
  list(APPEND CMAKE_JS_LIB -fsanitize=undefined)
endif()

# This line will give our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

add_compile_definitions(NAPI_VERSION=${napi_build_version})

# ntohs on windows requires linking to ws2_32
if(WIN32)
  list(APPEND CMAKE_JS_LIB ws2_32)
endif()

# Essential library files to link to a node addon (+ portaudio/opus),
# you should add this line in every CMake.js based project.
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} portaudio_static Opus::opus)
