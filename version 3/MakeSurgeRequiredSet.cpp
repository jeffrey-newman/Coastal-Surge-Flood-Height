//
//  MakeSurgeRequiredSet.cpp
//  coastal-surge-inundation
//
//  Created by a1091793 on 13/10/2016.
//
//

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <list>
#include <cmath>
#include <algorithm>
#include <set>
#include <limits>
#include <tuple>

#ifdef WITH_MAC_FRAMEWORK
#include <GDAL/cpl_string.h> // part of GDAL
#include <GDAL/gdal.h>       // part of GDAL
#else
#include <cpl_string.h> // part of GDAL
#include <gdal.h>       // part of GDAL
#endif

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>

#include <blink/raster/gdal_raster.h>
#include <blink/raster/utility.h> // To open rasters
#include <blink/iterator/zip_range.h>

#include "Types.h"
#include "CoastalSurgeParameters.hpp"

#include "GZArchive.hpp"

int main(int argc, char **argv)
{

    /**********************************/
    /*        Program options         */
    /**********************************/
    // Need to specify elevation grid
    // Need to specify channel
    CoastalSurgeParameters params;
    processCmdLineArguments(argc, argv, params);

    std::cout << "\n\n*************************************\n";
    std::cout <<     "*             Read in DEM map       *\n";
    std::cout <<     "*************************************" << std::endl;

    boost::shared_ptr< raster_util::gdal_raster<float> > dem_map = raster_util::open_gdal_rasterSPtr<float>
            (params.dem_file.second, GA_ReadOnly);

    bool dem_has_null = true;
    int success = 0;
    float dem_null_value = const_cast<GDALRasterBand *>(dem_map->get_gdal_band())->GetNoDataValue(&success);
    if (success == 0 )
    {
        dem_has_null = false;
        std::cout << "Error: DEM does not have a nodata field, which represents non-land sea areas\n";
        return EXIT_FAILURE;
    }

    //Create list ourselves from DEM
    // Assume no-data values are sea. Find cells adjacent to these.
    //Intiial front is the pixels on the land-sea boundary
    std::cout << "\n\n*************************************\n";
    std::cout << "*  Making list of surge front pixels:   *\n";
    std::cout << "*************************************" << std::endl;
    boost::progress_display show_progress1((dem_map->nRows()*dem_map->nCols()));

    bool hasNoData = true;
    int success = 0;
    float no_val = const_cast<GDALRasterBand *>(dem_map->get_gdal_band())->GetNoDataValue(&success);
    if (success == 0 ) hasNoData = false;


    int max_y = dem_map->nRows() - 1;
    int max_x = dem_map->nCols() - 1;
    SurgePixelSPtrVecSPtr surge_front_pixels(new SurgePixelSPtrVec);

    if (hasNoData)
    {
        //Intiial front is the pixels on the land-sea boundary
        for ( int i = 0; i < dem_map->nRows(); ++i)
        {
            int end_y = (int) std::min(i + 1, max_y);
            int begin_y = (int) std::max(i - 1, 0);

            for ( int j = 0; j < dem_map->nCols(); ++j)
            {
                float val = dem_map->get(Coord(i, j));
                if (val == no_val)	// IS Ocean. assumes ocean is no_val, and all land pixels have a value.
                {
                    // Want to check if there is land adjacent to this sea pixel.
                    int end_x = (int)std::min(j + 1, max_x);
                    int begin_x = (int)std::max(j - 1, 0);

                    for (int ti = begin_y; ti <= end_y; ++ti)
                    {
                        for (int tj = begin_x; tj <= end_x; ++tj)
                        {
                            if (dem_map->get(Coord(ti, tj)) != no_val)    /// if not no-val, assume to be land.
                            {
                                SurgePixelSPtr front_pixel(new SurgePixel(raster_util::coordinate_2d(i,j)));
                                surge_front_pixels->push_back(front_pixel);
                            }
                        }
                    }
                }
                ++show_progress1;
            }
        }
    }

    // save data to archive
    {
        // create and open a character archive for output
        std::ofstream ofs("surge-front-list.txt");
        boost::archive::text_oarchive oa(ofs);
        // write class instance to archive
        oa << surge_front_pixels;
        // archive and stream closed when destructors are called
    }
}
