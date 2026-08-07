cmake_minimum_required(VERSION 3.24...4.0)

cmake_policy(SET CMP0091 NEW)

project(gamer_time LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/CMakeLists.txt")
    message(FATAL_ERROR
        "SDL source is missing. Run scripts/build-windows.ps1 or initialize the SDL submodule."
    )
endif()

add_subdirectory(external/SDL)
find_package(Vulkan REQUIRED)

function(gt_configure_target target_name)
    target_include_directories(${target_name} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(gt_core INTERFACE)
target_include_directories(gt_core INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_library(gt_assets STATIC
    src/assets/atlas_asset.cpp
    src/assets/image_loader.cpp
    src/assets/tmx_map_loader.cpp
)
gt_configure_target(gt_assets)
target_link_libraries(gt_assets PUBLIC
    gt_core
    SDL3::SDL3-static
)

add_library(gt_platform STATIC
    src/platform/camera_controller.cpp
    src/platform/sdl_platform.cpp
)
gt_configure_target(gt_platform)
target_link_libraries(gt_platform PUBLIC
    gt_core
    SDL3::SDL3-static
)

add_library(gt_game STATIC
    src/game/collision_world.cpp
    src/game/command_queue.cpp
    src/game/fog_of_war_system.cpp
    src/game/map_world.cpp
    src/game/navigation_system.cpp
    src/game/selection_system.cpp
    src/game/tile_map.cpp
    src/game/world.cpp
)
gt_configure_target(gt_game)
target_link_libraries(gt_game PUBLIC
    gt_assets
    gt_core
    gt_platform
)

add_library(gt_gpu STATIC
    src/gpu/buffer_utils.cpp
    src/gpu/gpu_resources.cpp
    src/gpu/swapchain_manager.cpp
    src/gpu/texture_utils.cpp
    src/gpu/vulkan_context.cpp
    src/render/text_overlay_renderer.cpp
)
gt_configure_target(gt_gpu)
target_link_libraries(gt_gpu PUBLIC
    gt_assets
    gt_core
    Vulkan::Vulkan
    SDL3::SDL3-static
)

add_library(gt_render STATIC
    src/render/batch_builder.cpp
    src/render/depth_sorter.cpp
    src/render/frustum_culler.cpp
    src/render/projection_system.cpp
    src/render/render_extractor.cpp
    src/render/scene_renderer.cpp
)
gt_configure_target(gt_render)
target_link_libraries(gt_render PUBLIC
    gt_core
    gt_game
    gt_gpu
    gt_platform
)

add_library(gt_app STATIC
    src/app/application.cpp
)
gt_configure_target(gt_app)
target_link_libraries(gt_app PUBLIC
    gt_assets
    gt_game
    gt_platform
    gt_render
)

if(WIN32)
    add_executable(gamer_time WIN32 src/main.cpp)
else()
    add_executable(gamer_time src/main.cpp)
endif()
gt_configure_target(gamer_time)
target_link_libraries(gamer_time PRIVATE gt_app)

set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(SHADER_NAMES
    triangle.vert
    triangle.frag
    text.vert
    text.frag
)

find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
)

set(SPIRV_OUTPUTS "")
foreach(SHADER_NAME IN LISTS SHADER_NAMES)
    set(SHADER_SOURCE "${SHADER_SOURCE_DIR}/${SHADER_NAME}")
    set(SPIRV_SOURCE "${SHADER_SOURCE}.spv")
    set(SPIRV_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    if(GLSLANG_VALIDATOR)
        add_custom_command(
            OUTPUT "${SPIRV_OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}"
            COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER_SOURCE}" -o "${SPIRV_OUTPUT}"
            DEPENDS "${SHADER_SOURCE}"
            COMMENT "Compiling shader ${SHADER_NAME}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT "${SPIRV_OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SPIRV_SOURCE}" "${SPIRV_OUTPUT}"
            DEPENDS "${SPIRV_SOURCE}"
            COMMENT "Copying precompiled shader ${SHADER_NAME}.spv"
            VERBATIM
        )
    endif()

    list(APPEND SPIRV_OUTPUTS "${SPIRV_OUTPUT}")
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(gamer_time shaders)

add_custom_command(TARGET gamer_time POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${SHADER_OUTPUT_DIR}"
        "$<TARGET_FILE_DIR:gamer_time>/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:gamer_time>/assets"
    COMMENT "Copying runtime shaders and game assets beside gamer_time"
    VERBATIM
)

add_custom_target(run
    COMMAND $<TARGET_FILE:gamer_time>
    DEPENDS gamer_time
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:gamer_time>"
    USES_TERMINAL
    COMMENT "Running gamer_time"
)

install(TARGETS gamer_time RUNTIME DESTINATION .)
install(DIRECTORY "${SHADER_OUTPUT_DIR}/" DESTINATION shaders
    FILES_MATCHING PATTERN "*.spv"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION assets)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/LICENSE.txt"
    DESTINATION licenses
)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME "gamer-time")
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "gamer-time-windows-x64")
else()
    set(CPACK_PACKAGE_FILE_NAME
        "gamer-time-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
    )
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
