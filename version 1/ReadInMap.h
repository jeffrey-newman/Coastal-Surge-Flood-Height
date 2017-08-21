#ifndef READ_IN_MAP
#define READ_IN_MAP

#include "boost/filesystem.hpp"
#include <exception>
#include <vector>
#include <tuple>
#include <boost/shared_ptr.hpp>
#include "Map_Matrix.h"
#include <gdal_priv.h>

//template <typename DataType>
//using Map_Matrix_SPtr = boost::shared_ptr<Map_Matrix<DataType> >;
//
//typedef std::pair<int, int> Position;
//typedef std::vector<Position> Set;
//
//typedef Map_Matrix<double> Map_Double;
//typedef boost::shared_ptr<Map_Double> Map_Double_SPtr;
//
//typedef Map_Matrix<int32_t> Map_Int;
//typedef boost::shared_ptr<Map_Int> Map_Int_SPtr;


namespace fs = boost::filesystem;

/*! Pixel data types */
typedef enum {
    /*! Unknown or unspecified type */ 		    GDT_Unknown_ = 0,
    /*! Eight bit unsigned integer */           GDT_Byte_ = 1,
    /*! Sixteen bit unsigned integer */         GDT_UInt16_ = 2,
    /*! Sixteen bit signed integer */           GDT_Int16_ = 3,
    /*! Thirty two bit unsigned integer */      GDT_UInt32_ = 4,
    /*! Thirty two bit signed integer */        GDT_Int32_ = 5,
    /*! Thirty two bit floating point */        GDT_Float32_ = 6,
    /*! Sixty four bit floating point */        GDT_Float64_ = 7,
    /*! Complex Int16 */                        GDT_CInt16_ = 8,
    /*! Complex Int32 */                        GDT_CInt32_ = 9,
    /*! Complex Float32 */                      GDT_CFloat32_ = 10,
    /*! Complex Float64 */                      GDT_CFloat64_ = 11,
    GDT_TypeCount_ = 12          /* maximum type # + 1 */
} GDALDataType_;

struct GeoTransform {
    double x_origin;
    double pixel_width;
    double x_line_space;
    double y_origin;
    double pixel_height;
    double y_line_space;

	GeoTransform()
		: x_origin(0), pixel_width(0), x_line_space(0), y_origin(0), pixel_height(0), y_line_space(0)
	{

	}
};

const bool NO_CATEGORISATION = false;
const bool CATEGORISATION = true;


template <typename DataFormat>
std::tuple<boost::shared_ptr<Map_Matrix<DataFormat> >, std::string, GeoTransform>  read_in_map(fs::path file_path, GDALDataType data_type, const bool doCategorise) throw(std::runtime_error);

template <typename DataFormat>
void write_map(fs::path file_path, GDALDataType data_type, boost::shared_ptr<Map_Matrix<DataFormat> > data, std::string WKTprojection, GeoTransform transform, std::string driverName) throw(std::runtime_error);

#endif // !READ_IN_MAP