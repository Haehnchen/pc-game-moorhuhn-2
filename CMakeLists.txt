cmake_minimum_required(VERSION 3.25)
project(moorhuhn2 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(MOORHUHN2_SANITIZERS "Enable ASan and UBSan on project targets" OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON CACHE BOOL "Enable link-time optimization" FORCE)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error LANGUAGES C CXX)
    if(NOT ipo_supported)
        message(FATAL_ERROR "Release link-time optimization is unavailable: ${ipo_error}")
    endif()
    if(MSVC)
        add_compile_options("/pathmap:${PROJECT_SOURCE_DIR}=." "/pathmap:${CMAKE_BINARY_DIR}=build")
    else()
        add_compile_options("-ffile-prefix-map=${PROJECT_SOURCE_DIR}=."
            "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build" -ffunction-sections -fdata-sections)
        if(APPLE)
            add_link_options("LINKER:-dead_strip" "LINKER:-headerpad_max_install_names")
        else()
            add_link_options("LINKER:--gc-sections")
        endif()
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # PipeWire inline functions also embed system header paths in SDL.
        add_compile_options("-ffile-prefix-map=/usr/include=include")
        set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
endif()
include(CTest)
include(cmake/Dependencies.cmake)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

add_library(moorhuhn2_project_options INTERFACE)
target_compile_features(moorhuhn2_project_options INTERFACE cxx_std_23)
target_include_directories(moorhuhn2_project_options INTERFACE "${PROJECT_SOURCE_DIR}/src")
if(MSVC)
    target_compile_options(moorhuhn2_project_options INTERFACE /W4 /permissive- /Zc:__cplusplus /WX)
