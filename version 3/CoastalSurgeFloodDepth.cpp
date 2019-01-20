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
#include "./Utilities/ReadGraphsFromFile.h"

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

	const float OUT_NO_DATA_VAL = 0.0;
	const float OUT_NOT_VISITED_VAL = -1.0;
	const float DIAG_DIST = std::sqrt(2.0);
	const float DIRECT_DIAG_DECAY = std::pow(params.direct_attenuation, DIAG_DIST);
	const float INDIRECT_DIAG_DECAY = std::pow(params.indirect_attenuation, DIAG_DIST);


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
        //boost::filesystem::copy_file(params.dem_file.second, output_path.second);
        //boost::shared_ptr<raster_util::gdal_raster<float> > output_map =
                //raster_util::open_gdal_rasterSPtr<float>(output_path.second, GA_Update);
		boost::shared_ptr<raster_util::gdal_raster<float> > output_map = 
			raster_util::create_gdal_rasterSPtr_from_model<float, float>(output_path.second, 
				*dem_map, blink::raster::native_gdal_data_type<float>::type);
		const_cast<GDALRasterBand *>(output_map->get_gdal_band())->SetNoDataValue(OUT_NO_DATA_VAL);

		int cols = output_map->nCols();
		int rows = output_map->nRows();
		unsigned long num_cells = cols;
		num_cells *= rows;
		boost::progress_display show_progress1(rows);

        //Change all land pixels to unprocessed value flag
		for (unsigned int i = 0; i < rows; ++i) 
		{
			++show_progress1;
			for (unsigned int j = 0; j < cols; ++j) 
			{
				output_map->put(Coord(i, j), OUT_NOT_VISITED_VAL);
			}
		}

        /*boost::progress_display show_progress1(output_map->nRows() * output_map->nCols());
        auto zip = raster_it::make_zip_range(std::ref(*output_map));
        for (auto i : zip)
        {
            auto &val = std::get<0>(i);
            if (val != dem_null_value) val = 0;
            ++show_progress1;
        }*/


        std::cout << "\n\n*************************************\n";
        std::cout << "*Reading surge front data:*\n";
        std::cout << "*************************************" << std::endl;
        //SurgePixelSPtrVecSPtr surge_pixels = loadSurgePixelVector(surge_front_path.second);
		/**********************************/
		/*       Create graph object      */
		/**********************************/
		Graph channel_grph;
		SurgePixelSPtrVecSPtr surge_pixels(new std::vector<SurgePixelSPtr>);

		/**********************************/
		/*         Read in Graph           */
		/**********************************/
		readGraphFromFile(surge_front_path.second, channel_grph);
		VertexIterator di, dj;
		boost::tie(di, dj) = boost::vertices(channel_grph);
		boost::progress_display show_progress2(boost::num_vertices(channel_grph));
		for (; di != dj; ++di)
		{
			ChannelNode & node = channel_grph[*di];
			if (node.level > 0) {
				SurgePixelSPtr surge_px(new SurgePixel(
					raster_util::coordinate_2d(node.row, node.col),
					node.level,
					node.level,
					0
				));
				surge_pixels->push_back(surge_px);
			}
			++show_progress2;
		}


		int iteration_count = 0;
        while (!(surge_pixels->empty()))
        {
			std::cout << "ITERATION: " << ++iteration_count << "\n";
            std::cout << "\n\n*************************************\n";
            std::cout << "*      Finding adjacent pixels:     *\n";
            std::cout << "*************************************" << std::endl;
            //Find next front of pixels
            std::map<int, std::map<int, SurgePixelSPtrVecSPtr> > adjacent_pixels;
			int adjacent_count = 0;
			boost::progress_display show_progress3(surge_pixels->size());
            for(SurgePixelSPtr cell: *surge_pixels)
            {
				if (cell->surge_height_direct > 0.01 || cell->surge_height_indirect > 0.01)
				{

					int end_y = (int)(std::min(int(cell->loc.row) + 1, max_y));
					int begin_y = (int)std::max(int(cell->loc.row) - 1, 0);
					int end_x = (int)std::min(int(cell->loc.col) + 1, max_x);
					int begin_x = (int)std::max(int(cell->loc.col) - 1, 0);
					for (int ti = begin_y; ti <= end_y; ++ti)
					{
						for (int tj = begin_x; tj <= end_x; ++tj)
						{
							Coord nh_loc(ti, tj);
							auto elevation_in_adjacent = dem_map->get(nh_loc);
							auto surge_in_adjacent = output_map->get(nh_loc);
							if (elevation_in_adjacent != dem_null_value)
							{
								if (surge_in_adjacent == OUT_NOT_VISITED_VAL)
								{
									if (!(adjacent_pixels[ti][tj])) adjacent_pixels[ti][tj] = SurgePixelSPtrVecSPtr(
											new SurgePixelSPtrVec);
									adjacent_pixels[ti][tj]->push_back(cell);
									++adjacent_count;
								}
								else if (surge_in_adjacent < cell->surge_height_indirect) //Can visit cells already visited by other inundation paths, but condition means that infinite loops in an individual inundation path should not occur.
								{
									if (!(adjacent_pixels[ti][tj]))
											adjacent_pixels[ti][tj] = SurgePixelSPtrVecSPtr(
												new SurgePixelSPtrVec);
									adjacent_pixels[ti][tj]->push_back(cell);
									++adjacent_count;
								}
							}
						}
					}
				}
				++show_progress3;
            }

			if (adjacent_count == 0) std::cout << "Note: there were no cells adjacent that could potentially be inundated from current surge front\n";

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

            float direct_attenuation_factor;
            float indirect_attenuation_factor;
			boost::progress_display show_progress4(adjacent_count);
            for(Pr1 &pr1: adjacent_pixels)
                        {
                            for(Pr2 &pr2: pr1.second)
                                        {
                                            SurgePixelSPtr new_pixel(
                                                    new SurgePixel(raster_util::coordinate_2d(pr1.first,
                                                                                              pr2.first),
                                                                   0, 0, std::numeric_limits<float>::max()));
											//Iterate through adjacent cells which made up the previous front.
                                            for(SurgePixelSPtr adjacent_cell: *(pr2.second))
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
															++show_progress4;
                                                        }
                                            next_front->push_back(new_pixel);
                                        }
                        }


            std::cout << "\n\n*************************************\n";
            std::cout << "*Saving inundation and making new surge front *\n";
            std::cout << "*************************************" << std::endl;
            surge_pixels->clear();
			boost::progress_display show_progress5(next_front->size());
            for(SurgePixelSPtr &cell: *next_front)
                        {
                            float elevation = dem_map->get(cell->loc);
                            float surge_height = std::max(cell->surge_height_indirect, cell->surge_height_direct);
                            if (surge_height > elevation)
                            {
                                output_map->put(cell->loc, surge_height);
                                surge_pixels->push_back(cell);
                            }
							++show_progress5;
                        }
        }

		std::cout << "\n\n*************************************\n";
		std::cout << "*     Finished inundating, saving   *\n";
		std::cout << "*************************************" << std::endl;
		boost::progress_display show_progress6(rows);
		for (unsigned int i = 0; i < rows; ++i) {
			++show_progress6;
			for (unsigned int j = 0; j < cols; ++j) {
				Coord loc(i, j);
				auto elevation = dem_map->get(loc);
				auto level = output_map->get(loc);
				if ((level != OUT_NOT_VISITED_VAL))
				{
					auto inundation = level - elevation;
					if (inundation < 0)
					{
						std::cout << "Error logic" << std::endl;
						output_map->put(loc, OUT_NO_DATA_VAL);
					}
					output_map->put(loc, inundation);
				}
				else
				{
					output_map->put(loc, OUT_NO_DATA_VAL);
				}
				
			}
		}
    }


return (EXIT_SUCCESS);
}







