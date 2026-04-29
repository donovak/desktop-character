cmake_minimum_required(VERSION 3.20)

project(DesktopCharacter LANGUAGES CXX)

add_executable(DesktopCharacter WIN32
    src/main.cpp
    src/App.cpp
    src/App.h
    src/AppConfig.cpp
    src/AppConfig.h
    src/DebugLog.cpp
    src/DebugLog.h
    src/DesktopIconService.cpp
    src/DesktopIconService.h
    src/DesktopWindow.cpp
    src/DesktopWindow.h
    src/Renderer.cpp
    src/Renderer.h
    src/Input.cpp
    src/Input.h
    src/IconInteractionController.cpp
    src/IconInteractionController.h
    src/Character.cpp
    src/Character.h
)

target_compile_features(DesktopCharacter PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(DesktopCharacter PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()

target_compile_definitions(DesktopCharacter PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _WIN32_WINNT=0x0A00
)

target_link_libraries(DesktopCharacter PRIVATE
    d2d1
    dwrite
    ole32
    oleaut32
    shell32
    shlwapi
    uuid
)
