//#include "stdafx.h"
#include "Map_Matrix.h"

template <typename Numeric_Type>
Map_Matrix<Numeric_Type>::Map_Matrix(unsigned long maxRows, unsigned long maxColumns) :
	ublas::matrix<Numeric_Type>(maxRows, maxColumns),
	isHasNoDataValue(true),
	noDataValue(-9999)
{

}

template <typename Numeric_Type>
Map_Matrix<Numeric_Type>::Map_Matrix(unsigned long maxRows, unsigned long maxColumns, Numeric_Type _val) :
	ublas::matrix<Numeric_Type>(maxRows, maxColumns),
	isHasNoDataValue(true),
	noDataValue(-9999)
{
    for (int i = 0; i < maxRows; i++)
    {
        for (int j = 0; j < maxColumns; j++)
        {
            this->at_element(i,j) = _val;
            valuesContained[_val].numOccurence++;
        }
        
    }
}

template <typename Numeric_Type>
Map_Matrix<Numeric_Type>::Map_Matrix() :
	ublas::matrix<Numeric_Type>(),
	isHasNoDataValue(true),
	noDataValue(-9999)
{

}

template <typename Numeric_Type>
Map_Matrix<Numeric_Type>::Map_Matrix(const Map_Matrix<Numeric_Type> & _mm) :
	ublas::matrix<Numeric_Type>(_mm),
	isHasNoDataValue(_mm.HasNoDataValue()),
	noDataValue(_mm.NoDataValue()),
    valuesContained(_mm.valuesContained)
{

}

template <typename Numeric_Type>
Map_Matrix<Numeric_Type>::~Map_Matrix(void)
{
}

template <typename Numeric_Type>
unsigned long Map_Matrix<Numeric_Type>::NCols(void) const
{
	return this->size2();
}

template <typename Numeric_Type>
unsigned long Map_Matrix<Numeric_Type>::NRows(void) const
{
	return (this->size1());
}

template <typename Numeric_Type>
void Map_Matrix<Numeric_Type>::Redim(unsigned long nRows, unsigned long nCols)
{
	this->resize(nRows, nCols);
    valuesContained.clear();
    for (int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++)
        {
            Numeric_Type val = this->at_element(i,j);
            valuesContained[val].numOccurence++;
        }
        
    }
}

template <typename Numeric_Type>
const Numeric_Type & Map_Matrix<Numeric_Type>::Get(int row_number, int col_number) const
{
	return this->operator()(row_number, col_number);
}

template <typename Numeric_Type>
Numeric_Type & Map_Matrix<Numeric_Type>::Get(int row_number, int col_number)
{
	return this->operator()(row_number, col_number);
}

template <typename Numeric_Type>
Numeric_Type * Map_Matrix<Numeric_Type>::get_data_ptr(void) 
{
	return &(this->data().operator[](0));
	//return &(this->data().op);
}

template <typename Numeric_Type>
void Map_Matrix<Numeric_Type>::Set(int row_number, int col_number, Numeric_Type value)
{
    Numeric_Type old_value = this->operator()(row_number, col_number);
    valuesContained[old_value].numOccurence--;
	this->operator()(row_number, col_number) = value;
    valuesContained[value].numOccurence++;
    
}

template <typename Numeric_Type>
bool Map_Matrix<Numeric_Type>::HasNoDataValue() const
{
	return (isHasNoDataValue);
}

template <typename Numeric_Type>
void Map_Matrix<Numeric_Type>::SetNoDataValue(Numeric_Type value)
{
	this->noDataValue = value;
}

template <typename Numeric_Type>
void Map_Matrix<Numeric_Type>::SetNoDataValue(bool _hasNoDataVal, Numeric_Type _value)
{
	this->isHasNoDataValue = _hasNoDataVal;
	this->noDataValue = _value;
}


template <typename Numeric_Type>
Numeric_Type Map_Matrix<Numeric_Type>::NoDataValue() const
{
	return (this->noDataValue);
}

template <typename Numeric_Type>
void Map_Matrix<Numeric_Type>::updateCategories()
{
    valuesContained.clear();
	for (int i = 0; i < this->NCols(); i++)
    {
        for (int j = 0; j < this->NRows(); j++)
        {
            Numeric_Type val = this->at_element(j,i);
            valuesContained[val].numOccurence++;
        }
    }
}

template <typename Numeric_Type>
int Map_Matrix<Numeric_Type>::nCats() const
{
    return (valuesContained.size());
}


template <typename Numeric_Type>
void Map_Matrix<Numeric_Type>::print()
{
    std::cout << "\t";
    for (int i = 0; i < this->NRows(); i++)
    {
        std::cout << "\t";
        for (int j = 0; j < this->NCols(); j++)
        {
            std::cout << this->at_element(i,j) << " ";
        }
        std::cout << "\n";
    }
}