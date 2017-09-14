SET(InfillSpuriousNullSrcs
        InfillSpuriousNulls.cpp
        ../Pathify.cpp
        )

ADD_EXECUTABLE(infillspuriousnulls ${InfillSpuriousNullSrcs} ${AHHZ_Utils})
target_link_libraries(infillspuriousnulls ${GDAL_LIBRARY} ${Boost_LIBRARIES} )

