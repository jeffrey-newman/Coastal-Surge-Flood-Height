//
//  GZArchive.hpp
//  coastal-surge-inundation
//
//   Created by a1091793 on 15/09/2016.
//    Code modified from http://www.boost.org/doc/libs/1_44_0/libs/iostreams/doc/tutorial/filter_usage.html
//    https://github.com/brownie/cashlib/blob/master/src/Serialize.cpp
//   No license in the repo, as far as I can see.
//

#ifndef GZArchive_h
#define GZArchive_h

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/stream.hpp>

//#include <boost/serialization/base_object.hpp>
//#include <boost/serialization/binary_object.hpp>
//#include <boost/serialization/vector.hpp>
//#include <boost/serialization/hash_map.hpp>
////#include <boost/serialization/map.hpp>
////#include <boost/serialization/string.hpp>
//#include <boost/serialization/split_free.hpp>
////#include <boost/serialization/export.hpp>
//#include <boost/serialization/shared_ptr.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <string>
#include <iostream>
#include <strstream>
#include <sstream>
#include <fstream>

namespace bio = boost::iostreams;

//fn is file name.
template <class T, class Ar>
inline void saveGZFile_(const T & o, const char* fn) {
    //std::ofstream ofs(fn, std::ios::out|std::ios::binary|std::ios::trunc);
    bio::filtering_stream<bio::output> out;
    out.push(bio::gzip_compressor() |
             bio::file_sink(fn, std::ios::binary|std::ios::trunc));
    //out.push(ofs);
    Ar oa(out);
    oa << o;
}

template <class T, class Ar>
inline void loadGZFile_(T & o, const char* fn) {
#if 1
    //std::ifstream ifs(fn, std::ios::in|std::ios::binary);
    //assert(ifs.good()); // catch if file not found
    bio::filtering_istream in;
    in.push(bio::gzip_decompressor());
    in.push(bio::file_source(fn, std::ios_base::binary));
    assert(in.is_complete());
    //in.sync();
    //bio::copy(in, std::cout);
    //in.push(ifs);
    //	try {
    //	std::cout << in << endl;
    Ar ia(in);
    ia >> o;
    //	} catch (std::exception& e) { throw e; }
    
#else
    //	std::ifstream ifs(fn, std::ios::in|std::ios::binary);
    //	assert(ifs.good()); // catch if file not found
    bio::filtering_istream f;
    f.push(bio::gzip_decompressor());
    f.push(bio::file_source(fn, std::ios_base::binary));
    while (f.get() != EOF) {}
    Ar ia(f);
    ia >> o;
#endif
}


#endif /* GZArchive_h */
