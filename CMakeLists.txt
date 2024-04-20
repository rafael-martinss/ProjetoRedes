cmake_minimum_required(VERSION 3.22)

project(socket-connector C ASM)

set(CMAKE_C_STANDARD 11)

if(NOT DEFINED CLIENT_MODE )
  set(CLIENT_MODE  "FALSE")
endif()

# For client case, it is necessary to indicate the arm gcc
if(CLIENT_MODE MATCHES "TRUE")
  add_definitions(-DCLIENT_MODE)
  set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc-10)
  message("Buiding for client mode")
else()
  message("Buiding for server mode")
endif()

set( SOURCES
        main.c
        mpu6050.c
        tcp.c
        thread_wrapper.c
        )
        
set( HEADERS
        mpu6050.h
        tcp.h
        thread_wrapper.h
)


add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} )

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
