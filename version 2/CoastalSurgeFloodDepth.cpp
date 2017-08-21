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

#include <GDAL.h>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/foreach.hpp>
#include <boost/progress.hpp>



	

#include <blink/raster/utility.h> // To open rasters
//#include <blink/iterator/zip_iterator.h>
#include <blink/iterator/zip_range.h>

#include "Types.h"
#include "Neighbourhood.h"

#include "GZArchive.hpp"

namespace filesys = boost::filesystem;
namespace raster_util = blink::raster;
namespace raster_it = blink::iterator;
namespace fs = boost::filesystem;
namespace io = boost::iostreams;
typedef raster_util::coordinate_2d Coord;

std::pair<std::string, std::string> at_option_parser(std::string const&s)
{
    if ('@' == s[0])
        return std::make_pair(std::string("cfg-file"), s.substr(1));
    else
        return std::pair<std::string, std::string>();
}


//#define GET_1_NEIGHBOURHOOD(x, y, map, null_val)                               \
//    int max_y = (int)(map->nRows() - 1)                                        \
//    int end_y = (int) std::min(y + 1, max_y);                                  \
//    int begin_y = (int) std::max(y - 1, 0);                                    \
//    int max_x = (int)(map->nCols() - 1)                                        \
//    int end_x = (int) std::min(x + 1, max_x);                                  \
//    int begin_x = (int) std::max(x - 1, 0);                                    \
//    for (int ti = begin_y; ti <= end_y; ++ti)                                  \
//    {                                                                          \
//        for (int tj = begin_x; tj <= end_x; ++tj)                              \
//        {                                                                      \
//            if (dem_map->get(Coord(ti, tj)) != no_val)                         \
//            {                                                                  \


int main(int argc, char **argv)
{
    
    /**********************************/
    /*        Program options         */
    /**********************************/
    // Need to specify elevation grid
    // Need to specify channel
    fs::path work_dir = fs::current_path();
    
    std::string dem_file;
    std::string coast_demark_file;
    std::string log_file;
//    std::string output_file;
    std::vector<std::string> output_file_v;
    std::string surge_front_file;
    std::vector<float> flood_height_v;
//    float flood_height;
    bool do_print = false;
    std::string file_print;
    std::string required_surge_file;
    std::string template_map_file;
    float threshold;
    int cells_per_file;
    int file_count = 1; // number of files the surge requried set is split between.
    bool record_propagation = false;
    std::string zero_raster_file = "no_file";
    
    namespace prog_opt = boost::program_options;
    prog_opt::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "produce help message")
//    ("flood-height,f", prog_opt::value<float>(&flood_height)->default_value(1.80), "height of flood (abs elevation)")
    ("flood-height,f", prog_opt::value<std::vector<float> >(&flood_height_v)->multitoken(), "height of floods to calculate for (abs elevation)")
    ("coast-demark-map,c", prog_opt::value<std::string>(&coast_demark_file), "path of the map that demarks where the boundary between the sea and land is")
    ("surge-front-list,s", prog_opt::value<std::string>(&surge_front_file), "path of the xml file containing list of pixels to be used as surge front")
    ("dem-map,d", prog_opt::value<std::string>(&dem_file), "path of the gdal capatible elevation data file")
    ("required-surge-height-set,e", prog_opt::value<std::string>(&required_surge_file), "path of the directory where files containing the set of required heights of surge for each pixel is saved/loaded from")
