//
// Created by a1091793 on 15/09/17.
//

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
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/undirected_dfs.hpp>

#include "../Pathify.hpp"
#include "../Types.h"
#include "PrintGraphsToFile.h"
#include "ReadGraphsFromFile.h"

struct CoastBoundaryGraphParameters
{
    CoastBoundaryGraphParameters()
    {

    }

    CmdLinePaths input_dem_file;
    CmdLinePaths in_coast_boundary_raster;
    CmdLinePaths out_coast_boundary_grph_file;

    double gap_threshold;
    unsigned int trim_level;
};

std::pair<std::string, std::string> at_option_parser(std::string const&s)
{
    if ('@' == s[0])
        return std::make_pair(std::string("cfg-file"), s.substr(1));
    else
        return std::pair<std::string, std::string>();
}

template <class Edge, class Graph>
class CycleTerminator : public boost::dfs_visitor<>
{

private:
    std::vector<Edge> & removal_list;

public:
    CycleTerminator(std::vector<Edge> & _removal_list) : removal_list(_removal_list)
    {

    }

    void back_edge(Edge e, const Graph& g) {
        removal_list.push_back(e);
    }
};

template <typename DataType>
boost::shared_ptr<Set> find_adjacents(boost::shared_ptr<blink::raster::gdal_raster<DataType> > map, unsigned long i, unsigned long j, unsigned long size)
{
    typedef raster_util::coordinate_2d Coord;
    int ii = (int) i;
    int jj = (int) j;
    int radius = (int) size;
    boost::shared_ptr<Set> adjacents(new Set);

    bool map_has_null = true;
    int success = 0;
    DataType map_null_value = const_cast<GDALRasterBand *>(map->get_gdal_band())->GetNoDataValue(&success);
    if (success == 0 )
    {
        map_has_null = false;
        map_null_value = -999;
        //std::cout << "Map has no null values, cannot delineate coast to make graph\n";
    }


    int end_y = (int)std::min(ii + radius, (int)(map->nRows() - 1));
    int begin_y = (int)std::max(ii - radius, 0);

    int end_x = (int)std::min(jj + radius, (int)(map->nCols() - 1));
    int begin_x = (int)std::max(jj - radius, 0);

    for (int ti = begin_y; ti <= end_y; ++ti)
    {
        for (int tj = begin_x; tj <= end_x; ++tj)
        {
            DataType val = map->get(Coord(ti, tj));
            //double dist = euclidean_distance(ti, tj, ii, jj);
            //if (dist < (double) radius && val != noData) adjacents->push_back(std::make_pair(ti - ii, tj - jj));
            ChannelNode node;
            node.row = ti;
            node.col = tj;
            if (val != map_null_value) adjacents->push_back(node);
        }
    }
    return (adjacents);
}


int
leafBranchVisit(Graph & channel_graph, EdgeDescriptor e, int distance, int cutoff, std::set<EdgeDescriptor> & edge_removal_list, std::set<VertexDescriptor> & vertex_removal_list)
{
    VertexDescriptor u = boost::target(e, channel_graph);
    ++distance;

    if (distance > cutoff) return (-1);

    if (channel_graph[u].type > 1)
    {
        edge_removal_list.insert(e);
        //vertex_removal_list.push_back(u);
        return (distance);
    }

    typedef boost::graph_traits<Graph>::out_edge_iterator OutIt;
    OutIt e_it, e_end;
    for (boost::tie(e_it, e_end) = boost::out_edges(u, channel_graph); e_it != e_end; e_it++)
    {
        if (*e_it != e)
        {
            int leaf_branch_dist = leafBranchVisit(channel_graph, *e_it, distance, cutoff, edge_removal_list, vertex_removal_list);
            if (leaf_branch_dist == -1) return (leaf_branch_dist);
            edge_removal_list.insert(e);
            vertex_removal_list.insert(u);
            return (leaf_branch_dist);
        }
    }
    return (-1);
}

