// ======================================================================
/*!
 * \file
 * \brief Implementation of the \c cropper command
 */
// ======================================================================

#include "NFmiCmdLine.h"
#include "NFmiStringTools.h"

#include <cstdlib>
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
 * \brief The main algorithm
 */
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  map<string,string> options;

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

  
  

  return 0;

}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  try
	{
	  return domain(argc, argv);
	}
  catch(exception & e)
	{
	  cerr << "Error: Caught an exception" << endl
		   << " --> " << e.what() << endl;
	}
  catch(...)
	{
	  cerr << "Error: Caught an unknown exception" << endl;
	}
  return 1;
}

// ======================================================================
