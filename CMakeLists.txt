cmake_minimum_required(VERSION 3.10)
project(dasAfterburner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Build options ────────────────────────────────────────────────────────────
option(ENABLE_NTCORE "Build with NetworkTables (ntcore) support" ON)
option(ENABLE_RKNN   "Build with RKNN NPU inference support"     ON)

# ── OpenCV (required) ────────────────────────────────────────────────────────
find_package(OpenCV REQUIRED)

# ── RKNN NPU SDK ─────────────────────────────────────────────────────────────
if(ENABLE_RKNN)
    find_path(RKNN_INCLUDE_DIR rknn_api.h
        PATHS /usr/local/include /usr/include)
    find_library(RKNN_LIB rknnrt
        PATHS /usr/local/lib /usr/lib)

    if(RKNN_INCLUDE_DIR AND RKNN_LIB)
        message(STATUS "RKNN found: ${RKNN_LIB}")
    else()
        message(WARNING "RKNN not found — building without NPU support. "
            "Run sudo scripts/setup_orangepi.sh to install.")
        set(ENABLE_RKNN OFF)
    endif()
endif()

# ── NetworkTables / ntcore ────────────────────────────────────────────────────
if(ENABLE_NTCORE)
    # Try CMake package first, then manual paths
    find_package(ntcore QUIET)
    if(ntcore_FOUND)
        message(STATUS "ntcore found via find_package")
    else()
        find_path(NTCORE_INCLUDE_DIR networktables/NetworkTableInstance.h
            PATHS /usr/local/include /usr/include)
        find_library(NTCORE_LIB ntcore
            PATHS /usr/local/lib /usr/lib)
        find_library(WPIUTIL_LIB wpiutil
            PATHS /usr/local/lib /usr/lib)
        find_library(WPINET_LIB wpinet
            PATHS /usr/local/lib /usr/lib)

        if(NTCORE_INCLUDE_DIR AND NTCORE_LIB)
            message(STATUS "ntcore found manually: ${NTCORE_LIB}")
            set(ntcore_FOUND TRUE)
        else()
            message(WARNING "ntcore not found — building without NetworkTables support. "
                "Run sudo scripts/setup_orangepi.sh to install.")
            set(ENABLE_NTCORE OFF)
        endif()
    endif()
endif()

# ── Source & includes ─────────────────────────────────────────────────────────
include_directories(include ${OpenCV_INCLUDE_DIRS})

if(ENABLE_RKNN)
    include_directories(${RKNN_INCLUDE_DIR})
endif()

if(ENABLE_NTCORE AND NTCORE_INCLUDE_DIR)
    include_directories(${NTCORE_INCLUDE_DIR})
endif()

add_executable(dasAfterburner src/main.cpp)

# ── Compile definitions ────────────────────────────────────────────────────────
if(ENABLE_NTCORE)
    target_compile_definitions(dasAfterburner PRIVATE HAVE_NTCORE)
endif()
if(ENABLE_RKNN)
    target_compile_definitions(dasAfterburner PRIVATE HAVE_RKNN)
endif()

# ── Link libraries ─────────────────────────────────────────────────────────────
target_link_libraries(dasAfterburner
    ${OpenCV_LIBS}
    pthread
    dl
)

if(ENABLE_NTCORE)
    if(ntcore_FOUND AND NOT NTCORE_LIB)
        # Found via find_package — use imported target
        target_link_libraries(dasAfterburner ntcore)
    else()
        target_link_libraries(dasAfterburner ${NTCORE_LIB})
        if(WPIUTIL_LIB)
            target_link_libraries(dasAfterburner ${WPIUTIL_LIB})
        endif()
        if(WPINET_LIB)
            target_link_libraries(dasAfterburner ${WPINET_LIB})
        endif()
    endif()
endif()

if(ENABLE_RKNN)
    target_link_libraries(dasAfterburner ${RKNN_LIB})
endif()

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS dasAfterburner DESTINATION bin)
install(DIRECTORY models DESTINATION .)

# ── Status summary ─────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== dasAfterburner build config ===")
message(STATUS "  NetworkTables (ntcore) : ${ENABLE_NTCORE}")
message(STATUS "  RKNN NPU               : ${ENABLE_RKNN}")
message(STATUS "  OpenCV                 : ${OpenCV_VERSION}")
message(STATUS "===================================")
message(STATUS "")
