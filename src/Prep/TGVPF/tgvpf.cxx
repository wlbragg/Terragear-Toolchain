// tgvpf.cxx -- generate polygons from VPF topologies.
//
// Written by David Megginson, started September 2001.   Based on
// code by Curtis Olson.
//
// Copyright (C) 2001  David Megginson  - david@megginson.com
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>

#include STL_IOSTREAM
#include STL_STRING
#include <vector>

#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(cerr);
SG_USING_STD(cout);
#endif
SG_USING_STD(string);
SG_USING_STD(vector);

#include <Geometry/line.hxx>
#include <Geometry/util.hxx>
#include <Polygon/index.hxx>
#include <Polygon/names.hxx>
#include <Polygon/polygon.hxx>
#include <Polygon/split.hxx>
#include <vpf/vpf.hxx>

#ifdef _MSC_VER
#  include <Win32/mkdir.hpp>
#endif



////////////////////////////////////////////////////////////////////////
// Program-wide variables.
////////////////////////////////////////////////////////////////////////

static const char * progname;



////////////////////////////////////////////////////////////////////////
// VPF conversion code.
////////////////////////////////////////////////////////////////////////

/**
 * Print out a VPF bounding rectangle.
 */
static ostream &
operator<< (ostream &output, const VpfRectangle &rect)
{
  output << rect.minX << ','
	 << rect.minY << ','
	 << rect.maxX << ','
	 << rect.maxY;
  return output;
}


/**
 * Convert a VPF point to a regular TerraGear point.
 */
static inline const Point3D
vpf2tg (const VpfPoint &p)
{
  return Point3D(p.x, p.y, p.z);
}


/**
 * Convert a VPF line to a regular TerraGear line.
 */
static const Line
vpf2tg (const VpfLine &l)
{
  Line result;
  int nPoints = l.getPointCount();
  for (int i = 0; i < nPoints; i++)
    result.addPoint(vpf2tg(l.getPoint(i)));
  return result;
}


/**
 * Convert a VPF polygon to a TerraGear polygon.
 */
static const FGPolygon
vpf2tg (const VpfPolygon &polygon)
{
  FGPolygon shape;

  shape.erase();
  int nContours = polygon.getContourCount();
  int contour_num = 0;
  for (int i = 0; i < nContours; i++) {
    const VpfContour contour = polygon.getContour(i);
    int nPoints = contour.getPointCount();
    for (int j = 0; j < nPoints; j++) {
      const VpfPoint p = contour.getPoint(j);
      shape.add_node(contour_num, Point3D(p.x, p.y, p.z));
    }
    shape.set_hole_flag(contour_num, (i > 0));
    contour_num++;
  }
  return shape;
}



////////////////////////////////////////////////////////////////////////
// Processing methods.
////////////////////////////////////////////////////////////////////////

/**
 * An attribute pattern.
 */
struct Attribute
{
  Attribute ()
    : state(true), name(""), value("")
  {}
  bool state;
  string name;
  string value;
};


/**
 * Check whether an attribute pattern matches a shape.
 */
static bool
checkAttribute (const VpfFeature &feature, int index, const Attribute &att)
{
  bool result = false;
  const VpfValue &value = feature.getPropertyValue(att.name, index);
  switch (value.getType()) {
  case VpfValue::TEXT:
    result = (att.value == value.getText());
    break;
  case VpfValue::INT:
    result = (atoi(att.value.c_str()) == value.getInt());
    break;
  case VpfValue::REAL:
    result = (strtod(att.value.c_str(), 0) == value.getReal());
    break;
  default:
    result = false;
    break;
  }
  return (att.state ? result : !result);
}



////////////////////////////////////////////////////////////////////////
// Main program.
////////////////////////////////////////////////////////////////////////


/**
 * Print the command-line usage and exit.
 */
static void
usage ()
{
  cerr << "Usage: "
       << progname
       << " [opts] <db> <library> <coverage> <feature>"
       << endl;
  cerr << "Options:" << endl;
  cerr << "--chunk=<chunk> (default: none)" << endl;
  cerr << "--min-lon=<longitude> (default: -180.0)" << endl;
  cerr << "--min-lat=<latitude> (default: -90.0)" << endl;
  cerr << "--max-lon=<longitude> (default: 180.0)" << endl;
  cerr << "--max-lat=<latitude> (default: 90.0)" << endl;
  cerr << "--area=<area_type> (default: Default)" << endl;
  cerr << "--width=<meters> (default: 50 line, 500 point)" << endl;
  cerr << "--work-dir=<dir> (default: .)" << endl;
  cerr << "--att=<item>:<value> (may be repeated)" << endl;
  cerr << "--att=!<item>:<value> (may be repeated)" << endl;
  exit(2);
}


/**
 * Parse a 10x10 degree chunk name.
 */
