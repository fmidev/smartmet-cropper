// ======================================================================
/*!
 * \file
 * \brief Implementation of the \c cropper command
 */
// ======================================================================

#include "NFmiCmdLine.h"
#include "NFmiFileSystem.h"
#include "NFmiStringTools.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <map>
#include <string>

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
	   << "   -p [geometry]\t<width>x<height>+<x1>+<y1>" << endl
	   << "   -c [centergeometry]\t<width>x<height>+<xc>+<yc>" << endl
	   << "   -n [namegeometry]\t<width>x<height>+<placename>/<mapname>" << endl
	   << "   -f [imagefile]" << endl
	   << "   -o [outputfile]" << endl
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
  if(getenv("QUERYSTRING") != 0 &&
	 getenv("HTTP_LAST_MODIFIED_SINCE") != 0)
	{
	  cout << "HTTP/1.0 304 Not Modified" << endl << endl;
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

  cout << "HTTP/1.1 200 OK" << endl
	   << "Content-Type: image/png" << endl
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
 * \brief The main algorithm
 */
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  typedef map<string,string> Options;
  Options options;

  if(getenv("QUERYSTRING") != 0)
	options = NFmiStringTools::ParseQueryString();
  else
	{
	  NFmiCmdLine cmdline(argc, argv, "f!p!c!n!o!h");

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
		options.insert(make_pair("f",cmdline.OptionValue('f')));
	  if(cmdline.isOption('p'))
		options.insert(make_pair("p",cmdline.OptionValue('p')));
	  if(cmdline.isOption('c'))
		options.insert(make_pair("c",cmdline.OptionValue('c')));
	  if(cmdline.isOption('n'))
		options.insert(make_pair("n",cmdline.OptionValue('n')));
	  if(cmdline.isOption('o'))
		options.insert(make_pair("o",cmdline.OptionValue('o')));

	}

  const Options::const_iterator end = options.end();

  const bool has_option_f = (options.find("f") != end);
  const bool has_option_p = (options.find("p") != end);
  const bool has_option_c = (options.find("c") != end);
  const bool has_option_n = (options.find("n") != end);
  const bool has_option_o = (options.find("o") != end);

  if(!has_option_f)
	throw runtime_error("Must give image name to be cropped");

  if(has_option_p + has_option_c + has_option_n > 1)
	throw runtime_error("Too many cropping geometries defined, use only one");

  // Check the image exists

  const string imagefile = options.find("f")->second;
  if(!NFmiFileSystem::FileExists(imagefile))
	throw runtime_error("The desired image '"+imagefile+"' does not exist");

  // Handle a possible HTTP_IF_MODIFIED_SINCE query

  if(not_modified(imagefile))
	return 0;

  // Quick special case

  if(has_option_p + has_option_c + has_option_n == 0)
	{
	  if(has_option_o)
		NFmiFileSystem::CopyFile(imagefile,options.find("o")->second);
	  else
		http_output_image(imagefile);
	  return 0;
	}

  return 0;

}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  const bool httpmode = (getenv("QUERYSTRING") != 0);

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
		  cout << "HTTP/1.1 409 Conflict" << endl
			   << "Content-Type: text/html" << endl
			   << endl
			   << e.what()
			   << endl;
		}
	}

  catch(...)
	{
	  if(!httpmode)
		cerr << "Error: Caught an unknown exception" << endl;
	  else
		{
		  cout << "HTTP/1.1 409 Conflict" << endl
			   << "Content-Type: text/html" << endl
			   << endl
			   << "Unknown exception occurred"
			   << endl;
		}
	}
  return 1;
}

// ======================================================================
