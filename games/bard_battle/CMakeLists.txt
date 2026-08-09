set(BARD_BATTLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(BARD_BATTLE_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

function(bard_battle_target target_name)
    attempt_engine_configure_target(${target_name})
    target_include_directories(${target_name} PUBLIC "${BARD_BATTLE_SOURCE_DIR}")
endfunction()

add_library(bard_battle_rhythm STATIC src/rhythm/song_config.cpp)
bard_battle_target(bard_battle_rhythm)
target_link_libraries(bard_battle_rhythm PUBLIC attempt_engine_core)

add_library(bard_battle_net STATIC src/net/protocol.cpp src/net/client.cpp)
bard_battle_target(bard_battle_net)
target_include_directories(bard_battle_net PUBLIC "${PROJECT_SOURCE_DIR}/external/enet/include")
target_link_libraries(bard_battle_net PUBLIC attempt_engine_core enet)
if(WIN32)
    target_link_libraries(bard_battle_net PUBLIC ws2_32 winmm)
endif()

add_library(bard_battle_match STATIC src/game/match_rules.cpp src/ui/ui_model.cpp)
bard_battle_target(bard_battle_match)
target_link_libraries(bard_battle_match PUBLIC bard_battle_net attempt_engine_core)

add_library(bard_battle_entity_animations STATIC src/render/entity_animations.cpp)
bard_battle_target(bard_battle_entity_animations)
target_link_libraries(bard_battle_entity_animations PUBLIC attempt_engine_core)

add_library(bard_battle_spawn STATIC src/server/spawn_zones.cpp)
bard_battle_target(bard_battle_spawn)
target_link_libraries(bard_battle_spawn PUBLIC attempt_engine_map bard_battle_net)

if(BUILD_TESTING)
    add_executable(bard_battle_asset_paths_tests tests/asset_paths_tests.cpp)
    bard_battle_target(bard_battle_asset_paths_tests)
    target_compile_definitions(bard_battle_asset_paths_tests PRIVATE BARD_BATTLE_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
    target_link_libraries(bard_battle_asset_paths_tests PRIVATE attempt_engine_map bard_battle_entity_animations)
    add_test(NAME bard_battle_asset_paths COMMAND bard_battle_asset_paths_tests)

    add_executable(bard_battle_protocol_tests tests/protocol_tests.cpp)
    bard_battle_target(bard_battle_protocol_tests)
    target_link_libraries(bard_battle_protocol_tests PRIVATE bard_battle_net)
    add_test(NAME bard_battle_protocol_round_trip COMMAND bard_battle_protocol_tests)

    add_executable(bard_battle_entity_animation_tests tests/entity_animation_tests.cpp)
    bard_battle_target(bard_battle_entity_animation_tests)
    target_link_libraries(bard_battle_entity_animation_tests PRIVATE bard_battle_entity_animations)
    add_test(NAME bard_battle_entity_animations COMMAND bard_battle_entity_animation_tests)

    add_executable(bard_battle_match_ui_tests tests/match_ui_tests.cpp)
    bard_battle_target(bard_battle_match_ui_tests)
    target_link_libraries(bard_battle_match_ui_tests PRIVATE bard_battle_match bard_battle_net)
    add_test(NAME bard_battle_match_ui COMMAND bard_battle_match_ui_tests)

    add_executable(bard_battle_combat_rules_tests tests/combat_rules_tests.cpp)
    bard_battle_target(bard_battle_combat_rules_tests)
    target_link_libraries(bard_battle_combat_rules_tests PRIVATE bard_battle_net attempt_engine_core)
    add_test(NAME bard_battle_combat_rules COMMAND bard_battle_combat_rules_tests)

    add_executable(bard_battle_spawn_zones_tests tests/spawn_zones_tests.cpp)
    bard_battle_target(bard_battle_spawn_zones_tests)
    target_compile_definitions(bard_battle_spawn_zones_tests PRIVATE BARD_BATTLE_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
    target_link_libraries(bard_battle_spawn_zones_tests PRIVATE bard_battle_spawn)
    add_test(NAME bard_battle_spawn_zones COMMAND bard_battle_spawn_zones_tests)

    foreach(test_name IN ITEMS server_config headless_server)
        add_executable(bard_battle_${test_name}_tests tests/${test_name}_tests.cpp src/server/server_config.cpp)
        bard_battle_target(bard_battle_${test_name}_tests)
        target_link_libraries(bard_battle_${test_name}_tests PRIVATE bard_battle_match bard_battle_rhythm bard_battle_net)
    endforeach()
    add_test(NAME bard_battle_server_config COMMAND bard_battle_server_config_tests)
    add_test(NAME bard_battle_headless_server_rounds COMMAND bard_battle_headless_server_tests)
endif()

if(BARD_BATTLE_BUILD_SERVER)
    add_executable(bard_battle_server src/server/server_main.cpp src/server/server_config.cpp)
    bard_battle_target(bard_battle_server)
    target_link_libraries(bard_battle_server PRIVATE attempt_engine_map bard_battle_net bard_battle_rhythm bard_battle_match bard_battle_spawn enet)

    add_executable(bard_battle_bot src/bot/bot_main.cpp)
    bard_battle_target(bard_battle_bot)
    target_link_libraries(bard_battle_bot PRIVATE bard_battle_net)
endif()

if(BARD_BATTLE_BUILD_CLIENT)
    add_library(bard_battle_entity_animation_packing STATIC src/render/entity_animation_packing.cpp)
    bard_battle_target(bard_battle_entity_animation_packing)
    target_link_libraries(bard_battle_entity_animation_packing PUBLIC bard_battle_entity_animations attempt_engine_assets)

    if(BUILD_TESTING)
        target_compile_definitions(bard_battle_asset_paths_tests PRIVATE BARD_BATTLE_TEST_ENTITY_PACKING=1)
        target_link_libraries(bard_battle_asset_paths_tests PRIVATE bard_battle_entity_animation_packing)
    endif()

    add_library(bard_battle_audio STATIC
        src/platform/audio_playback_device.cpp
        src/platform/effect_player.cpp
        src/platform/song_player.cpp)
    bard_battle_target(bard_battle_audio)
    target_link_libraries(bard_battle_audio PUBLIC attempt_engine_platform bard_battle_rhythm bard_battle_net SDL3::SDL3-static)

    add_library(bard_battle_scene STATIC
        src/render/scene_renderer.cpp
        src/render/text_overlay_renderer.cpp
        src/rhythm/rhythm_hud.cpp)
    bard_battle_target(bard_battle_scene)
    target_include_directories(bard_battle_scene PRIVATE "${PROJECT_SOURCE_DIR}")
    target_link_libraries(bard_battle_scene PUBLIC attempt_engine_gpu attempt_engine_render attempt_engine_platform bard_battle_match bard_battle_net)

    if(BUILD_TESTING)
        add_executable(bard_battle_audio_playback_tests tests/audio_playback_tests.cpp)
        bard_battle_target(bard_battle_audio_playback_tests)
        target_compile_definitions(bard_battle_audio_playback_tests PRIVATE BARD_BATTLE_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
        target_link_libraries(bard_battle_audio_playback_tests PRIVATE bard_battle_audio)
        add_test(NAME bard_battle_audio_playback COMMAND bard_battle_audio_playback_tests)

        add_executable(bard_battle_rhythm_tests tests/rhythm_tests.cpp)
        bard_battle_target(bard_battle_rhythm_tests)
        target_compile_definitions(bard_battle_rhythm_tests PRIVATE BARD_BATTLE_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
        target_link_libraries(bard_battle_rhythm_tests PRIVATE bard_battle_scene bard_battle_rhythm)
        add_test(NAME bard_battle_rhythm_gameplay COMMAND bard_battle_rhythm_tests)
    endif()

    add_library(bard_battle_app STATIC src/app/application.cpp)
    bard_battle_target(bard_battle_app)
    target_link_libraries(bard_battle_app PUBLIC attempt_engine_assets attempt_engine_world attempt_engine_render bard_battle_audio bard_battle_scene bard_battle_net bard_battle_rhythm bard_battle_match bard_battle_entity_animations bard_battle_entity_animation_packing)

    if(WIN32)
        add_executable(bard_battle WIN32 src/main.cpp)
    else()
        add_executable(bard_battle src/main.cpp)
    endif()
    bard_battle_target(bard_battle)
    target_link_libraries(bard_battle PRIVATE bard_battle_app)

    set(ATTEMPT_ENGINE_SHADER_SOURCE_DIR "${PROJECT_SOURCE_DIR}/attempt_engine/shaders")
    set(BARD_BATTLE_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(BARD_BATTLE_SHADER_NAMES triangle.vert triangle.frag text.vert text.frag)
    find_program(GLSLANG_VALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
    foreach(shader_name IN LISTS BARD_BATTLE_SHADER_NAMES)
        set(shader_source "${ATTEMPT_ENGINE_SHADER_SOURCE_DIR}/${shader_name}")
        set(shader_output "${BARD_BATTLE_SHADER_OUTPUT_DIR}/${shader_name}.spv")
        if(GLSLANG_VALIDATOR)
            add_custom_command(OUTPUT "${shader_output}" COMMAND "${CMAKE_COMMAND}" -E make_directory "${BARD_BATTLE_SHADER_OUTPUT_DIR}" COMMAND "${GLSLANG_VALIDATOR}" -V "${shader_source}" -o "${shader_output}" DEPENDS "${shader_source}" VERBATIM)
        else()
            add_custom_command(OUTPUT "${shader_output}" COMMAND "${CMAKE_COMMAND}" -E make_directory "${BARD_BATTLE_SHADER_OUTPUT_DIR}" COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${shader_source}.spv" "${shader_output}" DEPENDS "${shader_source}.spv" VERBATIM)
        endif()
        list(APPEND BARD_BATTLE_SPIRV_OUTPUTS "${shader_output}")
    endforeach()
    add_custom_target(bard_battle_shaders ALL DEPENDS ${BARD_BATTLE_SPIRV_OUTPUTS})
    add_dependencies(bard_battle bard_battle_shaders)
    add_custom_command(TARGET bard_battle POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${BARD_BATTLE_SHADER_OUTPUT_DIR}" "$<TARGET_FILE_DIR:bard_battle>/shaders"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${BARD_BATTLE_ASSET_DIR}" "$<TARGET_FILE_DIR:bard_battle>/assets"
        VERBATIM)
    add_custom_target(run_bard_battle COMMAND $<TARGET_FILE:bard_battle> --name LocalPlayer DEPENDS bard_battle WORKING_DIRECTORY "$<TARGET_FILE_DIR:bard_battle>" USES_TERMINAL)

    install(TARGETS bard_battle RUNTIME DESTINATION .)
    install(DIRECTORY "${BARD_BATTLE_SHADER_OUTPUT_DIR}/" DESTINATION shaders FILES_MATCHING PATTERN "*.spv")
    install(DIRECTORY "${BARD_BATTLE_ASSET_DIR}/" DESTINATION assets)
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/external/SDL/LICENSE.txt" "${PROJECT_SOURCE_DIR}/external/enet/LICENSE" DESTINATION licenses)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_NAME "bard-battle")
    if(WIN32)
        set(CPACK_PACKAGE_FILE_NAME "bard-battle-windows-x64")
    else()
        set(CPACK_PACKAGE_FILE_NAME "bard-battle-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    include(CPack)
endif()
