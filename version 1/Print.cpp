#include "Print.h"

template <typename DataFormat>
void
print(boost::shared_ptr<Map_Matrix<DataFormat> > map, std::ostream & os)
{
	for (unsigned long j = 0; j < map->NCols(); j++)
	{
		os << j << " ";
	}
	os << "\n";

	for (unsigned long i = 0; i < map->NRows(); i++)
	{
		os << i << " ";
		for (unsigned long j = 0; j < map->NCols(); j++)
		{
			DataFormat val = map->Get(i, j);
			if (val == map->NoDataValue())
			{
				os << "NoData ";
			}
			else
			{
				os << val << " ";
			}
		}
		os << "\n";
	}
}