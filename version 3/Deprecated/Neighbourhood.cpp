//
//  neighbourhood.cpp
//  move_creek
//
//  Created by a1091793 on 23/01/2015.
//  Copyright (c) 2015 University of Adelaide. All rights reserved.
//

#include "Neighbourhood.h"
#include <stdio.h>
#include <cmath>
#include <algorithm>



//template <typename DataType>
//boost::shared_ptr<Set> get_neighbourhood(boost::shared_ptr<Map_Matrix<DataType> > map, unsigned long i, unsigned long j, unsigned long size)
//{
//    int ii = (int) i;
//    int jj = (int) j;
//    int sizei = (int) size;
//    boost::shared_ptr<Set> neighbourhood(new Set);
//    DataType noData = map->NoDataValue();
//    
//    int end_y = (int) std::min(ii+sizei, (int) (map->NRows() - 1) );
//    int begin_y = (int) std::max(ii-sizei, 0);
//    
//    int end_x = (int) std::min(jj+sizei, (int) (map->NCols() - 1) );
//    int begin_x = (int) std::max(jj-sizei, 0);
//    
//    for (int ti = begin_y; ti <= end_y; ++ti)
//    {
//        for (int tj = begin_x; tj <= end_x; ++tj)
//        {
//            if (euclidean_distance(ti, tj, ii, jj) < double(sizei) && map->Get(ti, tj) != noData ) neighbourhood->push_back(std::make_pair(ti, tj));
//        }
//    }
//    return (neighbourhood);
//}



template <typename DataType>
boost::shared_ptr<CooridinateSet> find_adjacents(boost::shared_ptr<raster_util::gdal_raster<DataType> > map, unsigned long i, unsigned long j, unsigned long size, DataType noData)
{
    int ii = (int) i;
    int jj = (int) j;
    int radius = (int) size;
    boost::shared_ptr<CooridinateSet> adjacents(new CooridinateSet);
//    DataType noData = map->NoDataValue();
    
	int end_y = (int)std::min(ii + radius, (int)(map->nRows() - 1));
	int begin_y = (int)std::max(ii - radius, 0);
    
	int end_x = (int)std::min(jj + radius, (int)(map->nCols() - 1));
	int begin_x = (int)std::max(jj - radius, 0);
    
    for (int ti = begin_y; ti <= end_y; ++ti)
    {
        for (int tj = begin_x; tj <= end_x; ++tj)
        {
            raster_util::coordinate_2d coord = {ti, tj};
            DataType val = (DataType) map->get(coord);
            if (val != noData) adjacents->insert(coord);
        }
    }
    return (adjacents);
}


;

//void inline
//process_adjacents(Map_Int_SPtr map, std::stack<Position> & surge_front, int i, int j)
//{
//	int noData = map->NoDataValue();
//
//	int end_y = (int)std::min(i + 1, (int)(map->NRows() - 1));
//	int begin_y = (int)std::max(i - 1, 0);
//
//	int end_x = (int)std::min(j + 1, (int)(map->NCols() - 1));
//	int begin_x = (int)std::max(j - 1, 0);
//
//	for (int ti = begin_y; ti <= end_y; ++ti)
//	{
//		for (int tj = begin_x; tj <= end_x; ++tj)
//		{
//			if (map->Get(ti, tj) == 0) surge_front.push(Position(ti,tj));
//			map->Get(ti, tj) = 2;
//		}
//	}
//}