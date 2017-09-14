//
// Created by a1091793 on 15/9/17.
//

#include <iostream>
#include <fstream>

#include <blink/raster/gdal_raster.h>
#include <blink/raster/utility.h> // To open rasters
#include <blink/iterator/zip_range.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>
#include <boost/program_options.hpp>

#include "../Pathify.hpp"

struct DEMInfillParameters
{
    DEMInfillParameters()
    {

    }

    CmdLinePaths input_dem_file;
    CmdLinePaths output_dem_file;
    float threshold;

};

std::pair<std::string, std::string> at_option_parser(std::string const&s)
{
    if ('@' == s[0])
        return std::make_pair(std::string("cfg-file"), s.substr(1));
    else
        return std::pair<std::string, std::string>();
}

void
processCmdLineArguments(int argc, char* argv[], DEMInfillParameters & _params) throw(std::runtime_error)
{
    namespace prog_opt = boost::program_options;
    prog_opt::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("input-dem-map,i", prog_opt::value<CmdLinePaths>(&_params.input_dem_file),
             "path of the gdal capatible elevation data file")
            ("output-dem-map,o", prog_opt::value<CmdLinePaths>(&_params.output_dem_file),
             "path of the gdal capatible elevation data file")
            ("threshold,t", prog_opt::value<float>(&_params.threshold),
             "threshold for infill if neighbouring pixel has height above this value")
            ("cfg-file,g", prog_opt::value<std::string>(),
             "can be specified with '@name', too")
            ;

    prog_opt::variables_map vm;
    prog_opt::store(prog_opt::command_line_parser(argc, argv).options(desc).extra_parser(at_option_parser).run(), vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return;
    }

    if (vm.count("cfg-file"))
    {
        // Load the file and tokenize it
        std::string cfg_file_path = vm["cfg-file"].as<std::string>();
        std::ifstream ifs(cfg_file_path.c_str());
        if (!ifs)
        {
            std::stringstream ss;
            ss << "Could not open the cfg file: " << cfg_file_path;
            throw std::runtime_error(ss.str());
        }
        prog_opt::store(prog_opt::parse_config_file(ifs, desc), vm);
    }

    prog_opt::notify(vm);

    try{
        pathify(_params.input_dem_file);
    }
    catch (boost::filesystem::filesystem_error & err)
    {
        std::stringstream ss;
        ss << "Filesystem error: " << err.what();
        throw std::runtime_error(ss.str());
    }

    try{
        pathify_mk(_params.output_dem_file);
    }
    catch (boost::filesystem::filesystem_error & err)
    {
        std::stringstream ss;
        ss << "Filesystem error: " << err.what();
        throw std::runtime_error(ss.str());
    }

}

int main(int argc, char* argv[])
{
    namespace raster_util = blink::raster;
    typedef raster_util::coordinate_2d Coord;

    DEMInfillParameters params;
    processCmdLineArguments(argc, argv, params);

    std::cout << "\n\n*************************************\n";
    std::cout << "*      Setting up output raster     *\n";
    std::cout << "*************************************" << std::endl;
    boost::filesystem::copy_file(params.input_dem_file.second, params.output_dem_file.second);
    boost::shared_ptr<raster_util::gdal_raster<float> > output_map =
            raster_util::open_gdal_rasterSPtr<float>(params.output_dem_file.second, GA_Update);

    bool dem_has_null = true;
    int success = 0;
    float dem_null_value = const_cast<GDALRasterBand *>(output_map->get_gdal_band())->GetNoDataValue(&success);
    if (success == 0 )
    {
        dem_has_null = false;
        dem_null_value = -999;
        std::cout << "DEM has no null values, cannot infill spurious ones\n";
        return EXIT_SUCCESS;
    }

    int cols = output_map->nCols();
    int rows = output_map->nRows();
    int max_y = rows - 1;
    int max_x = cols - 1;

    boost::progress_display show_progress1(cols * rows);

        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                ++show_progress1;
                Coord cell_loc(i,j);
                if (output_map->get(cell_loc) == dem_has_null)
                {
                    bool do_infill = false;
                    float infill_val = 0;
                    int num_neighbours = 0;

                    int end_y = (int) (std::min(j + 1, max_y));
                    int begin_y = (int) std::max(j - 1, 0);
                    int end_x = (int) std::min(i + 1, max_x);
                    int begin_x = (int) std::max(i - 1, 0);
                    for (int ti = begin_y; ti <= end_y; ++ti)
                    {
                        for (int tj = begin_x; tj <= end_x; ++tj)
                        {
                            Coord nh_loc(ti, tj);
                            float height_in_adjacent = output_map->get(nh_loc);
                            if (height_in_adjacent != dem_null_value)
                            {
                                if (height_in_adjacent > params.threshold) do_infill = true;
                                infill_val += height_in_adjacent;
                                num_neighbours++;
                            }
                        }
                    }
                    infill_val /= num_neighbours;
                    if (do_infill)  output_map->put(cell_loc, infill_val);
                }
            }
        }
}