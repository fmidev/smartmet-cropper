// ======================================================================
/*!
 * \file
 * \brief Implementation of the \c cropper command
 */
// ======================================================================

#include "NFmiImage.h"

#include "NFmiArea.h"
#include "NFmiAreaFactory.h"
#include "NFmiCmdLine.h"
#include "NFmiFileSystem.h"
#include "NFmiLocationFinder.h"
#include "NFmiStringTools.h"

#include <cstdlib>
#include <ctime>
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

void usage()
{
  cout << "Usage: cropper [options]" << endl
	   << endl
	   << "Available options are:" << endl
	   << endl
	   << "   -g [geometry]\t<width>x<height>+<x1>+<y1>" << endl
	   << "   -c [centergeometry]\t<width>x<height>+<xc>+<yc>" << endl
	   << "   -l [latlongeometry]\t<width>x<height>+<lon,lat>:<mapname>" << endl
	   << "   -p [namegeometry]\t<width>x<height>+<placename>:<mapname>" << endl
	   << "   -f [imagefile]" << endl
	   << "   -o [outputfile]" << endl
	   << "   -C" << endl
	   << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Format a time for HTTP output
 *
 * The output is generated with strftime using format
 * "%a,  %d  %b  %Y  %H:%M:%S  %z" as adviced in the
 * man-pages for strftime.
 */
// ----------------------------------------------------------------------

const string format_time(const ::time_t theTime)
{
  const struct ::tm * t = localtime(&theTime);
  const ::size_t MAXLEN = 100;
  char buffer[MAXLEN];
  ::size_t n = strftime(buffer,MAXLEN,"%a, %d %b %Y %H:%M:%S %z",t);
  string ret(buffer,0,n);
  return ret;
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
  if(getenv("QUERY_STRING") != 0 &&
	 getenv("HTTP_LAST_MODIFIED_SINCE") != 0)
	{
	  cout << "Status: 304 Not Modified" << endl;
	  return true;
	}
  else
	return false;
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

  cout << "Content-Type: image/png" << endl
	   << "Expires: " << format_time(expiration_time) << endl
	   << "Last-Modified: " << format_time(last_modified) << endl
	   << "Cache-Control: max-age=" << maxage << endl
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

const string cachename(const string theQueryString)
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
  string token;
  while(in >> token)
	{
	  if(token == "projection")
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

const NFmiPoint find_location(const string theName)
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
			   + ".png");
  else
	tmpfile = cachename(getenv("QUERY_STRING"));

  theImage.WritePng(tmpfile);

  ifstream in(tmpfile.c_str(), ios::in|ios::binary);
  if(!in)
	throw runtime_error("Was unable to create temporary file");

  cout << "Content-Type: image/png" << endl
	   << "Expires: " << format_time(expiration_time) << endl
	   << "Last-Modified: " << format_time(last_modified) << endl
	   << "Cache-Control: max-age=" << maxage << endl
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
 */
// ----------------------------------------------------------------------

void parse_latlon_geometry(const string & theGeometry,
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
  
  const NFmiPoint center = area->ToXY(NFmiPoint(lon,lat));

  xc = static_cast<int>(0.5+center.X());
  yc = static_cast<int>(0.5+center.Y());

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

void parse_named_geometry(const string & theGeometry,
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
  
  const NFmiPoint center = area->ToXY(city);

  xc = static_cast<int>(0.5+center.X());
  yc = static_cast<int>(0.5+center.Y());

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
 * \return auto_ptr to the cropped image
 */
// ----------------------------------------------------------------------

auto_ptr<Imagine::NFmiImage> crop_corner(const Imagine::NFmiImage & theImage,
										 int theX1,
										 int theY1,
										 int theWidth,
										 int theHeight)
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
 * \return auto_ptr to the cropped image
 */
// ----------------------------------------------------------------------

auto_ptr<Imagine::NFmiImage> crop_center(const Imagine::NFmiImage & theImage,
										 int theXC,
										 int theYC,
										 int theWidth,
										 int theHeight)
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

  auto_ptr<Imagine::NFmiImage> image(new Imagine::NFmiImage(width,height));
  for(int i=x1; i<x2;i++)
	for(int j=y1; j<y2; j++)
	  (*image)(i-x1,j-y1) = theImage(i,j);

  return image;
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

  if(getenv("QUERY_STRING") != 0)
	{
	  options = NFmiStringTools::ParseQueryString();
	}
  else
	{
	  NFmiCmdLine cmdline(argc, argv, "f!g!c!l!p!o!h");

	  if(cmdline.Status().IsError())
		throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

	  if(cmdline.NumberofParameters() != 0)
		throw runtime_error("No command line parameters are expected");

	  if(cmdline.isOption('h'))
		{
		  usage();
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

	}

  const Options::const_iterator end = options.end();

  const bool has_option_f = (options.find("f") != end);
  const bool has_option_g = (options.find("g") != end);
  const bool has_option_c = (options.find("c") != end);
  const bool has_option_p = (options.find("p") != end);
  const bool has_option_l = (options.find("l") != end);
  const bool has_option_o = (options.find("o") != end);
  const bool has_option_C = (options.find("C") != end);

  // throw runtime_error(shit.str());

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

  if(has_option_g + has_option_c + has_option_p + has_option_l == 0)
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

  Imagine::NFmiImage image(imagefile);

  auto_ptr<Imagine::NFmiImage> cropped;

  if(has_option_p)
	{
	  int xc,yc,width,height;
	  parse_named_geometry(options.find("p")->second,xc,yc,width,height);
	  cropped.reset(crop_center(image,xc,yc,width,height).release());
	}
  if(has_option_l)
	{
	  int xc,yc,width,height;
	  parse_latlon_geometry(options.find("l")->second,xc,yc,width,height);
	  cropped.reset(crop_center(image,xc,yc,width,height).release());
	}
  if(has_option_c)
	{
	  int xc,yc,width,height;
	  parse_center_geometry(options.find("c")->second,xc,yc,width,height);
	  cropped.reset(crop_center(image,xc,yc,width,height).release());
	}
  if(has_option_g)
	{
	  int x1,y1,width,height;
	  parse_geometry(options.find("g")->second,x1,y1,width,height);
	  cropped.reset(crop_corner(image,x1,y1,width,height).release());
	}

  if(has_option_o)
	cropped->WritePng(options.find("o")->second);
  else
	http_output_image(*cropped,imagefile,has_option_C);

  return 0;

}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  const bool httpmode = (getenv("QUERY_STRING") != 0);

  try
	{
	  return domain(argc, argv);
	}

  catch(exception & e)
	{
	  if(!httpmode)
		{
		  cerr << "Error: Caught an exception" << endl
			   << " --> " << e.what() << endl;
		}
	  else
		{
		  cout << "Status: 409 " << e.what() << endl;
		}
	}

  catch(...)
	{
	  if(!httpmode)
		cerr << "Error: Caught an unknown exception" << endl;
	  else
		cout << "Status: 409 Unknown exception occurred" << endl;
	}
  return 1;
}

// ======================================================================
