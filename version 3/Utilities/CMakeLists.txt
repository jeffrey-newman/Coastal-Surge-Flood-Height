SET(InfillSpuriousNullSrcs
        InfillSpuriousNulls.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(infillspuriousnulls ${InfillSpuriousNullSrcs} ${AHHZ_Utils})
target_link_libraries(infillspuriousnulls ${GDAL_LIBRARY} ${Boost_LIBRARIES} )

SET(CoastalBoundarySrcs
        CoastalBoundary.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(coastalboundary ${CoastalBoundarySrcs} ${AHHZ_Utils})
target_link_libraries(coastalboundary ${GDAL_LIBRARY} ${Boost_LIBRARIES} )

SET(CoastBoundaryGraphSrcs
        MakeCoastBoundaryGraph.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(coastboundarygraph ${CoastBoundaryGraphSrcs} ${AHHZ_Utils})
target_link_libraries(coastboundarygraph ${GDAL_LIBRARY} ${Boost_LIBRARIES} )

SET(MakeSurgeFrontSrcs
        MakeSurgeFront.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(surgefront ${MakeSurgeFrontSrcs} ${AHHZ_Utils})
target_link_libraries(surgefront ${GDAL_LIBRARY} ${Boost_LIBRARIES} )