void
leafBranchSearch(Graph& channel_graph, VertexDescriptor channel_vertex, int cutoff, std::set<EdgeDescriptor> & edge_removal_list, std::set<VertexDescriptor> & vertex_removal_list)
{
    typedef boost::graph_traits<Graph>::out_edge_iterator OutIt;
    OutIt e_it, e_end;
    int count = 0;
    int distance = 0;
    for (boost::tie(e_it, e_end) = boost::out_edges(channel_vertex, channel_graph); e_it != e_end; e_it++)
    {
        //There should only be one....
        distance = leafBranchVisit(channel_graph, *e_it, distance, cutoff, edge_removal_list, vertex_removal_list);
        ++count;
    }
    if (count > 1)
    {
        std::cerr << "Error: should only be one out edge of a terminal node" << std::endl;
    }
    if (distance != -1) vertex_removal_list.insert(channel_vertex);
}


void
processCmdLineArguments(int argc, char* argv[], CoastBoundaryGraphParameters & _params) throw(std::runtime_error)
{
    namespace prog_opt = boost::program_options;
    prog_opt::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("input-dem-map,d", prog_opt::value<CmdLinePaths>(&_params.input_dem_file),
             "path of the gdal capatible elevation data file - required for elevation property of nodes")
            ("coastal-boundary-map,c", prog_opt::value<CmdLinePaths>(&_params.in_coast_boundary_raster),
             "path of the gdal capatible coastal boundary raster")
            ("ouput-graph,o", prog_opt::value<CmdLinePaths>(&_params.out_coast_boundary_grph_file),
             "path of where to save the graph representation of coastal boundary - specify up to file name but without file name sufix (i.e. no .dot, or .graphml, these are added on within the code")
            ("trim-branches,t", prog_opt::value<unsigned int>(&_params.trim_level)->default_value(0),
             "remove branches if they are composed with a number of pixels less than this amount")
            ("bridge-gaps-threshold,b", prog_opt::value<double>(&_params.gap_threshold)->default_value(0),
             "Threshold distance in which channel graph will bridge a gap in the channel raster")
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
        pathify(_params.in_coast_boundary_raster);
    }
    catch (boost::filesystem::filesystem_error & err)
    {
        std::stringstream ss;
        ss << "Filesystem error: " << err.what();
        throw std::runtime_error(ss.str());
    }

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
        pathify_mk(_params.out_coast_boundary_grph_file);
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

    CoastBoundaryGraphParameters params;
    processCmdLineArguments(argc, argv, params);

    /**********************************/
    /*       Create graph object      */
    /**********************************/
    Graph channel_grph;

    /**********************************/
    /*         Read in maps           */
    /**********************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*             Read in maps          *\n";
    std::cout <<     "*************************************" << std::endl;
    //First the channel
    boost::shared_ptr<raster_util::gdal_raster<int> > feature_map =
            raster_util::open_gdal_rasterSPtr<int>(params.in_coast_boundary_raster.second, GA_ReadOnly);

    double gt_data[6];
    double* geotransform = gt_data;
    CPLErr error_status
            = const_cast<GDALDataset*>(feature_map->get_gdal_dataset())->GetGeoTransform(geotransform);

    // Second the elevation data
    boost::shared_ptr<raster_util::gdal_raster<float> > dem_map =
            raster_util::open_gdal_rasterSPtr<float>(params.input_dem_file.second, GA_ReadOnly);

    //Check maps for consistency (same dimensions)
    if (feature_map->nCols() != dem_map->nCols())
    {
        throw std::runtime_error("Number of columns in the two comparison maps non-identical");
    }

    if (feature_map->nRows() != dem_map->nRows())
    {
        throw std::runtime_error("Number of rows in the two comparison maps non-identical");
    }

    /**********************************/
    /*  Create list of channel pixels */
    /**********************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*  Making list of channel pixels:   *\n";
    std::cout <<     "*************************************" << std::endl;


    bool feature_has_null = true;
    int success = 0;
    float feature_null_value = const_cast<GDALRasterBand *>(feature_map->get_gdal_band())->GetNoDataValue(&success);
    if (success == 0 )
    {
        feature_null_value = false;
        feature_has_null = -999;
        std::cout << "Feature has no null values, cannot delineate coast to make graph\n";
        return EXIT_SUCCESS;
    }

    const int cols = feature_map->nCols();
    int rows = feature_map->nRows();
    unsigned long num_cells = cols;
    num_cells *= rows;
    boost::progress_display show_progress1(rows);

    std::map<int, std::map<int, VertexDescriptor>  > channel_pixels;
//    bool hasNoData = feature_map->HasNoDataValue();


    int node_id_count = 0;
    for (unsigned int i = 0; i < rows; ++i)
    {
        for (unsigned int j = 0; j < cols; ++j)
        {
            int val = feature_map->get(Coord(i, j));
            if (val != feature_null_value)
            {
                //channel_pixels.insert(std::make_pair(val, Position(i, j)));
                VertexDescriptor v = boost::add_vertex(channel_grph);
                channel_grph[v].row = i;
                channel_grph[v].col = j;
                channel_grph[v].node_id = node_id_count++;
                channel_grph[v].elevation = dem_map->get(Coord(i,j));
                channel_grph[v].x_coord = geotransform[0] + j*geotransform[1] + i*geotransform[2];
                channel_grph[v].y_coord = geotransform[3] + j*geotransform[4] + i*geotransform[5];
//                if (val == GUAGE_CNTRL)
//                {
//                    channel_grph[v].type = GUAGE_CNTRL;
//                }
                channel_pixels[i].insert(std::make_pair(j, v));
            }
            
        }
		++show_progress1;
    }


    /**********************************/
    /*    Create graph of channel     */
    /**********************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*       Making channel graph:       *\n";
    std::cout <<     "*************************************" << std::endl;



    // Add edges between all adjacent channel cells
    boost::progress_display show_progress2(channel_pixels.size());

    typedef std::pair<const int, std::map<int, VertexDescriptor> > RowIteratorType;
    typedef std::pair<const int, VertexDescriptor> ColIteratorType;
    BOOST_FOREACH(RowIteratorType & row_it, channel_pixels)
                {
                    BOOST_FOREACH(ColIteratorType & channel_vertex, row_it.second)
                                {
                                    ChannelNode & v_location = channel_grph[channel_vertex.second];
                                    boost::shared_ptr<Set> Af = find_adjacents(feature_map, v_location.row, v_location.col, 1);
                                    BOOST_FOREACH(ChannelNode & n_location, *Af)
                                                {
                                                    int row = n_location.row;
                                                    int col = n_location.col;
                                                    VertexDescriptor adjacent_vertex = channel_pixels[row][col];
                                                    boost::add_edge(channel_vertex.second, adjacent_vertex, channel_grph);
                                                }
                                }
                    ++show_progress2;
                }



    /**********************************/
    /*     Remove loops in graph      */
    /**********************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*      Removing loops in graph      *\n";
    std::cout <<     "*************************************" << std::endl;
    VertexDescMap idxMap;
    VertexIDMap idMap;
    boost::associative_property_map<VertexDescMap> indexMap(idxMap);
    VertexIterator di, dj;
    boost::tie(di, dj) = boost::vertices(channel_grph);
    for(int i = 0; di != dj; ++di,++i)
    {
        boost::put(indexMap, (*di), channel_grph[*di].node_id);
        idMap.insert(std::make_pair(channel_grph[*di].node_id, (*di)));
    }

    typedef std::map<VertexDescriptor, boost::default_color_type> VertexColourMap;
    typedef std::map<EdgeDescriptor, boost::default_color_type> EdgeColourMap;
    VertexColourMap vertex_colour_map;
    EdgeColourMap edge_colour_map;
    boost::associative_property_map<EdgeColourMap> edge_colours(edge_colour_map);
    boost::associative_property_map<VertexColourMap> vertex_colours(vertex_colour_map);


    std::vector<EdgeDescriptor> removal_list;
    CycleTerminator<EdgeDescriptor, Graph> vis(removal_list);
    boost::undirected_dfs(channel_grph,boost::visitor(vis).vertex_color_map(vertex_colours).edge_color_map(edge_colours).root_vertex(*vertices(channel_grph).first).vertex_index_map(indexMap));
    BOOST_FOREACH(EdgeDescriptor edge, removal_list)
                {
                    boost::remove_edge(edge, channel_grph);
                }

    /**********************************/
    /*   Identify controls in Graph   */
    /**********************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*  Identifying controls in graph:   *\n";
    std::cout <<     "*************************************" << std::endl;

    //Identify terminal nodels and junctional nodes based on number of out_edges.
    boost::progress_display show_progress3(boost::num_vertices(channel_grph));

    std::pair<VertexIterator, VertexIterator> vp;
    for (vp = boost::vertices(channel_grph); vp.first != vp.second; ++vp.first)
    {
        OutDegreeType num_edges = boost::out_degree(*(vp.first), channel_grph);
        if (num_edges == 1)
        {
            channel_grph[*vp.first].type = TERMINAL_CNTRL;
//            terminal_nodes.push_back(*vp.first);
        }
        if (num_edges > 2)
        {
            channel_grph[*vp.first].type = JUNCT_CNTRL;
        }
        ++show_progress3;
    }

    /*****************************************************/
    /*   Remove leaf tributaries if length < trim_level  */
    /*****************************************************/
    std::cout << "\n\n******************************************************\n";
    std::cout <<     "*   Remove leaf tributaries if length < trim_level   *\n";
    std::cout <<     "******************************************************" << std::endl;
    for (int it_trim_levl = 0; it_trim_levl <= params.trim_level ; ++it_trim_levl)
    {
        std::set<EdgeDescriptor> edge_removal_list;
        std::set<VertexDescriptor> vertex_removal_list;
        for (vp = boost::vertices(channel_grph); vp.first != vp.second; ++vp.first)
        {
            if(channel_grph[*vp.first].type == TERMINAL_CNTRL)
            {
                leafBranchSearch(channel_grph, *vp.first, it_trim_levl, edge_removal_list, vertex_removal_list);
            }
        }
        BOOST_FOREACH(EdgeDescriptor e, edge_removal_list)
                    {
                        //std::cout << "deleting edge ( " << channel_grph[boost::source(e,channel_grph)].node_id << " , " << channel_grph[boost::target(e,channel_grph)].node_id << " )" << std::endl;
                        boost::remove_edge(e, channel_grph);
                    }
        BOOST_FOREACH(VertexDescriptor v, vertex_removal_list)
                    {
                        //std::cout << "deleting vertex " << v << std::endl;
                        boost::remove_vertex(v, channel_grph);
                    }
        for (vp = boost::vertices(channel_grph); vp.first != vp.second; ++vp.first)
        {
            if(channel_grph[*vp.first].type == JUNCT_CNTRL)
            {
                if (boost::out_degree(*vp.first, channel_grph) < 3)
                {
                    channel_grph[*vp.first].type = NOT_CNTRL;
                }
            }
            if (boost::out_degree(*(vp.first), channel_grph) == 1)
            {
                channel_grph[*vp.first].type = TERMINAL_CNTRL;
            }

        }
    }

    /**********************************/
    /*   Bridging gaps in graph:   */
    /**********************************/
    std::cout << "\n\n*************************************\n";
    std::cout << "*  Bridging gaps in graph:   *\n";
    std::cout << "*************************************" << std::endl;


    std::list<VertexDescriptor> terminal_nodes;
    for (vp = boost::vertices(channel_grph); vp.first != vp.second; ++vp.first) {
        OutDegreeType num_edges = boost::out_degree(*(vp.first), channel_grph);
        if (num_edges == 1) {
            terminal_nodes.push_back(*vp.first);
        }
    }

    boost::progress_display show_progress4(terminal_nodes.size());
    if (params.gap_threshold > 0)
    {
        for (std::list<VertexDescriptor>::const_iterator it = terminal_nodes.begin(); it != terminal_nodes.end(); ++it)
        {
            double min_dist = std::numeric_limits<double>::max();
            VertexDescriptor adjacent_v = *it;

            for (std::list<VertexDescriptor>::const_iterator it2 = terminal_nodes.begin(); it2 != terminal_nodes.end(); ++it2)
            {
                //distance between it and it2
                if (channel_grph[*it].node_id != channel_grph[*it2].node_id)
                {
                    int row1 = channel_grph[*it].row;
                    int col1 = channel_grph[*it].col;
                    int row2 = channel_grph[*it2].row;
                    int col2 = channel_grph[*it2].col;
                    double dist = sqrt(pow(double(row2 - row1), 2) + pow(double(col2 - col1), 2));
                    if (dist < min_dist)
                    {
                        min_dist = dist;
                        adjacent_v = *it2;
                    }
                }
            }

            if (min_dist <= params.gap_threshold)
            {
                boost::add_edge(*it, adjacent_v, channel_grph);
                channel_grph[*it].type = NOT_CNTRL;
                channel_grph[adjacent_v].type = NOT_CNTRL;
            }
            ++show_progress4;
        }
    }






