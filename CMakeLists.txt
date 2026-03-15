cmake_minimum_required(VERSION 3.10)
project(Tools)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

############################################################
#                     Clear Screen                         #
############################################################
set(CLEAR_SOURCE
    src/clear/main.c
)

add_executable(clear ${CLEAR_SOURCE})
############################################################


############################################################
#                         Tree                             #
############################################################
set(TREE_SOURCE
    src/tree/main.c
)

add_executable(tree ${TREE_SOURCE})
############################################################

############################################################
#                        Touch                             #
############################################################
set(TOUCH_SOURCE
    src/touch/main.c
)

add_executable(touch ${TOUCH_SOURCE})
############################################################

set(ALL_TOOLS
    clear
    tree
    touch
)

foreach(tool ${ALL_TOOLS})
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${tool} PRIVATE DEBUG)
    endif()

    if(MSVC)
        target_compile_definitions(${tool} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${tool} PRIVATE /std:c11)
    endif()

    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${tool} PRIVATE -Wno-implicit-function-declaration)
    endif()
endforeach()
