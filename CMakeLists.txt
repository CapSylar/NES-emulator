cmake_minimum_required(VERSION 3.16)
project(NESX C)

set(CMAKE_C_STANDARD 11)

add_executable(NESX main.c 6502.c nes.c 6502.h ppu.c ppu.h)