//    /********************************************/
//    /*        Create control node graph         */
//    /********************************************/
//    std::cout << "\n\n*************************************\n";
//    std::cout <<     "*    Creating control node graph    *\n";
//    std::cout <<     "*************************************" << std::endl;
//
//    Graph controls_grph;
//    VertexIDMap node_id_map;
////    std::pair<VertexIterator, VertexIterator> vp;
//    for (vp = boost::vertices(channel_grph); vp.first != vp.second; ++vp.first)
//    {
//        if (channel_grph[*(vp.first)].type > 1)
//        {
//            VertexDescriptor v = boost::add_vertex(controls_grph);
//            controls_grph[v] = channel_grph[*(vp.first)];
//            node_id_map.insert(std::make_pair(channel_grph[*(vp.first)].node_id, v));
//            channel_grph[*(vp.first)].up_cntrl_id = channel_grph[*(vp.first)].node_id;
//            channel_grph[*(vp.first)].up_cntrl_dist = 1;
//            channel_grph[*(vp.first)].down_cntrl_id = channel_grph[*(vp.first)].node_id;
//            channel_grph[*(vp.first)].down_cntrl_dist = 1;
//        }
//    }
//
//    // Create links in controls graph using a search across the channels graph at each point.
//    for (vp = boost::vertices(controls_grph); vp.first != vp.second; ++vp.first)
//    {
//        int control_id = controls_grph[*(vp.first)].node_id;
//        VertexDescriptor channel_vertex = idMap[control_id];
//        controlSearch(channel_grph, channel_vertex, controls_grph, *(vp.first), node_id_map);
//    }



    /********************************************/
    /*       Print Control graphs to file       */
    /********************************************/
    //std::cout << "\n\n*************************************\n";
    //std::cout <<     "*  Printing control Graph to file   *\n";
    //std::cout <<     "*************************************" << std::endl;
    //std::string controls_file_name = "control_graph";
    //printGraphsToFile(controls_grph, controls_file_name);

    /********************************************/
    /*       Print Channel graphs to file       */
    /********************************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*  Printing channel Graph to file   *\n";
    std::cout <<     "*************************************" << std::endl;
    printGraphsToFile(channel_grph, params.out_coast_boundary_grph_file.first);

    /********************************************/
    /*       Print Channel graphs to SVG       */
    /********************************************/
    std::cout << "\n\n*************************************\n";
    std::cout <<     "*  Printing channel Graph to SVG   *\n";
    std::cout <<     "*************************************" << std::endl;
    printGraphToSVG(channel_grph, params.out_coast_boundary_grph_file.first);

//    /********************************************/
//    /*    Print terminal node ids to file       */
//    /********************************************/
//    std::cout << "\n\n*************************************\n";
//    std::cout <<     "*   Print terminal node ids to file *\n";
//    std::cout <<     "*************************************" << std::endl;
//    std::ofstream tn_fs(control_file_name.c_str());
//    if (tn_fs.is_open())
//    {
//        for (vp = boost::vertices(channel_grph); vp.first != vp.second; ++vp.first)
//        {
//            if (channel_grph[*(vp.first)].type == TERMINAL_CNTRL)
//            {
//                tn_fs << channel_grph[*(vp.first)].node_id << "\tterminal_type\t3\t" << channel_grph[*(vp.first)].elevation << "\n";
//            }
//        }
//    }

    return (EXIT_SUCCESS);

}