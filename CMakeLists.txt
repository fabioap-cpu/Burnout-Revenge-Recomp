# burnoutrevenge - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(burnoutrevenge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(BURNOUTREVENGE_SOURCES
    src/main.cpp
    src/burnoutrevenge_hooks.cpp
)

if(WIN32)
    add_executable(burnoutrevenge WIN32 ${BURNOUTREVENGE_SOURCES})
else()
    add_executable(burnoutrevenge ${BURNOUTREVENGE_SOURCES})
endif()

rexglue_setup_target(burnoutrevenge GPU_PLUGINS xenos)

# rex_app.cpp (added by rexglue_configure_target) needs imgui.h, but rexruntime
# links imgui PRIVATE -- that propagates the .lib to the final link but not
# the include directory. Link it directly so this consumer sees the header.
# (Same fix already applied in ForzaHorizon2_Recomp/CMakeLists.txt.)
target_link_libraries(burnoutrevenge PRIVATE imgui)
