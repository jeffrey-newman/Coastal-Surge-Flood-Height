project(coastal-surge-inundation)
cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS filesystem system program_options serialization iostreams)
find_package(GDAL REQUIRED)
find_package(ZLIB REQUIRED)

set(AHHZ_RASTER_UTIL "Not Specified" CACHE PATH "Path to Alex Hagen-Zanker's Raster Utility Include direction (select above blink)")
set(AHHZ_RASTER_ITERATOR "Not Specified" CACHE PATH "Path to Alex Hagen-Zanker's Raster Iterator Include direction (select folder above blink)")

include_directories(${GDAL_INCLUDE_DIR} )
include_directories(${Boost_INCLUDE_DIRS} )
include_directories(${AHHZ_RASTER_UTIL} )
include_directories(${AHHZ_RASTER_ITERATOR} )
include_directories(${ZLIB_INCLUDE_DIRS})


set(CoastInundateSrcs
        CoastalSurgeFloodDepth.cpp
        CoastalSurgeParameters.cpp
        Pathify.cpp
    )

set(CoastInundateHdrs
        Types.h
        CoastalSurgeParameters.hpp
        GZArchive.hpp
        Pathify.hpp
)

message(STATUS "AHHZ raster path is:" ${AHHZ_RASTER_UTIL})
message(STATUS "AHHZ iterator path is:" ${AHHZ_RASTER_ITERATOR})

file(GLOB_RECURSE AHHZ_Utils
"${AHHZ_RASTER_UTIL}/*.h"
"${AHHZ_RASTER_UTIL}/*.hpp"
"${AHHZ_RASTER_UTIL}/*.cpp"
"${AHHZ_RASTER_ITERATOR}/*.h"
"${AHHZ_RASTER_ITERATOR}/*.hpp"
"${AHHZ_RASTER_ITERATOR}/*.cpp"
)
message(STATUS "AHHZ files found:" ${AHHZ_Utils})

add_executable(coastal-inundation ${CoastInundateSrcs} ${CoastInundateHdrs} ${AHHZ_Utils})
target_link_libraries(coastal-inundation ${GDAL_LIBRARY} ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})

add_subdirectory("Utilities")