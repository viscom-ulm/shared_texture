# Copyright 2023 Visual Computing Group, Ulm University
# Author: Jan Eric Haßler

cmake_minimum_required(VERSION 3.26)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
project(shared_texture)

# LIBRARY
add_library(shared_texture
    SHARED
        src/share.c
        src/share.h
        src/unity.c
        src/unity.h
        src/vk_funcs.h
        src/vk_utils.h
)
set_property(TARGET shared_texture PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set_source_files_properties(
    src/share.h
    src/unity.c
    src/unity.h
    src/vk_funcs.h
    src/vk_utils.h
    PROPERTIES HEADER_FILE_ONLY TRUE
)
target_include_directories(shared_texture PRIVATE include)
target_link_libraries(shared_texture opengl32.lib)

# DEMO
option(COMPILE_DEMO "compile the demo application." ON)
if(COMPILE_DEMO)
    add_executable(shared_texture_demo
        demo/main.c
        demo/gl_funcs.h
        demo/opengl.c
        demo/vulkan.c
        demo/sdl.c
        src/share.h
        src/vk_funcs.h
        src/vk_utils.h
    )
    set_property(TARGET shared_texture_demo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set_source_files_properties(
        src/share.h
        src/vk_funcs.h
        src/vk_utils.h
        src/gl_funcs.h
        demo/opengl.c
        demo/vulkan.c
        demo/sdl.c
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
    target_include_directories(shared_texture_demo PRIVATE include)
    target_include_directories(shared_texture_demo PRIVATE src)
    target_link_libraries(shared_texture_demo opengl32.lib)
    target_link_libraries(shared_texture_demo shared_texture)

    include(external/SDL2/cmake/sdl2-config.cmake)
    add_custom_command(TARGET shared_texture_demo POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_LIBDIR}/SDL2.dll"
        $<TARGET_FILE_DIR:shared_texture_demo>)
    target_include_directories(shared_texture_demo PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_directories(shared_texture_demo PRIVATE ${SDL2_LIBDIR})
    target_link_libraries(shared_texture_demo Shell32.lib ${SDL2_LIBRARIES})
endif()
