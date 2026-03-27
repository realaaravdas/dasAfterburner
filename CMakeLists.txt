cmake_minimum_required(VERSION 3.10)
project(dasAfterburner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_RKNN "Build with RKNN NPU inference support" ON)

# ── OpenCV (required) ─────────────────────────────────────────────────────────
find_package(OpenCV REQUIRED)

# ── libgpiod (required) ───────────────────────────────────────────────────────
find_path(GPIOD_INCLUDE_DIR gpiod.h PATHS /usr/include /usr/local/include)
find_library(GPIOD_LIB gpiod PATHS /usr/lib /usr/local/lib)

if(NOT GPIOD_INCLUDE_DIR OR NOT GPIOD_LIB)
    message(FATAL_ERROR "libgpiod not found. Run: sudo apt-get install libgpiod-dev")
endif()
message(STATUS "libgpiod: ${GPIOD_LIB}")

# ── RKNN NPU SDK ──────────────────────────────────────────────────────────────
if(ENABLE_RKNN)
    find_path(RKNN_INCLUDE_DIR rknn_api.h PATHS /usr/local/include /usr/include)
    find_library(RKNN_LIB rknnrt PATHS /usr/local/lib /usr/lib)

    if(RKNN_INCLUDE_DIR AND RKNN_LIB)
        message(STATUS "RKNN: ${RKNN_LIB}")
    else()
        message(WARNING "RKNN not found — HSV fallback will be used. "
            "Run sudo scripts/setup_orangepi.sh to install.")
        set(ENABLE_RKNN OFF)
    endif()
endif()

# ── Build ─────────────────────────────────────────────────────────────────────
include_directories(include ${OpenCV_INCLUDE_DIRS} ${GPIOD_INCLUDE_DIR})

if(ENABLE_RKNN)
    include_directories(${RKNN_INCLUDE_DIR})
endif()

add_executable(dasAfterburner src/main.cpp)

if(ENABLE_RKNN)
    target_compile_definitions(dasAfterburner PRIVATE HAVE_RKNN)
endif()

target_link_libraries(dasAfterburner
    ${OpenCV_LIBS}
    ${GPIOD_LIB}
    pthread
    dl
)

if(ENABLE_RKNN)
    target_link_libraries(dasAfterburner ${RKNN_LIB})
endif()

install(TARGETS dasAfterburner DESTINATION bin)
install(DIRECTORY models DESTINATION .)

message(STATUS "")
message(STATUS "=== dasAfterburner build config ===")
message(STATUS "  RKNN NPU  : ${ENABLE_RKNN}")
message(STATUS "  OpenCV    : ${OpenCV_VERSION}")
message(STATUS "  libgpiod  : ${GPIOD_LIB}")
message(STATUS "===================================")
message(STATUS "")
