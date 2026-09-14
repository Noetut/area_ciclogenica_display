cmake_minimum_required(VERSION 3.16)
project(PatronAnimation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Source files
set(SOURCES
    src/main.cpp
    src/display/DisplayManager.cpp
    src/render/RenderEngine.cpp
    src/app/Application.cpp
    src/model/PatternGrid.cpp
    src/model/PatternConfig.cpp
    src/calib/CalibrationController.cpp
    src/anim/AnimationParser.cpp
    src/anim/AnimationController.cpp
)

# Headers
set(HEADERS
    src/display/DisplayManager.h
    src/render/RenderEngine.h
    src/app/Application.h
    src/model/Geometry.h
    src/model/ProjectionArea.h
    src/model/PatternGrid.h
    src/model/PatternConfig.h
    src/calib/CalibrationController.h
    src/anim/AnimationTypes.h
    src/anim/AnimationParser.h
    src/anim/AnimationController.h
    src/util/StringUtil.h
)

# Executable output
add_executable(PatronAnimation ${SOURCES} ${HEADERS})

# nlohmann/json is a heavily templated single header; on MinGW the translation
# unit that includes it can exceed the default COFF section limit.
if(MINGW)
    set_source_files_properties(src/model/PatternConfig.cpp
        PROPERTIES COMPILE_OPTIONS "-Wa,-mbig-obj")
endif()

# Link Win32 libraries: gdi32 (GDI graphics), user32 (Windowing & Monitor API), msimg32 (AlphaBlend)
target_link_libraries(PatronAnimation PRIVATE gdi32 user32 msimg32)

message(STATUS "Configured PatronAnimation target for Win32 build.")
