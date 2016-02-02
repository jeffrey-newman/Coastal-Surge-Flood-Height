#ifndef ADD_ADJACENT
#define ADD_ADJACENT

#include "Types.h"
#include <boost/optional.hpp>

boost::optional<VertexDescriptor>
add_adjacent(VertexDescriptor v, Graph & g, Map_Double_SPtr dem_map, Map_Int_SPtr feature_map, Map_Bool_SPtr processed_map);

#endif //ADD_ADJACENT