cmake_minimum_required(VERSION 3.16)
project(USBStorageMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(USBStorageMonitor
    src/main.cpp
    src/USBMonitor.cpp
    src/Logger.cpp
    src/EmailSender.cpp
    src/DeviceController.cpp
    src/WebServer.cpp
)

if(WIN32)
    target_link_libraries(USBStorageMonitor
        setupapi
        ws2_32
        wininet
        crypt32
        advapi32
    )
endif()

if(MSVC)
    target_compile_options(USBStorageMonitor PRIVATE /W4)
    target_compile_definitions(USBStorageMonitor PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

install(TARGETS USBStorageMonitor RUNTIME DESTINATION bin)
