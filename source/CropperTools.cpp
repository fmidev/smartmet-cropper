// ======================================================================
/*!
 * \file
 * \brief Utility subroutines for cropper
 */
// ======================================================================

#include "CropperTools.h"

// newbase
#include "NFmiAlignment.h"
#include "NFmiArea.h"
#include "NFmiAreaFactory.h"
#include "NFmiCmdLine.h"
#include "NFmiFace.h"
#include "NFmiFileSystem.h"
#include "NFmiFileSystem.h"
#include "NFmiFreeType.h"
#include "NFmiImage.h"
#include "NFmiImageTools.h"
#include "NFmiLocationFinder.h"
#include "NFmiPath.h"
#include "NFmiStringTools.h"
#include "NFmiStringTools.h"

// imagine
#include "NFmiImageTools.h"

// system

#include <algorithm>
#include <clocale>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// For getpid:
#include "sys/types.h"
#include "unistd.h"

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage information
 */
// ----------------------------------------------------------------------

void usage(const string & theProgName)
{
  cout << "Usage: " << theProgName << " [options]" << endl
	   << endl
	   << "Available options are:" << endl
	   << endl
	   << "   -g [geometry]\t<width>x<height>+<x1>+<y1>" << endl
	   << "   -c [centergeometry]\t<width>x<height>+<xc>+<yc>" << endl
	   << "   -l [latlongeometry]\t<width>x<height>+<lon>+<lat>:<mapname>" << endl
	   << "   -p [namegeometry]\t<width>x<height>+<placename>:<mapname>" << endl
	   << "   -M [image]\t\t<filename> or square or square:color" << endl
	   << "   -L [labelspecs]\t<text>,<lon>,<lat>,<dx>,<dy>,<align>,<xmargin>,<ymargin>,<font>,<color>,<bgcolor>" << endl
	   << "   -T [stampspecs]\t<x>,<y>,<format>,<type>,<xmargin>,<ymargin>,<font>,<color>,<bgcolor>" << endl
	   << "   -t [locale]\t\tEurope/Helsinki, UTC etc" << endl
	   << "   -k [lang]\t\tLanguage, for example fi_FI" << endl
	   << "   -I [imagespecs]\t<imagefile>,<x>,<y>,..." << endl
	   << "   -Z [RGBA]\t\tReduce color accuracy, default = 5550" << endl
	   << "   -A\t\t\tKeep alpha channel" << endl
	   << "   -f [imagefile]" << endl
	   << "   -o [outputfile]" << endl
	   << "   -C" << endl
	   << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Test whether the area implies a shifted meridian
 */
// ----------------------------------------------------------------------

bool centralmeridian(const NFmiArea & theArea)
{
  // the area longitudes
  
  double x1 = theArea.BottomLeftLatLon().X();
  double x2 = theArea.TopRightLatLon().X();
  
  // make sure the corners are in incremental order
  if(x2 < x1) x2+=360;
  
  if(x1 < -180 && x2 >= -180)
	return -180;
  if(x1 <= 180 && x2 > 180)
	return 180;
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Adjust latlon to different meridian systems if necessary
 */
// ----------------------------------------------------------------------

NFmiPoint checkmeridian(const NFmiPoint & theLatLon,
						const NFmiArea & theArea)
{
  const float meridian = centralmeridian(theArea);
  if(meridian == 0)
	return theLatLon;

  if(std::abs(theLatLon.X()-meridian) < 180)
	return theLatLon;

  const float shift = (meridian < 0 ? -360 : 360);
  return NFmiPoint(theLatLon.X()+shift,theLatLon.Y());
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the given timezone
 *
 * \param theZone The time zone descriptor
 */
// ----------------------------------------------------------------------

void set_timezone(const string & theZone)
{
  // must be static, see putenv specs!
  static string tzvalue = "TZ="+theZone;
  putenv(const_cast<char *>(tzvalue.c_str()));
  tzset();
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a time for HTTP output
 *
 * The output is generated with strftime using format
 * "%a,  %d  %b  %Y  %H:%M:%S" as adviced in the
 * man-pages for strftime.
 */
// ----------------------------------------------------------------------

const string format_time(const ::time_t theTime)
{
  const struct ::tm * t = gmtime(&theTime);
  const ::size_t MAXLEN = 100;
  char buffer[MAXLEN];
  ::size_t n = strftime(buffer,MAXLEN,"%a, %d %b %Y %H:%M:%S GMT",t);
  string ret(buffer,0,n);
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Output the given imagefile
 */
// ----------------------------------------------------------------------

void http_output_image(const string & theFile)
{
  ifstream in(theFile.c_str(), ios::in|ios::binary);
  if(!in)
	throw runtime_error("Image '"+theFile+"' was lost!");

  // We expire everything in 24 hours
  const long maxage = 24*3600;
  ::time_t expiration_time = time(0) + maxage;
  ::time_t last_modified = NFmiFileSystem::FileModificationTime(theFile);

  string mime = Imagine::NFmiImageTools::MimeType(theFile);

  cout << "Content-Type: image/" << mime << endl
	   << "Expires: " << format_time(expiration_time) << endl
	   << "Last-Modified: " << format_time(last_modified) << endl
	   << "Cache-Control: max-age=" << maxage << ", public" << endl
	   << "Content-Length: " << NFmiFileSystem::FileSize(theFile) << endl
	   << endl
	   << in.rdbuf();
  in.close();
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert QUERY_STRING value to cache name
 */
// ----------------------------------------------------------------------

const string cachename(const string & theQueryString)
{
  string path = "/tmp/cropper";

  if(!NFmiFileSystem::CreateDirectory(path))
	throw runtime_error("Unable to create path /tmp/cropper for temporary files");

  // Encode
  string name1 = NFmiStringTools::UrlEncode(theQueryString);
  // Remove %-characters as dangerous
  string name2;
  remove_copy(name1.begin(),name1.end(),back_inserter(name2),'%');

  return (path+'/'+name2);

}

// ----------------------------------------------------------------------
/*!
 * \brief Output a "not modified" response if possible
 *
 * \param theFile The file whose modification time is requested
 * \return True, if a not-modified response was sent
 */
// ----------------------------------------------------------------------

bool not_modified(const string & theFile)
{
  if(getenv("QUERY_STRING") == 0 ||
	 getenv("HTTP_IF_MODIFIED_SINCE") == 0)
	return false;

  // If cached file exists and is newer than the original
  // file, respond "Not Modified"

  const string tmpfile = cachename(getenv("QUERY_STRING"));

  // Safety checks

  if(!NFmiFileSystem::FileExists(tmpfile) ||
	 !NFmiFileSystem::FileExists(theFile))
	{
	  cout << "Status: 304 Not Modified" << endl;
	  return true;
	}

  // Age check
  if(NFmiFileSystem::FileModificationTime(theFile) >=
	 NFmiFileSystem::FileModificationTime(tmpfile))
	return false;

  cout << "Status: 304 Not Modified" << endl;
  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Output image from cache if possible
 *
 * \param theQueryString The value of the QUERY_STRING variable
 * \return True, if a cached image was output
 */
// ----------------------------------------------------------------------

bool http_output_cache(const char * theQueryString)
{
  if(theQueryString == 0)
	return false;

  const string filename = cachename(theQueryString);
  if(!NFmiFileSystem::FileExists(filename))
	return false;

  http_output_image(filename);
  return true;

}

// ----------------------------------------------------------------------
/*!
 * \brief Establish the map projection for the given map name
 *
 * Throws if the map does not have a system description
 *
 * \param theMap The map name
 * \return The created NFmiArea object
 */
// ----------------------------------------------------------------------

auto_ptr<NFmiArea> create_map(const string & theMap)
{
  const string areafile = "/data/share/maps/" + theMap + "/area.cnf";
  if(!NFmiFileSystem::FileExists(areafile))
	throw runtime_error("Map name '"+theMap+"' is not recognized");

  ifstream in(areafile.c_str(),ios::in);
  if(!in)
	throw runtime_error("Failed to open system file '"+areafile+"' for reading");

  // Seek the first "projection" token, the description follows
  // However we must be sure to skip comment rows

  string token;
  while(in >> token)
	{
	  if(token == "#")
		{
		  in.ignore(1000000,'\n');
		}
	  else if(token == "projection")
		{
		  in >> token;
		  return NFmiAreaFactory::Create(token);
		}
	}
  throw runtime_error("The file '"+areafile+"' does not contain a projection description with a projection command");

}

// ----------------------------------------------------------------------
/*!
 * \brief Establish the coordinate for a named location
 *
 * Throws if the location name is unknown
 *
 * \param theName The location name
 * \return The location longitude and latitude
 */
// ----------------------------------------------------------------------

const NFmiPoint find_location(const string & theName)
{
  const string coordfile = "/data/share/coordinates/kaikki.txt";

  NFmiLocationFinder finder;
  if(!finder.AddFile(coordfile,false))
	throw runtime_error("Failed to read '"+coordfile+"'");

  const NFmiPoint lonlat = finder.Find(theName);
  if(finder.LastSearchFailed())
	throw runtime_error("Location name '"+theName+"' coordinates unknown");
  
  return lonlat;
  
}

// ----------------------------------------------------------------------
/*!
 * \brief Output the given imagefile
 */
// ----------------------------------------------------------------------

void http_output_image(const Imagine::NFmiImage & theImage,
					   const string & theFile,
					   const string & theType,
					   bool theCacheFlag)
{
  // We expire everything in 24 hours
  const long maxage = 24*3600;
  ::time_t expiration_time = time(0) + maxage;
  ::time_t last_modified = NFmiFileSystem::FileModificationTime(theFile);

  // This name is unique since the process number is unique

  string tmpfile;
  if(theCacheFlag)
	tmpfile = ("/tmp/cropper/"
			   + NFmiStringTools::Convert(::getpid())
			   + "." + theType);
  else
	tmpfile = cachename(getenv("QUERY_STRING"));

  theImage.Write(tmpfile,theType);

  ifstream in(tmpfile.c_str(), ios::in|ios::binary);
  if(!in)
	throw runtime_error("Was unable to create temporary file");

  cout << "Content-Type: image/" << theType << endl
	   << "Expires: " << format_time(expiration_time) << endl
	   << "Last-Modified: " << format_time(last_modified) << endl
	   << "Cache-Control: max-age=" << maxage << ", public" << endl
	   << "Content-Length: " << NFmiFileSystem::FileSize(theFile) << endl
	   << endl
	   << in.rdbuf();
  in.close();

  if(theCacheFlag)
	NFmiFileSystem::RemoveFile(tmpfile);

}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a cornered geometry string
 *
 * The string format is <width>x<height>+<x1>+<y1>.
 *
 * Throws if parsing the string fails.
 *
 * \param theGeometry The geometry string
 * \param x1 Reference to variable in which to store x1
 * \param y1 Reference to variable in which to store y1
 * \param width Reference to variable in which to store width
 * \param height Reference to variable in which to store height
 */
// ----------------------------------------------------------------------

void parse_geometry(const string & theGeometry,
					int & x1,
					int & y1,
					int & width,
					int & height)
{
  if(theGeometry.empty())
	throw runtime_error("The geometry specification is empty!");

  istringstream geom(theGeometry);
  char ch1, ch2, ch3;
  geom >> width >> ch1 >> height >> ch2 >> x1 >> ch3 >> y1;
  if(geom.fail() || ch1 != 'x' || ch2 != '+' || ch3!='+')
	throw runtime_error("Failed to parse geometry '"+theGeometry+"'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a centered geometry string
 *
 * The string format is <width>x<height>+<xc>+<yc>.
 *
 * Throws if parsing the string fails.
 *
 * \param theGeometry The geometry string
 * \param xc Reference to variable in which to store xc
 * \param yc Reference to variable in which to store yc
 * \param width Reference to variable in which to store width
 * \param height Reference to variable in which to store height
 */
// ----------------------------------------------------------------------

void parse_center_geometry(const string & theGeometry,
						   int & xc,
						   int & yc,
						   int & width,
						   int & height)
{
  if(theGeometry.empty())
	throw runtime_error("The geometry specification is empty!");

  istringstream geom(theGeometry);
  char ch1, ch2, ch3;
  geom >> width >> ch1 >> height >> ch2 >> xc >> ch3 >> yc;
  if(geom.fail() || ch1 != 'x' || ch2 != '+' || ch3!='+')
	throw runtime_error("Failed to parse geometry '"+theGeometry+"'");
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a latlon geometry string
 *
 * The string format is <width>x<height>+<lon>+<lat>:<mapname>
 *
 * Throws if parsing the string fails.
 *
 * \param theGeometry The geometry string
 * \param xc Reference to variable in which to store xc
 * \param yc Reference to variable in which to store yc
 * \param width Reference to variable in which to store width
 * \param height Reference to variable in which to store height
 * \return The projection
 */
// ----------------------------------------------------------------------

auto_ptr<NFmiArea> parse_latlon_geometry(const string & theGeometry,
										 int & xc,
										 int & yc,
										 int & width,
										 int & height)
{
  if(theGeometry.empty())
	throw runtime_error("The geometry specification is empty!");

  istringstream geom(theGeometry);
  string mapname;
  double lon,lat;
  char ch1,ch2;
  geom >> width >> ch1 >> height >> lon >> lat >> ch2 >> mapname;
  if(geom.fail() || ch1 != 'x' || ch2 != ':')
	throw runtime_error("Failed to parse geometry '"+theGeometry+"'");

  if(lon<-180 || lon>180)
	throw runtime_error("Longitude out of bounds in '"+theGeometry+"'");

  if(lat<-90 || lat>90)
	throw runtime_error("Latitude out of bounds in '"+theGeometry+"'");

  auto_ptr<NFmiArea> area = create_map(mapname);

  NFmiPoint center = checkmeridian(NFmiPoint(lon,lat),*area);
  center = area->ToXY(center);

  xc = static_cast<int>(0.5+center.X());
  yc = static_cast<int>(0.5+center.Y());

  return area;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a named geometry string
 *
 * The string format is <width>x<height>+<placename>:<mapname>
 *
 * Throws if parsing the string fails.
 *
 * \param theGeometry The geometry string
 * \param xc Reference to variable in which to store xc
 * \param yc Reference to variable in which to store yc
 * \param width Reference to variable in which to store width
 * \param height Reference to variable in which to store height
 */
// ----------------------------------------------------------------------

auto_ptr<NFmiArea> parse_named_geometry(const string & theGeometry,
										int & xc,
										int & yc,
										int & width,
										int & height)
{
  if(theGeometry.empty())
	throw runtime_error("The geometry specification is empty!");

  istringstream geom(theGeometry);
  char ch1,ch2;
  geom >> width >> ch1 >> height >> ch2;
  string cityname,mapname;
  getline(geom,cityname,':');
  getline(geom,mapname);
  if(geom.fail() || ch1 != 'x' || ch2 != '+')
	throw runtime_error("Failed to parse geometry '"+theGeometry+"'");
  
  const NFmiPoint city = find_location(cityname);

  auto_ptr<NFmiArea> area = create_map(mapname);
  
  NFmiPoint center = checkmeridian(city,*area);
  center = area->ToXY(center);

  xc = static_cast<int>(0.5+center.X());
  yc = static_cast<int>(0.5+center.Y());

  return area;

}

// ----------------------------------------------------------------------
/*!
 * \brief Crop an image given cornered geometry
 *
 * \param theImage The image to crop
 * \param theX1 The corner X-coordinate
 * \param theY1 The corner Y-coordinate
 * \param theWidth The width
 * \param theHeight The height
 * \param theXoff The new X-origin
 * \param theYoff The new Y-origin
 * \return auto_ptr to the cropped image
 */
// ----------------------------------------------------------------------

auto_ptr<Imagine::NFmiImage> crop_corner(const Imagine::NFmiImage & theImage,
										 int theX1,
										 int theY1,
										 int theWidth,
										 int theHeight,
										 int & theXoff,
										 int & theYoff)
{
  if(theWidth < 1 || theHeight < 1)
	throw runtime_error("Image width and height must be positive");

  // Shrink size if desired size is larger than image
  const int width = min(theWidth,theImage.Width());
  const int height = min(theHeight,theImage.Height());

  // Make sure start point is not negative
  int x1 = max(0,theX1);
  int y1 = max(0,theY1);
  // We final end points (+1) would be
  int x2 = min(theImage.Width(),x1+width);
  int y2 = min(theImage.Height(),y1+height);
  // And then the possibly adjusted start points are
  x1 = x2-width;
  y1 = y2-height;

  theXoff = x1;
  theYoff = y1;

  auto_ptr<Imagine::NFmiImage> image(new Imagine::NFmiImage(width,height));
  for(int i=x1; i<x2;i++)
	for(int j=y1; j<y2; j++)
	  (*image)(i-x1,j-y1) = theImage(i,j);

  return image;
}

// ----------------------------------------------------------------------
/*!
 * \brief Crop an image given centered geometry
 *
 * \param theImage The image to crop
 * \param theXC The center X-coordinate
 * \param theYC The center Y-coordinate
 * \param theWidth The width
 * \param theHeight The height
 * \param theXoff The new X-origin
 * \param theYoff The new Y-origin
 * \return auto_ptr to the cropped image
 */
// ----------------------------------------------------------------------

auto_ptr<Imagine::NFmiImage> crop_center(const Imagine::NFmiImage & theImage,
										 int theXC,
										 int theYC,
										 int theWidth,
										 int theHeight,
										 int & theXoff,
										 int & theYoff)
{
  if(theWidth < 1 || theHeight < 1)
	throw runtime_error("Image width and height must be positive");

  // Shrink size if desired size is larger than image
  const int width = min(theWidth,theImage.Width());
  const int height = min(theHeight,theImage.Height());

  // Make sure start point is not negative
  int x1 = max(0,theXC-width/2);
  int y1 = max(0,theYC-height/2);
  // We final end points (+1) would be
  int x2 = min(theImage.Width(),x1+width);
  int y2 = min(theImage.Height(),y1+height);
  // And then the possibly adjusted start points are
  x1 = x2-width;
  y1 = y2-height;

  theXoff = x1;
  theYoff = y1;

  auto_ptr<Imagine::NFmiImage> image(new Imagine::NFmiImage(width,height));
  for(int i=x1; i<x2;i++)
	for(int j=y1; j<y2; j++)
	  (*image)(i-x1,j-y1) = theImage(i,j);

  return image;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a color description
 */
// ----------------------------------------------------------------------

Imagine::NFmiColorTools::Color parse_color(const string & theColor)
{
  if(theColor.empty())
	return Imagine::NFmiColorTools::MissingColor;

  // Handle hex format number AARRGGBB or RRGGBB
  
  const char ch1 = theColor[0];
  if(ch1=='#')
	return Imagine::NFmiColorTools::HexToColor(theColor.substr(1));

  // Handle ascii format

  return Imagine::NFmiColorTools::ColorValue(theColor);
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract YYYYMMDDHHMI timestamps from given string
 *
 * \param theString The string from which to extract the stamps
 * \return Vector of found timestamp strings
 */
// ----------------------------------------------------------------------

const vector<string> extract_timestamps(const string & theString)
{
  vector<string> ret;
  
  string::size_type pos1 = 0;

  while(pos1 < theString.size())
	{
	  string::size_type pos2 = pos1;
	  while(pos2<theString.size() && ::isdigit(theString[pos2]))
		++pos2;

	  if(pos2-pos1 < 12)
		pos1 = pos2 + 1;
	  else
		{
		  ret.push_back(theString.substr(pos1,12));
		  pos1 += 12;
		}
	}

  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse a timestamp of form YYYYMMDDHHMI into local time
 */
// ----------------------------------------------------------------------

const ::tm parse_stamp(const string & theStamp)
{
  // As UTC time
  ::tm utc;
  utc.tm_sec = 0;
  utc.tm_min  = NFmiStringTools::Convert<int>(theStamp.substr(10,2));
  utc.tm_hour = NFmiStringTools::Convert<int>(theStamp.substr(8,2));
  utc.tm_mday = NFmiStringTools::Convert<int>(theStamp.substr(6,2));
  utc.tm_mon  = NFmiStringTools::Convert<int>(theStamp.substr(4,2)) - 1;
  utc.tm_year = NFmiStringTools::Convert<int>(theStamp.substr(0,4)) - 1900;
  utc.tm_wday = -1;
  utc.tm_yday = -1;
  utc.tm_isdst = -1;

  ::time_t epochtime = ::timegm(&utc);	// Linux extension

  struct ::tm * local = ::localtime(&epochtime);

  // Return by value, localtime owns the above struct
  struct ::tm ret = *local;
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Make the timestamp text
 */
// ----------------------------------------------------------------------

string make_timestamp(const string & theFilename,
					  const string & theType,
					  const string & theFormat)
{
  string ret;
  
  const vector<string> stamps = extract_timestamps(theFilename);
  const string obsstamp = (stamps.size() >= 1 ? stamps[0] : "");
  const string forstamp = (stamps.size() >= 2 ? stamps[1] : "");

  const int MAXSIZE = 100;
  char buffer[MAXSIZE+1];

  ::tm obstime;
  ::tm fortime;

  if(!obsstamp.empty()) obstime = parse_stamp(obsstamp);
  if(!forstamp.empty()) fortime = parse_stamp(forstamp);

  if(theType == "obs" && !obsstamp.empty())
	{
	  ::strftime(buffer,MAXSIZE,theFormat.c_str(),&obstime);
	  ret = buffer;
	}
  else if(theType == "for" && !forstamp.empty())
	{
	  ::strftime(buffer,MAXSIZE,theFormat.c_str(),&fortime);
	  ret = buffer;
	}
  else if(theType == "forobs" && !forstamp.empty())
	{
	  ::strftime(buffer,MAXSIZE,theFormat.c_str(),&fortime);
	  ret = buffer;
	  // append forecast length
	  ::time_t otime = ::mktime(&obstime);
	  ::time_t ftime = ::mktime(&fortime);
	  int hours = (otime-ftime)/3600; 
	  ret += ' ';
	  ret += (hours < 0 ? '-' : '+');
	  ret += NFmiStringTools::Convert(hours);
	}
  else
	{
	  ret = theFormat;
	}
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Draw a timestamp onto the image
 *
 * \param theImage The image to draw into
 * \param theOptions The options in string form
 */
// ----------------------------------------------------------------------

void draw_timestamp(Imagine::NFmiImage & theImage,
					const string & theOptions,
					const string & theFilename)
{
  // Initialize the defaults

  string format = "%H:%M";
  string type = "obs";
  int xmargin = 1;
  int ymargin = 1;
  string font = "misc/6x13.pcf.gz:6x13";
  string color = "black";
  string backgroundcolor = "#20B4B4B4";

  vector<string> parts = NFmiStringTools::Split(theOptions);

  // Compulsory options

  if(parts.size() < 2)
	throw runtime_error("Too short option string '"+theOptions+"'");

  int x = NFmiStringTools::Convert<int>(parts[0]);
  int y = NFmiStringTools::Convert<int>(parts[1]);

  // Optional parts

  vector<string>::size_type i = 1;
  if(parts.size() > i+1 && !parts[++i].empty()) format = parts[i];
  if(parts.size() > i+1 && !parts[++i].empty()) type = parts[i];
  if(parts.size() > i+1 && !parts[++i].empty()) xmargin = ymargin = NFmiStringTools::Convert<int>(parts[i]);
  if(parts.size() > i+1 && !parts[++i].empty()) ymargin = NFmiStringTools::Convert<int>(parts[i]);
  if(parts.size() > i+1 && !parts[++i].empty()) font = parts[i];
  if(parts.size() > i+1 && !parts[++i].empty()) color = parts[i];
  if(parts.size() > i+1 && !parts[++i].empty()) backgroundcolor = parts[i];

  // Extra parts

  if(parts.size() > i+1)
	throw runtime_error("Too many -T parts in option '"+theOptions+"'");

  // Parse the font option

  vector<string> fontparts = NFmiStringTools::Split(font,":");
  if(fontparts.size() != 2)
	throw runtime_error("Invalid font specification for option -T : '"+font+"'");
  font = fontparts[0];
  fontparts = NFmiStringTools::Split(fontparts[1],"x");
  if(fontparts.size() != 2)
	throw runtime_error("Invalid font size specification for option -T : '"+fontparts[1]+"'");
  const int width = NFmiStringTools::Convert<int>(fontparts[0]);
  const int height = NFmiStringTools::Convert<int>(fontparts[1]);
  
  // Parse the font color option

  Imagine::NFmiColorTools::Color fontcolor = parse_color(color);
  if(fontcolor == Imagine::NFmiColorTools::MissingColor)
	throw runtime_error("Unknown font color '"+color+"'");

  // Parse the background color option

  Imagine::NFmiColorTools::Color backcolor = parse_color(backgroundcolor);
  if(backcolor == Imagine::NFmiColorTools::MissingColor)
	throw runtime_error("Unknown font color '"+backgroundcolor+"'");

  // Establish text coordinates and alignment

  Imagine::NFmiAlignment align = Imagine::kFmiAlignNorthWest;
  int xx = x;
  int yy = y;
  if(x >= 0 && y >= 0)
	;
  else if(x >= 0 && y < 0)
	{
	  yy = theImage.Height() + y + 1;
	  align = Imagine::kFmiAlignSouthWest;
	}
  else if(x < 0 && y >= 0)
	{
	  xx = theImage.Width() + x + 1;
	  align = Imagine::kFmiAlignNorthEast;
	}
  else
	{
	  xx = theImage.Width() + x + 1;
	  yy = theImage.Height() + y + 1;
	  align = Imagine::kFmiAlignSouthEast;
	}
  
  // Create the text to be rendered

  string text = make_timestamp(theFilename,type,format);

  // Create the face and setup the background

  Imagine::NFmiFace face(font,width,height);
  face.Background(true);
  face.BackgroundColor(backcolor);
  face.BackgroundMargin(xmargin,ymargin);

  // Draw
  
  face.Draw(theImage,xx,yy,text,align,fontcolor);

}

// ----------------------------------------------------------------------
/*!
 * \brief Draw label(s) onto the image
 *
 * The option syntax is
 * \code
 * <spec>::<spec>::<spec>...
 * \endcode
 * where an individual spec is of form
 * \code
 * text,lon,lat,dx,dy,alignment,xmargin,ymargin,font,color,backgroundcolor
 * \endcode
 *
 * \param theImage The image to draw into
 * \param theArea The projection
 * \param theXoff The pixel x-offset from cropping
 * \param theYoff The pixel y-offset from cropping
 * \param theOptions The command line option string
 */
// ----------------------------------------------------------------------

void draw_labels(Imagine::NFmiImage & theImage,
				 const NFmiArea & theArea,
				 int theXoff,
				 int theYoff,
				 const string & theOptions)
{
  vector<string> options = NFmiStringTools::Split(theOptions,"::");
  for(vector<string>::const_iterator it = options.begin();
	  it != options.end();
	  ++it)
	{
	  // defaults

	  int dx = 0;
	  int dy = 0;
	  string alignment = "Center";
	  int xmargin = 1;
	  int ymargin = 1;
	  string font = "misc/6x13.pcf.gz:6x13";
	  string color = "black";
	  string backgroundcolor = "transparent";

	  // parse the options

	  vector<string> words = NFmiStringTools::Split(*it);

	  // compulsory parts: text,lon,lat

	  if(words.size() < 3)
		throw runtime_error("Too short option string '"+theOptions+"' for option -L");

	  const string text = words[0];
	  const double lon = NFmiStringTools::Convert<double>(words[1]);
	  const double lat = NFmiStringTools::Convert<double>(words[2]);

	  vector<string>::size_type i = 2;

	  if(words.size() > i+1 && !words[++i].empty()) dx = NFmiStringTools::Convert<int>(words[i]);
	  if(words.size() > i+1 && !words[++i].empty()) dy = NFmiStringTools::Convert<int>(words[i]);
	  if(words.size() > i+1 && !words[++i].empty()) alignment = words[i];
	  if(words.size() > i+1 && !words[++i].empty()) xmargin = ymargin = NFmiStringTools::Convert<int>(words[i]);
	  if(words.size() > i+1 && !words[++i].empty()) ymargin = NFmiStringTools::Convert<int>(words[i]);
	  if(words.size() > i+1 && !words[++i].empty()) font = words[i];
	  if(words.size() > i+1 && !words[++i].empty()) color = words[i];
	  if(words.size() > i+1 && !words[++i].empty()) backgroundcolor = words[i];

	  // Extra parts

	  if(words.size() > i+1)
		throw runtime_error("Too many -L parts in option '"+*it+"'");

	  // Parse the font option

	  vector<string> fontparts = NFmiStringTools::Split(font,":");
	  if(fontparts.size() != 2)
		throw runtime_error("Invalid font specification for option -L : '"+font+"'");
	  font = fontparts[0];
	  fontparts = NFmiStringTools::Split(fontparts[1],"x");
	  if(fontparts.size() != 2)
		throw runtime_error("Invalid font size specification for option -L : '"+fontparts[1]+"'");
	  const int width = NFmiStringTools::Convert<int>(fontparts[0]);
	  const int height = NFmiStringTools::Convert<int>(fontparts[1]);
  
	  // Parse the font color option
	  
	  Imagine::NFmiColorTools::Color fontcolor = parse_color(color);
	  if(fontcolor == Imagine::NFmiColorTools::MissingColor)
		throw runtime_error("Unknown font color '"+color+"'");

	  // Parse the background color option
	  
	  Imagine::NFmiColorTools::Color backcolor = parse_color(backgroundcolor);
	  if(backcolor == Imagine::NFmiColorTools::MissingColor)
		throw runtime_error("Unknown font color '"+backgroundcolor+"'");
	  
	  // Parse the alignment option

	  Imagine::NFmiAlignment align = Imagine::AlignmentValue(alignment);
	  if(align == Imagine::kFmiAlignMissing)
		throw runtime_error("Unknown alignment '"+alignment+"'");

	  // Calculate the text coordinates

	  NFmiPoint xy = checkmeridian(NFmiPoint(lon,lat),theArea);
	  xy = theArea.ToXY(xy);
	  int xx = FmiRound(xy.X() + dx - theXoff);
	  int yy = FmiRound(xy.Y() + dy - theYoff);

	  // Create the face and setup the background

	  Imagine::NFmiFace face(font,width,height);
	  face.Background(true);
	  face.BackgroundColor(backcolor);
	  face.BackgroundMargin(xmargin,ymargin);

	  // Draw
  
	  face.Draw(theImage,xx,yy,text,align,fontcolor);

	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Draw a marker onto the projection center
 *
 * \param theImage The image to draw into
 * \param theOptions The marker option string
 * \param theX The X-coordinate
 * \param theY The Y-coordinate
 */
// ----------------------------------------------------------------------

void draw_center(Imagine::NFmiImage & theImage,
				 const string & theOptions,
				 int theX,
				 int theY)
{
  if(theOptions.substr(0,6) == "square")
	{
	  const vector<string> parts = NFmiStringTools::Split(theOptions,":");
	  const string colorname = (parts.size() < 2 ? "black" : parts[1]);
	  Imagine::NFmiColorTools::Color color = parse_color(colorname);

	  const int sz = 2;
	  Imagine::NFmiPath path;
	  path.MoveTo(theX-sz,theY-sz);
	  path.LineTo(theX+sz,theY-sz);
	  path.LineTo(theX+sz,theY+sz);
	  path.LineTo(theX-sz,theY+sz);
	  path.CloseLineTo();

	  path.Fill(theImage,
				color,
				Imagine::NFmiColorTools::kFmiColorOnOpaque);
	}
  else
	{
	  Imagine::NFmiImage marker(theOptions);
	  theImage.Composite(marker,
						 Imagine::NFmiColorTools::kFmiColorOnOpaque,
						 Imagine::kFmiAlignCenter,
						 theX,
						 theY,
						 1.0);
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Draw another image onto the image
 *
 * \param theImage The image to draw onto
 * \param theOptions The options string
 */
// ----------------------------------------------------------------------

void draw_image(Imagine::NFmiImage & theImage,
				const string & theOptions)
{

  // Parse the options

  const vector<string> parts = NFmiStringTools::Split(theOptions);
  if(parts.size() % 3 != 0)
	throw runtime_error("Option -I argument should be of form image,x,y,...");

  for(unsigned int i=0; i<parts.size(); i+=3)
	{
	  const string filename = parts[i];
	  const int x = NFmiStringTools::Convert<int>(parts[i+1]);
	  const int y = NFmiStringTools::Convert<int>(parts[i+2]);

	  // Establish alignment and corrected coordinates
	  
	  Imagine::NFmiAlignment align = Imagine::kFmiAlignNorthWest;
	  int xx = x;
	  int yy = y;
	  if(x >= 0 && y >= 0)
		;
	  else if(x >= 0 && y < 0)
		{
		  yy = theImage.Height() + y + 1;
		  align = Imagine::kFmiAlignSouthWest;
		}
	  else if(x < 0 && y >= 0)
		{
		  xx = theImage.Width() + x + 1;
		  align = Imagine::kFmiAlignNorthEast;
		}
	  else
		{
		  xx = theImage.Width() + x + 1;
		  yy = theImage.Height() + y + 1;
		  align = Imagine::kFmiAlignSouthEast;
		}
	  
	  // Render the image
	  
	  Imagine::NFmiImage img(filename);
	  theImage.Composite(img,
						 Imagine::NFmiColorTools::kFmiColorOnOpaque,
						 align,
						 xx,
						 yy,
						 1.0);
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Reduce colors in the image
 *
 * \param theImage The image to operate on
 * \param theSpec The reduction specs, for example "5550"
 */
// ----------------------------------------------------------------------

void reduce_colors(Imagine::NFmiImage & theImage, const string & theSpecs)
{
  if(theSpecs.size() != 4)
	throw runtime_error("Invalid color reduction specification '"+theSpecs+"'");
  int r = theSpecs[0] - '0';
  int g = theSpecs[1] - '0';
  int b = theSpecs[2] - '0';
  int a = theSpecs[3] - '0';

  Imagine::NFmiImageTools::CompressBits(theImage,r,g,b,a);

}

// ----------------------------------------------------------------------
/*!
 * \brief The main algorithm
 */
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  typedef map<string,string> Options;
  Options options;

  const string default_timezone = "Europe/Helsinki";

  if(getenv("QUERY_STRING") != 0)
	{
	  options = NFmiStringTools::ParseQueryString();
	}
  else
	{
	  NFmiCmdLine cmdline(argc, argv, "f!g!c!l!p!o!T!t!M!I!L!AZ:hk!");

	  if(cmdline.Status().IsError())
		throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

	  if(cmdline.NumberofParameters() != 0)
		throw runtime_error("No command line parameters are expected");

	  if(cmdline.isOption('h'))
		{
		  usage("cropper_auth");
		  exit(0);
		}

	  if(cmdline.isOption('f'))
		options.insert(Options::value_type("f",cmdline.OptionValue('f')));
	  if(cmdline.isOption('g'))
		options.insert(Options::value_type("g",cmdline.OptionValue('g')));
	  if(cmdline.isOption('c'))
		options.insert(Options::value_type("c",cmdline.OptionValue('c')));
	  if(cmdline.isOption('l'))
		options.insert(Options::value_type("l",cmdline.OptionValue('l')));
	  if(cmdline.isOption('p'))
		options.insert(Options::value_type("p",cmdline.OptionValue('p')));
	  if(cmdline.isOption('o'))
		options.insert(Options::value_type("o",cmdline.OptionValue('o')));
	  if(cmdline.isOption('T'))
		options.insert(Options::value_type("T",cmdline.OptionValue('T')));
	  if(cmdline.isOption('t'))
		options.insert(Options::value_type("t",cmdline.OptionValue('t')));
	  if(cmdline.isOption('M'))
		options.insert(Options::value_type("M",cmdline.OptionValue('M')));
	  if(cmdline.isOption('I'))
		options.insert(Options::value_type("I",cmdline.OptionValue('I')));
	  if(cmdline.isOption('L'))
		options.insert(Options::value_type("L",cmdline.OptionValue('L')));
	  if(cmdline.isOption('A'))
		options.insert(Options::value_type("A","1"));
	  if(cmdline.isOption('k'))
		options.insert(Options::value_type("k",cmdline.OptionValue('k')));
	  if(cmdline.isOption('Z'))
		if(cmdline.OptionValue('Z'))
		  options.insert(Options::value_type("Z",cmdline.OptionValue('Z')));
		else
		  options.insert(Options::value_type("Z","5550"));
	}

  const Options::const_iterator end = options.end();

  const bool has_option_f = (options.find("f") != end);
  const bool has_option_g = (options.find("g") != end);
  const bool has_option_c = (options.find("c") != end);
  const bool has_option_p = (options.find("p") != end);
  const bool has_option_l = (options.find("l") != end);
  const bool has_option_o = (options.find("o") != end);
  const bool has_option_T = (options.find("T") != end);
  const bool has_option_t = (options.find("t") != end);
  const bool has_option_M = (options.find("M") != end);
  const bool has_option_I = (options.find("I") != end);
  const bool has_option_C = (options.find("C") != end);
  const bool has_option_L = (options.find("L") != end);
  const bool has_option_A = (options.find("A") != end);
  const bool has_option_Z = (options.find("Z") != end);
  const bool has_option_k = (options.find("k") != end);

  // -o does not modify the image
  const bool has_modifying_options
	= (options.size() > 1 ||
	   (options.size() == 1 && options.find("o")!=end));

  if(!has_option_f)
	throw runtime_error("Must give image name to be cropped");

  if(has_option_g + has_option_c + has_option_p + has_option_l > 1)
	throw runtime_error("Too many cropping geometries defined, use only one");

  // Check the image exists

  const string imagefile = options.find("f")->second;
  if(!NFmiFileSystem::FileExists(imagefile))
	throw runtime_error("The desired image '"+imagefile+"' does not exist");

  // Handle a possible HTTP_IF_MODIFIED_SINCE query
  if(not_modified(imagefile))
	return 0;

  // Quick special case

  if(!has_modifying_options)
	{
	  if(has_option_o)
		NFmiFileSystem::CopyFile(imagefile,options.find("o")->second);
	  else
		http_output_image(imagefile);
	  return 0;
	}

  // Use cache if possible

  if(!has_option_C && !has_option_o)
	{
	  if(http_output_cache(getenv("QUERY_STRING")))
		return 0;
	}

  // Set timestring language

  if(has_option_k)
	setlocale(LC_TIME,options.find("k")->second.c_str());

  auto_ptr<Imagine::NFmiImage> cropped(new Imagine::NFmiImage(imagefile));
  const string imagetype = cropped->Type();

  bool has_center = false;
  int xm = 0;
  int ym = 0;

  // How much was removed from the image
  int xoff = 0;
  int yoff = 0;

  // The established projection, if any

  auto_ptr<NFmiArea> area;

  if(has_option_p)
	{
	  int xc,yc,width,height;
	  has_center = true;
	  area = parse_named_geometry(options.find("p")->second,xc,yc,width,height);
	  cropped = crop_center(*cropped,xc,yc,width,height,xoff,yoff);
	  xm = xc - xoff;
	  ym = yc - yoff;
	}
  else if(has_option_l)
	{
	  int xc,yc,width,height;
	  has_center = true;
	  area = parse_latlon_geometry(options.find("l")->second,xc,yc,width,height);
	  cropped = crop_center(*cropped,xc,yc,width,height,xoff,yoff);
	  xm = xc - xoff;
	  ym = yc - yoff;
	}
  else if(has_option_c)
	{
	  int xc,yc,width,height;
	  has_center = true;
	  parse_center_geometry(options.find("c")->second,xc,yc,width,height);
	  cropped = crop_center(*cropped,xc,yc,width,height,xoff,yoff);
	  xm = xc - xoff;
	  ym = yc - yoff;
	}
  else if(has_option_g)
	{
	  int x1,y1,width,height;
	  parse_geometry(options.find("g")->second,x1,y1,width,height);
	  cropped = crop_corner(*cropped,x1,y1,width,height,xoff,yoff);
	}

  if(has_option_L)
	{
	  if(area.get() == 0)
		throw runtime_error("Cannot draw labels onto image without a projection obtained from cropping");
	  draw_labels(*cropped,*area,xoff,yoff,options.find("L")->second);
	}

  if(has_option_T)
	{
	  set_timezone(!has_option_t ? default_timezone : options.find("t")->second);
	  draw_timestamp(*cropped,options.find("T")->second,imagefile);
	}

  if(has_option_I)
	{
	  draw_image(*cropped,options.find("I")->second);
	}

  if(has_option_M && has_center)
	{
	  draw_center(*cropped,options.find("M")->second,xm,ym);
	}

  if(has_option_Z)
	reduce_colors(*cropped,options.find("Z")->second);

  cropped->SaveAlpha(false);
  if(has_option_A && options.find("A")->second != "0")
	cropped->SaveAlpha(true);

  cropped->WantPalette(true);

  if(has_option_o)
	{
	  string & filename = options.find("o")->second;
	  string suffix = NFmiStringTools::Suffix(filename);
	  cropped->Write(filename,suffix);
	}
  else
	http_output_image(*cropped,imagefile,imagetype,has_option_C);

  return 0;

}

