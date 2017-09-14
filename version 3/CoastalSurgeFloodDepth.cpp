// IdentifyCreekPixel.cpp : Defines the entry point for the console application.
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

#include <gdal.h>

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
    namespace filesys = boost::filesystem;
    namespace raster_util = blink::raster;
    namespace raster_it = blink::iterator;
    namespace fs = boost::filesystem;
    namespace io = boost::iostreams;
    typedef raster_util::coordinate_2d Coord;

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
        dem_null_value = -999;
    }
//    float unprocessed_flag = -998;
//    if (dem_null_value == -998) unprocessed_flag = -999;

    int max_y = (int)(dem_map->nRows() - 1);
    int max_x = (int)(dem_map->nCols() - 1);

//    std::cout << "\n\n*************************************\n";
//    std::cout <<     "*  Read in Coastal demarkation map  *\n";
//    std::cout <<     "*************************************" << std::endl;
//    boost::shared_ptr<raster_util::gdal_raster<int> > coast_demark_map = raster_util::open_gdal_rasterSPtr<int>
//            (params.coast_demark_file.second, GA_ReadOnly);


    for (int k = 0; k < params.surge_front_files_v.size(); ++k)
    {
        const CmdLinePaths &surge_front_path = params.surge_front_files_v[k];
        const CmdLinePaths &output_path = params.output_files_v[k];

        std::cout << "\n\n*************************************\n";
        std::cout << "* Calculating inundation for: " << surge_front_path.first << " *\n";
        std::cout << "*************************************" << std::endl;


        std::cout << "\n\n*************************************\n";
        std::cout << "*      Setting up output raster     *\n";
        std::cout << "*************************************" << std::endl;
        boost::filesystem::copy_file(params.dem_file.second, output_path.second);
        boost::shared_ptr<raster_util::gdal_raster<float> > output_map =
                raster_util::open_gdal_rasterSPtr<float>(output_path.second, GA_Update);

        //Change all land pixels to unprocessed value flag
        boost::progress_display show_progress1(output_map->nRows() * output_map->nCols());
        auto zip = raster_it::make_zip_range(std::ref(*output_map));
        for (auto i : zip)
        {
            auto &val = std::get<0>(i);
            if (val != dem_null_value) val = 0;
            ++show_progress1;
        }


        std::cout << "\n\n*************************************\n";
        std::cout << "*Reading list of surge front pixels:*\n";
        std::cout << "*************************************" << std::endl;
        SurgePixelSPtrVecSPtr surge_pixels = loadSurgePixelVector(surge_front_path.second);

        while (!(surge_pixels->empty()))
        {

            std::cout << "\n\n*************************************\n";
            std::cout << "*      Finding adjacent pixels:     *\n";
            std::cout << "*************************************" << std::endl;
            //Find next front of pixels
            std::map<int, std::map<int, SurgePixelSPtrVecSPtr> > adjacent_pixels;
            BOOST_FOREACH(SurgePixelSPtr cell, *surge_pixels)
                        {


                            int end_y = (int) (std::min(int(cell->loc.row) + 1, max_y));
                            int begin_y = (int) std::max(int(cell->loc.row) - 1, 0);
                            int end_x = (int) std::min(int(cell->loc.col) + 1, max_x);
                            int begin_x = (int) std::max(int(cell->loc.col) - 1, 0);
                            for (int ti = begin_y; ti <= end_y; ++ti)
                            {
                                for (int tj = begin_x; tj <= end_x; ++tj)
                                {
                                    Coord nh_loc(ti, tj);
                                    auto surge_in_adjacent = output_map->get(nh_loc);
                                    if (surge_in_adjacent != dem_null_value)
                                    {
                                        if (surge_in_adjacent == 0)
                                        {
                                            adjacent_pixels[ti][tj]->push_back(cell);
                                        }
                                        else if (surge_in_adjacent < cell->surge_height_indirect)
                                        {
                                            adjacent_pixels[ti][tj]->push_back(cell);
                                        }
                                    }
                                }
                            }
                        }

            std::cout << "\n\n*************************************\n";
            std::cout << "*Finding direct&indirect inundation *\n";
            std::cout << "*************************************" << std::endl;
            //Calculate the shortest (direct) distance inundation and indirect inundation
            SurgePixelSPtrVecSPtr next_front(new std::vector<SurgePixelSPtr>);
            typedef std::pair<const int, std::map<int, SurgePixelSPtrVecSPtr> > Pr1;
            typedef std::pair<const int, SurgePixelSPtrVecSPtr> Pr2;
            float indirect_surge_height;
            float direct_surge_height;
            float dist;
            const float DIAG_DIST = std::sqrt(2.0);
            const float DIRECT_DIAG_DECAY = std::pow(params.direct_attenuation, DIAG_DIST);
            const float INDIRECT_DIAG_DECAY = std::pow(params.indirect_attenuation, DIAG_DIST);
            float direct_attenuation_factor;
            float indirect_attenuation_factor;
            BOOST_FOREACH(Pr1 &pr1, adjacent_pixels)
                        {
                            BOOST_FOREACH(Pr2 &pr2, pr1.second)
                                        {
                                            SurgePixelSPtr new_pixel(
                                                    new SurgePixel(raster_util::coordinate_2d(pr1.first,
                                                                                              pr2.first),
                                                                   0, 0, std::numeric_limits<float>::max()));
                                            BOOST_FOREACH(SurgePixelSPtr adjacent_cell, *(pr2.second))
                                                        {

                                                            if (adjacent_cell->loc.row != pr1.first &&
                                                                adjacent_cell->loc
                                                                        .col != pr2.first)
                                                            {
                                                                dist = adjacent_cell->direct_distance + DIAG_DIST;
                                                                direct_attenuation_factor = DIRECT_DIAG_DECAY;
                                                                indirect_attenuation_factor = INDIRECT_DIAG_DECAY;

                                                            }
                                                            else
                                                            {
                                                                dist = adjacent_cell->direct_distance + 1;
                                                                direct_attenuation_factor = params.direct_attenuation;
                                                                indirect_attenuation_factor = params.indirect_attenuation;
                                                            }
                                                            if (dist < new_pixel->direct_distance)
                                                            {
                                                                new_pixel->direct_distance = dist;
                                                                new_pixel->surge_height_direct =
                                                                        adjacent_cell->surge_height_direct *
                                                                        direct_attenuation_factor;
                                                            }
                                                            if (dist == new_pixel->direct_distance)
                                                            {
                                                                direct_surge_height =
                                                                        adjacent_cell->surge_height_direct *
                                                                                direct_attenuation_factor;
                                                                if (direct_surge_height > new_pixel->surge_height_direct)
                                                                {
                                                                    new_pixel->surge_height_direct = direct_surge_height;
                                                                }
                                                            }
                                                            indirect_surge_height =
                                                                    adjacent_cell->surge_height_indirect *
                                                                    indirect_attenuation_factor;
                                                            if (indirect_surge_height > new_pixel->surge_height_indirect)
                                                            {
                                                                new_pixel->surge_height_indirect = indirect_surge_height;
                                                            }
                                                        }
                                            next_front->push_back(new_pixel);
                                        }
                        }


            std::cout << "\n\n*************************************\n";
            std::cout << "*Saving inundation and making new surge front *\n";
            std::cout << "*************************************" << std::endl;
            surge_pixels->clear();
            BOOST_FOREACH(SurgePixelSPtr &cell, *next_front)
                        {
                            float elevation = dem_map->get(cell->loc);
                            float surge_height = std::max(cell->surge_height_indirect, cell->surge_height_direct);
                            if (surge_height > elevation)
                            {
                                output_map->put(cell->loc, surge_height);
                                surge_pixels->push_back(cell);
                            }
                        }
        }
    }


return (EXIT_SUCCESS);
}







