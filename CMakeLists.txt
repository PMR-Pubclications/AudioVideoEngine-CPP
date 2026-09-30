cmake_minimum_required(VERSION 3.18)
project(AudioVideoEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(audio_engine STATIC
    src/lmsAdaptiveFiter.cpp
)

target_include_directories(audio_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_executable(audio_video_engine
        src/main.cpp
        src/LinuxCamera.cpp
    )

    target_link_libraries(audio_video_engine
        PRIVATE
            audio_engine
    )

    target_include_directories(audio_video_engine
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

elseif(ANDROID)
    add_library(audio_video_engine SHARED
        src/main.cpp
        src/AndroidCamera.cpp
    )

    target_link_libraries(audio_video_engine
        PRIVATE
            audio_engine
            log
    )

    target_include_directories(audio_video_engine
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

elseif(APPLE)
    add_library(audio_video_engine STATIC
        src/main.cpp
        src/IOSCamera.mm
    )

    target_link_libraries(audio_video_engine
        PRIVATE
            audio_engine
    )

    target_include_directories(audio_video_engine
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

else()
    message(FATAL_ERROR "Unsupported target platform. This standalone project currently supports Linux, Android, and Apple platforms.")
endif()
