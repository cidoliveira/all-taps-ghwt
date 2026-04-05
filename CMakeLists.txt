cmake_minimum_required(VERSION 3.25)

# Force static CRT before project() so it applies to FetchContent targets too
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(all_taps_xinput VERSION 1.0.0 CXX)

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

# --- Distribution packaging ---
install(TARGETS xinput1_3
    RUNTIME DESTINATION .
)
install(FILES "${CMAKE_SOURCE_DIR}/README.md"
    DESTINATION .
)
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
    DESTINATION .
)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "all-taps-ghwt")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win32")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "All-Taps mod for Guitar Hero World Tour: Definitive Edition")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

include(CPack)