//    ("output-flood-height-map,o", prog_opt::value<std::string>(&output_file)->default_value("coastal_flood_height.tif"), "path of the output map where each pixel is assigned the flood height at that pixel")
    ("output-flood-height-map,o", prog_opt::value<std::vector<std::string> >(&output_file_v)->multitoken(), "path of the output map where each pixel is assigned the flood height at that pixel")
    ("template-map,t", prog_opt::value<std::string>(&template_map_file), "path of template map to copy geospatial positioning info from for output map")
    ("threshold,a", prog_opt::value<float>(&threshold)->default_value(15), "Threshold value, required surge for inundation truncated for cells in DEM with elevation above this value")
    ("cells-in-file,i", prog_opt::value<int>(&cells_per_file)->default_value(1.0E6), "Number of cells to store in each file - required s")
    ("log_file,l", prog_opt::value<std::string>(&log_file)->default_value("moves.log"), "path of the file which logs feature cell movements")
    ("print,p", prog_opt::value<std::string>(&file_print), "Print map to text file (space seperated values) - specify file name prefix")
    ("cfg-file,g", prog_opt::value<std::string>(), "can be specified with '@name', too")
    ("record-progression,r", prog_opt::value<bool>()->default_value(false), "Map that shows propagation of coastal surge wave inward inland")
    ("zero-rast,z", prog_opt::value<std::string>(&zero_raster_file), "path of the gdal capatible zero raster file")
    ;
    
    prog_opt::variables_map vm;
    prog_opt::store(prog_opt::command_line_parser(argc, argv).options(desc).extra_parser(at_option_parser).run(), vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }
    
    if (vm.count("cfg-file")) {
        // Load the file and tokenize it
        std::ifstream ifs(vm["cfg-file"].as<std::string>().c_str());
        if (!ifs) {
            std::cout << "Could not open the cfg file\n";
            return 1;
        }
        prog_opt::store(prog_opt::parse_config_file(ifs, desc), vm);
//        // Read the whole file into a string
//        std::stringstream ss;
//        ss << ifs.rdbuf();
//        // Split the file content
//        char_separator<char> sep(" \n\r");
//        std::string ResponsefileContents( ss.str() );
//        tokenizer<char_separator<char> > tok(ResponsefileContents, sep);
//        vector<string> args;
//        copy(tok.begin(), tok.end(), back_inserter(args));
//        // Parse the file and store the options
//        store(command_line_parser(args).options(desc).run(), vm);
    }
    
    prog_opt::notify(vm);
    
    if (vm.count("print"))
    {
        do_print = true;
    }
    
    fs::path dem_file_path(dem_file);
    fs::path coastal_demark_file_path(coast_demark_file);
