CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(msgpack C CXX)

SET(VERSION "0.5.7")

STRING(REGEX REPLACE "([0-9]*)\\.([0-9]*).*" "\\1" VERSION_MAJOR "${VERSION}")
STRING(REGEX REPLACE "([0-9]*)\\.([0-9]*).*" "\\2" VERSION_MINOR "${VERSION}")
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/msgpack/version.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/msgpack/version.h"
	)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/include")

SET(SOURCE_FILES
	src/gcc_atomic.cpp
	src/gcc_atomic.h
	src/msgpack
	src/msgpack.h
	src/msgpack.hpp
	src/object.cpp
	src/objectc.c
	src/unpack.c
	src/version.c
	src/vrefbuffer.c
	src/zone.c
	)
ADD_LIBRARY(msgpack STATIC ${SOURCE_FILES})
SET_TARGET_PROPERTIES(msgpack PROPERTIES DEBUG_POSTFIX d)

INSTALL(
	TARGETS msgpack
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	)
INSTALL(
	DIRECTORY src/
	DESTINATION include
	FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hpp"
		PATTERN "gcc_atomic.h" EXCLUDE
	)
INSTALL(
	DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/"
	DESTINATION include
	)
