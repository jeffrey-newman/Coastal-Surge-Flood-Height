//
//  Types.h
//  move_creek
//
//  Created by a1091793 on 23/01/2015.
//  Copyright (c) 2015 University of Adelaide. All rights reserved.
//

#ifndef Types_h
#define Types_h

#include <vector>
#include <tuple>
#include <boost/shared_ptr.hpp>
#include <boost/graph/adjacency_list.hpp>
#include "Map_Matrix.h"

template <typename DataType>
using Map_Matrix_SPtr = boost::shared_ptr<Map_Matrix<DataType> >;

typedef std::pair<int, int> Position;
typedef std::vector<Position> Set;

typedef Map_Matrix<double> Map_Double;
typedef boost::shared_ptr<Map_Double> Map_Double_SPtr;

typedef Map_Matrix<int32_t> Map_Int;
typedef boost::shared_ptr<Map_Int> Map_Int_SPtr;

typedef Map_Matrix<bool> Map_Bool;
typedef boost::shared_ptr<Map_Bool> Map_Bool_SPtr;

typedef Map_Matrix<float> Map_Float;
typedef boost::shared_ptr<Map_Float> Map_Float_SPtr;

typedef std::vector<std::pair<Position, Position> > Move_Log;

namespace graph = boost::graph;
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Position > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor VertexDescriptor;
typedef boost::graph_traits<Graph>::edge_descriptor EdgeDescriptor;
typedef boost::color_traits<boost::default_color_type> ColourT;
typedef boost::vector_property_map<boost::default_color_type> ColourMap;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_desc;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;

const int NON_FEATURE = 0;

typedef std::list<Position> ChannelLocList;

#endif
