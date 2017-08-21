//
//  neighbourhood.h
//  move_creek
//
//  Created by a1091793 on 23/01/2015.
//  Copyright (c) 2015 University of Adelaide. All rights reserved.
//

#ifndef __move_creek__neighbourhood__
#define __move_creek__neighbourhood__

#include "Types.h"
#include <unordered_set>
#include <cmath>
#include <boost/shared_ptr.hpp>
#include <blink/raster/gdal_raster.h>

namespace raster_util = blink::raster;

//inline double
//euclidean_distance(int i_x, int i_y, int j_x, int j_y)
//{
//    int dx = abs(i_x - j_x);
//    int dy = std::abs(i_y - j_y);
//    return (std::sqrt((double)(dx*dx + dy*dy)));
//}

//template <typename DataType>
//boost::shared_ptr<Set> get_neighbourhood(boost::shared_ptr<Map_Matrix<DataType> > map, unsigned long i, unsigned long j, unsigned long size);

struct HashCoordinate2D
{
    size_t operator()(const raster_util::coordinate_2d &coord) const
    {
        return std::hash<long>()(coord.row) ^ std::hash<long>()(coord.col);
    }
};

typedef std::unordered_set<raster_util::coordinate_2d, HashCoordinate2D> CooridinateSet;

template <typename DataType>
boost::shared_ptr<CooridinateSet> find_adjacents(boost::shared_ptr<raster_util::gdal_raster<DataType> > map, unsigned long i, unsigned long j, unsigned long size, DataType noData);


//void inline
//process_adjacents(Map_Int_SPtr map, std::stack<Position> & surge_front, int i, int j);

#endif /* defined(__move_creek__neighbourhood__) */
