cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID -DSMALL_SCREEN -DSTYLUS_BASED -DNO_PRINTING -DCOMBINED -DVIVID_COLOURS")
include_directories(puzzles)
file(GLOB puzzles_SRC
        "*.c"
        "puzzles/*.c"
        )
list(REMOVE_ITEM puzzles_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/emcc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/gtk.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/list.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/nestedvm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/nullfe.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/nullgame.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/ps.c
        ${CMAKE_CURRENT_SOURCE_DIR}/puzzles/windows.c
        )
add_library(puzzles SHARED ${puzzles_SRC})
# Only executables with library-ish filenames are included in the APK and unpacked on devices
add_executable(libpuzzlesgen.so "executable/android-gen.c")
target_link_libraries(libpuzzlesgen.so puzzles)