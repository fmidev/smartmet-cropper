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
#include "CropperException.h"
#include "WebAuthenticator.h"

#include <iostream>
#include <cstdlib>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char *argv[])
{
  const bool httpmode = (getenv("QUERY_STRING") != 0);

  if (httpmode)
  {
    // Authenticate query string
    WebAuthenticator authorizer;
    if (authorizer.isValidQuery(getenv("QUERY_STRING")) == false)
    {
      // Query string not validated, print error message and quit program
      cout << "Content-Type: text/plain" << endl
           << "Status: 409 Authentication Failed" << endl
           << endl;

      return 1;
    }

    // Authentication was accepted. We remove the authentication information
    // from the query string by replacing the data on environmental variable
    // QUERY_STRING with a canonized query string.
    //
    // This is a simple but effective fix, as the cropper solely
    // relies on QUERY_STRING environmental variable.
    setenv("QUERY_STRING", authorizer.canonizeQuery(getenv("QUERY_STRING")).c_str(), true);
  }

  try
  {
    return domain(argc, argv);
  }

  catch (CropperException &e)
  {
    if (!httpmode)
    {
      cerr << "Error: Caught an exception:" << endl << e.what() << endl;
    }
    else
    {
      cout << "Content-Type: text/plain" << endl
           << "Status: " << e.status() << ' ' << e.what() << endl
           << endl;
    }
  }

  catch (exception &e)
  {
    if (!httpmode)
    {
      cerr << "Error: Caught an exception" << endl << " --> " << e.what() << endl;
    }
    else
    {
      cout << "Content-Type: text/plain" << endl << "Status: 409 " << e.what() << endl << endl;
    }
  }

  catch (...)
  {
    if (!httpmode)
      cerr << "Error: Caught an unknown exception" << endl;
    else
      cout << "Content-Type: text/plain" << endl
           << "Status: 409 Unknown exception occurred" << endl
           << endl;
  }
  return 1;
}

// ======================================================================
