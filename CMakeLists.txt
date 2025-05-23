cmake_minimum_required(VERSION 3.10)
project(simple-todo C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(lib)
include_directories(src)

# SQLite3 library from lib directory
add_library(sqlite3 STATIC lib/sqlite3.c)
target_include_directories(sqlite3 PUBLIC lib)

# Platform-specific libraries and settings
if(WIN32)
    # Windows-specific libraries
    set(PLATFORM_LIBS ws2_32)
    # Add Windows console subsystem for better terminal support
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
elseif(UNIX)
    # Unix/Linux libraries
    set(PLATFORM_LIBS pthread dl m)
    
    # Find ncurses for tdui
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(NCURSES ncurses)
    endif()
    
    if(NOT NCURSES_FOUND)
        find_library(NCURSES_LIBRARY 
            NAMES ncurses curses
            HINTS /usr/lib /usr/local/lib
        )
        find_path(NCURSES_INCLUDE_DIR 
            NAMES curses.h ncurses.h
            HINTS /usr/include /usr/local/include /usr/include/ncurses
        )
        
        if(NCURSES_LIBRARY AND NCURSES_INCLUDE_DIR)
            set(NCURSES_FOUND TRUE)
            set(NCURSES_LIBRARIES ${NCURSES_LIBRARY})
            set(NCURSES_INCLUDE_DIRS ${NCURSES_INCLUDE_DIR})
        endif()
    endif()
    
    if(NOT NCURSES_FOUND)
        message(WARNING "ncurses not found. tdui may not build properly.")
        set(NCURSES_LIBRARIES "")
        set(NCURSES_INCLUDE_DIRS "")
    endif()
endif()

# td executable (command line interface)
add_executable(td 
    src/td.c 
    src/db.c
)
target_link_libraries(td sqlite3 ${PLATFORM_LIBS})

# tdui executable (terminal UI)
add_executable(tdui 
    src/tdui.c
)

if(WIN32)
    add_library(pdcurses STATIC
        lib/PDCurses-3.9/pdcurses/addch.c
        lib/PDCurses-3.9/pdcurses/addchstr.c
        lib/PDCurses-3.9/pdcurses/addstr.c
        lib/PDCurses-3.9/pdcurses/attr.c
        lib/PDCurses-3.9/pdcurses/beep.c
        lib/PDCurses-3.9/pdcurses/bkgd.c
        lib/PDCurses-3.9/pdcurses/border.c
        lib/PDCurses-3.9/pdcurses/clear.c
        lib/PDCurses-3.9/pdcurses/color.c
        lib/PDCurses-3.9/pdcurses/debug.c
        lib/PDCurses-3.9/pdcurses/delch.c
        lib/PDCurses-3.9/pdcurses/deleteln.c
        lib/PDCurses-3.9/pdcurses/getch.c
        lib/PDCurses-3.9/pdcurses/getstr.c
        lib/PDCurses-3.9/pdcurses/getyx.c
        lib/PDCurses-3.9/pdcurses/inch.c
        lib/PDCurses-3.9/pdcurses/inchstr.c
        lib/PDCurses-3.9/pdcurses/initscr.c
        lib/PDCurses-3.9/pdcurses/inopts.c
        lib/PDCurses-3.9/pdcurses/insch.c
        lib/PDCurses-3.9/pdcurses/insstr.c
        lib/PDCurses-3.9/pdcurses/instr.c
        lib/PDCurses-3.9/pdcurses/kernel.c
        lib/PDCurses-3.9/pdcurses/keyname.c
        lib/PDCurses-3.9/pdcurses/mouse.c
        lib/PDCurses-3.9/pdcurses/move.c
        lib/PDCurses-3.9/pdcurses/outopts.c
        lib/PDCurses-3.9/pdcurses/overlay.c
        lib/PDCurses-3.9/pdcurses/pad.c
        lib/PDCurses-3.9/pdcurses/panel.c
        lib/PDCurses-3.9/pdcurses/printw.c
        lib/PDCurses-3.9/pdcurses/refresh.c
        lib/PDCurses-3.9/pdcurses/scanw.c
        lib/PDCurses-3.9/pdcurses/scr_dump.c
        lib/PDCurses-3.9/pdcurses/scroll.c
        lib/PDCurses-3.9/pdcurses/slk.c
        lib/PDCurses-3.9/pdcurses/termattr.c
        lib/PDCurses-3.9/pdcurses/touch.c
        lib/PDCurses-3.9/pdcurses/util.c
        lib/PDCurses-3.9/pdcurses/window.c
    )
    target_include_directories(pdcurses PUBLIC lib/PDCurses-3.9)
    target_compile_definitions(pdcurses PRIVATE PDC_WIDE)
    
    target_link_libraries(tdui pdcurses ${PLATFORM_LIBS})
else()
    # Unix/Linux
    if(NCURSES_FOUND)
        target_include_directories(tdui PRIVATE ${NCURSES_INCLUDE_DIRS})
        target_link_libraries(tdui ${NCURSES_LIBRARIES} ${PLATFORM_LIBS})
    else()
        # Fallback - try linking with common names
        target_link_libraries(tdui ncurses ${PLATFORM_LIBS})
    endif()
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(td PRIVATE -Wall -Wextra)
    target_compile_options(tdui PRIVATE -Wall -Wextra)
    target_compile_options(sqlite3 PRIVATE -w)  # Suppress warnings for SQLite
endif()

# Install targets
install(TARGETS td tdui DESTINATION bin)