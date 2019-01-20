project(coastal-surge-inundation)
cmake_minimum_required(VERSION 3.1)
set (CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED COMPONENTS filesystem system program_options serialization iostreams graph)
find_package(GDAL REQUIRED)
find_package(ZLIB REQUIRED)

set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    ON)

set(AHHZ_RASTER_UTIL "Not Specified" CACHE PATH "Path to Alex Hagen-Zanker's Raster Utility Include direction (select above blink)")
set(AHHZ_RASTER_ITERATOR "Not Specified" CACHE PATH "Path to Alex Hagen-Zanker's Raster Iterator Include direction (select folder above blink)")

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

#message(STATUS "AHHZ raster path is:" ${AHHZ_RASTER_UTIL})
#message(STATUS "AHHZ iterator path is:" ${AHHZ_RASTER_ITERATOR})

file(GLOB_RECURSE AHHZ_Utils
"${AHHZ_RASTER_UTIL}/*.h"
"${AHHZ_RASTER_UTIL}/*.hpp"
"${AHHZ_RASTER_UTIL}/*.cpp"
"${AHHZ_RASTER_ITERATOR}/*.h"
"${AHHZ_RASTER_ITERATOR}/*.hpp"
"${AHHZ_RASTER_ITERATOR}/*.cpp"
)
#message(STATUS "AHHZ files found:" ${AHHZ_Utils})

SET(Boost_LIBRARIES_used
        Boost::system
        Boost::filesystem
        Boost::program_options
        Boost::serialization
        Boost::iostreams
        Boost::graph 
		# These last two options are required for the version of the boost libraries I obtained on Windows. They may need to be uncommented for other systems/versions of boost. 
        Boost::disable_autolinking
		Boost::dynamic_linking
)

add_executable(coastal-inundation ${CoastInundateSrcs} ${CoastInundateHdrs} ${AHHZ_Utils})
target_include_directories(coastal-inundation PUBLIC ${GDAL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${AHHZ_RASTER_UTIL} ${AHHZ_RASTER_ITERATOR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(coastal-inundation ${GDAL_LIBRARY}  ${Boost_LIBRARIES_used} ${ZLIB_LIBRARIES})

add_subdirectory("Utilities")