// ======================================================================
/*!
 * \file
 * \brief Implementation of the \c cropper command
 *
 * \note  THIS FILE AND PROGRAM IS DEPRICATED -- YOU MUST
 * \note  USE AUTHENTICATING VERSION cropper_auth IN NEW
 * \note  PRODUCTS AND SERVICES
 */
// ======================================================================

#include "CropperTools.h"
#include <iostream>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief The main program. REMEMBER, THIS IS DEPRICATED VERSION OF CROPPER!
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
