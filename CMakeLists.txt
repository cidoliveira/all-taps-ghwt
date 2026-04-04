cmake_minimum_required(VERSION 3.20)

# Force static CRT before project() so it applies to FetchContent targets too
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(all_taps_xinput CXX)

include(FetchContent)

# Fetch MinHook 1.3.4 — used in later phases for function hooking
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        v1.3.4
)
FetchContent_MakeAvailable(minhook)

# DLL proxy target
# src/dllmain.cpp and src/xinput_proxy.cpp are created in Plan 02
add_library(xinput1_3 SHARED
    src/dllmain.cpp
    src/xinput_proxy.cpp
)

# Ordinal export map — must precede any link step
set_target_properties(xinput1_3 PROPERTIES
    LINK_FLAGS    "/DEF:\"${CMAKE_SOURCE_DIR}/xinput1_3.def\""
    OUTPUT_NAME   "xinput1_3"
    SUFFIX        ".dll"
)

target_include_directories(xinput1_3 PRIVATE src)

# MinHook is scaffolded here for Phase 3+; not actively called yet.
# Do NOT link against 'xinput' — xinput.lib adds xinput1_3.dll as an import
# of our own proxy DLL, creating a circular dependency at load time.
# XInput types come from #include <xinput.h> (header-only, no .lib needed).
target_link_libraries(xinput1_3 PRIVATE minhook)
