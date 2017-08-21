#include <gdal_priv.h>
#include <cpl_conv.h> //For CPLMalloc, whatever this is?
#include "ReadInMap.h"
#include <tuple>



//Some typedefs
namespace fs = boost::filesystem;

template <typename DataFormat>
std::tuple<boost::shared_ptr<Map_Matrix<DataFormat> >, std::string, GeoTransform>  read_in_map(fs::path file_path, GDALDataType data_type, const bool doCategorise) throw(std::runtime_error)
{

    std::string projection;
    GeoTransform transformation;
    GDALDriver driver;
    
	//Check that the file name is valid
	if (!(fs::is_regular_file(file_path)))
	{
		throw std::runtime_error("Input file is not a regular file");
	}	

	// Get GDAL to open the file - code is based on the tutorial at http://www.gdal.org/gdal_tutorial.html
	GDALDataset *poDataset;
	GDALAllRegister(); //This registers all availble raster file formats for use with this utility. How neat is that. We can input any GDAL supported rater file format.

	//Open the Raster by calling GDALOpen. http://www.gdal.org/gdal_8h.html#a6836f0f810396c5e45622c8ef94624d4
	//char pszfilename[] = file_path.c_str(); //Set this to the file name, as GDALOpen requires the standard C char pointer as function parameter.
	poDataset = (GDALDataset *) GDALOpen (file_path.string().c_str(), GA_ReadOnly);
	if (poDataset == NULL)
	{
		throw std::runtime_error("Unable to open file");
	}
	
//	Print some general information about the raster
	double        adfGeoTransform[6]; //An array of doubles that will be used to save information about the raster - where the origin is, what the raster pizel size is.
    
    
    printf( "Driver: %s/%s\n",
            poDataset->GetDriver()->GetDescription(),
            poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

    printf( "Size is %dx%dx%d\n", 
            poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
            poDataset->GetRasterCount() );

    if( poDataset->GetProjectionRef()  != NULL )
    {
        printf( "Projection is `%s'\n", poDataset->GetProjectionRef() );
        projection = poDataset->GetProjectionRef();
    }

    if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        printf( "Origin = (%.6f,%.6f)\n",
                adfGeoTransform[0], adfGeoTransform[3] );

        printf( "Pixel Size = (%.6f,%.6f)\n",
                adfGeoTransform[1], adfGeoTransform[5] );
        
        transformation.x_origin = adfGeoTransform[0];
        transformation.pixel_width = adfGeoTransform[1];
        transformation.x_line_space = adfGeoTransform[2];
        transformation.y_origin = adfGeoTransform[3];
        transformation.pixel_height = adfGeoTransform[4];
        transformation.y_line_space = adfGeoTransform[5];
        
    }


	/// Some raster file formats allow many layers of data (called a 'band', with each having the same pixel size and origin location and spatial extent). We will get the data for the first layer into a Boost Array.
	//Get the data from the first band, 
	// TODO implement method with input to specify what band.
	    GDALRasterBand  *poBand;
        int             nBlockXSize, nBlockYSize;
        int             bGotMin, bGotMax;
        double          adfMinMax[2];
        
        poBand = poDataset->GetRasterBand( 1 );
        poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
        printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
                nBlockXSize, nBlockYSize,
                GDALGetDataTypeName(poBand->GetRasterDataType()),
                GDALGetColorInterpretationName(
                    poBand->GetColorInterpretation()) );

        adfMinMax[0] = poBand->GetMinimum( &bGotMin );
        adfMinMax[1] = poBand->GetMaximum( &bGotMax );
        if( ! (bGotMin && bGotMax) )
            GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);

        printf( "Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1] );
        
        if( poBand->GetOverviewCount() > 0 )
            printf( "Band has %d overviews.\n", poBand->GetOverviewCount() );

        if( poBand->GetColorTable() != NULL )
            printf( "Band has a color table with %d entries.\n", 
                     poBand->GetColorTable()->GetColorEntryCount() );

		;
        int   nXSize = poBand->GetXSize();
		int   nYSize = poBand->GetYSize();
    
		// Note: Map Matrixes indexes are in opposite order to C arrays. e.g. map matrix is indexed by (row, Col) which is (y, x) and c matrices are done by (x, y) which is (Col, Row)
		boost::shared_ptr<Map_Matrix<DataFormat> > in_map(new Map_Matrix<DataFormat>(nYSize, nXSize));
        //get a c array of this size and read into this.
		DataFormat * pafScanline = in_map->get_data_ptr();
        //pafScanline = (float *) CPLMalloc(sizeof(float)*nXSize);
        poBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, 
			pafScanline, nXSize, nYSize, data_type,
                          0, 0 );
    
        //Copy into Map_Matrix.
        //int pafIterator = 0;
		
        //for (int i = 0; i < nXSize; i++)
        //{
        //    for(int j = 0; j < nYSize; j++)
        //    {
        //        in_map->Get(j, i) = pafScanline[pafIterator];
        //        pafIterator++;
        //    }
        //}		
		//for (int i = 0; i < nYSize; i++) //rows
		//{
		//	for (int j = 0; j < nXSize; j++) //cols
		//	{
		//		in_map->Get(i, j) = pafScanline[pafIterator];
		//		pafIterator++;
		//	}
		//}


    
    //free the c array storage
    //delete pafScanline;
	int pbsuccess; // can be used with get no data value
    in_map->SetNoDataValue(poBand->GetNoDataValue(&pbsuccess));
    //This creates a list (map?) listing all the unique values contained in the raster.
	if (doCategorise) in_map->updateCategories();

    
	//Close GDAL, freeing the memory GDAL is using
	GDALClose( (GDALDatasetH)poDataset);

    return (std::make_tuple(in_map, projection, transformation));
}


