CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(server)

# specify the C++ standard
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_library(timer src/timer.cpp)
add_executable(server src/server.cpp)

target_link_libraries(server PUBLIC timer)