//    fs::path output_file_path(output_file);
    std::vector<fs::path> output_file_path_v;
    for (int i = 0; i < output_file_v.size(); ++i)
    {
        output_file_path_v.push_back(fs::path(output_file_v[i]));
    }
    fs::path surge_front_path(surge_front_file);
    fs::path required_surge_path(required_surge_file);
    fs::path template_path(template_map_file);
    fs::path zero_path(zero_raster_file);
    
    // Check file exists
    if (!fs::exists(template_path))
    {
        std::stringstream ss;
        ss << template_path << " does not exist";
        throw std::runtime_error(ss.str());
        return (EXIT_FAILURE);
    }
    
    if (!(boost::filesystem::exists(required_surge_path)))
    {
        boost::filesystem::create_directory(required_surge_path);
        std::cout << "path " << required_surge_path << " did not exist, so created\n";
    }
    
    
    //set of pixels, ordered by surge requried for inundation. This is saved.
    //std::multiset<Pixel, SortByRequiredSurgeHeight> surge_required_set;
    std::vector<Pixel> surge_required_set;
    
    bool make_surge_required_set = true;
    
    if (vm.count("required-surge-height-set"))
    {
        
        std::cout << "\n\n*************************************\n";
        std::cout << "*  Reading required surge height list *\n";
        std::cout << "*************************************" << std::endl;
        
        // Check file exists
        if (!fs::exists(required_surge_path))
        {
            if(!boost::filesystem::is_directory(required_surge_path))
            {

            std::stringstream ss;
            ss << required_surge_path << " does not exist";
            throw std::runtime_error(ss.str());
            return (EXIT_FAILURE);
            }
        }
        else
        {
            fs::path first_set_file = required_surge_path / "required-surge-1.setgz";
            if (fs::exists(first_set_file)) make_surge_required_set = false;
        }
    }
    
    
    if (make_surge_required_set)
    {
        
        
        //    /**********************************/
        //    /*         Read in maps           */
        //    /**********************************/
            std::cout << "\n\n*************************************\n";
            std::cout << "*             Read in DEM map          *\n";
            std::cout << "*************************************" << std::endl;
        
        boost::shared_ptr< raster_util::gdal_raster<float> > dem_map;
        if(vm.count("dem-map"))
        {
            if (!fs::exists(dem_file_path))
            {
                std::stringstream ss;
                ss << dem_file_path << " does not exist";
                throw std::runtime_error(ss.str());
                return (EXIT_FAILURE);
            }
            
            // Second the elevation data
            dem_map = raster_util::open_gdal_rasterSPtr<float>(dem_file_path, GA_ReadOnly);
            //	std::tuple<boost::shared_ptr<Map_Matrix<float> >, std::string, GeoTransform> gdal_dem_map = read_in_map<float>(dem_file_path, GDT_Float32, NO_CATEGORISATION);
            //	boost::shared_ptr<Map_Matrix<float> > dem_map(std::get<0>(gdal_dem_map));
            //	std::string & demWKTprojection(std::get<1>(gdal_dem_map));
            //	GeoTransform & demTransform(std::get<2>(gdal_dem_map));
        }
        
        boost::shared_ptr<raster_util::gdal_raster<int32_t> > coastal_demark_map;
        boost::shared_ptr<std::vector<Coord> > surge_front_pixels(new std::vector<Coord>);
        
        
        if (vm.count("surge-front-list"))
        {
            
            if (!fs::exists(surge_front_path))
            {
                std::stringstream ss;
                ss << surge_front_path << " does not exist";
                throw std::runtime_error(ss.str());
                return (EXIT_FAILURE);
            }
            
            // Read file into list.
            std::ifstream ifs(surge_front_path.c_str());
            boost::archive::text_iarchive ia(ifs);
            ia >> surge_front_pixels;
            
        }
        else if(vm.count("coast-demark-map"))
        {
            const int SEA = 0;
            const int LAND = 1;
            
            // Check file exists
            if (!fs::exists(coastal_demark_file_path))
            {
                std::stringstream ss;
                ss << coastal_demark_file_path << " does not exist";
                throw std::runtime_error(ss.str());
                return (EXIT_FAILURE);
            }
            
            //First the demarkation
            coastal_demark_map = (raster_util::open_gdal_rasterSPtr<int32_t>(coastal_demark_file_path, GA_ReadOnly));
            //	std::tuple<Map_Int_SPtr, std::string, GeoTransform> gdal_coastal_demark_map = read_in_map<int32_t>(coastal_demark_file_path, GDT_Int32, NO_CATEGORISATION);
            //	Map_Int_SPtr coastal_demark_map(std::get<0>(gdal_coastal_demark_map));
            //	std::string & coastal_demark_WKTprojection(std::get<1>(gdal_coastal_demark_map));
            //	GeoTransform & coastal_demark_Transform(std::get<2>(gdal_coastal_demark_map));
            
            //Check maps for consistency (same dimensions)
            if (coastal_demark_map->nCols() != dem_map->nCols())
            {
                throw std::runtime_error("Number of columns in the two comparison maps non-identical");
            }
            
            if (coastal_demark_map->nRows() != dem_map->nRows())
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
            boost::progress_display show_progress1((coastal_demark_map->nRows()*coastal_demark_map->nCols()));
            bool hasNoData = true;
            int success = 0;
            int32_t no_val = const_cast<GDALRasterBand *>(coastal_demark_map->get_gdal_band())->GetNoDataValue(&success);
            if (success ==0 ) hasNoData = false;
                        
            int max_y = coastal_demark_map->nRows() - 1;
            int max_x = coastal_demark_map->nCols() - 1;
            
            
            //Intiial front is the pixels on the land-sea boundary
            for ( int i = 0; i < coastal_demark_map->nRows(); ++i)
            {
                int end_y = (int) std::min(i + 1, max_y);
                int begin_y = (int) std::max(i - 1, 0);
                
                
                for ( int j = 0; j < coastal_demark_map->nCols(); ++j)
                {
                    
                    int32_t val = coastal_demark_map->get(Coord(i, j));
                    if (val == LAND)	// IS Land
                    {
                        int end_x = (int)std::min(j + 1, max_x);
                        int begin_x = (int)std::max(j - 1, 0);
                        
                        for (int ti = begin_y; ti <= end_y; ++ti)
                        {
                            for (int tj = begin_x; tj <= end_x; ++tj)
                            {
                                if (coastal_demark_map->get(Coord(ti, tj)) == SEA) surge_front_pixels->push_back(Coord(i, j));
                            }
                        }
                        
                        
                    }
                    ++show_progress1;
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
        else
        {
            //Create list ourselves from DEM
            // Assume no-data values are sea. Find cells adjacent to these.
            //Intiial front is the pixels on the land-sea boundary
            /**********************************/
            /*  Create list of surge front */
            /**********************************/
            std::cout << "\n\n*************************************\n";
            std::cout << "*  Making list of surge front pixels:   *\n";
            std::cout << "*************************************" << std::endl;
            boost::progress_display show_progress1((dem_map->nRows()*dem_map->nCols()));
            
            bool hasNoData = true;
            int success = 0;
            float no_val = const_cast<GDALRasterBand *>(dem_map->get_gdal_band())->GetNoDataValue(&success);
            if (success ==0 ) hasNoData = false;
            
            
            int max_y = dem_map->nRows() - 1;
            int max_x = dem_map->nCols() - 1;
            
            fs::path inundation_required_path = work_dir / "surge-front-map.tif";
//            boost::shared_ptr< raster_util::gdal_raster<int32_t> > surge_front_map = raster_util::create_gdal_rasterSPtr_from_model<int32_t>(inundation_required_path, *dem_map);
            
            boost::shared_ptr< raster_util::gdal_raster<int32_t> > surge_front_map = raster_util::create_gdal_rasterSPtr<int32_t>(inundation_required_path, dem_map->nRows(), dem_map->nCols(), GDT_Int32);
            float surge_front_no_data = 0;
            const_cast<GDALRasterBand *>(surge_front_map->get_gdal_band())->SetNoDataValue(surge_front_no_data);
            auto zip = raster_it::make_zip_range(std::ref(*surge_front_map));
            for (auto i : zip)
            {
                auto & val = std::get<0>(i);
                val = surge_front_no_data;
                ++show_progress1;
            }
            
            show_progress1.restart((dem_map->nRows()*dem_map->nCols()));
            
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
                                    if (dem_map->get(Coord(ti, tj)) != no_val)
                                    {
                                        surge_front_pixels->push_back(Coord(i, j));   /// if not no-val, assume to be land.
                                        surge_front_map->put(Coord(i, j), 1);
   
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
        
        
        /*****************************************************************************/
        /*     Calculate height of surge required to inundate each cell in raster    */
        /*****************************************************************************/
        std::cout << "\n\n/*****************************************************************************/\n";
        std::cout << "/*     Calculate height of surge required to inundate each cell in raster    */\n";
        std::cout << "/*****************************************************************************/" << std::endl;
        
        
        //map of required surge to inundate. This is saved.
        fs::path inundation_required_path = work_dir / "surge_required_for_inundation.tif";
        boost::shared_ptr< raster_util::gdal_raster<float> > output_map = raster_util::create_gdal_rasterSPtr_from_model<float>(inundation_required_path, *dem_map);
        bool hasNoData = true;
        int success = 0;
        float dem_null_value = const_cast<GDALRasterBand *>(dem_map->get_gdal_band())->GetNoDataValue(&success);
        if (success == 0 )
        {
            hasNoData = false;
            dem_null_value = -999;
        }
        const_cast<GDALRasterBand *>(output_map->get_gdal_band())->SetNoDataValue(dem_null_value);
        std::cout << "SETTING RASTER TO NULL" << std::endl;
        boost::progress_display show_progress1((output_map->nRows()*output_map->nCols()));
        
        
        auto zip = raster_it::make_zip_range(std::ref(*output_map));
        for (auto i : zip)
        {
            auto & val = std::get<0>(i);
            val = dem_null_value;
            ++show_progress1;
        }
        
        std::cout << "CALCULTING REQUIRED SURGE" << std::endl;
        
        // A map that stores whether a pixel has been processed or not.
        std::vector<bool> temp_cnstr(dem_map->nCols(), false);
        std::vector<std::vector<bool> > processed(dem_map->nRows(), temp_cnstr);
        
        //Data
        //set of surge front pixels, ordered by elevation of pixel
        std::multiset<Pixel, SortByElevation> surge_front_set;
        // Fill in set which stores pixels on the wetting front
        std::cout << "Sorting the surge front" << std::endl;
        boost::progress_display show_progress2(surge_front_pixels->size());
        BOOST_FOREACH(Coord & pos, *surge_front_pixels)
        {
            if (processed[pos.row][pos.col] == false)
            {
                Pixel front_pixel(pos, dem_map->get(pos));
                surge_front_set.insert(front_pixel);
                // For each pixel on the surge front, set that it is 'processed', and add it to the surge front set.
                processed[pos.row][pos.col] = true;
            }
            ++show_progress2;
        }
        
        std::cout << " setting up raster for tracking processing " << std::endl;
        boost::progress_display show_progress4(output_map->nRows()*output_map->nCols());
        // set processed to true for any cells with no_data
        if (success != 0 )
        {
            for ( int i = 0; i < dem_map->nRows(); ++i)
            {
                for ( int j = 0; j < dem_map->nCols(); ++j)
                {
                    Coord coord(i,j);
                    if (dem_map->get(coord) == dem_null_value)
                    {
                        processed[i][j] = true;
                        ++show_progress4;
                    }
                }
            }
        }
        
        
//        // For each pixel on the surge front set:
//        std::set<Pixel, SortByElevation>::iterator it;
//        for (it = surge_front_set.begin(); it != surge_front_set.end(); )
//        {
//            const Pixel & cell = *it;
//            
//            //calculate surge required for inundation, and store this value associated with this cell to map and to set of pixels
//            float surge_rise_required = cell.elevation; // - min_downstream;
//            if (surge_rise_required < 0) surge_rise_required = 0;
//            //        float required_surge = min_downstream + surge_rise_required;
//            output_map->put(cell.loc, surge_rise_required);
//            surge_required_set.insert(Pixel(cell.loc, cell.elevation, surge_rise_required));
//            
//            //find adjacent cells that are not 'processed', and add them to the set, and assign them as 'processed'
//            boost::shared_ptr<CooridinateSet> nh = find_adjacents(dem_map, cell.loc.row, cell.loc.col, 1, dem_null_value);
//            BOOST_FOREACH(const Coord & nh_loc, *nh)
//            {
//                if (processed[nh_loc.row][nh_loc.col] == false)
//                {
//                    Pixel front_pixel(nh_loc, dem_map->get(nh_loc));
//                    surge_front_set.insert(front_pixel);
//                    processed[nh_loc.row][nh_loc.col] = true;
//                }
//            }
//            //remove cell from the set of surge front pixels.
//            it = surge_front_set.erase(it);
//        }
        boost::progress_display show_progress3((output_map->nRows()*output_map->nCols()));
        
        
        while (!surge_front_set.empty())
        {
            //SCAN and SET
            // do scan and set until no pixels in surge_front_pixels.
            
            
            // Get the lowest elevation pixel on the surge front set. create copy as we will just delete this now as well.
            const Pixel cell = *(surge_front_set.begin());
            //remove cell from the set of surge front pixels.
            surge_front_set.erase(surge_front_set.begin());
            
            
            // Find neighbouring processed cell with minimum surge required for inundation.
            //find adjacent cells that are not 'processed', and add them to the set, and assign them as 'processed'
            float min_downstream = std::numeric_limits<float>::max();
            int max_y = (int)(dem_map->nRows() - 1);
            int end_y = (int)(std::min(int(cell.loc.row) + 1, max_y));
            int begin_y = (int) std::max(int(cell.loc.row) - 1, 0);
            int max_x = (int)(dem_map->nCols() - 1);
            int end_x = (int) std::min(int(cell.loc.col) + 1, max_x);
            int begin_x = (int) std::max(int(cell.loc.col) - 1, 0);
            for (int ti = begin_y; ti <= end_y; ++ti)                         
            {                                                                 
                for (int tj = begin_x; tj <= end_x; ++tj)                     
                {
                    Coord nh_loc(ti,tj);
                    auto val = dem_map->get(nh_loc);
                    if (val != dem_null_value)
                    {
                        float nh_surge_rq = val;
                        if ((nh_surge_rq != dem_null_value) && (nh_surge_rq < min_downstream)) min_downstream = nh_surge_rq;
                        
                        if (processed[nh_loc.row][nh_loc.col] == false)
                        {
                            if (val <= threshold)
                            {
                                Pixel front_pixel(nh_loc, dem_map->get(nh_loc));
                                surge_front_set.insert(front_pixel);
                            }
                            processed[nh_loc.row][nh_loc.col] = true;
                        }
                    }
                }
            }
            if (min_downstream == std::numeric_limits<float>::max()) min_downstream = 0;
            
            //calculate surge required for inundation, and store this value associated with this cell to map and to set of pixels
            if (cell.elevation == dem_null_value)
            {
                output_map->put(cell.loc, 0);
            }
            else
            {
                float surge_rise_required = cell.elevation - min_downstream;
                if (surge_rise_required < 0) surge_rise_required = 0;
                float required_surge = min_downstream + surge_rise_required;
                output_map->put(cell.loc, required_surge);
                surge_required_set.push_back(Pixel(cell.loc, cell.elevation, required_surge));
            }
//            surge_required_set.insert(Pixel(cell.loc, cell.elevation, required_surge));
            
            
            if (surge_required_set.size() > cells_per_file)
            {
                // save data to archive
                {
                    // Changed surge required set as sorting in surge_front_set I think ensures order in terms of inundation required.
                    
                    std::string file_name = "required-surge-" + std::to_string(file_count) + ".setgz";
                    fs::path save_path = required_surge_path / file_name;
                    saveGZFile_< std::vector<Pixel>, boost::archive::binary_oarchive>(surge_required_set, save_path.c_str());
//                    saveGZFile_< std::multiset<Pixel, SortByRequiredSurgeHeight>, boost::archive::binary_oarchive>(surge_required_set, "required-surge.setgz");
                    // create and open a character archive for output
                    //            std::ofstream ofs("required-surge-set.txt");
                    //            boost::archive::text_oarchive oa(ofs);
                    //            // write class instance to archive
                    //            oa << surge_required_set;
                    //            // archive and stream closed when destructors are called
                    ++file_count;
                }
                surge_required_set.clear();
            }
            
            
            ++show_progress3;
            
        }
    
        // save data to archive
//        {
//            saveGZFile_< std::multiset<Pixel, SortByRequiredSurgeHeight>, boost::archive::binary_oarchive>(surge_required_set, "required-surge.setgz");
//            // create and open a character archive for output
////            std::ofstream ofs("required-surge-set.txt");
////            boost::archive::text_oarchive oa(ofs);
////            // write class instance to archive
////            oa << surge_required_set;
////            // archive and stream closed when destructors are called
//        }
        
        
        
    }
    
    
    /*****************************************************************************/
    /*     Calculate height of surge required to inundate each cell in raster    */
    /*****************************************************************************/
    std::cout << "\n\n/*****************************************************************************/\n";
    std::cout <<     "/*                              Calculating inundation                       */\n";
    std::cout <<     "/*****************************************************************************/" << std::endl;

    
    
    boost::shared_ptr< raster_util::gdal_raster<float> > template_map = raster_util::open_gdal_rasterSPtr<float>(template_path, GA_ReadOnly);
    
    
    // Count number of files in directory
    file_count = 1;
    do {
        std::string file_name = "required-surge-" + std::to_string(file_count + 1) + ".setgz";
        fs::path file_path = required_surge_path / file_name;
        if (fs::exists(file_path)) {++file_count;}
        else {break;}
    } while (true);
    
    for (int i = 0; i < flood_height_v.size(); ++i)
    {
        float flood_height = flood_height_v[i];
        std::cout << "INUNDATION FOR SURGE HEIGHT: " << flood_height << std::endl;
        
        boost::shared_ptr< raster_util::gdal_raster<float> > output_map;
        boost::shared_ptr< raster_util::gdal_raster<float> > output_map_bool;
        boost::shared_ptr< raster_util::gdal_raster<float> > propagation_map;
        
        if (zero_raster_file == "no_file")
        {
            output_map = raster_util::create_gdal_rasterSPtr_from_model<float>(output_file_path_v[i], *template_map);
            float null_val = -999.99;
            const_cast<GDALRasterBand *>(output_map->get_gdal_band())->SetNoDataValue(null_val);
            std::cout << "Setting raster to null" << std::endl;
            boost::progress_display show_progress2((output_map->nRows()*output_map->nCols()));
            auto zip = raster_it::make_zip_range(std::ref(*output_map));
            for (auto i : zip)
            {
                auto & val = std::get<0>(i);
                val = null_val;
                ++show_progress2;
            }
            
            filesys::path bool_path = output_file_path_v[i].parent_path() / (output_file_path_v[i].stem().string() + "_bool" + output_file_path_v[i].extension().string());
            output_map_bool = raster_util::create_gdal_rasterSPtr_from_model<float>(bool_path, *template_map);
            float null_val_bool = 0;
            const_cast<GDALRasterBand *>(output_map_bool->get_gdal_band())->SetNoDataValue(null_val_bool);
            std::cout << "Setting bool raster to null" << std::endl;
            boost::progress_display show_progress_bool((output_map_bool->nRows()*output_map_bool->nCols()));
            auto zip_bool = raster_it::make_zip_range(std::ref(*output_map_bool));
            for (auto i : zip_bool)
            {
                auto & val = std::get<0>(i);
                val = null_val_bool;
                ++show_progress_bool;
            }
            
            
            if(record_propagation)
            {
                filesys::path progation_path = output_file_path_v[i].parent_path() / (output_file_path_v[i].stem().string() + "_propagation" + output_file_path_v[i].extension().string());
                propagation_map = raster_util::create_gdal_rasterSPtr_from_model<float>(progation_path, *template_map);
                float null_val = -999.99;
                const_cast<GDALRasterBand *>(propagation_map->get_gdal_band())->SetNoDataValue(null_val);
                std::cout << "Setting propagation raster to null" << std::endl;
                boost::progress_display show_progress2((propagation_map->nRows()*propagation_map->nCols()));
                auto zip = raster_it::make_zip_range(std::ref(*propagation_map));
                for (auto i : zip)
                {
                    auto & val = std::get<0>(i);
                    val = null_val;
                    ++show_progress2;
                }
            }
            else
            {
                filesys::path progation_path = output_file_path_v[i].parent_path() / (output_file_path_v[i].stem().string() + "_propagation" + output_file_path_v[i].extension().string());
                //            raster_util::create_gdal_raster_from_model<float>(progation_path, *template_map);
                //            raster_util::create_gdal_raster<float>(progation_path, 10, 10);
                propagation_map = raster_util::create_gdal_rasterSPtr<float>(progation_path, 10, 10);
                //            propagation_map = raster_util::create_gdal_rasterSPtr_from_model<float>(progation_path, *template_map);
            }
        }
        else
        {
            boost::filesystem::copy(zero_path, output_file_path_v[i]);
            output_map = raster_util::open_gdal_rasterSPtr<float>(output_file_path_v[i], GA_Update);
            float null_val = -999.99;
            const_cast<GDALRasterBand *>(output_map->get_gdal_band())->SetNoDataValue(null_val);
            
            filesys::path bool_path = output_file_path_v[i].parent_path() / (output_file_path_v[i].stem().string() + "_bool" + output_file_path_v[i].extension().string());
            boost::filesystem::copy(zero_path, bool_path);
            output_map_bool = raster_util::open_gdal_rasterSPtr<float>(bool_path, GA_Update);
            const_cast<GDALRasterBand *>(output_map->get_gdal_band())->SetNoDataValue(null_val);
            
            if(record_propagation)
            {
                filesys::path progation_path = output_file_path_v[i].parent_path() / (output_file_path_v[i].stem().string() + "_propagation" + output_file_path_v[i].extension().string());
                boost::filesystem::copy(zero_path, progation_path);
                propagation_map = raster_util::open_gdal_rasterSPtr<float>(progation_path, GA_Update);
                const_cast<GDALRasterBand *>(propagation_map->get_gdal_band())->SetNoDataValue(null_val);
            }
            else
            {
                filesys::path progation_path = output_file_path_v[i].parent_path() / (output_file_path_v[i].stem().string() + "_propagation" + output_file_path_v[i].extension().string());
                //            raster_util::create_gdal_raster_from_model<float>(progation_path, *template_map);
                //            raster_util::create_gdal_raster<float>(progation_path, 10, 10);
                propagation_map = raster_util::create_gdal_rasterSPtr<float>(progation_path, 10, 10);
                //            propagation_map = raster_util::create_gdal_rasterSPtr_from_model<float>(progation_path, *template_map);
            }
        }
        
        
        std::cout << "Calculating flood depth" << std::endl;
        
        int file_it = 1;
        bool finished = false;
        float progression = 1;
        float step_progression = (2E30-2) / (propagation_map->nRows()*propagation_map->nCols());
        if (step_progression > 1.0) step_progression = 1.0;
        do
        {
            std::cout << " File " << file_it << ":" << std::endl;
            
            surge_required_set.clear();
            std::string file_name = "required-surge-" + std::to_string(file_it) + ".setgz";
            fs::path file_path = required_surge_path / file_name;
            if (fs::exists(file_path))
            {
                loadGZFile_<std::vector<Pixel>, boost::archive::binary_iarchive>(surge_required_set, file_path.c_str());
                boost::progress_display show_progress5(surge_required_set.size());
                
                BOOST_FOREACH(Pixel & cell, surge_required_set)
                {
                    float inundation_depth = flood_height - cell.surge_required_2_inundate;
                    if (inundation_depth < 0)
                    {
                        finished = true;
                        break;
                    }
                    output_map->put(cell.loc, inundation_depth);
                    output_map_bool->put(cell.loc, 1);
                    if(record_propagation)
                    {
                        propagation_map->put(cell.loc, progression);
                        progression += step_progression;
                    }
                    ++show_progress5;
                }
                ++file_it;
            }
            else
            {
                file_it = file_count + 1;
                finished = true;
                std::cout << "Error: looking for file " << file_path << " But not found. Inundation not yet complete for surge height of " << flood_height << std::endl;
            }
        } while (file_it <= file_count && !finished);
        
    }
    

return (EXIT_SUCCESS);
}


// Read in from file
//        loadGZFile_<std::multiset<Pixel, SortByRequiredSurgeHeight>, boost::archive::binary_iarchive>(surge_required_set, required_surge_path.c_str());
//        std::ifstream ifs(required_surge_path.c_str());
//        boost::archive::text_iarchive ia(ifs);
//        ia >> surge_required_set;