static VpfRectangle
parseChunk (string chunk)
{
  VpfRectangle bounds;
  int x_factor;
  int y_factor;

  if (chunk.size() != 7) {
    cerr << "Bad length for chunk specifier " << chunk << endl;
    usage();
  }

  if (chunk[0] == 'w')
    x_factor = -1;
  else if (chunk[0] == 'e')
    x_factor = 1;
  else {
    cerr << "Chunk specifier must begin with 'e' or 'w'" << endl;
    usage();
  }

  if (chunk[4] == 's')
    y_factor = -1;
  else if (chunk[4] == 'n')
    y_factor = 1;
  else {
    cerr << "Second part of chunk specifier must begin with 's' or 'n'"
	 << endl;
    usage();
  }

  bounds.minX = atoi(chunk.substr(1,3).c_str()) * x_factor;
  bounds.minY = atoi(chunk.substr(5).c_str()) * y_factor;
  bounds.maxX = bounds.minX + 10;
  bounds.maxY = bounds.minY + 10;

  return bounds;
}


/**
 * Parse an attribute value specification from the command line.
 */
static const Attribute
parseAttribute (string arg)
{
  Attribute att;

  if (arg[0] == '!') {
    att.state = false;
    arg = arg.substr(1);
  } else {
    att.state = true;
  }

  int pos = arg.find(':');
  if (pos == -1) {
    cerr << "Bad attribute specification: " << arg << endl;
    usage();
  }

  att.name = arg.substr(0, pos);
  att.value = arg.substr(pos + 1);

  return att;
}


/**
 * Main entry point.
 */
