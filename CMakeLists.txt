cmake_minimum_required(VERSION 3.10)

project(SocketServer)
# set(CMAKE_CXX_COMPILER:FILEPATH "D:\\DEV\\VS2022\\VC\\Tools\\MSVC\\14.42.34433\\bin\\Hostx64\\x64\\cl.exe")
set(OPENSSL_ROOT_DIR "D:\\DEV\\vcpkg\\packages\\openssl_x64-windows")

# add_definitions("/wd4244")

find_package(OpenSSL REQUIRED)
# set(OPENSSL_INCLUDE_DIR "D:/DEV/OpenSSL/include")
file(GLOB SRC_CPP "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB SRC_HPP "${CMAKE_SOURCE_DIR}/src/*.hpp")

add_executable(SocketServer main.cpp ${SRC_CPP} ${SRC_HPP})
# add_executable(SocketServer main.cpp )
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(SocketServer PRIVATE ws2_32)
target_link_libraries(SocketServer PRIVATE wsock32)
target_link_libraries(SocketServer PRIVATE OpenSSL::SSL )
target_link_libraries(SocketServer PRIVATE OpenSSL::Crypto)