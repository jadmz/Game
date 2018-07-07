cmake_minimum_required (VERSION 2.8.9)
project (Game)

IF(MSVC)
	set(SDL2_PATH "C:\\Users\\Joe\\Documents\\Libs\\SDL2-devel-2.0.8-VC\\SDL2-2.0.8\\")
	set(SDL2_IMAGE_PATH "C:\\Users\\Joe\\Documents\\Libs\\SDL2_image-2.0.3-VC\\SDL2_image-2.0.3\\")
ELSEIF (APPLE)
	set(CMAKE_CXX_FLAGS_DEBUG  "-g")
ENDIF(APPLE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

message(${CMAKE_MODULE_PATH})

set (Game_VERSION_MAJOR 1)
set (Game_VERSION_MINOR 0)

find_file(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

message("SDL Include Dir: ${SDL2_INCLUDE_DIR}")
message("SDL Libraries: ${SDL2_LIBRARY}")
message("SDL Image Include Dir: ${SDL2_IMAGE_INCLUDE_DIR}")
message("SDL Image Libraries: ${SDL2_IMAGE_LIBRARIES}")


# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/include/GameConfig.h.in"
  "${PROJECT_BINARY_DIR}/include/GameConfig.h"
  )

include_directories(
	"${PROJECT_SOURCE_DIR}/include" 
	"${PROJECT_BINARY_DIR}/include"
	"${SDL2_INCLUDE_DIR}"
	"${SDL2_IMAGE_INCLUDE_DIR}")

file(GLOB SOURCES "src/*.cpp")

add_executable(Game ${SOURCES})

target_link_libraries(Game 
	${SDL2_LIBRARY} 
	${SDL2_IMAGE_LIBRARIES})

link_directories(
	${SDL2_LIBRARY} 
	${SDL2_IMAGE_LIBRARIES})	

