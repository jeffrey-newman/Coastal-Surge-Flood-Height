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
#include <blink/raster/coordinate_2d.h>


namespace raster_util = blink::raster;


struct Pixel
{
    Pixel() : elevation(0), loc({0,0}), surge_required_2_inundate(0)
    {
        
    }
    
    
    Pixel(raster_util::coordinate_2d _loc, float _elevation) :
        elevation(_elevation), loc(_loc), surge_required_2_inundate(0)
    {
        
    }
    Pixel(raster_util::coordinate_2d _loc, float _elevation, float _surge_required) :
    elevation(_elevation), loc(_loc), surge_required_2_inundate(_surge_required)
    {
        
    }
    
    float elevation;
    raster_util::coordinate_2d loc;
    float surge_required_2_inundate;
    
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & elevation;
        ar & loc;
        ar & surge_required_2_inundate;
    }
    
};


struct SortByElevation
{
    bool operator()(const Pixel &lhs, const Pixel &rhs) const
    {
        return lhs.elevation < rhs.elevation;
    }
};

struct SortByRequiredSurgeHeight
{
    bool operator()(const Pixel &lhs, const Pixel &rhs) const
    {
        return lhs.surge_required_2_inundate < rhs.surge_required_2_inundate;
    }
};

#endif
