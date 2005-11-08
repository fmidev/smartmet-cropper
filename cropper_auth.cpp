// ======================================================================
/*!
 * \file
 * \brief Implementation of the \c cropper command
 *
 * \note  THIS IMPLEMENTATION REQUIRES AUTHENTICATION TICKET
 *        WHEN USED AS A CGI SCRIPT!
 */
// ======================================================================

#include "CropperTools.h"
#include "webauthenticator.h"

#include <iostream>

using namespace std;

WebAuthenticator Authorize;

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  const bool httpmode = (getenv("QUERY_STRING") != 0);

  if ( httpmode )
	{
	  // Authenticate query string
	  if ( Authorize.isValidQuery(getenv("QUERY_STRING")) == false)
		{
		  // Query string not validated, print error message and quit program
		  cout << "Status: 409 Authentication Failed" << endl;

		  return 1;
		}


	  // Authentication was accepted. We remove the authentication information
	  // from the query string by replacing the data on environmental variable
	  // QUERY_STRING with a canonized query string.
	  //
	  // This is a simple but effective fix, as the cropper solely
	  // relies on QUERY_STRING environmental variable.
	  setenv("QUERY_STRING", Authorize.canonizeQuery(getenv("QUERY_STRING")).c_str(), true);
	}

  

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