int
main (int argc, const char **argv)
{
				// Store the program name for future
				// reference.
  progname = argv[0];

  vector<Attribute> attributes;
  VpfRectangle bounds;

				// Enable logging.
  sglog().setLogLevels( SG_ALL, SG_DEBUG );


				// Default values
  bool invert = false;
  bounds.minX = -180;
  bounds.minY = -90;
  bounds.maxX = 180;
  bounds.maxY = 90;
  AreaType area_type = DefaultArea;
  int width = -1;		// use default
  string work_dir = ".";


  //
  // Process command-line options.
  //
  int argPos = 1;
  while (argPos < argc) {
    string arg = argv[argPos];

    if (arg.find("--chunk=") == 0) {
      bounds = parseChunk(arg.substr(8));
      argPos++;
    }

    else if (arg.find("--min-lon=") == 0) {
      bounds.minX = strtod(arg.substr(10).c_str(), 0);
      argPos++;
    }

    else if (arg.find("--min-lat=") == 0) {
      bounds.minY = strtod(arg.substr(10).c_str(), 0);
      argPos++;
    }

    else if (arg.find("--max-lon=") == 0) {
      bounds.maxX = strtod(arg.substr(10).c_str(), 0);
      argPos++;
    } 

    else if (arg.find("--max-lat=") == 0) {
      bounds.maxY = strtod(arg.substr(10).c_str(), 0);
      argPos++;
    }

    else if (arg.find("--area=") == 0) {
      area_type = get_area_type(arg.substr(7).c_str());
      argPos++;
    }

    else if (arg.find("--width=") == 0) {
      width = atoi(arg.substr(8).c_str());
      argPos++;
    }

    else if (arg.find("--work-dir=") == 0) {
      work_dir = arg.substr(11);
      argPos++;
    }

    else if (arg.find("--invert") == 0) {
      invert = true;
      argPos++;
    }

    else if (arg.find("--att=") == 0) {
      attributes.push_back(parseAttribute(arg.substr(6)));
      argPos++;
    }

    else if (arg == "--") {
      argPos++;
      break;
    }

    else if (arg.find("-") == 0) {
      cerr << "Unrecognized option: " << arg << endl;
      usage();
    }

    else {
      break;
    }
  }

  //
  // Sanity check on bounds.
  //
  if (bounds.minX < -180) {
    cerr << "Minimum longitude out of range (-180:180): "
	 << bounds.minX << endl;
    usage();
  } else if (bounds.maxX > 180) {
    cerr << "Maximum longitude out of range (-180:180): "
	 << bounds.maxX << endl;
    usage();
  } else if (bounds.minY < -90) {
    cerr << "Minimum latitude out of range (-90:90): "
	 << bounds.minY << endl;
    usage();
  } else if (bounds.maxY > 90) {
    cerr << "Maximum latitude out of range (-90:90): "
	 << bounds.maxY << endl;
    usage();
  } else if (bounds.minX >= bounds.maxX) {
    cerr << "Minimum longitude less than maximum longitude" << endl;
    usage();
  } else if (bounds.minY >= bounds.maxY) {
    cerr << "Minimum latitude less than maximum latitude" << endl;
    usage();
  }


  //
  // Process command-line arguments.
  //

  if (argPos != (argc - 4))
    usage();

  const char * database_name = argv[argPos++];
  const char * library_name = argv[argPos++];
  const char * coverage_name = argv[argPos++];
  const char * feature_name = argv[argPos++];

				// Make sure the destination
				// directory exists.
#ifdef _MSC_VER
  fg_mkdir(work_dir.c_str());
#else
  string command = "mkdir -p " + work_dir;
  system(command.c_str());
#endif

  //
  // Make the TerraGear polygon for the bounds.
  //
  FGPolygon bounds_poly;
  bounds_poly.add_node(0, Point3D(bounds.minX, bounds.minY, 0));
  bounds_poly.add_node(0, Point3D(bounds.maxX, bounds.minY, 0));
  bounds_poly.add_node(0, Point3D(bounds.maxX, bounds.maxY, 0));
  bounds_poly.add_node(0, Point3D(bounds.minX, bounds.maxY, 0));


  //
  // Show settings.
  //
  cout << "Database path: " << database_name << endl;
  cout << "Library name: " << library_name << endl;
  cout << "Coverage name: " << coverage_name << endl;
  cout << "Feature name: " << feature_name << endl;
  cout << "Working directory: " << work_dir << endl;
  cout << "Area type: " << get_area_name(area_type) << endl;
  cout << "Point and line width (-1 for default): " << width << endl;
  cout << "Bounding rectangle: " << bounds << endl;
  for (int x = 0; x < attributes.size(); x++) {
    cout << "Attribute " << attributes[x].name
	 << (attributes[x].state ? " = " : " != ")
	 << attributes[x].value << endl;
  }


  //
  // Main processing loop.
  //

				// Initialize the persistant polygon counter.
  string counter_file = work_dir + "/../poly_counter";
  poly_index_init( counter_file );
    
  try {
    VpfDataBase db(database_name);
    VpfLibrary lib = db.getLibrary(library_name);
    VpfFeature feature = lib.getCoverage(coverage_name)
      .getFeature(feature_name);

    const VpfRectangle rect = lib.getBoundingRectangle();
    if (!overlap(rect, bounds)) {
      cerr << "Library coverage does not overlap with area" << endl;
      cerr << "Library: " << rect << endl;
      cerr << "Requested: " << bounds << endl;
      return 1;
    }

    int nTopologies = feature.getTopologyCount();
    int type = feature.getTopologyType();
    cerr << "Searching through " << nTopologies << " topologies" << endl;
    int nAttributes = attributes.size();

    FGPolygon mask;
    for (int i = 0; i < nTopologies; i++) {
      if ((i % 1000) == 0)
	cerr << i << "..." << endl;
      if (feature.isTiled()) {
	VpfRectangle rect = feature.getTile(i).getBoundingRectangle();
	if (!overlap(rect, bounds))
	  continue;
      }

      bool skip = false;
      for (int j = 0; j < nAttributes; j++) {
	if (!checkAttribute(feature, i, attributes[j])) {
	  skip = true;
	  break;
	}
      }
      if (skip)
	continue;
	  
      FGPolygon shape;
      switch (type) {
	// FIXME: check for attributes as well
      case VpfFeature::POINT: {
	const VpfPoint p = feature.getPoint(i);
	if (!inside(p, bounds))
	  continue;
	makePolygon(vpf2tg(p), (width == -1 ? 500 : width), shape);
	break;
      }
      case VpfFeature::LINE: {
	const VpfLine line = feature.getLine(i);
	if (!overlap(line.getBoundingRectangle(), bounds))
	  continue;
	makePolygon(vpf2tg(line), (width == -1 ? 50 : width), shape);
	break;
      }
      case VpfFeature::POLYGON: {
	const VpfPolygon polygon = feature.getPolygon(i);
	if (!overlap(polygon.getBoundingRectangle(), bounds))
	  continue;
	shape = vpf2tg(polygon);
	break;
      }
      case VpfFeature::LABEL: {
	const VpfPoint p = feature.getLabel(i).getPoint();
	if (!inside(p, bounds))
	  continue;
	makePolygon(vpf2tg(p), (width == -1 ? 500 : width), shape);
	break;
      }
      default:
	throw VpfException("Unsupported topology type");
      }

      if (invert) {
	mask = polygon_union(mask, shape);
      } else {
	shape = polygon_int(shape, bounds_poly);
	if (shape.total_size() >= 3) {
	  cout << "Polygon with " << shape.total_size() << " points in "
	     << shape.contours() << " contour(s)" << endl;
	  split_polygon(work_dir, area_type, shape);
	}
      }
    }

				// If we are inverting, we have to
				// wait until the end (and hope for
				// not too large a polygon)
    if (invert) {
      mask = polygon_diff(bounds_poly, mask);
      if (mask.total_size() >= 3) {
	cout << "Inverse polygon with " << mask.total_size() << " points in "
	     << mask.contours() << " contour(s)" << endl;
	split_polygon(work_dir, area_type, mask);
      } else {
	cout << "Inverse polygon is empty" << endl;
      }
    }
    
  } catch (VpfException &e) {
    cerr << "Processing failed with VPF exception: " << e.getMessage() << endl;
    return 1;
  }

  return 0;
}

// end of tgvpf.cxx