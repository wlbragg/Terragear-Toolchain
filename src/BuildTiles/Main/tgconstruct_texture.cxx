
// tgconstruct_texture.cxx --Handle texture coordinate generation in tgconstruct
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

#include <simgear/debug/logstream.hxx>
#include "tgconstruct.hxx"

void TGConstruct::CalcTextureCoordinates( void )
{
    for ( unsigned int area = 0; area < TG_MAX_AREA_TYPES; area++ ) {
        for( unsigned int p = 0; p < polys_clipped.area_size(area); p++ ) {
            tgPolygon poly = polys_clipped.get_poly(area, p);
            SG_LOG( SG_CLIPPER, SG_DEBUG, "Texturing " << get_area_name( (AreaType)area ) << "(" << area << "): " <<
                    p+1 << " of " << polys_clipped.area_size(area) << " with " << poly.GetMaterial() );

            poly.Texture( );
            polys_clipped.set_poly(area, p, poly);
        }
    }
}