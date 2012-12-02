// TGLandclass.cxx -- Class toSimnplify dealing with shape heiarchy:
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "tglandclass.hxx"
#include <simgear/io/lowlevel.hxx>

void TGLandclass::clear(void)
{
    int i;

    for (i=0; i<TG_MAX_AREA_TYPES; i++) {
        polys[i].clear();
    }
}

void TGLandclass::LoadFromGzFile(gzFile& fp)
{
    int i, j, count;

    // Load all landclass shapes
    for (i=0; i<TG_MAX_AREA_TYPES; i++) {
        sgReadInt( fp, &count );

        for (j=0; j<count; j++) {
            tgPolygon poly;

            poly.LoadFromGzFile( fp );
            polys[i].push_back( poly );
        }
    }
}
std::ostream& operator<< ( std::ostream& out, const TGLandclass& lc )
{
    int i, j, count;
    tgPolygon poly;

    // Save all landclass shapes
    for (i=0; i<TG_MAX_AREA_TYPES; i++) {
        count = lc.polys[i].size();
        out << count << "\n";
        for (j=0; j<count; j++) {
            out << lc.polys[i][j] << " ";
        }
        out << "\n";
    }

    return out;
}
void TGLandclass::SaveToGzFile(gzFile& fp)
{
    int i, j, count;
    tgPolygon shape;

    // Save all landclass shapes
    for (i=0; i<TG_MAX_AREA_TYPES; i++) {
        count = polys[i].size();
        sgWriteInt( fp, count );

        for (j=0; j<count; j++) {
            polys[i][j].SaveToGzFile( fp );
        }
    }
}