template <typename DataFormat>
void write_map(fs::path file_path, GDALDataType data_type, boost::shared_ptr<Map_Matrix<DataFormat> > data, std::string WKTprojection, GeoTransform transform, std::string driverName) throw(std::runtime_error)
{
	GDALAllRegister(); //This registers all availble raster file formats for use with this utility. How neat is that. We can input any GDAL supported rater file format.
    
    const char *pszFormat = driverName.c_str();
    GDALDriver * poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if (poDriver == NULL)
    {
        throw std::runtime_error("No driver for file tyle found");
    }
    
    char ** papszMetadata = poDriver->GetMetadata();
    if (!(CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATE, FALSE)))
    {
        throw std::runtime_error("Driver does not support raster creation");
    }
    
    char **papszOptions = NULL;
    
    GDALDataset *poDstDS = poDriver->Create(file_path.string().c_str(), (int)data->NCols(), (int)data->NRows(), 1, data_type, papszOptions);
    
    double adfGeoTransform[6] = {1, 1, 1, 1, 1, 1};
    adfGeoTransform[0] = transform.x_origin;
    adfGeoTransform[1] = transform.pixel_width;
    adfGeoTransform[2] = transform.x_line_space;
    adfGeoTransform[3] = transform.y_origin;
    adfGeoTransform[4] = transform.pixel_height;
    adfGeoTransform[5] = transform.y_line_space;
    
    const char * psz_WKT = WKTprojection.c_str();
    poDstDS->SetGeoTransform(adfGeoTransform);             
    poDstDS->SetProjection(psz_WKT);
    
 //   DataFormat * pafScanline = new DataFormat[data->NCols() * data->NRows()];
 //   int pafIterator = 0;
	//for (int i = 0; i < data->NRows(); i++)
 //   {
	//	for (int j = 0; j < data->NCols(); j++)
 //       {
 //           pafScanline[pafIterator] = data->Get(i, j);
 //           pafIterator++;
 //       }
 //   }

	DataFormat * pafScanline = data->get_data_ptr();
    
    GDALRasterBand * poBand = poDstDS->GetRasterBand(1);
    poBand->SetNoDataValue(data->NoDataValue());
    poBand->RasterIO(GF_Write, 0, 0, (int) data->NCols(), (int) data->NRows(), pafScanline, (int) data->NCols(), (int) data->NRows(), data_type, 0, 0);
    
    GDALClose( (GDALDatasetH) poDstDS);
    delete[] pafScanline;
}




