#ifndef _TGSHAPEFILE_HXX
#define _TGSHAPEFILE_HXX

#include "tg_polygon.hxx"
#include "tg_contour.hxx"
#include "tg_rectangle.hxx"
#include "tg_misc.hxx"
#include "clipper.hpp"

class tgShapefile
{
public:

    typedef enum {
        LT_POINT,
        LT_LINE,
        LT_POLY
    } shapefile_layer_t;

    static void  FromContour( const tgContour& subject, bool asPolygon, bool withNumber, const std::string& datasource, const std::string& layer, const std::string& description );
    static void  FromContourList( const std::vector<tgContour>& list, bool asPolygon, bool withNumber, const std::string& datasource, const std::string& layer, const std::string& description );
    
    static void  FromPolygon( const tgPolygon& subject, bool asPolygon, bool withTriangles, const std::string& datasource, const std::string& layer, const std::string& description );
    static void  FromPolygonList( const std::vector<tgPolygon>& list, bool asPolygon, bool withTriangles, const std::string& datasource, const std::string& layer, const std::string& description );
    
    static void  FromGeod( const SGGeod& pt, const std::string& datasource, const std::string& layer, const std::string& description );
    static void  FromGeodList( const std::vector<SGGeod>& list, bool show_dir, const std::string& datasource, const std::string& layer, const std::string& description );

    static void  FromSegment( const tgSegment& subject, bool show_dir, const std::string& datasource, const std::string& layer, const std::string& description );
    static void  FromSegmentList( const std::vector<tgSegment>& list, bool show_dir, const std::string& datasource, const std::string& layer, const std::string& description );

    static void  FromRay( const tgRay& subject, const std::string& datasource, const std::string& layer, const std::string& description );
    static void  FromRayList( const std::vector<tgRay>& list, const std::string& datasource, const std::string& layer, const std::string& description );
    
    static void  FromLine( const tgLine& subject, const std::string& datasource, const std::string& layer, const std::string& description );
    static void  FromLineList( const std::vector<tgLine>& list, const std::string& datasource, const std::string& layer, const std::string& description );

    static void  FromRectangle( const tgRectangle& subject, const std::string& datasource, const std::string& layer, const std::string& description );

    static void  FromClipper( const ClipperLib::Paths& subject, bool asPolygon, const std::string& datasource, const std::string& layer, const std::string& description );
    
    static tgPolygon ToPolygon( const void* subject );

private:
    static bool initialized;

    static void  FromContour( void *lid, const tgContour& subject, bool asPolygon, bool withNumber, const std::string& description );
    static void  FromPolygon( void *lid, const tgPolygon& subject, bool asPolygon, bool withTriangles, const std::string& description );
    static void  FromSegment( void* lid, const tgSegment& subject, bool show_dir, const std::string& description );

    static void* OpenDatasource( const char* datasource_name );
    static void* OpenLayer( void* ds_id, const char* layer_name, shapefile_layer_t type );
    static void* CloseDatasource( void* ds_id );
};

#endif // _TGSHAPEFILE_HXX