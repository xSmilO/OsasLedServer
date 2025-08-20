cmake_minimum_required(VERSION 3.10)

project(OsasLedServer)
# set(CMAKE_CXX_COMPILER:FILEPATH
# "D:\\DEV\\VS2022\\VC\\Tools\\MSVC\\14.42.34433\\bin\\Hostx64\\x64\\cl.exe")
# set(OPENSSL_ROOT_DIR "D:\\DEV\\vcpkg\\packages\\openssl_x64-windows")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# add_definitions("/wd4244")

find_package(OpenSSL REQUIRED)
# set(OPENSSL_INCLUDE_DIR "D:/DEV/OpenSSL/include")
file(GLOB SRC_CPP "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB SRC_HPP "${CMAKE_SOURCE_DIR}/src/*.hpp")

# add_executable(${PROJECT_NAME} main.cpp ${SRC_CPP} ${SRC_HPP})
add_executable(
  ${PROJECT_NAME}
  main.cpp src/LedController.cpp src/Pixel.cpp src/SerialPort.cpp
  include/SerialPort.hpp src/WebSocketServer.cpp)
# add_executable(SocketServer main.cpp )
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/utils")
# target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
# target_link_libraries(${PROJECT_NAME} PRIVATE wsock32)
# target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
# target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)
