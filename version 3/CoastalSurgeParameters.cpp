//
// Created by a1091793 on 22/8/17.
//

#include "CoastalSurgeParameters.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>

std::pair<std::string, std::string> at_option_parser(std::string const&s)
{
    if ('@' == s[0])
        return std::make_pair(std::string("cfg-file"), s.substr(1));
    else
        return std::pair<std::string, std::string>();
}

CoastalSurgeParameters::CoastalSurgeParameters():
        work_dir(boost::filesystem::current_path()),
        record_propagation(false)
{

}

void
processCmdLineArguments(int argc, char **argv, CoastalSurgeParameters & _params) throw(std::runtime_error)
{
    namespace prog_opt = boost::program_options;
    prog_opt::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            //("coast-demark-map,c", prog_opt::value<CmdLinePaths>(&_params.coast_demark_file),
            //      "path of the map that demarks where the boundary between the sea and land is")
            ("surge-height-graphs,s", prog_opt::value<std::vector<CmdLinePaths> >(&_params.surge_front_files_v)->multitoken(),
                  "path of gdal compatible rasters which specify surge height at coastline")
            ("dem-map,d", prog_opt::value<CmdLinePaths>(&_params.dem_file),
                  "path of the gdal capatible elevation data file")
            ("output-flood-height-maps,o", prog_opt::value<std::vector<CmdLinePaths> >(&_params.output_files_v)->multitoken(),
                  "path of the output maps where each pixel is assigned the flood height at that pixel")
            ("log_file,l", prog_opt::value<CmdLinePaths>(&_params.log_file)->default_value("moves.log"),
                  "path of the file which logs feature cell movements")
            ("record-progression,r", prog_opt::value<bool>()->default_value(false),
                  "Map that shows propagation of coastal surge wave inward inland")
			("direct-attenuation,a", prog_opt::value<float>(&_params.direct_attenuation)->default_value(1.0), 
				"Attenuation of surge 'wave' along shortest path")
		    ("indirect-attenuation,i", prog_opt::value<float>(&_params.indirect_attenuation)->default_value(0.99), 
				"Attenuation of surge 'wave' along shortest path")
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
        pathify(_params.dem_file);
    }
    catch (boost::filesystem::filesystem_error & err)
    {
        std::stringstream ss;
        ss << "Filesystem error: " << err.what();
        throw std::runtime_error(ss.str());
    }

    //try{
    //    pathify(_params.coast_demark_file);
    //}
    //catch (boost::filesystem::filesystem_error & err)
    //{
    //    std::stringstream ss;
    //    ss << "Filesystem error: " << err.what();
    //    throw std::runtime_error(ss.str());
    //}

    BOOST_FOREACH(CmdLinePaths & path, _params.surge_front_files_v)
                {
                    try{
                        pathify(path);
                    }
                    catch (boost::filesystem::filesystem_error & err)
                    {
                        std::stringstream ss;
                        ss << "When pathify surge front files, Filesystem error: " << err.what();
                        throw std::runtime_error(ss.str());
                    }

                }

    BOOST_FOREACH(CmdLinePaths & path, _params.output_files_v)
                {
                    try{
                        pathify_mk(path);
                    }
                    catch (boost::filesystem::filesystem_error & err)
                    {
                        std::stringstream ss;
                        ss << "When pathify_mk output file directories, Filesystem error: " << err.what();
                        throw std::runtime_error(ss.str());
                    }

                }
}