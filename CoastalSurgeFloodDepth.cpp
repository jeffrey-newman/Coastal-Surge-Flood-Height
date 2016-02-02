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

#include <GDAL.h>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>
#include <boost/foreach.hpp>

#include "Types.h"
#include "ReadInMap.h"
#include "Print.h"
#include "AddAdjacent.h"
#include "Neighbourhood.h"


int main(int argc, char **argv)
{

	/**********************************/
	/*        Program options         */
	/**********************************/
	// Need to specify elevation grid
	// Need to specify channel 

	std::string dem_file;
	std::string coast_demark_file;
	std::string log_file;
	std::string output_file;
	double flood_height;
	bool do_print = false;
	std::string file_print;

	namespace prog_opt = boost::program_options;
	prog_opt::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("flood-height,f", prog_opt::value<double>(&flood_height)->default_value(1.80), "height of flood (abs elevation)")
		("coast-demark-map,c", prog_opt::value<std::string>(&coast_demark_file), "path of the map that demarks where the boundary between the sea and land is")
		("dem-map,d", prog_opt::value<std::string>(&dem_file), "path of the gdal capatible elevation data file")
		("output-flood-height-map,o", prog_opt::value<std::string>(&output_file)->default_value("coastal_flood_height.tif"), "path of the output map where each pixel is assigned the flood height at that pixel")
		("log_file,l", prog_opt::value<std::string>(&log_file)->default_value("moves.log"), "path of the file which logs feature cell movements")
		("print,r", prog_opt::value<std::string>(&file_print), "Print map to text file (space seperated values) - specify file name prefix");

	prog_opt::variables_map vm;
	prog_opt::store(prog_opt::parse_command_line(argc, argv, desc), vm);
	prog_opt::notify(vm);
	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return 1;
	}
	if (vm.count("print"))
	{
		do_print = true;
	}

	boost::filesystem::path dem_file_path(dem_file);
	boost::filesystem::path coastal_demark_file_path(coast_demark_file);
	boost::filesystem::path output_file_path(output_file);

	// Check file exists
	if (!fs::exists(coastal_demark_file_path))
	{
		std::stringstream ss;
		ss << coastal_demark_file_path << " does not exist";
		throw std::runtime_error(ss.str());
		return (EXIT_FAILURE);
	}

	if (!fs::exists(dem_file_path))
	{
		std::stringstream ss;
		ss << dem_file_path << " does not exist";
		throw std::runtime_error(ss.str());
		return (EXIT_FAILURE);
	}

	/**********************************/
	/*         Read in maps           */
	/**********************************/
	std::cout << "\n\n*************************************\n";
	std::cout << "*             Read in maps          *\n";
	std::cout << "*************************************" << std::endl;
	//First the demarkation
	std::tuple<Map_Int_SPtr, std::string, GeoTransform> gdal_coastal_demark_map = read_in_map<int32_t>(coastal_demark_file_path, GDT_Int32, NO_CATEGORISATION);
	Map_Int_SPtr coastal_demark_map(std::get<0>(gdal_coastal_demark_map));
	std::string & coastal_demark_WKTprojection(std::get<1>(gdal_coastal_demark_map));
	GeoTransform & coastal_demark_Transform(std::get<2>(gdal_coastal_demark_map));

	// Second the elevation data
	std::tuple<boost::shared_ptr<Map_Matrix<float> >, std::string, GeoTransform> gdal_dem_map = read_in_map<float>(dem_file_path, GDT_Float32, NO_CATEGORISATION);
	boost::shared_ptr<Map_Matrix<float> > dem_map(std::get<0>(gdal_dem_map));
	std::string & demWKTprojection(std::get<1>(gdal_dem_map));
	GeoTransform & demTransform(std::get<2>(gdal_dem_map));


	//Check maps for consistency (same dimensions)
	if (coastal_demark_map->NCols() != dem_map->NCols())
	{
		throw std::runtime_error("Number of columns in the two comparison maps non-identical");
	}

	if (coastal_demark_map->NRows() != dem_map->NRows())
	{
		throw std::runtime_error("Number of rows in the two comparison maps non-identical");
	}

	//Should also check the dem as well.

	/*
	NOTATION OF MAPS COORDINATES
	(row, col)  ie. (y, x)
	positve direction to down and right.
	*/

	/**********************************/
	/*  Create list of surge front */
	/**********************************/
	std::cout << "\n\n*************************************\n";
	std::cout << "*  Making list of surge front pixels:   *\n";
	std::cout << "*************************************" << std::endl;
	boost::progress_display show_progress1((coastal_demark_map->NRows()*coastal_demark_map->NCols()));
	//std::multimap<double, Position> channel_pixels;
	std::stack<Position> surge_front_pixels;
	bool hasNoData = coastal_demark_map->HasNoDataValue();
	int32_t noDataVal = coastal_demark_map->NoDataValue();

	int max_y = coastal_demark_map->NRows() - 1;
	int max_x = coastal_demark_map->NCols() - 1;

	

	



	//Intiial front is the pixels on the land-sea boundary
	for ( int i = 0; i < coastal_demark_map->NRows(); ++i)
	{
		int end_y = (int) std::min(i + 1, max_y);
		int begin_y = (int) std::max(i - 1, 0);

		
		for ( int j = 0; j < coastal_demark_map->NCols(); ++j)
		{
			

			int32_t val = coastal_demark_map->Get(i, j);
			if (val == 0)	// IS Land
			{
				int end_x = (int)std::min(j + 1, max_x);
				int begin_x = (int)std::max(j - 1, 0);

				for (int ti = begin_y; ti <= end_y; ++ti)
				{
					for (int tj = begin_x; tj <= end_x; ++tj)
					{
						if (coastal_demark_map->Get(ti, tj) == 0) surge_front_pixels.push(Position(ti, tj));
						coastal_demark_map->Get(ti, tj) = 2;
					}
				}

				//process_adjacents(coastal_demark_map, surge_front_pixels, i, j);

				////explore neighbourhood. If any pixels with val = 0 {i.e. land} in neighbourhood, add these to the surge_front_pixel list
				//boost::shared_ptr<Set> nh = find_adjacents(coastal_demark_map,i, j, 1);

				//BOOST_FOREACH(Position & loc, *nh)
				//{
				//	if (coastal_demark_map->Get(loc.first, loc.second) == 0) surge_front_pixels.push(loc);
				//	coastal_demark_map->Get(i, j) = 2;
				//}
			}
			++show_progress1;
		}
	}


	/*******************************************************************/
	/*     Propagate surge front inland                                */
	/*******************************************************************/
	std::cout << "\n\n*************************************\n";
	std::cout << "*   Propagating surge front inland    *\n";
	std::cout << "*************************************" << std::endl;

	//Create output map.
	double null_value = -999;
	if (dem_map->HasNoDataValue()) null_value = dem_map->NoDataValue();
	Map_Double_SPtr output_map(new Map_Double(dem_map->NRows(), dem_map->NCols(), null_value));

	do
	{
		/*****************************************************
		*          Get top most Position from stack          *
		*****************************************************/
		Position loc = surge_front_pixels.top();
		size_t size = surge_front_pixels.size();
		surge_front_pixels.pop();

		/*****************************************************
		*          Calculate flood height      *
		*****************************************************/
		double pixel_elevation = dem_map->Get(loc.first, loc.second);
		if (pixel_elevation < flood_height)
		{
			output_map->Get(loc.first, loc.second) = (flood_height - pixel_elevation);
			/*****************************************************************************************
			*          Search neighbourhood (if pixel had water)     *
			******************************************************************************************/
			//explore neighbourhood. If any pixels with val = 0 {i.e. land} in neighbourhood, add these to the surge_front_pixel list
			boost::shared_ptr<Set> nh = find_adjacents(coastal_demark_map, loc.first, loc.second, 1);
			BOOST_FOREACH(Position & nh_loc, *nh)
			{
				if ((coastal_demark_map->Get(nh_loc.first, nh_loc.second) == 0) && (output_map->Get(nh_loc.first, nh_loc.second) != null_value))
				{
					surge_front_pixels.push(nh_loc);
				}
			}
		}
		else
		{
			output_map->Get(loc.first, loc.second) = 0;
		}

	} while (!surge_front_pixels.empty());

	//Where the flood elevation is 0, make these null valued cells
	for (unsigned int i = 0; i < output_map->NRows(); ++i)
	{
		for (unsigned int j = 0; j < output_map->NCols(); ++j)
		{
			double val = output_map->Get(i, j);
			if (val == 0)	output_map->Get(i, j) = null_value;
		}
	}
	

	/********************************************/
	/* Print resultent DEM					    */
	/********************************************/
	std::cout << "\n\n*************************************\n";
	std::cout << "*         Saving output file        *\n";
	std::cout << "*************************************" << std::endl;
	std::string driverName = "GTiff";
	write_map(output_file_path, GDT_Float64, output_map, demWKTprojection, demTransform, driverName);

	return (EXIT_SUCCESS);
}

