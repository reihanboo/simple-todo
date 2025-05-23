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
    # On Windows, try to use PDCurses or similar
    find_library(CURSES_LIBRARY 
        NAMES pdcurses curses
        HINTS ${CMAKE_SOURCE_DIR}/lib
    )
    if(CURSES_LIBRARY)
        target_link_libraries(tdui ${CURSES_LIBRARY} ${PLATFORM_LIBS})
    else()
        message(WARNING "No curses library found for Windows. tdui may not build.")
        target_link_libraries(tdui ${PLATFORM_LIBS})
    endif()
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