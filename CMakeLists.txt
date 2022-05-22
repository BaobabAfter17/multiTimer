cmake_minimum_required(VERSION 3.0.0)
project(server)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# find and include boost library
find_package( Boost 1.40 COMPONENTS program_options REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )

# compile server executable
add_executable(server src/server.cpp src/timer.cpp)
