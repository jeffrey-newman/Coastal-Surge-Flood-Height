//
//  Neighbourhood_Impl.cpp
//  move_creek
//
//  Created by Jeffrey Newman on 28/01/2015.
//  Copyright (c) 2015 University of Adelaide. All rights reserved.
//

#include <stdio.h>
#include "Types.h"
#include "Neighbourhood.cpp"

//template
//boost::shared_ptr<Set> get_neighbourhood(Map_Double_SPtr map, unsigned long i, unsigned long j, unsigned long size);

template
boost::shared_ptr<CooridinateSet> find_adjacents(boost::shared_ptr<raster_util::gdal_raster<float> > map, unsigned long i, unsigned long j, unsigned long size, float noData);

template
boost::shared_ptr<CooridinateSet> find_adjacents(boost::shared_ptr<raster_util::gdal_raster<int> > map, unsigned long i, unsigned long j, unsigned long size, int noData);

