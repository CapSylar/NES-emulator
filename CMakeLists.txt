cmake_minimum_required(VERSION 3.16)
project(NESX C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2IMAGE REQUIRED SDL2_image)



#include_directories(${GTK3_INCLUDE_DIRS})
#link_directories(${GTK3_LIBRARY_DIRS})
#
#add_definitions(${GTK3_CFLAGS_OTHER})

set(GCC_COVERAGE_COMPILE_FLAGS "-Wall" )
SET(GCC_COVERAGE_LINK_FLAGS    "-lm -lSDL2_image")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_executable(NESX main.c 6502.c nes.c 6502.h ppu.c ppu.h interface.c interface.h)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})

#target_link_libraries(NESX ${GTK3_LIBRARIES})