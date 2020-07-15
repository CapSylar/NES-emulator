cmake_minimum_required(VERSION 3.16)
project(NESX C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2IMAGE REQUIRED SDL2_image)

set(GCC_COVERAGE_COMPILE_FLAGS "-Wall" )
SET(GCC_COVERAGE_LINK_FLAGS    "-lm -lSDL2_image -lSDL2_ttf")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_executable(NESX main.c 6502.c nes.c 6502.h ppu.c ppu.h interface.c interface.h mapper0.c mapper.h mapper0.h card_load.c controller.c controller.h color_def.h mapper2.c mapper2.h mapper3.c mapper3.h)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})
