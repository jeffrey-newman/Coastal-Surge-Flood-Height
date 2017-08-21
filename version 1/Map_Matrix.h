//#pragma once

#ifndef MAP_MATRIX
#define MAP_MATRIX

#include <map>
#include <boost/numeric/ublas/matrix.hpp>




//Some typedefs
//For the Matrices defined in boost::numeric::ublas
namespace ublas = boost::numeric::ublas;

//const int MAX_RASTER_X_RESOLUTION = 50;
//const int MAX_RASTER_Y_RESOLUTION = 50;

struct MapStats
{
    int numOccurence;
    std::vector<std::pair<int, int> > occurenceLocations;
};


template<typename Numeric_Type>
class Map_Matrix :
	public ublas::matrix<Numeric_Type>
{
public:
	Map_Matrix(unsigned long maxRows, unsigned long maxColumns);
	Map_Matrix(unsigned long maxRows, unsigned long maxColumns, Numeric_Type _val);
	Map_Matrix();
	Map_Matrix(const Map_Matrix<Numeric_Type> & _mm);
	virtual ~Map_Matrix(void);


	unsigned long NCols(void) const;
	unsigned long NRows(void) const;
	void Redim(unsigned long nRows, unsigned long nCols);
	const Numeric_Type & Get(int row_number, int col_number) const;
	Numeric_Type & Get(int row_number, int col_number);
	Numeric_Type * get_data_ptr(void);
	void Set(int row_number, int col_number, Numeric_Type value);
	bool HasNoDataValue() const;
	void SetNoDataValue(Numeric_Type value);
	void SetNoDataValue(bool _hasNoDataVal, Numeric_Type _value);
    void updateCategories();
    int nCats() const;
	Numeric_Type NoDataValue() const;
    void print();

private:
	bool isHasNoDataValue;
	Numeric_Type noDataValue;
	std::map<Numeric_Type, MapStats> valuesContained;
};

//#include "Map_Matrix.hxx"


#endif // !MAP_MATRIX
