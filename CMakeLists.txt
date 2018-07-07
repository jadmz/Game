cmake_minimum_required (VERSION 2.8.9)
set(CMAKE_CXX_FLAGS_DEBUG  "-g")
project (Game)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

message(${CMAKE_MODULE_PATH})

set (Game_VERSION_MAJOR 1)
set (Game_VERSION_MINOR 0)

find_file(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)


# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/GameConfig.h.in"
  "${PROJECT_BINARY_DIR}/include/GameConfig.h"
  )

include_directories(
	"${PROJECT_SOURCE_DIR}/include" 
	"${PROJECT_BINARY_DIR}/include"
	"${SDL2_INCLUDE_DIRS}"
	"${SDL2_IMAGE_INCLUDE_DIR}")

file(GLOB SOURCES "src/*.cpp")

string(STRIP ${SDL2_INCLUDE_DIRS} SDL2_INCLUDE_DIRS)
string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)

add_executable(Game ${SOURCES})

target_link_libraries(Game ${SDL2_LIBRARIES})    
target_link_libraries(Game ${SDL2_IMAGE_LIBRARIES})    

