cmake_minimum_required(VERSION 3.16)
project(CursorOSApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(cursoros
  src/main.cpp
  src/DesktopShell.cpp
  src/DesktopShell.hpp
  src/FloatingWindow.cpp
  src/FloatingWindow.hpp
  src/Theme.hpp
)

target_link_libraries(cursoros PRIVATE Qt6::Widgets)

install(TARGETS cursoros RUNTIME DESTINATION bin)
