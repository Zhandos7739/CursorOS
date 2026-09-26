cmake_minimum_required(VERSION 3.16)
project(CursorOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CursorOS WIN32 src/CursorOS.cpp)
target_compile_definitions(CursorOS PRIVATE UNICODE _UNICODE)

if (MSVC)
  target_compile_options(CursorOS PRIVATE /W4 /utf-8)
else()
  target_compile_options(CursorOS PRIVATE -Wall -Wextra)
  target_link_options(CursorOS PRIVATE -static-libgcc -static-libstdc++ -mwindows)
endif()

target_link_libraries(CursorOS PRIVATE user32 gdi32 msimg32 shell32)

install(TARGETS CursorOS RUNTIME DESTINATION bin)
