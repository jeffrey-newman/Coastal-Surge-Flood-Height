#include <boost/foreach.hpp>
#include "AddAdjacent.h"
#include "Neighbourhood.h"


boost::optional<VertexDescriptor>
add_adjacent(VertexDescriptor v, Graph & g, Map_Double_SPtr dem_map, Map_Int_SPtr feature_map, Map_Bool_SPtr processed_map)
{
	// Search the neighbourhood.
	//Find out how many unprocessed adjacent creek features there are.
	Position v_location = g[v];
	boost::shared_ptr<Set> Af = find_adjacents(feature_map, v_location.first, v_location.second, 1);
	std::vector<Position> adjacents;
	BOOST_FOREACH(Position & n_location, *Af)
	{
		// Test if adjacent pixel is processed and is creek
		double val = feature_map->Get(n_location.first, n_location.second);
		bool is_processed = processed_map->Get(n_location.first, n_location.second);
		//If not processed and a creek, add to graph.
		if ((val != feature_map->NoDataValue()) && !is_processed)
		{
			adjacents.push_back(n_location);
		}

	}
	//If there are no adjacents, then we are at the end of the channel
	if (adjacents.empty())
	{
		return (boost::none);
	}

	//If there are adjacents, add the highest adjacent as the next downstream node, and form a link.
	//Find highest adjacent, and add this as the downstream node
	Position highest;
	double highest_val = -999;
	BOOST_FOREACH(Position & location, adjacents)
	{
		double val = dem_map->Get(location.first, location.second);
		if (val > highest_val)
		{
			highest = location;
			highest_val = val;
		}
	}
	//Add vertex for node
	VertexDescriptor new_v = boost::add_vertex(g);
	g[new_v].first = highest.first;
	g[new_v].second = highest.second;
	processed_map->Get(highest.first, highest.second) = true; // We have now processed this cell.
	// Add link joining to source node.
	EdgeDescriptor edgeD;
	bool inserted;
	boost::tie(edgeD, inserted) = boost::add_edge(v, new_v, g);
	if (inserted)
	{
		return (new_v);
	}
	else
	{
		return (boost::none);
	}
}
