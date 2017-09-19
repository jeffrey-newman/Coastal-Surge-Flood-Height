//
//  Types.h
//  move_creek
//
//  Created by a1091793 on 23/01/2015.
//  Copyright (c) 2015 University of Adelaide. All rights reserved.
//

#ifndef Types_h
#define Types_h

#include <functional>
#include <vector>
#include <tuple>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <blink/raster/coordinate_2d.h>
#include "GZArchive.hpp"

namespace raster_util = blink::raster;


struct SurgePixel
{
    SurgePixel() :
            loc({0,0}),
            surge_height_direct(0),
            surge_height_indirect(0),
            direct_distance(0)
    {
        
    }

    SurgePixel(const raster_util::coordinate_2d & _loc) :
            loc(_loc),
            surge_height_direct(0),
            surge_height_indirect(0),
            direct_distance(0)
    {

    }


    SurgePixel(const raster_util::coordinate_2d & _loc,
               const float & _surge_height_direct,
               const float & _surge_height_indirect,
               const float & _direct_distance) :
        loc(_loc),
        surge_height_direct(_surge_height_direct),
        surge_height_indirect(_surge_height_indirect),
        direct_distance(_direct_distance)
    {
        
    }

    raster_util::coordinate_2d loc;
    float direct_distance;
    float surge_height_direct;
    float surge_height_indirect;
    
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & loc;
        ar & direct_distance;
        ar & surge_height_direct;
        ar & surge_height_indirect;
    }
};


//struct SortBySurgeHeight
//{
//    bool operator()(const SurgePixel &lhs, const SurgePixel &rhs) const
//    {
//        return lhs.surge_height < rhs.surge_height;
//    }
//};

struct SortByLocation
{
    bool operator()(const SurgePixel &lhs, const SurgePixel &rhs) const
    {
        return lhs.loc.col < rhs.loc.col;
    }
};


//typedef std::vector<SurgePixel> SurgePixelVector;
//typedef std::vector<SurgePixel *> SurgePixelPtrVector;
typedef boost::shared_ptr<SurgePixel> SurgePixelSPtr;
typedef std::vector<SurgePixelSPtr> SurgePixelSPtrVec;
typedef boost::shared_ptr<SurgePixelSPtrVec > SurgePixelSPtrVecSPtr;



SurgePixelSPtrVecSPtr
loadSurgePixelVector(const boost::filesystem::path & file_path) throw(std::runtime_error)
{
    SurgePixelSPtrVecSPtr surge_pixels(new std::vector<SurgePixelSPtr>);
    if (!boost::filesystem::exists(file_path))
    {
        std::stringstream ss;
        ss << "Reading in surge pixels: " << file_path << ". Path does not exist";
        throw std::runtime_error(ss.str());
    }

    // Read file into list.
    loadGZFile_< SurgePixelSPtrVecSPtr, boost::archive::binary_iarchive>(surge_pixels, file_path.string().c_str());
    return surge_pixels;
}

void
saveSurgePixelVector(const boost::filesystem::path & file_path, SurgePixelSPtrVecSPtr surge_pixels)
{
    saveGZFile_< SurgePixelSPtrVecSPtr, boost::archive::binary_oarchive>(surge_pixels, file_path.string().c_str());
}

enum TNodeType{NOT_CNTRL = 1, TERMINAL_CNTRL = 2, GUAGE_CNTRL = 4, JUNCT_CNTRL = 8};
enum TerminalType{NOT_TERMINAL = 1, OUTFLOW = 2, SOURCE=3};

struct ChannelNode;
struct ChannelLink;
//namespace graph = boost::graph;
//typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, ChannelNode > Graph;
typedef boost::adjacency_list<boost::setS, boost::setS, boost::undirectedS, ChannelNode, ChannelLink > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor VertexDescriptor;
typedef boost::graph_traits<Graph>::edge_descriptor EdgeDescriptor;
typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
typedef std::map<VertexDescriptor, size_t> VertexDescMap;
typedef std::map<int, VertexDescriptor> VertexIDMap;
typedef boost::graph_traits<Graph>::degree_size_type OutDegreeType;

struct ChannelNode
{
    int node_id;
    int cntrl_node_id;
    int row;
    int col;
    int down_cntrl_id;
    int down_cntrl_dist;
    int up_cntrl_id;
    int up_cntrl_dist;
    int type;
    int terminal_type;
    double level;
    double elevation;
    double x_coord;
    double y_coord;

    ChannelNode
            (): node_id(-1), cntrl_node_id(-1), row(-1), col(-1), down_cntrl_id(-1), down_cntrl_dist(-1), up_cntrl_id(-1), up_cntrl_dist(-1), type(NOT_CNTRL), terminal_type(NOT_TERMINAL), level(-999), elevation(-999), x_coord(-999), y_coord(-999)
    {

    }
};

struct ChannelLink
{
    int link_id;
    int distance;

    ChannelLink(): link_id(-1), distance(-1)
    {

    }
};


typedef std::vector<ChannelNode> Set;



#endif
