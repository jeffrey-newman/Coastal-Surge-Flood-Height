//
// Created by a1091793 on 22/8/17.
//

#ifndef COASTAL_SURGE_INUNDATION_COASTALSURGEPARAMETERS_HPP
#define COASTAL_SURGE_INUNDATION_COASTALSURGEPARAMETERS_HPP

#include "Pathify.hpp"
#include <boost/filesystem.hpp>

struct CoastalSurgeParameters
{
    CoastalSurgeParameters();

    boost::filesystem::path work_dir;
    CmdLinePaths dem_file;
    CmdLinePaths coast_demark_file;
    CmdLinePaths log_file;
    std::vector<CmdLinePaths> output_files_v;
    std::vector<CmdLinePaths> surge_front_files_v;
    bool record_propagation;
    float direct_attenuation;
    float indirect_attenuation;
//    std::string zero_raster_file = "no_file";



};

void
processCmdLineArguments(int argc, char **argv, CoastalSurgeParameters & _params)  throw(std::runtime_error);

#endif //COASTAL_SURGE_INUNDATION_COASTALSURGEPARAMETERS_HPP
