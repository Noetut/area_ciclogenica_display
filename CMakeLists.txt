cmake_minimum_required(VERSION 3.16)
project(PatronAnimation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/display/DisplayManager.cpp
    src/render/RenderEngine.cpp
    src/app/Application.cpp
    src/model/PatternGrid.cpp
)

# Headers
set(HEADERS
    src/display/DisplayManager.h
    src/render/RenderEngine.h
    src/app/Application.h
    src/model/PatternGrid.h
    src/model/PatternBoxes.h
)

# Executable output
add_executable(PatronAnimation ${SOURCES} ${HEADERS})

# Link Win32 libraries: gdi32 (GDI graphics), user32 (Windowing & Monitor API)
target_link_libraries(PatronAnimation PRIVATE gdi32 user32)

message(STATUS "Configured PatronAnimation target for Win32 build.")
message(STATUS "Run command: ./PatronAnimation --display 1 --blink 0.5 (omit --blink to keep it off)")
