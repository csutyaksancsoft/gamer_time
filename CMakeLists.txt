cmake_minimum_required(VERSION 3.24...4.0)
cmake_policy(SET CMP0091 NEW)
project(gamer_time LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(GT_BUILD_CLIENT "Build the SDL/Vulkan game client" ON)
option(GT_BUILD_SERVER "Build the headless authoritative server and bot" ON)
include(CTest)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

function(gt_configure_target target_name)
    target_include_directories(${target_name} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    if(MSVC)
        target_compile_definitions(${target_name} PRIVATE NOMINMAX)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/enet/CMakeLists.txt")
    message(FATAL_ERROR "ENet source is missing. Run the platform build script to download pinned dependencies.")
endif()
set(ENET_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ENET_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/enet EXCLUDE_FROM_ALL)

add_library(gt_core INTERFACE)
target_include_directories(gt_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(gt_match STATIC src/game/match_rules.cpp src/ui/ui_model.cpp)
gt_configure_target(gt_match)
target_link_libraries(gt_match PUBLIC gt_core)

add_library(gt_map STATIC
    src/assets/atlas_asset.cpp
    src/assets/tmx_map_loader.cpp
    src/game/collision_world.cpp
    src/game/map_world.cpp
)
gt_configure_target(gt_map)
target_link_libraries(gt_map PUBLIC gt_core)

add_library(gt_rhythm STATIC src/rhythm/song_config.cpp)
gt_configure_target(gt_rhythm)
target_link_libraries(gt_rhythm PUBLIC gt_core)

add_library(gt_net STATIC src/net/protocol.cpp src/net/client.cpp)
gt_configure_target(gt_net)
target_include_directories(gt_net PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/enet/include")
target_link_libraries(gt_net PUBLIC gt_core enet)
if(WIN32)
    target_link_libraries(gt_net PUBLIC ws2_32 winmm)
endif()

if(BUILD_TESTING)
    add_executable(gt_collision_world_tests tests/collision_world_tests.cpp)
    gt_configure_target(gt_collision_world_tests)
    target_compile_definitions(gt_collision_world_tests PRIVATE GT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(gt_collision_world_tests PRIVATE gt_map)
    add_test(NAME collision_channels COMMAND gt_collision_world_tests)
    add_executable(gt_tmx_animation_tests tests/tmx_animation_tests.cpp)
    gt_configure_target(gt_tmx_animation_tests)
    target_link_libraries(gt_tmx_animation_tests PRIVATE gt_map)
    add_test(NAME tmx_animation COMMAND gt_tmx_animation_tests)
    add_executable(gt_protocol_tests tests/protocol_tests.cpp)
    gt_configure_target(gt_protocol_tests)
    target_link_libraries(gt_protocol_tests PRIVATE gt_net)
    add_test(NAME protocol_round_trip COMMAND gt_protocol_tests)
    add_executable(gt_render_safety_tests tests/render_safety_tests.cpp)
    gt_configure_target(gt_render_safety_tests)
    target_link_libraries(gt_render_safety_tests PRIVATE gt_core)
    add_test(NAME render_safety COMMAND gt_render_safety_tests)
    add_executable(gt_match_ui_tests tests/match_ui_tests.cpp)
    gt_configure_target(gt_match_ui_tests)
    target_link_libraries(gt_match_ui_tests PRIVATE gt_match gt_net)
    add_test(NAME match_ui COMMAND gt_match_ui_tests)
    add_executable(gt_server_config_tests tests/server_config_tests.cpp src/server/server_config.cpp)
    gt_configure_target(gt_server_config_tests)
    target_link_libraries(gt_server_config_tests PRIVATE gt_rhythm gt_net)
    add_test(NAME server_config COMMAND gt_server_config_tests)
    add_executable(gt_headless_server_tests tests/headless_server_tests.cpp src/server/server_config.cpp)
    gt_configure_target(gt_headless_server_tests)
    target_link_libraries(gt_headless_server_tests PRIVATE gt_match gt_rhythm gt_net)
    add_test(NAME headless_server_rounds COMMAND gt_headless_server_tests)
endif()

if(GT_BUILD_SERVER)
    add_executable(gamer_time_server src/server/server_main.cpp src/server/server_config.cpp)
    gt_configure_target(gamer_time_server)
    target_link_libraries(gamer_time_server PRIVATE gt_map gt_net gt_rhythm gt_match enet)

    add_executable(gamer_time_bot src/bot/bot_main.cpp)
    gt_configure_target(gamer_time_bot)
    target_link_libraries(gamer_time_bot PRIVATE gt_net)
endif()

if(GT_BUILD_CLIENT)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/CMakeLists.txt")
        message(FATAL_ERROR "SDL source is missing. Run scripts/build-windows.ps1 or initialize the SDL submodule.")
    endif()
    add_subdirectory(external/SDL)
    find_package(Vulkan REQUIRED)

    add_library(gt_assets STATIC src/assets/image_loader.cpp)
    gt_configure_target(gt_assets)
    target_link_libraries(gt_assets PUBLIC gt_map gt_core SDL3::SDL3-static)

    add_library(gt_platform STATIC
        src/platform/camera_controller.cpp
        src/platform/effect_player.cpp
        src/platform/sdl_platform.cpp
        src/platform/song_player.cpp
    )
    gt_configure_target(gt_platform)
    target_link_libraries(gt_platform PUBLIC gt_core gt_rhythm SDL3::SDL3-static)

    add_library(gt_game STATIC
        src/game/command_queue.cpp
        src/game/fog_of_war_system.cpp
        src/game/navigation_system.cpp
        src/game/selection_system.cpp
        src/game/tile_map.cpp
        src/game/world.cpp
    )
    gt_configure_target(gt_game)
    target_link_libraries(gt_game PUBLIC gt_map gt_core gt_platform)

    add_library(gt_gpu STATIC
        src/gpu/buffer_utils.cpp
        src/gpu/gpu_resources.cpp
        src/gpu/swapchain_manager.cpp
        src/gpu/texture_utils.cpp
        src/gpu/vulkan_context.cpp
        src/render/text_overlay_renderer.cpp
    )
    gt_configure_target(gt_gpu)
    target_link_libraries(gt_gpu PUBLIC gt_assets gt_core Vulkan::Vulkan SDL3::SDL3-static)

    add_library(gt_render STATIC
        src/render/batch_builder.cpp
        src/render/depth_sorter.cpp
        src/render/frustum_culler.cpp
        src/render/projection_system.cpp
        src/render/render_extractor.cpp
        src/render/scene_renderer.cpp
        src/rhythm/rhythm_hud.cpp
    )
    gt_configure_target(gt_render)
    target_link_libraries(gt_render PUBLIC gt_core gt_game gt_gpu gt_platform gt_net)

    if(BUILD_TESTING)
        add_executable(gt_collision_debug_tests tests/collision_debug_tests.cpp)
        gt_configure_target(gt_collision_debug_tests)
        target_compile_definitions(gt_collision_debug_tests PRIVATE GT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(gt_collision_debug_tests PRIVATE gt_render)
        add_test(NAME collision_debug COMMAND gt_collision_debug_tests)
        add_executable(gt_rhythm_tests tests/rhythm_tests.cpp)
        gt_configure_target(gt_rhythm_tests)
        target_compile_definitions(gt_rhythm_tests PRIVATE GT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(gt_rhythm_tests PRIVATE gt_render gt_rhythm)
        add_test(NAME rhythm_gameplay COMMAND gt_rhythm_tests)
    endif()

    add_library(gt_app STATIC src/app/application.cpp)
    gt_configure_target(gt_app)
    target_link_libraries(gt_app PUBLIC gt_assets gt_game gt_net gt_platform gt_render gt_rhythm gt_match)

    if(WIN32)
        add_executable(gamer_time WIN32 src/main.cpp)
    else()
        add_executable(gamer_time src/main.cpp)
    endif()
    gt_configure_target(gamer_time)
    target_link_libraries(gamer_time PRIVATE gt_app)

    set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(SHADER_NAMES triangle.vert triangle.frag text.vert text.frag)
    find_program(GLSLANG_VALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
    set(SPIRV_OUTPUTS "")
    foreach(SHADER_NAME IN LISTS SHADER_NAMES)
        set(SHADER_SOURCE "${SHADER_SOURCE_DIR}/${SHADER_NAME}")
        set(SPIRV_SOURCE "${SHADER_SOURCE}.spv")
        set(SPIRV_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
        if(GLSLANG_VALIDATOR)
            add_custom_command(OUTPUT "${SPIRV_OUTPUT}" COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}" COMMAND "${GLSLANG_VALIDATOR}" -V "${SHADER_SOURCE}" -o "${SPIRV_OUTPUT}" DEPENDS "${SHADER_SOURCE}" VERBATIM)
        else()
            add_custom_command(OUTPUT "${SPIRV_OUTPUT}" COMMAND "${CMAKE_COMMAND}" -E make_directory "${SHADER_OUTPUT_DIR}" COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SPIRV_SOURCE}" "${SPIRV_OUTPUT}" DEPENDS "${SPIRV_SOURCE}" VERBATIM)
        endif()
        list(APPEND SPIRV_OUTPUTS "${SPIRV_OUTPUT}")
    endforeach()
    add_custom_target(shaders ALL DEPENDS ${SPIRV_OUTPUTS})
    add_dependencies(gamer_time shaders)
    add_custom_command(TARGET gamer_time POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${SHADER_OUTPUT_DIR}" "$<TARGET_FILE_DIR:gamer_time>/shaders"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:gamer_time>/assets"
        VERBATIM)
    add_custom_target(run COMMAND $<TARGET_FILE:gamer_time> --name LocalPlayer DEPENDS gamer_time WORKING_DIRECTORY "$<TARGET_FILE_DIR:gamer_time>" USES_TERMINAL)

    install(TARGETS gamer_time RUNTIME DESTINATION .)
    install(DIRECTORY "${SHADER_OUTPUT_DIR}/" DESTINATION shaders FILES_MATCHING PATTERN "*.spv")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION assets)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/LICENSE.txt" "${CMAKE_CURRENT_SOURCE_DIR}/external/enet/LICENSE" DESTINATION licenses)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_NAME "gamer-time")
    if(WIN32)
        set(CPACK_PACKAGE_FILE_NAME "gamer-time-windows-x64")
    else()
        set(CPACK_PACKAGE_FILE_NAME "gamer-time-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    include(CPack)
endif()
