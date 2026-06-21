cmake_minimum_required(VERSION 3.28)
project(SurviveTheSemester)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif ()

add_executable(SurviveTheSemester
        bagel.h
        src/main.cpp
        src/Game.cpp
        src/GameRestart.cpp
        src/GameStateAccess.cpp
        src/Physics.cpp
        src/EntityFactory.cpp
        src/Sprites.cpp
        src/systems/input.cpp
        src/systems/render.cpp
        src/systems/ball_paddle.cpp
        src/systems/brick_progress.cpp
        src/systems/drops.cpp
        src/systems/progress.cpp
        src/systems/hud.cpp
        src/systems/exam.cpp
        src/systems/endgame.cpp
        src/systems/year.cpp
        src/systems/graduation.cpp
        src/systems/cleanup.cpp
)
target_include_directories(SurviveTheSemester PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src)

option(DEBUG_GRADUATION_STAGE "Boot directly into graduation stage (stage 2 preview)" OFF)
if(DEBUG_GRADUATION_STAGE)
    target_compile_definitions(SurviveTheSemester PRIVATE DEBUG_GRADUATION_STAGE=1)
endif()

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(lib/SDL)
target_link_libraries(SurviveTheSemester PUBLIC SDL3-static)

set(BUILD_SHARED_LIBS OFF)
# Only PNG/JPG (built-in STB backend) are needed. Disable formats whose
# vendored decoders live in external/ submodules that weren't copied in.
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_JXL OFF)
set(SDLIMAGE_TIF OFF)
set(SDLIMAGE_WEBP OFF)
add_subdirectory(lib/SDL_image)
target_link_libraries(SurviveTheSemester PUBLIC SDL3_image-static)

set(BOX2D_SAMPLES OFF)
set(BOX2D_BENCHMARKS OFF)
set(BOX2D_DOCS OFF)
set(BOX2D_PROFILE OFF)
set(BOX2D_VALIDATE OFF)
set(BOX2D_UNIT_TESTS OFF)
add_subdirectory(lib/box2d)
target_link_libraries(SurviveTheSemester PUBLIC box2d)

add_custom_command(TARGET SurviveTheSemester POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${PROJECT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:SurviveTheSemester>/res")

add_executable(brick_prereq_tests
        tests/test_brick_prereqs.cpp
        src/GameStateAccess.cpp
        src/Sprites.cpp
        src/systems/brick_progress.cpp
        src/EntityFactory.cpp
        src/Physics.cpp
)
target_include_directories(brick_prereq_tests PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(brick_prereq_tests PRIVATE SDL3-static SDL3_image-static box2d)

enable_testing()
add_test(NAME brick_prereq_tests COMMAND brick_prereq_tests)

add_executable(graduation_year_tests
        tests/test_graduation_year.cpp
        src/GameStateAccess.cpp
        src/systems/year.cpp
        src/systems/graduation.cpp
        src/systems/cleanup.cpp
        src/EntityFactory.cpp
        src/Physics.cpp
        src/Sprites.cpp
        src/systems/brick_progress.cpp
)
target_include_directories(graduation_year_tests PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(graduation_year_tests PRIVATE SDL3-static SDL3_image-static box2d)
add_test(NAME graduation_year_tests COMMAND graduation_year_tests)

add_executable(year_announce_tests
        tests/test_year_announce.cpp
        src/GameStateAccess.cpp
        src/systems/year.cpp
        src/systems/graduation.cpp
        src/systems/cleanup.cpp
        src/EntityFactory.cpp
        src/Physics.cpp
        src/Sprites.cpp
        src/systems/brick_progress.cpp
)
target_include_directories(year_announce_tests PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(year_announce_tests PRIVATE SDL3-static SDL3_image-static box2d)
add_test(NAME year_announce_tests COMMAND year_announce_tests)

add_executable(play_again_reset_tests
        tests/test_play_again_reset.cpp
        src/GameRestart.cpp
        src/GameStateAccess.cpp
        src/systems/graduation.cpp
        src/systems/cleanup.cpp
        src/EntityFactory.cpp
        src/Physics.cpp
        src/Sprites.cpp
        src/systems/brick_progress.cpp
)
target_include_directories(play_again_reset_tests PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(play_again_reset_tests PRIVATE SDL3-static SDL3_image-static box2d)
add_test(NAME play_again_reset_tests COMMAND play_again_reset_tests)
