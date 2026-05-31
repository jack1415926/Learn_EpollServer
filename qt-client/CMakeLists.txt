cmake_minimum_required(VERSION 3.16)

project(EpollQtClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

set(CLIENT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/TcpClient.cpp
    src/Protocol.cpp
)

set(CLIENT_HEADERS
    src/MainWindow.h
    src/TcpClient.h
    src/Protocol.h
    shared/protocol_types.h
)

add_executable(EpollQtClient ${CLIENT_SOURCES} ${CLIENT_HEADERS})

target_include_directories(EpollQtClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
)

target_link_libraries(EpollQtClient PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    set_target_properties(EpollQtClient PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
