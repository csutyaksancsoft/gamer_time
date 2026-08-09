set(ATTEMPT_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(attempt_engine_core INTERFACE)
target_include_directories(attempt_engine_core INTERFACE "${ATTEMPT_ENGINE_SOURCE_DIR}")

add_library(attempt_engine_map STATIC
    src/assets/atlas_asset.cpp
    src/assets/tmx_map_loader.cpp
    src/game/collision_world.cpp
    src/game/map_world.cpp)
attempt_engine_configure_target(attempt_engine_map)
target_link_libraries(attempt_engine_map PUBLIC attempt_engine_core)

add_library(attempt_engine_world STATIC
    src/game/command_queue.cpp
    src/game/fog_of_war_system.cpp
    src/game/navigation_system.cpp
    src/game/selection_system.cpp
    src/game/tile_map.cpp
    src/game/world.cpp)
attempt_engine_configure_target(attempt_engine_world)
target_link_libraries(attempt_engine_world PUBLIC attempt_engine_map attempt_engine_core)

add_library(attempt_engine_render STATIC
    src/render/batch_builder.cpp
    src/render/depth_sorter.cpp
    src/render/frustum_culler.cpp
    src/render/projection_system.cpp
    src/render/render_extractor.cpp)
attempt_engine_configure_target(attempt_engine_render)
target_link_libraries(attempt_engine_render PUBLIC attempt_engine_world attempt_engine_map attempt_engine_core)

if(TARGET SDL3::SDL3-static AND TARGET Vulkan::Vulkan)
    add_library(attempt_engine_assets STATIC src/assets/image_loader.cpp)
    attempt_engine_configure_target(attempt_engine_assets)
    target_link_libraries(attempt_engine_assets PUBLIC attempt_engine_map attempt_engine_core SDL3::SDL3-static)

    add_library(attempt_engine_platform STATIC
        src/platform/camera_controller.cpp
        src/platform/sdl_platform.cpp)
    attempt_engine_configure_target(attempt_engine_platform)
    target_link_libraries(attempt_engine_platform PUBLIC attempt_engine_core SDL3::SDL3-static)

    add_library(attempt_engine_gpu STATIC
        src/gpu/buffer_utils.cpp
        src/gpu/gpu_resources.cpp
        src/gpu/swapchain_manager.cpp
        src/gpu/texture_utils.cpp
        src/gpu/vulkan_context.cpp)
    attempt_engine_configure_target(attempt_engine_gpu)
    target_link_libraries(attempt_engine_gpu PUBLIC attempt_engine_assets attempt_engine_core Vulkan::Vulkan SDL3::SDL3-static)
endif()

if(BUILD_TESTING)
    foreach(test_name IN ITEMS collision_world fog_of_war tmx_animation render_safety)
        add_executable(attempt_engine_${test_name}_tests tests/${test_name}_tests.cpp)
        attempt_engine_configure_target(attempt_engine_${test_name}_tests)
    endforeach()
    target_compile_definitions(attempt_engine_collision_world_tests PRIVATE ATTEMPT_ENGINE_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
    target_link_libraries(attempt_engine_collision_world_tests PRIVATE attempt_engine_map)
    target_link_libraries(attempt_engine_fog_of_war_tests PRIVATE attempt_engine_world)
    target_link_libraries(attempt_engine_tmx_animation_tests PRIVATE attempt_engine_map)
    target_link_libraries(attempt_engine_render_safety_tests PRIVATE attempt_engine_core)
    add_test(NAME attempt_engine_collision_channels COMMAND attempt_engine_collision_world_tests)
    add_test(NAME attempt_engine_fog_of_war COMMAND attempt_engine_fog_of_war_tests)
    add_test(NAME attempt_engine_tmx_animation COMMAND attempt_engine_tmx_animation_tests)
    add_test(NAME attempt_engine_render_safety COMMAND attempt_engine_render_safety_tests)

    add_executable(attempt_engine_collision_debug_tests tests/collision_debug_tests.cpp)
    attempt_engine_configure_target(attempt_engine_collision_debug_tests)
    target_compile_definitions(attempt_engine_collision_debug_tests PRIVATE ATTEMPT_ENGINE_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
    target_link_libraries(attempt_engine_collision_debug_tests PRIVATE attempt_engine_render)
    add_test(NAME attempt_engine_collision_debug COMMAND attempt_engine_collision_debug_tests)
endif()
