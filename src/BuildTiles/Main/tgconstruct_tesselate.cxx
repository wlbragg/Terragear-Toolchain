// construct.cxx -- Class to manage the primary data used in the
//                  construction process
//
// Written by Curtis Olson, started May 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// $Id: construct.cxx,v 1.4 2004-11-19 22:25:49 curt Exp $

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

//#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>

#include <Geometry/poly_support.hxx>
//#include <Geometry/poly_extra.hxx>

#include "tgconstruct.hxx"

//using std::string;

void TGConstruct::TesselatePolys( void )
{
    // tesselate the polygons and prepair them for final output
    point_list poly_extra;
    SGVec3d min, max;

    for (unsigned int area = 0; area < TG_MAX_AREA_TYPES; area++) {
        for (unsigned int shape = 0; shape < polys_clipped.area_size(area); shape++ ) {
            unsigned int id = polys_clipped.get_shape( area, shape ).id;

            if ( IsDebugShape( id ) ) {
                WriteDebugShape( "preteselate", polys_clipped.get_shape(area, shape) );
            }

            for ( unsigned int segment = 0; segment < polys_clipped.shape_size(area, shape); segment++ ) {
                TGPolygon poly = polys_clipped.get_poly(area, shape, segment);

                poly.get_bounding_box(min, max);
                poly_extra = nodes.get_geod_inside( Point3D::fromSGVec3(min), Point3D::fromSGVec3(max) );

                SG_LOG( SG_CLIPPER, SG_INFO, "Tesselating " << get_area_name( (AreaType)area ) << "(" << area << "): " <<
                        shape+1 << "-" << segment << " of " << (int)polys_clipped.area_size(area) <<
                        ": id = " << id );

                if ( IsDebugShape( id ) ) {
                    SG_LOG( SG_CLIPPER, SG_INFO, poly );
                }

                TGPolygon tri = polygon_tesselate_alt_with_extra_cgal( poly, poly_extra, false );

                // ensure all added nodes are accounted for
                for (int k=0; k< tri.contours(); k++) {
                    for (int l = 0; l < tri.contour_size(k); l++) {
                        // ensure we have all nodes...
                        nodes.unique_add( tri.get_pt( k, l ) );
                    }
                }

                // Save the triangulation
                polys_clipped.set_tris( area, shape, segment, tri );
            }
        }
    }
}