else()
    target_compile_options(moorhuhn2_project_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()
if(MOORHUHN2_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT MSVC)
        target_compile_options(moorhuhn2_project_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(moorhuhn2_project_options INTERFACE -fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "MOORHUHN2_SANITIZERS requires GCC or Clang with ASan and UBSan")
    endif()
endif()

add_library(moorhuhn2_viewport src/platform/viewport.cpp)
target_link_libraries(moorhuhn2_viewport PUBLIC moorhuhn2_project_options)

add_library(moorhuhn2_clock src/platform/clock.cpp)
target_link_libraries(moorhuhn2_clock PUBLIC moorhuhn2_project_options)
add_library(moorhuhn2_keyboard src/platform/keyboard.cpp)
target_link_libraries(moorhuhn2_keyboard PUBLIC moorhuhn2_project_options)
add_library(moorhuhn2_audio_resampling src/audio/resampling.cpp)
target_link_libraries(moorhuhn2_audio_resampling PUBLIC moorhuhn2_project_options)
add_library(moorhuhn2_audio_mixing src/audio/mixing.cpp)
target_link_libraries(moorhuhn2_audio_mixing PUBLIC moorhuhn2_project_options)

add_library(moorhuhn2_ui
    src/ui/screens.cpp
    src/ui/title.cpp
    src/ui/scores.cpp
    src/ui/house88_results.cpp
    src/ui/outro.cpp
    src/ui/results_transition.cpp
    src/ui/round_exit.cpp
    src/ui/results_celebration.cpp
    src/ui/results_details.cpp
    src/ui/loading.cpp)
target_link_libraries(moorhuhn2_ui PUBLIC moorhuhn2_project_options)
add_library(moorhuhn2_bonus
    src/bonus/computer.cpp
    src/bonus/snake.cpp
    src/bonus/making_of.cpp)
target_link_libraries(moorhuhn2_bonus PUBLIC moorhuhn2_project_options moorhuhn2_ui)
add_library(moorhuhn2_world
    src/world/boat.cpp
    src/world/stone.cpp
    src/world/balloon.cpp
    src/world/village.cpp
    src/world/frog.cpp
    src/world/spider.cpp
    src/world/notice.cpp
    src/world/branch.cpp
    src/world/tree.cpp
    src/world/scarecrow.cpp
    src/world/hunting_stand.cpp
    src/world/hidden_chicken.cpp
    src/world/rose.cpp
    src/world/big_chicken.cpp
    src/world/special_chicken.cpp
    src/world/airplane.cpp
    src/world/blimp.cpp
    src/world/persistent_leaves.cpp)
target_link_libraries(moorhuhn2_world PUBLIC moorhuhn2_project_options)
add_library(moorhuhn2_game
    src/game/round.cpp
    src/game/round_targets.cpp
    src/game/round_effects.cpp
    src/game/round_actions.cpp
    src/game/round_hits.cpp
    src/game/round_collision.cpp
    src/game/round_update.cpp
    src/game/scene_object.cpp
    src/game/round_draw.cpp
    src/game/specials/multi_hit.cpp)
target_link_libraries(moorhuhn2_game PUBLIC moorhuhn2_project_options moorhuhn2_world)
add_library(moorhuhn2_package src/assets/asset_package.cpp)
target_link_libraries(moorhuhn2_package PUBLIC moorhuhn2_project_options)
add_library(moorhuhn2_render
    src/render/image_bank.cpp
    src/render/framebuffer.cpp
    src/render/frame_catalog.cpp
    src/world/presentation.cpp)
target_link_libraries(moorhuhn2_render PUBLIC moorhuhn2_project_options moorhuhn2_world moorhuhn2_assets)

add_library(moorhuhn2_round_scene src/render/round_scene.cpp)
target_link_libraries(moorhuhn2_round_scene PUBLIC moorhuhn2_game moorhuhn2_render moorhuhn2_ui)

add_library(moorhuhn2_platform
    src/platform/display.cpp
    src/platform/input.cpp
    src/platform/paths.cpp)
target_link_libraries(moorhuhn2_platform PUBLIC moorhuhn2_viewport moorhuhn2_keyboard SDL3::SDL3)

add_library(moorhuhn2_assets src/assets/asset_store.cpp)
target_link_libraries(moorhuhn2_assets PUBLIC moorhuhn2_project_options
    PRIVATE moorhuhn2_package SDL3::SDL3)

add_library(moorhuhn2_audio src/audio/engine.cpp)
target_link_libraries(moorhuhn2_audio PUBLIC moorhuhn2_project_options moorhuhn2_assets
    PRIVATE moorhuhn2_audio_resampling moorhuhn2_audio_mixing SDL3_mixer::SDL3_mixer)

set(image_package "${CMAKE_CURRENT_BINARY_DIR}/images.pak")
set(audio_package "${CMAKE_CURRENT_BINARY_DIR}/audio.pak")
file(GLOB_RECURSE prepared_assets CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/*")
add_custom_command(OUTPUT "${image_package}" "${audio_package}"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/pack_assets.py"
        --manifest "${PROJECT_SOURCE_DIR}/assets/manifest.txt"
        --output-dir "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${PROJECT_SOURCE_DIR}/tools/pack_assets.py" ${prepared_assets}
    COMMENT "Pack prepared MH2 assets" VERBATIM)
add_custom_target(moorhuhn2_asset_package ALL DEPENDS "${image_package}" "${audio_package}")
add_executable(moorhuhn2
    src/app/main.cpp
    src/app/application.cpp
    src/app/application_phases.cpp
    src/app/application_round.cpp
    src/app/audio_sink.cpp
    src/app/loading.cpp
    src/app/round_audio.cpp)
if(WIN32)
    enable_language(RC)
    configure_file(src/platform/windows.rc.in "${CMAKE_CURRENT_BINARY_DIR}/moorhuhn2.rc" @ONLY)
    target_sources(moorhuhn2 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/moorhuhn2.rc")
    set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/moorhuhn2.rc" APPEND PROPERTY
        OBJECT_DEPENDS "${PROJECT_SOURCE_DIR}/assets/moorhuhn2.ico")
    set_target_properties(moorhuhn2 PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
add_dependencies(moorhuhn2 moorhuhn2_asset_package)
target_link_libraries(moorhuhn2 PRIVATE moorhuhn2_platform moorhuhn2_assets
    moorhuhn2_audio moorhuhn2_round_scene moorhuhn2_ui moorhuhn2_bonus moorhuhn2_clock)
add_custom_target(run COMMAND "$<TARGET_FILE:moorhuhn2>" DEPENDS moorhuhn2
    VERBATIM USES_TERMINAL)

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
file(GLOB_RECURSE project_cpp CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/tests/*.cpp" "${PROJECT_SOURCE_DIR}/tests/*.hpp")
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(format-check COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${project_cpp}
        COMMENT "Check project C++ formatting" VERBATIM)
else()
    add_custom_target(format-check
        COMMAND "${CMAKE_COMMAND}" -E echo "format-check requires clang-format; install it and reconfigure."
        COMMAND "${CMAKE_COMMAND}" -E false
        VERBATIM)
endif()

if(BUILD_TESTING)
    include(cmake/Tests.cmake)
    add_subdirectory(tests/platform)
    add_subdirectory(tests/ui)
    add_subdirectory(tests/bonus)
    add_subdirectory(tests/game)
    add_subdirectory(tests/integration/game)
    add_subdirectory(tests/render)
    add_subdirectory(tests/world)
    add_subdirectory(tests/assets)
    add_subdirectory(tests/audio)
    add_subdirectory(tests/integration/audio)
    add_subdirectory(tests/integration/render)
    add_test(NAME tools.assets
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/tools/test_pack_assets.py" -v)
    set_tests_properties(tools.assets PROPERTIES LABELS "assets;tools")
    add_test(NAME tools.release_package
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/tools/test_package_release.py" -v)
    set_tests_properties(tools.release_package PROPERTIES LABELS "packaging;tools")
    add_test(NAME tools.verify_archives
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/tools/test_verify_archives.py" -v)
    set_tests_properties(tools.verify_archives PROPERTIES LABELS "packaging;tools")
    if(UNIX)
        add_test(NAME tools.makefile
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/tools/test_makefile.py" -v)
        set_tests_properties(tools.makefile PROPERTIES LABELS "tools")
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(NAME tools.linux_package
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/tools/test_linux_package.py" -v)
        set_tests_properties(tools.linux_package PROPERTIES LABELS "packaging;tools")
        add_test(NAME integration.private_runtime
            COMMAND "${Python3_EXECUTABLE}" -S "${PROJECT_SOURCE_DIR}/tests/integration/test_private_runtime.py")
        set_tests_properties(integration.private_runtime PROPERTIES LABELS "integration;isolation" TIMEOUT 60)
        add_test(NAME integration.native_images
            COMMAND "${Python3_EXECUTABLE}" -S "${PROJECT_SOURCE_DIR}/tests/integration/test_native_images.py")
        set_tests_properties(integration.native_images PROPERTIES LABELS "integration;render" TIMEOUT 15)
    endif()
endif()
