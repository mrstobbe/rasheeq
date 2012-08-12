cmake_minimum_required(VERSION 2.8)
project(rasheeq)

include(CheckIncludeFiles)


check_include_files(sys/epoll.h RASHEEQ_HAVE_EPOLL)
if(NOT RASHEEQ_HAVE_EPOLL)
	SET(RASHEEQ_HAVE_EPOLL 0)
endif()
check_include_files(netinet/sctp.h RASHEEQ_HAVE_SCTP)
if(NOT RASHEEQ_HAVE_SCTP)
	SET(RASHEEQ_HAVE_SCTP 0)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#TODO: Dynamically change to lib64 as needed... or maybe just on target
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_definitions(-DRASHEEQ_HAVE_EPOLL=${RASHEEQ_HAVE_EPOLL} -DRASHEEQ_HAVE_SCTP=${RASHEEQ_HAVE_SCTP})
set(CMAKE_CXX_FLAGS "-std=c++0x")
if (CMAKE_BUILD_TYPE MATCHES "Debug")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -ggdb")
elseif (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
	set(CMAKE_CXX_FLAGS_DEBUG "-O3 -ggdb")
elseif (CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
	set(CMAKE_CXX_FLAGS_DEBUG "-Os")
else()
	set(CMAKE_BUILD_TYPE "Release")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()
message("** Configured for ${CMAKE_BUILD_TYPE}")

add_subdirectory(src)

