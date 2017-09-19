//
// Created by a1091793 on 15/09/17.
//

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

struct CoastalBoundaryParameters
{
    CoastalBoundaryParameters()
    {

    }

    CmdLinePaths input_dem_file;
    CmdLinePaths output_coast_boundary_file;
};

std::pair<std::string, std::string> at_option_parser(std::string const&s)
{
    if ('@' == s[0])
        return std::make_pair(std::string("cfg-file"), s.substr(1));
    else
        return std::pair<std::string, std::string>();
}

void
processCmdLineArguments(int argc, char* argv[], CoastalBoundaryParameters & _params) throw(std::runtime_error)
{
    namespace prog_opt = boost::program_options;
    prog_opt::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("input-dem-map,i", prog_opt::value<CmdLinePaths>(&_params.input_dem_file),
             "path of the gdal capatible elevation data file")
            ("output-coastal-boundary-map,o", prog_opt::value<CmdLinePaths>(&_params.output_coast_boundary_file),
             "path of the gdal capatible elevation data file")
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
        pathify_mk(_params.output_coast_boundary_file);
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

    CoastalBoundaryParameters params;
    processCmdLineArguments(argc, argv, params);

    std::cout << "\n\n*************************************\n";
    std::cout << "*      Setting up output raster     *\n";
    std::cout << "*************************************" << std::endl;

    boost::shared_ptr<raster_util::gdal_raster<float> > dem_map = raster_util::open_gdal_rasterSPtr<float>(params.input_dem_file.second, GA_Update);
    boost::shared_ptr<raster_util::gdal_raster<int> > output_map = raster_util::create_gdal_rasterSPtr_from_model<int, float>(params.output_coast_boundary_file.second, *dem_map, GDT_Int32);
    int out_no_data_val = 0;
    const_cast<GDALRasterBand *>(output_map->get_gdal_band())->SetNoDataValue(out_no_data_val);

    bool dem_has_null = true;
    int success = 0;
    float dem_null_value = const_cast<GDALRasterBand *>(dem_map->get_gdal_band())->GetNoDataValue(&success);
    if (success == 0 )
    {
        dem_has_null = false;
        dem_null_value = -999;
        std::cout << "DEM has no null values, cannot find boundary\n";
        return EXIT_SUCCESS;
    }

    int cols = output_map->nCols();
    int rows = output_map->nRows();
    int max_row = rows - 1;
    int max_col = cols - 1;

    unsigned long num_cells = cols;
    num_cells *= rows;
    boost::progress_display show_progress1(num_cells);

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            ++show_progress1;
            Coord cell_loc(r,c);
            if (dem_map->get(cell_loc) == dem_null_value)
            {
                bool is_coastline = false;
                int end_row = (int) (std::min(r + 1, max_row));
                int begin_row = (int) std::max(r - 1, 0);
                int end_col = (int) std::min(c + 1, max_col);
                int begin_col = (int) std::max(c - 1, 0);
                for (int ti = begin_row; ti <= end_row; ++ti)
                {
                    for (int tj = begin_col; tj <= end_col; ++tj)
                    {
                        Coord nh_loc(ti, tj);
                        float height_in_adjacent = dem_map->get(nh_loc);
                        if (height_in_adjacent != dem_null_value) is_coastline = true;
                    }
                }
                if (is_coastline)
                {
                    output_map->put(cell_loc, 1);
                }
                else
                {
                    output_map->put(cell_loc, out_no_data_val);
                }
            }
            else
            {
                output_map->put(cell_loc, out_no_data_val);
            }
        }
    }
}