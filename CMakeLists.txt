cmake_minimum_required(VERSION 3.10)
project(dasAfterburner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 2. RKNN API (Assumed to be in /usr/include or specified via CMAKE_PREFIX_PATH)
# If rknn_api.h is in a custom location, add it here:
# include_directories(/path/to/rknpu2/include)
# link_directories(/path/to/rknpu2/lib)

# 3. NetworkTables (ntcore) and WPILib Dependencies
# These usually need to be downloaded as static/shared libs for ARM64
# For now, we'll search for them.
find_package(ntcore QUIET)
if(NOT ntcore_FOUND)
    message(STATUS "ntcore not found via find_package. You may need to specify paths.")
    # include_directories(external/ntcore/include)
    # link_directories(external/ntcore/lib)
endif()

include_directories(include)

add_executable(dasAfterburner src/main.cpp)

target_link_libraries(dasAfterburner
    ${OpenCV_LIBS}
    # ntcore # Uncomment when library is available
    # rknn_api # Uncomment when library is available
    pthread
    dl
)

# Installation
install(TARGETS dasAfterburner DESTINATION bin)
install(DIRECTORY models DESTINATION .)
