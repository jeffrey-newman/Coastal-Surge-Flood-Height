
//
//  ReadGraphsFromFile.h
//  InciseDEM
//
//  Created by a1091793 on 2/10/2015.
//  Copyright © 2015 University of Adelaide. All rights reserved.
//

#ifndef ReadGraphsFromFile_h
#define ReadGraphsFromFile_h

#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/graph/graphml.hpp>
#include "../Types.h"


void readGraphFromFile(boost::filesystem::path graph_path, Graph & grph)
{
    boost::dynamic_properties dp;
    dp.property("node_id", boost::get(&ChannelNode::node_id, grph));
    dp.property("row", boost::get(&ChannelNode::row, grph));
    dp.property("col", boost::get(&ChannelNode::col, grph));
    dp.property("down_cntrol_id", boost::get(&ChannelNode::down_cntrl_id, grph));
    dp.property("down_cntrl_dist", boost::get(&ChannelNode::down_cntrl_dist, grph));
    dp.property("up_cntrl_id", boost::get(&ChannelNode::up_cntrl_id, grph));
    dp.property("up_cntrl_dist", boost::get(&ChannelNode::up_cntrl_dist, grph));
    dp.property("control_node_id", boost::get(&ChannelNode::cntrl_node_id, grph));
    dp.property("type", boost::get(&ChannelNode::type, grph));
    dp.property("terminal_type", boost::get(&ChannelNode::terminal_type, grph));
    dp.property("level", boost::get(&ChannelNode::level, grph));
    dp.property("elevation", boost::get(&ChannelNode::elevation, grph));
    dp.property("x_coord", boost::get(&ChannelNode::x_coord, grph));
    dp.property("y_coord", boost::get(&ChannelNode::y_coord, grph));
    
    dp.property("distance", boost::get(&ChannelLink::distance, grph));
    
    std::ifstream graph_stream(graph_path.string().c_str());
    
    
    if (graph_stream.is_open())
    {
        read_graphml(graph_stream, grph, dp);
    }
    
}


#endif /* ReadGraphsFromFile_h */

