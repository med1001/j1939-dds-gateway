cmake_minimum_required(VERSION 3.20)

project(j1939_dds_gateway VERSION 1.0.0 LANGUAGES C)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

option(J1939_DDS_ENABLE_CYCLONEDDS "Build the Cyclone DDS publisher and subscriber" OFF)
option(J1939_DDS_FETCH_CYCLONEDDS "Fetch Cyclone DDS when it is not installed" OFF)
option(J1939_DDS_BUILD_TESTING "Build unit and integration tests" ON)
option(J1939_DDS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(J1939_DDS_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(j1939_dds_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(J1939_DDS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(J1939_DDS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(j1939_dds_enable_sanitizers target)
    if(J1939_DDS_ENABLE_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

add_library(j1939_gateway_core STATIC
    src/decoder.c
    src/replay_source.c
)
target_include_directories(j1939_gateway_core PUBLIC include)
j1939_dds_set_warnings(j1939_gateway_core)
j1939_dds_enable_sanitizers(j1939_gateway_core)

set(J1939_GATEWAY_SOURCES
    src/main.c
    src/stdout_publisher.c
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND J1939_GATEWAY_SOURCES src/linux_j1939_source.c)
endif()

if(J1939_DDS_ENABLE_CYCLONEDDS)
    find_package(CycloneDDS QUIET)

    if(NOT CycloneDDS_FOUND AND J1939_DDS_FETCH_CYCLONEDDS)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_DDSPERF OFF CACHE BOOL "" FORCE)
        set(BUILD_IDLC_XTESTS OFF CACHE BOOL "" FORCE)
        set(ENABLE_SSL NO CACHE STRING "" FORCE)
        set(_j1939_saved_skip_install_rules ${CMAKE_SKIP_INSTALL_RULES})
        set(_j1939_saved_c_extensions ${CMAKE_C_EXTENSIONS})
        set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(CMAKE_SKIP_INSTALL_RULES TRUE)
        set(CMAKE_C_EXTENSIONS ON)
        FetchContent_Declare(
            CycloneDDS
            GIT_REPOSITORY https://github.com/eclipse-cyclonedds/cyclonedds.git
            GIT_TAG 0.10.5
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(CycloneDDS)
        if(TARGET ddsc AND NOT TARGET CycloneDDS::ddsc)
            add_library(CycloneDDS::ddsc ALIAS ddsc)
        endif()
        if(TARGET idlc AND NOT TARGET CycloneDDS::idlc)
            add_executable(CycloneDDS::idlc ALIAS idlc)
        endif()
        set(BUILD_TESTING ${J1939_DDS_BUILD_TESTING} CACHE BOOL "" FORCE)
        set(CMAKE_SKIP_INSTALL_RULES ${_j1939_saved_skip_install_rules})
        set(CMAKE_C_EXTENSIONS ${_j1939_saved_c_extensions})
    elseif(NOT CycloneDDS_FOUND)
        message(FATAL_ERROR
            "Cyclone DDS was not found. Install it or configure with "
            "-DJ1939_DDS_FETCH_CYCLONEDDS=ON.")
    endif()

    idlc_generate(
        TARGET vehicle_telemetry_types
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/idl/VehicleTelemetry.idl"
    )
    list(APPEND J1939_GATEWAY_SOURCES src/cyclone_publisher.c)
endif()

add_executable(j1939_dds_gateway ${J1939_GATEWAY_SOURCES})
target_link_libraries(j1939_dds_gateway PRIVATE j1939_gateway_core)
target_include_directories(j1939_dds_gateway PRIVATE include)
j1939_dds_set_warnings(j1939_dds_gateway)
j1939_dds_enable_sanitizers(j1939_dds_gateway)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(j1939_dds_gateway PRIVATE
        J1939_DDS_HAVE_LINUX_J1939=1
        _POSIX_C_SOURCE=200809L)

    add_executable(j1939_demo_sender src/j1939_demo_sender.c)
    target_include_directories(j1939_demo_sender PRIVATE include)
    j1939_dds_set_warnings(j1939_demo_sender)
    j1939_dds_enable_sanitizers(j1939_demo_sender)
endif()

if(J1939_DDS_ENABLE_CYCLONEDDS)
    target_compile_definitions(j1939_dds_gateway PRIVATE J1939_DDS_HAVE_CYCLONEDDS=1)
    target_link_libraries(j1939_dds_gateway PRIVATE vehicle_telemetry_types CycloneDDS::ddsc)

    add_executable(telemetry_subscriber src/telemetry_subscriber.c)
    target_link_libraries(telemetry_subscriber PRIVATE vehicle_telemetry_types CycloneDDS::ddsc)
    j1939_dds_set_warnings(telemetry_subscriber)
    j1939_dds_enable_sanitizers(telemetry_subscriber)
endif()

if(J1939_DDS_BUILD_TESTING)
    set(BUILD_TESTING ON CACHE BOOL "Build project tests" FORCE)
    include(CTest)

    add_executable(test_decoder tests/test_decoder.c)
    target_link_libraries(test_decoder PRIVATE j1939_gateway_core)
    j1939_dds_set_warnings(test_decoder)
    j1939_dds_enable_sanitizers(test_decoder)
    add_test(NAME decoder COMMAND test_decoder)

    add_executable(test_replay_pipeline tests/test_replay_pipeline.c)
    target_link_libraries(test_replay_pipeline PRIVATE j1939_gateway_core)
    target_compile_definitions(test_replay_pipeline PRIVATE
        TEST_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/j1939_frames.csv")
    j1939_dds_set_warnings(test_replay_pipeline)
    j1939_dds_enable_sanitizers(test_replay_pipeline)
    add_test(NAME replay_pipeline COMMAND test_replay_pipeline)
endif()
