cmake_minimum_required(VERSION 3.20)
project(slashopt)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DUNICODE -D_UNICODE)

add_library(mhook STATIC
        mhook/mhook-lib/mhook.cpp
        mhook/mhook-lib/mhook.h
        mhook/disasm-lib/cpu.c
        mhook/disasm-lib/cpu.h
        mhook/disasm-lib/disasm.c
        mhook/disasm-lib/disasm.h
        mhook/disasm-lib/disasm_x86.c
        mhook/disasm-lib/disasm_x86.h
        mhook/disasm-lib/disasm_x86_tables.h
        mhook/disasm-lib/misc.c
        mhook/disasm-lib/misc.h
)
target_include_directories(mhook PUBLIC mhook/mhook-lib)

add_library(slashopt SHARED DllMain.cpp DllMain.cpp)
target_link_libraries(slashopt mhook)
set_target_properties(slashopt PROPERTIES SUFFIX ".asi")
