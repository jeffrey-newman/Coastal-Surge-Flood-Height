SET(InfillSpuriousNullSrcs
        InfillSpuriousNulls.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(infillspuriousnulls ${InfillSpuriousNullSrcs} ${AHHZ_Utils})
target_include_directories(infillspuriousnulls PUBLIC ${GDAL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${AHHZ_RASTER_UTIL} ${AHHZ_RASTER_ITERATOR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(infillspuriousnulls ${GDAL_LIBRARY} ${Boost_LIBRARIES_used} )

SET(CoastalBoundarySrcs
        CoastalBoundary.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(coastalboundary ${CoastalBoundarySrcs} ${AHHZ_Utils})
target_include_directories(coastalboundary PUBLIC ${GDAL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${AHHZ_RASTER_UTIL} ${AHHZ_RASTER_ITERATOR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(coastalboundary ${GDAL_LIBRARY} ${Boost_LIBRARIES_used} )

SET(CoastBoundaryGraphSrcs
        MakeCoastBoundaryGraph.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(coastboundarygraph ${CoastBoundaryGraphSrcs} ${AHHZ_Utils})
target_include_directories(coastboundarygraph PUBLIC ${GDAL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${AHHZ_RASTER_UTIL} ${AHHZ_RASTER_ITERATOR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(coastboundarygraph ${GDAL_LIBRARY} ${Boost_LIBRARIES_used} )

SET(MakeSurgeFrontSrcs
        MakeSurgeFront.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(surgefront ${MakeSurgeFrontSrcs} ${AHHZ_Utils})
target_include_directories(surgefront PUBLIC ${GDAL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${AHHZ_RASTER_UTIL} ${AHHZ_RASTER_ITERATOR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(surgefront ${GDAL_LIBRARY} ${Boost_LIBRARIES_used} )

SET(MakeSurgeFront2Srcs
        MakeSurgeFront2.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(surgefront2 ${MakeSurgeFront2Srcs} ${AHHZ_Utils})
target_include_directories(surgefront2 PUBLIC ${GDAL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${AHHZ_RASTER_UTIL} ${AHHZ_RASTER_ITERATOR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(surgefront2 ${GDAL_LIBRARY} ${Boost_LIBRARIES_used} )