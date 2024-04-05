cmake_minimum_required(VERSION 3.22)

project(socket-connector C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set( SOURCES
        main.c
        tcp.c
        thread_wrapper.c
        )
        
set( HEADERS
        tcp.h
        thread_wrapper.h
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} )

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
# target_link_libraries(${CMAKE_PROJECT_NAME}
# )