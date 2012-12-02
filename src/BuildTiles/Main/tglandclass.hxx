// TGLandclass.hxx -- Class toSimnplify dealing with shape heiarchy:
//                    landclass contains each area (layer) of a tile
//                    Each area is a list of shapes
//                    A shape has 1 or more segments
//                    (when the shape represents line data)
//                    And the segment is a superpoly, containing
//                      - a polygon, triangulation, point normals, face normals, etc.
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
// $Id: construct.hxx,v 1.13 2004-11-19 22:25:49 curt Exp $

#ifndef _TGLANDCLASS_HXX
#define _TGLANDCLASS_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/math/sg_types.hxx>
#include <simgear/io/lowlevel.hxx>

#include <Polygon/polygon.hxx>

#define TG_MAX_AREA_TYPES       128

class TGLandclass
{
public:
    void clear(void);

    inline unsigned int area_size( unsigned int area )
    {
        return polys[area].size();
    }

    inline tgPolygon const& get_poly( unsigned int area, unsigned int poly ) const
    {
        return polys[area][poly];
    }
    inline tgPolygon & get_poly( unsigned int area, unsigned int poly )
    {
        return polys[area][poly];
    }
    inline void add_poly( unsigned int area, const tgPolygon& p )
    {
        polys[area].push_back( p );
    }
    inline void set_poly( unsigned int area, unsigned int poly, const tgPolygon& p )
    {
        polys[area][poly] = p;
    }

    inline tgpolygon_list& get_polys( unsigned int area )
    {
        return polys[area];
    }
    
    inline const SGVec3f& get_face_normal( unsigned int area, unsigned int poly, unsigned int tri ) const
    {
        return polys[area][poly].GetTriFaceNormal( tri );
    }

    inline double get_face_area( unsigned int area, unsigned int poly, unsigned int tri )
    {
        return polys[area][poly].GetTriFaceArea( tri );
    }

    inline std::string get_material( unsigned int area, unsigned int poly )
    {
        return polys[area][poly].GetMaterial();
    }
    inline const tgTexParams& get_texparams( unsigned int area, unsigned int poly )
    {
        return polys[area][poly].GetTexParams();
    }

    void SaveToGzFile( gzFile& fp );
    void LoadFromGzFile( gzFile& fp );

    // Friend for output to stream
    friend std::ostream& operator<< ( std::ostream&, const TGLandclass& );

private:
    tgpolygon_list polys[TG_MAX_AREA_TYPES];
};

#endif // _TGLANDCLASS_HXX
