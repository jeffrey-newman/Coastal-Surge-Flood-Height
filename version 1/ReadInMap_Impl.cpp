//
//  ReadInMap_Impl.cpp
//  move_creek
//
//  Created by Jeffrey Newman on 28/01/2015.
//  Copyright (c) 2015 University of Adelaide. All rights reserved.
//

#include <stdio.h>
#include "Types.h"
#include "ReadInMap.cpp"


template
std::tuple<Map_Int_SPtr, std::string, GeoTransform>  read_in_map(fs::path file_path, GDALDataType data_type, const bool doCategorise);

template
std::tuple<Map_Double_SPtr, std::string, GeoTransform>  read_in_map(fs::path file_path, GDALDataType data_type, const bool doCategorise);

template
std::tuple<Map_Float_SPtr, std::string, GeoTransform>  read_in_map(fs::path file_path, GDALDataType data_type, const bool doCategorise);

template
void write_map(fs::path file_path, GDALDataType data_type, Map_Int_SPtr data, std::string WKTprojection, GeoTransform transform, std::string driverName);

template
void write_map(fs::path file_path, GDALDataType data_type, Map_Double_SPtr data, std::string WKTprojection, GeoTransform transform, std::string driverName);

template
void write_map(fs::path file_path, GDALDataType data_type, Map_Float_SPtr data, std::string WKTprojection, GeoTransform transform, std::string driverName);