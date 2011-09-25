#ifndef _RUNWAY_H_
#define _RUNWAY_H_

#include <stdio.h>
#include <stdlib.h>

#include <Polygon/polygon.hxx>
#include <Polygon/superpoly.hxx>
#include <Geometry/point3d.hxx>

#include "texparams.hxx"

#include <osg/Group>

using std::string;

class Runway
{
public:
    Runway(char* def);

    bool IsPrecision()
    {
        return true;
    }

    Point3D GetStart(void)
    {
        return ( Point3D( lon[0], lat[0], 0.0f ));
    }

    Point3D GetEnd(void)
    {
        return ( Point3D( lon[1], lat[1], 0.0f ));
    }

    Point3D GetMidpoint(void)
    {
        return ( Point3D( (lon[0]+lon[1])/2.0f, (lat[0]+lat[1])/2.0f, 0.0f) );
    }

    int BuildOsg( osg::Group* airport );
    int BuildBtg( float alt_m, superpoly_list* rwy_polys, texparams_list* texparams, TGPolygon* accum, TGPolygon* apt_base, TGPolygon* apt_clearing );
    
private:
    // data for whole runway
    int     surface;
    int     shoulder;
    int     centerline_lights;
    int     edge_lights;
    int     dist_remain_signs;
    
    double  width;
    double  length;
    double  heading;
    double  smoothness;

    // data for each end
    char    rwnum[2][16];
    double  lat[2];
    double  lon[2];
    double  threshold[2];
    double  overrun[2];

    int     marking[2];
    int     approach_lights[2];
    int     tz_lights[2];
    int     reil[2];

    // Build Helpers
    TGPolygon gen_wgs84_area( Point3D origin, double length_m, double displ1, double displ2, double width_m, double heading_deg, double alt_m, bool add_mid );
    TGPolygon gen_runway_w_mid( double alt_m, double length_extend_m, double width_extend_m );
//    void      gen_runway_section( const TGPolygon& runway, double startl_pct, double endl_pct, double startw_pct, double endw_pct, double minu, double maxu, double minv, double maxv, double heading,
//                                  const string& prefix, const string& material, superpoly_list *rwy_polys, texparams_list *texparams, TGPolygon *accum  );
//    void      gen_runway_stopway( const TGPolygon& runway_a, const TGPolygon& runway_b, const string& prefix, superpoly_list *rwy_polys, texparams_list *texparams, TGPolygon* accum );
    TGPolygon gen_runway_area_w_extend( double alt_m, double length_extend, double displ1, double displ2, double width_extend );

    void        gen_simple_rwy(         double alt_m, const string& material, superpoly_list *rwy_polys, texparams_list *texparams, TGPolygon *accum );
    void        gen_marked_rwy(         double alt_m, const string& material, superpoly_list *rwy_polys, texparams_list *texparams, TGPolygon *accum );
};

typedef std::vector <Runway *> RunwayList;

#endif